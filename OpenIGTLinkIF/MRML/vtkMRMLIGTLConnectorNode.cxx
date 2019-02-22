/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLConnectorNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/
// OpenIGTLinkIO include
#include "igtlioConnector.h"
#include "igtlioDeviceFactory.h"
#include "igtlioImageDevice.h"
#include "igtlioStatusDevice.h"
#include "igtlioTransformDevice.h"
#include "igtlioPolyDataDevice.h"
#include "igtlioStringDevice.h"
#include "igtlOSUtil.h"
#include "igtlioCommand.h"

#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  #include "igtlioVideoDevice.h"
#endif

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLIGTLStatusNode.h"
#include "vtkMRMLTextNode.h"

// MRML includes
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLStreamingVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include "vtkMRMLVolumeNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkMutexLock.h>
#include <vtkPolyData.h>
#include <vtkTimerLog.h>

// vtksys includes
#include <vtksys/SystemTools.hxx>

// SlicerQt includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>

#define MEMLNodeNameKey "MEMLNodeName"

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLConnectorNode);

//---------------------------------------------------------------------------
class vtkMRMLIGTLConnectorNode::vtkInternal
{
public:
  //---------------------------------------------------------------------------
  vtkInternal(vtkMRMLIGTLConnectorNode* external);
  virtual ~vtkInternal();

  ///  Send the given command from the given device.
  /// - If using BLOCKING, the call blocks until a response appears or timeout. Return response.
  /// - If using ASYNCHRONOUS, wait for the CommandResponseReceivedEvent event. Return device.
  ///
  igtlioCommandPointer SendCommand(igtlioCommandPointer command);

  /// Send a command response from the given device. Asynchronous.
  /// Precondition: The given device has received a query that is not yet responded to.
  /// Return device.
  int SendCommandResponse(igtlioCommandPointer command);

  unsigned int AssignOutGoingNodeToDevice(vtkMRMLNode* node, igtlioDevicePointer device);

  void ProcessNewDeviceEvent(vtkObject *caller, unsigned long event, void *callData);

  void ProcessIncomingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlioDevice * modifiedDevice);

  void ProcessOutgoingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlioDevice * modifiedDevice);

  void DeviceAboutToReceiveEvent(igtlioDevice* modifiedDevice);

  /// Called when command responses are received.
  /// Handles igtlioCommandPointer related details on command response.
  /// If there is a corresponding igtlioCommandPointer, an event is invoked on it.
  void ReceiveCommandResponse(igtlioCommandPointer commandDevice);

  vtkMRMLNode* GetMRMLNodeforDevice(igtlioDevice* device);

  vtkMRMLIGTLConnectorNode* External;
  igtlioConnector* IOConnector;

  typedef std::map<std::string, igtlioConnector::NodeInfoType>   NodeInfoMapType;
  typedef std::map<std::string, vtkSmartPointer <igtlioDevice> > MessageDeviceMapType;
  typedef std::map<std::string, std::vector<std::string> > DeviceTypeToNodeTagMapType;
  typedef std::map<std::string, vtkSmartPointer<vtkStreamingVolumeFrame> > FrameMapType;

  // Calling StartModify() on incoming nodes when messages are received, and EndModify() once all incoming messages have been parsed is a neccesary step.
  // If a Modified() event is triggered on an incoming node while incoming messages are still being processed, it can trigger an early Render.
  // To prevent this, StartModify is called on all incoming nodes in: ProcessIncomingDeviceModifiedEvent() and EndModify() is called on modified nodes in PeriodicProcess().
  struct NodeModification
  {
    int Modifying;
    vtkSmartPointer<vtkMRMLNode> Node;
  };
  std::deque<NodeModification> PendingNodeModifications;

  NodeInfoMapType IncomingMRMLNodeInfoMap;
  MessageDeviceMapType  OutgoingMRMLIDToDeviceMap;
  MessageDeviceMapType  IncomingMRMLIDToDeviceMap;
  DeviceTypeToNodeTagMapType DeviceTypeToNodeTagMap;
  FrameMapType          PreviousIncomingFramesMap;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkInternal::vtkInternal(vtkMRMLIGTLConnectorNode* external)
  : External(external)
{
  this->IOConnector = igtlioConnector::New();
}


//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkInternal::~vtkInternal()
{
  IOConnector->Delete();
}

//----------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::vtkInternal::AssignOutGoingNodeToDevice(vtkMRMLNode* node, igtlioDevicePointer device)
{
  this->OutgoingMRMLIDToDeviceMap[node->GetID()] = device;
  unsigned int modifiedEvent = 0;
  if (device->GetDeviceType().compare("IMAGE") == 0)
  {
    igtlioImageDevice* imageDevice = static_cast<igtlioImageDevice*>(device.GetPointer());
    vtkMRMLVolumeNode* imageNode = vtkMRMLVolumeNode::SafeDownCast(node);
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    imageNode->GetIJKToRASMatrix(mat);
    igtlioImageConverter::ContentData content = { imageNode->GetImageData(), mat };
    imageDevice->SetContent(content);
    modifiedEvent = vtkMRMLVolumeNode::ImageDataModifiedEvent;
  }
  else if (device->GetDeviceType().compare("STATUS") == 0)
  {
    igtlioStatusDevice* statusDevice = static_cast<igtlioStatusDevice*>(device.GetPointer());

    vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(node);
    igtlioStatusConverter::ContentData content = { static_cast<int>(statusNode->GetCode()), static_cast<int>(statusNode->GetSubCode()), statusNode->GetErrorName(), statusNode->GetStatusString() };
    statusDevice->SetContent(content);
    modifiedEvent = vtkMRMLIGTLStatusNode::StatusModifiedEvent;
  }
  else if (device->GetDeviceType().compare("TRANSFORM") == 0)
  {
    igtlioTransformDevice* transformDevice = static_cast<igtlioTransformDevice*>(device.GetPointer());
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
    transformNode->GetMatrixTransformToParent(mat);
    igtlioTransformConverter::ContentData content = { mat, transformNode->GetName(),"",""};
    transformDevice->SetContent(content);
    modifiedEvent = vtkMRMLLinearTransformNode::TransformModifiedEvent;
  }
  else if (device->GetDeviceType().compare("POLYDATA") == 0)
  {
    igtlioPolyDataDevice* polyDevice = static_cast<igtlioPolyDataDevice*>(device.GetPointer());
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
    igtlioPolyDataConverter::ContentData content = { modelNode->GetPolyData(), modelNode->GetName() };
    polyDevice->SetContent(content);
    modifiedEvent = vtkMRMLModelNode::MeshModifiedEvent;
  }
  else if (device->GetDeviceType().compare("STRING") == 0)
  {
    igtlioStringDevice* stringDevice = static_cast<igtlioStringDevice*>(device.GetPointer());
    vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(node);
    igtlioStringConverter::ContentData content = { static_cast<unsigned int>(textNode->GetEncoding()), textNode->GetText() };
    stringDevice->SetContent(content);
    modifiedEvent = vtkMRMLTextNode::TextModifiedEvent;
  }
  else if (device->GetDeviceType().compare("COMMAND") == 0)
  {
    return 0;
    // Process the modified event from command device.
  }
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  else if (device->GetDeviceType().compare("VIDEO") == 0)
  {
    igtlioVideoDevice* videoDevice = static_cast<igtlioVideoDevice*>(device.GetPointer());
    vtkMRMLStreamingVolumeNode* streamingVolumeNode = vtkMRMLStreamingVolumeNode::SafeDownCast(node);
    igtlioVideoConverter::ContentData content;
    content.image = streamingVolumeNode->GetImageData();
    content.frameType = FrameTypeUnKnown;
    strncpy(content.codecName, videoDevice->GetCurrentCodecType().c_str(), IGTL_VIDEO_CODEC_NAME_SIZE);
    content.keyFrameMessage = NULL;
    content.keyFrameUpdated = false;
    content.videoMessage = NULL;
    videoDevice->SetContent(content);
    modifiedEvent = vtkCommand::ModifiedEvent;
  }
#endif
  return modifiedEvent;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessNewDeviceEvent(
  vtkObject * vtkNotUsed(caller), unsigned long vtkNotUsed(event), void *vtkNotUsed(callData))
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessOutgoingDeviceModifiedEvent(
  vtkObject * vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlioDevice * modifiedDevice)
{
  this->IOConnector->SendMessage(igtlioDeviceKeyType::CreateDeviceKey(modifiedDevice), modifiedDevice->MESSAGE_PREFIX_NOT_DEFINED);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessIncomingDeviceModifiedEvent(
  vtkObject * vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlioDevice * modifiedDevice)
{
  vtkMRMLNode* modifiedNode = this->GetMRMLNodeforDevice(modifiedDevice);
  if (!modifiedNode)
  {
    // Could not find or add node.
    return;
  }

  const std::string deviceType = modifiedDevice->GetDeviceType();
  const std::string deviceName = modifiedDevice->GetDeviceName();
  if (this->External->GetNodeTagFromDeviceType(deviceType.c_str()).size() > 0)
  {
    if (strcmp(deviceType.c_str(), "IMAGE") == 0)
    {
      igtlioImageDevice* imageDevice = reinterpret_cast<igtlioImageDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(modifiedNode);
        if (volumeNode)
        {
          volumeNode->SetIJKToRASMatrix(imageDevice->GetContent().transform);
          volumeNode->SetAndObserveImageData(imageDevice->GetContent().image);
          volumeNode->Modified();
        }
      }
    }
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
    else if (strcmp(deviceType.c_str(), "VIDEO") == 0)
    {
      igtlioVideoDevice* videoDevice = reinterpret_cast<igtlioVideoDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkSmartPointer<vtkUnsignedCharArray> frameData = vtkSmartPointer<vtkUnsignedCharArray>::New();
        frameData->Allocate(videoDevice->GetContent().videoMessage->GetBitStreamSize());
        memcpy(frameData->GetPointer(0), videoDevice->GetContent().frameData->GetPointer(0), videoDevice->GetContent().videoMessage->GetBitStreamSize());

        std::string codecName = videoDevice->GetCurrentCodecType().substr(0, 4);
        vtkSmartPointer<vtkStreamingVolumeFrame> frame = vtkSmartPointer<vtkStreamingVolumeFrame>::New();
        frame->SetFrameData(frameData);
        frame->SetFrameType(videoDevice->GetContent().frameType == igtl::FrameTypeKey ? vtkStreamingVolumeFrame::IFrame : vtkStreamingVolumeFrame::PFrame);
        videoDevice->GetContent().videoMessage->Unpack(false);
        frame->SetDimensions(videoDevice->GetContent().videoMessage->GetWidth(),
                             videoDevice->GetContent().videoMessage->GetHeight(),
                             videoDevice->GetContent().videoMessage->GetAdditionalZDimension());
        frame->SetNumberOfComponents(videoDevice->GetContent().grayscale ? 1 : 3);
        frame->SetCodecFourCC(codecName);
        if (!frame->IsKeyFrame() && this->PreviousIncomingFramesMap.find(videoDevice->GetDeviceName()) != this->PreviousIncomingFramesMap.end())
        {
          // If the current frame is not a keyframe, then it should maintain a reference to the previously received frame
          // so that the current frame can be decoded
          frame->SetPreviousFrame(this->PreviousIncomingFramesMap[videoDevice->GetDeviceName()]);
        }
        this->PreviousIncomingFramesMap[videoDevice->GetDeviceName()] = frame;

        vtkMRMLStreamingVolumeNode* streamingVolumeNode = vtkMRMLStreamingVolumeNode::SafeDownCast(modifiedNode);
        streamingVolumeNode->SetIJKToRASMatrix(videoDevice->GetContent().transform);
        streamingVolumeNode->SetAndObserveFrame(frame);
      }
    }
#endif
    else if (strcmp(deviceType.c_str(), "STATUS") == 0)
    {
      igtlioStatusDevice* statusDevice = reinterpret_cast<igtlioStatusDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(modifiedNode);
        statusNode->SetStatus(statusDevice->GetContent().code, statusDevice->GetContent().subcode, statusDevice->GetContent().errorname.c_str(), statusDevice->GetContent().statusstring.c_str());
        statusNode->Modified();
      }
    }
    else if (strcmp(deviceType.c_str(), "TRANSFORM") == 0)
    {
      igtlioTransformDevice* transformDevice = reinterpret_cast<igtlioTransformDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(modifiedNode);
        vtkSmartPointer<vtkMatrix4x4> transfromMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
        transfromMatrix->DeepCopy(transformDevice->GetContent().transform);
        transformNode->SetMatrixTransformToParent(transfromMatrix.GetPointer());
        transformNode->Modified();

        // Copy transform status from metadata to node attributes
        for (igtl::MessageBase::MetaDataMap::const_iterator iter = modifiedDevice->GetMetaData().begin(); iter != modifiedDevice->GetMetaData().end(); ++iter)
        {
          if (iter->first.find("Status") != std::string::npos)
          {
            transformNode->SetAttribute(iter->first.c_str(), iter->second.second.c_str());
          }
        }
      }
    }
    else if (strcmp(deviceType.c_str(), "POLYDATA") == 0)
    {
      igtlioPolyDataDevice* polyDevice = reinterpret_cast<igtlioPolyDataDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(modifiedNode);
        modelNode->SetAndObservePolyData(polyDevice->GetContent().polydata);
        modelNode->Modified();
      }
    }
    else if (strcmp(deviceType.c_str(), "STRING") == 0)
    {
      igtlioStringDevice* stringDevice = reinterpret_cast<igtlioStringDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(modifiedNode);
        textNode->SetEncoding(stringDevice->GetContent().encoding);
        textNode->SetText(stringDevice->GetContent().string_msg.c_str());
        textNode->Modified();
      }
    }
    else if (strcmp(deviceType.c_str(), "COMMAND") == 0)
    {
      // Process the modified event from command device.
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::DeviceAboutToReceiveEvent(igtlioDevice* modifiedDevice)
{
  if (!modifiedDevice)
  {
    vtkErrorWithObjectMacro(modifiedDevice, "Invalid device");
    return;
  }

  vtkMRMLNode* modifiedNode = this->GetMRMLNodeforDevice(modifiedDevice);
  if (!modifiedNode)
  {
    // Could not find or add node.
    return;
  }

  vtkInternal::NodeModification modifying;
  modifying.Node = modifiedNode;
  modifying.Modifying = modifiedNode->StartModify();
  this->PendingNodeModifications.push_back(modifying);
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::vtkInternal::GetMRMLNodeforDevice(igtlioDevice* device)
{
  if (device)
  {
    if (device->GetMessageDirection() != igtlioDevice::MESSAGE_DIRECTION_IN)
    {
      // we are only interested in incomming devices
      return NULL;
    }
  }

  if (!this->External->GetScene())
  {
    // No scene to add nodes to.
    return NULL;
  }

  // Found the node and return the node;
  NodeInfoMapType::iterator inIter;
  for (inIter = this->IncomingMRMLNodeInfoMap.begin();
    inIter != this->IncomingMRMLNodeInfoMap.end();
    inIter++)
  {
    vtkMRMLNode* node = this->External->GetScene()->GetNodeByID((inIter->first));
    if (node)
    {
      const std::string deviceType = device->GetDeviceType();
      const std::string deviceName = device->GetDeviceName();
      bool typeMatched = false;
      for (unsigned int i = 0; i < this->External->GetNodeTagFromDeviceType(deviceType.c_str()).size(); i++)
      {
        const std::string nodeTag = this->External->GetNodeTagFromDeviceType(deviceType.c_str())[i];
        if (strcmp(node->GetNodeTagName(), nodeTag.c_str()) == 0)
        {
          typeMatched = 1;
          break;
        }
      }
      if (typeMatched && strcmp(node->GetName(), deviceName.c_str()) == 0)
      {
        return node;
      }
    }
  }

  // Device name is empty, we will not be able to find a node in the scene
  if (device->GetDeviceName().empty())
  {
    std::string deviceType = device->GetDeviceType();

    // Status messages with no device name are ignored
    // For other message types, a warning is logged
    if (deviceType != igtlioStatusConverter::GetIGTLTypeName())
    {
      vtkWarningWithObjectMacro(this->External, "Incoming " << deviceType << " device has no device name!");
    }
    return NULL;
  }

  // Node not found and add the node
  if (strcmp(device->GetDeviceType().c_str(), "IMAGE") == 0)
  {
    vtkSmartPointer<vtkMRMLVolumeNode> volumeNode;
    int numberOfComponents = 1;
    vtkSmartPointer<vtkImageData> image;
    std::string deviceName = "";
    igtlioImageDevice* imageDevice = reinterpret_cast<igtlioImageDevice*>(device);
    igtlioImageConverter::ContentData content = imageDevice->GetContent();
    if (!content.image)
    {
      // Image data has not been set yet
      return NULL;
    }
    numberOfComponents = content.image->GetNumberOfScalarComponents(); //to improve the io module to be able to cope with video data
    image = content.image;
    deviceName = imageDevice->GetDeviceName().c_str();
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
    if (this->External->UseStreamingVolume)
    {
      volumeNode = vtkMRMLStreamingVolumeNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLStreamingVolumeNode"));
      if (volumeNode)
      {
        this->External->RegisterIncomingMRMLNode(volumeNode, device);
        return volumeNode;
      }
      volumeNode = vtkSmartPointer<vtkMRMLStreamingVolumeNode>::New();
    }
    else
    {
      if (numberOfComponents>1)
      {
        volumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLVectorVolumeNode"));
        if (volumeNode)
        {
          this->External->RegisterIncomingMRMLNode(volumeNode, device);
          return volumeNode;
        }
        volumeNode = vtkSmartPointer<vtkMRMLVectorVolumeNode>::New();
      }
      else
      {
        volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLScalarVolumeNode"));
        if (volumeNode)
        {
          this->External->RegisterIncomingMRMLNode(volumeNode, device);
          return volumeNode;
        }
        volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
      }
    }
#else
    if (numberOfComponents>1)
    {
      volumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLVectorVolumeNode"));
      if (volumeNode)
      {
        this->External->RegisterIncomingMRMLNode(volumeNode, device);
        return volumeNode;
      }
      volumeNode = vtkSmartPointer<vtkMRMLVectorVolumeNode>::New();
    }
    else
    {
      volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLScalarVolumeNode"));
      if (volumeNode)
      {
        this->External->RegisterIncomingMRMLNode(volumeNode, device);
        return volumeNode;
      }
      volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    }
#endif
    volumeNode->SetAndObserveImageData(image);
    volumeNode->SetName(deviceName.c_str());
    this->External->GetScene()->SaveStateForUndo();
    volumeNode->SetDescription("Received by OpenIGTLink");
    vtkDebugWithObjectMacro(this->External, "Name vol node " << volumeNode->GetClassName());
    this->External->GetScene()->AddNode(volumeNode);

    vtkDebugWithObjectMacro(this->External, "Set basic display info");
    bool scalarDisplayNodeRequired = (numberOfComponents == 1);
    vtkSmartPointer<vtkMRMLVolumeDisplayNode> displayNode;
    if (scalarDisplayNodeRequired)
    {
      displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    }
    else
    {
      displayNode = vtkSmartPointer<vtkMRMLVectorVolumeDisplayNode>::New();
    }

    this->External->GetScene()->AddNode(displayNode);

    if (scalarDisplayNodeRequired)
    {
      const char* colorTableId = vtkMRMLColorLogic::GetColorTableNodeID(vtkMRMLColorTableNode::Grey);
      displayNode->SetAndObserveColorNodeID(colorTableId);
    }
    else
    {
      displayNode->SetDefaultColorMap();
    }

    volumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
    this->External->RegisterIncomingMRMLNode(volumeNode, device);
    return volumeNode;
  }
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  else if (strcmp(device->GetDeviceType().c_str(), "VIDEO") == 0)
  {
    igtlioVideoDevice* videoDevice = reinterpret_cast<igtlioVideoDevice*>(device);
    igtlioVideoConverter::ContentData content = videoDevice->GetContent();
    if (!content.frameData)
    {
      // frame data has not been set yet
      return NULL;
    }
    int numberOfComponents = content.grayscale ? 1 : 3;

    std::string deviceName = videoDevice->GetDeviceName().c_str();

    vtkSmartPointer<vtkMRMLStreamingVolumeNode> streamingVolumeNode =
      vtkMRMLStreamingVolumeNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLStreamingVolumeNode"));
    if (streamingVolumeNode)
    {
      this->External->RegisterIncomingMRMLNode(streamingVolumeNode, device);
      return streamingVolumeNode;
    }

    this->External->GetScene()->SaveStateForUndo();
    bool scalarDisplayNodeRequired = (numberOfComponents == 1);
    streamingVolumeNode = vtkSmartPointer<vtkMRMLStreamingVolumeNode>::New();
    streamingVolumeNode->SetName(deviceName.c_str());
    streamingVolumeNode->SetDescription("Received by OpenIGTLink");
    this->External->GetScene()->AddNode(streamingVolumeNode);
    vtkSmartPointer<vtkMRMLVolumeDisplayNode> displayNode;
    if (scalarDisplayNodeRequired)
    {
      displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    }
    else
    {
      displayNode = vtkSmartPointer<vtkMRMLVectorVolumeDisplayNode>::New();
    }
    this->External->GetScene()->AddNode(displayNode);
    if (scalarDisplayNodeRequired)
    {
      const char* colorTableId = vtkMRMLColorLogic::GetColorTableNodeID(vtkMRMLColorTableNode::Grey);
      displayNode->SetAndObserveColorNodeID(colorTableId);
    }
    else
    {
      displayNode->SetDefaultColorMap();
    }
    streamingVolumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
    this->External->RegisterIncomingMRMLNode(streamingVolumeNode, device);
    return streamingVolumeNode;
  }
#endif
  else if (strcmp(device->GetDeviceType().c_str(), "STATUS") == 0)
  {
    std::string deviceName = device->GetDeviceName();

   vtkSmartPointer<vtkMRMLIGTLStatusNode> statusNode =
     vtkMRMLIGTLStatusNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLIGTLStatusNode"));
   if (statusNode)
   {
     this->External->RegisterIncomingMRMLNode(statusNode, device);
     return statusNode;
   }

    statusNode = vtkSmartPointer<vtkMRMLIGTLStatusNode>::New();
    statusNode->SetName(deviceName.c_str());
    statusNode->SetDescription("Received by OpenIGTLink");
    this->External->GetScene()->AddNode(statusNode);
    this->External->RegisterIncomingMRMLNode(statusNode, device);
    return statusNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "TRANSFORM") == 0)
  {

    std::string deviceName = device->GetDeviceName();

    vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode =
      vtkMRMLLinearTransformNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLLinearTransformNode"));
    if (transformNode)
    {
      this->External->RegisterIncomingMRMLNode(transformNode, device);
      return transformNode;
    }

    transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    transformNode->SetName(deviceName.c_str());
    transformNode->SetDescription("Received by OpenIGTLink");

    vtkMatrix4x4* transform = vtkMatrix4x4::New();
    transform->Identity();
    transformNode->ApplyTransformMatrix(transform);
    transform->Delete();
    this->External->GetScene()->AddNode(transformNode);
    this->External->RegisterIncomingMRMLNode(transformNode, device);
    return transformNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "POLYDATA") == 0)
  {

    std::string deviceName = device->GetDeviceName();

    vtkSmartPointer<vtkMRMLModelNode> modelNode =
      vtkMRMLModelNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLModelNode"));
    if (modelNode)
    {
      this->External->RegisterIncomingMRMLNode(modelNode, device);
      return modelNode;
    }

    igtlioPolyDataDevice* polyDevice = reinterpret_cast<igtlioPolyDataDevice*>(device);
    igtlioPolyDataConverter::ContentData content = polyDevice->GetContent();

    std::string mrmlNodeTagName = "";
    if (device->GetMetaDataElement(MEMLNodeNameKey, mrmlNodeTagName))
    {
      std::string className = this->External->GetScene()->GetClassNameByTag(mrmlNodeTagName.c_str());
      vtkMRMLNode * createdNode = this->External->GetScene()->CreateNodeByClass(className.c_str());
      if (createdNode)
      {
        modelNode = vtkMRMLModelNode::SafeDownCast(createdNode);
      }
      else
      {
        modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      }
    }
    else
    {
      modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
    }
    modelNode->SetName(deviceName.c_str());
    modelNode->SetDescription("Received by OpenIGTLink");
    this->External->GetScene()->AddNode(modelNode);
    modelNode->SetAndObservePolyData(content.polydata);
    modelNode->CreateDefaultDisplayNodes();
    this->External->RegisterIncomingMRMLNode(modelNode, device);
    return modelNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "STRING") == 0)
  {
    std::string deviceName = device->GetDeviceName();

    vtkSmartPointer<vtkMRMLTextNode> textNode =
      vtkMRMLTextNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLTextNode"));
    if (textNode)
    {
      this->External->RegisterIncomingMRMLNode(textNode, device);
      return textNode;
    }

    igtlioStringDevice* modifiedDevice = reinterpret_cast<igtlioStringDevice*>(device);
    textNode = vtkSmartPointer<vtkMRMLTextNode>::New();
    textNode->SetEncoding(modifiedDevice->GetContent().encoding);
    textNode->SetText(modifiedDevice->GetContent().string_msg.c_str());
    textNode->SetName(deviceName.c_str());
    textNode->SetDescription("Received by OpenIGTLink");
    this->External->GetScene()->AddNode(textNode);
    this->External->RegisterIncomingMRMLNode(textNode, device);
    return textNode;
  }
  return NULL;
}

//----------------------------------------------------------------------------
igtlioCommandPointer vtkMRMLIGTLConnectorNode::vtkInternal::SendCommand(igtlioCommandPointer command)
{
  this->IOConnector->SendCommand(command);
  return command;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::vtkInternal::SendCommandResponse(igtlioCommandPointer command)
{
  return this->IOConnector->SendCommandResponse(command);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ReceiveCommandResponse(igtlioCommandPointer command)
{
  // Invoke command responses both on the igtlioCommand and the connectorNode.
  this->External->InvokeEvent(CommandResponseReceivedEvent, command);
}

//----------------------------------------------------------------------------
// vtkMRMLIGTLConnectorNode method

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkMRMLIGTLConnectorNode()
{
  this->Internal = new vtkInternal(this);
  this->HideFromEditors = false;
  this->UseStreamingVolume = false;
  this->QueryQueueMutex = vtkMutexLock::New();
  this->ConnectEvents();

  this->IncomingNodeReferenceRole=NULL;
  this->IncomingNodeReferenceMRMLAttributeName=NULL;
  this->OutgoingNodeReferenceRole=NULL;
  this->OutgoingNodeReferenceMRMLAttributeName=NULL;

  this->SetIncomingNodeReferenceRole("incoming");
  this->SetIncomingNodeReferenceMRMLAttributeName("incomingNodeRef");
  this->AddNodeReferenceRole(this->GetIncomingNodeReferenceRole(),
                             this->GetIncomingNodeReferenceMRMLAttributeName());

  this->SetOutgoingNodeReferenceRole("outgoing");
  this->SetOutgoingNodeReferenceMRMLAttributeName("outgoingNodeRef");
  this->AddNodeReferenceRole(this->GetOutgoingNodeReferenceRole(),
                             this->GetOutgoingNodeReferenceMRMLAttributeName());

  this->Internal->DeviceTypeToNodeTagMap.clear();
  std::string volumeTags[] = {"Volume", "VectorVolume", "StreamingVolume"};
  this->Internal->DeviceTypeToNodeTagMap["IMAGE"] = std::vector<std::string>(volumeTags, volumeTags+3);
  this->Internal->DeviceTypeToNodeTagMap["VIDEO"] = std::vector<std::string>(1,"StreamingVolume");
  this->Internal->DeviceTypeToNodeTagMap["STATUS"] = std::vector<std::string>(1,"IGTLStatus");
  this->Internal->DeviceTypeToNodeTagMap["TRANSFORM"] = std::vector<std::string>(1,"LinearTransform");
  std::string modelTags[] = {"Model", "FiberBundle"};
  this->Internal->DeviceTypeToNodeTagMap["POLYDATA"] = std::vector<std::string>(modelTags, modelTags+2);
  this->Internal->DeviceTypeToNodeTagMap["STRING"] = std::vector<std::string>(1,"Text");

}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::~vtkMRMLIGTLConnectorNode()
{
  if (this->QueryQueueMutex)
    {
    this->QueryQueueMutex->Delete();
    }

  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ConnectEvents()
{
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->ConnectedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DisconnectedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->ActivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DeactivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->NewDeviceEvent, this,  &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->RemovedDeviceEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);

  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandReceivedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandResponseEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandCompletedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkMRMLIGTLConnectorNode::GetNodeTagFromDeviceType(const char * deviceType)
{
  vtkInternal::DeviceTypeToNodeTagMapType::iterator iter;
  if(this->Internal->DeviceTypeToNodeTagMap.find(std::string(deviceType)) != this->Internal->DeviceTypeToNodeTagMap.end())
    {
    return this->Internal->DeviceTypeToNodeTagMap.find(std::string(deviceType))->second;
    }
  return std::vector<std::string>(0);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkMRMLIGTLConnectorNode::GetDeviceTypeFromMRMLNodeType(const char* NodeTag)
{
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  if(strcmp(NodeTag, "StreamingVolume")==0)
    {
    std::vector<std::string> DeviceTypes;
    DeviceTypes.push_back("VIDEO");
    DeviceTypes.push_back("IMAGE");
    return DeviceTypes;
    }
#endif
  if(strcmp(NodeTag, "Volume")==0 || strcmp(NodeTag, "VectorVolume")==0)
    {
    std::vector<std::string> DeviceTypes;
    DeviceTypes.push_back("IMAGE");
    return DeviceTypes;
    }
  if(strcmp(NodeTag, "IGTLStatus")==0)
    {
    return std::vector<std::string>(1, "STATUS");
    }
  if(strcmp(NodeTag, "LinearTransform")==0)
    {
    return std::vector<std::string>(1, "TRANSFORM");
    }
  if(strcmp(NodeTag, "Model")==0 || strcmp(NodeTag, "FiberBundle")==0)
    {
    return std::vector<std::string>(1, "POLYDATA");
    }
  if(strcmp(NodeTag, "Text")==0)
    {
    return std::vector<std::string>(1, "STRING");
    }
  return std::vector<std::string>(0);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(caller);
  if (!node)
    {
    return;
    }
  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());

  for (int i = 0; i < n; i ++)
  {
    const char* id = GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      this->PushNode(node);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents(vtkObject *caller, unsigned long event, void *callData )
{
  igtlioConnector* connector = igtlioConnector::SafeDownCast(caller);
  if (connector == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }
  int mrmlEvent = -1;
  switch (event)
    {
    case igtlioConnector::ConnectedEvent: mrmlEvent = ConnectedEvent; break;
    case igtlioConnector::DisconnectedEvent: mrmlEvent = DisconnectedEvent; break;
    case igtlioConnector::ActivatedEvent: mrmlEvent = ActivatedEvent; break;
    case igtlioConnector::DeactivatedEvent: mrmlEvent = DeactivatedEvent; break;
    case igtlioConnector::NewDeviceEvent: mrmlEvent = NewDeviceEvent; break;
    case igtlioConnector::RemovedDeviceEvent: mrmlEvent = DeviceModifiedEvent; break;
    }

  if (mrmlEvent == ConnectedEvent)
    {
    vtkInfoMacro("Connected: " << connector->GetServerHostname() << ":" << connector->GetServerPort());

    // Slicer wants to make use of meta data in IGTL messages, so tell the server we can support v2 messages
    igtlioDeviceKeyType key;
    key.name = "";
    key.type = "STATUS";
    vtkSmartPointer<igtlioStatusDevice> statusDevice = igtlioStatusDevice::SafeDownCast(this->Internal->IOConnector->GetDevice(key));
    if (!statusDevice)
    {
      statusDevice = vtkSmartPointer<igtlioStatusDevice>::New();
      connector->AddDevice(statusDevice);
    }
    statusDevice->SetMetaDataElement("dummy", "dummy"); // existence of metadata makes the IO connector send a header v2 message
    connector->SendMessage(igtlioDeviceKeyType::CreateDeviceKey(statusDevice));
    connector->RemoveDevice(statusDevice);

    this->PushOnConnect();
    }
  else if (mrmlEvent == DisconnectedEvent)
    {
    vtkInfoMacro("Disconnected: " << connector->GetServerHostname() << ":" << connector->GetServerPort());
    }
  else if (event == igtlioConnector::NewDeviceEvent)
    {
    igtlioDevice* modifiedDevice = static_cast<igtlioDevice*>(callData);
    if (modifiedDevice)
      {
      // no action perform at this stage, wait until the message content in the device is unpacked,
      // As we need the message content data to create mrmlnode.
      // Also the newly added device could also be a outgoing message from IF module
      //this->ProcessNewDeviceEvent(caller, event, callData );
      //modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(),  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
      if (modifiedDevice->MessageDirectionIsIn())
        {
        modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(), this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);
        modifiedDevice->AddObserver(igtlioDevice::AboutToReceiveEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);
        }
      }
    }

  if (mrmlEvent > 0)
    {
    this->InvokeEvent(mrmlEvent, callData);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PushOnConnect()
{
  for (int i = 0; i < this->GetNumberOfOutgoingMRMLNodes(); ++i)
    {
    vtkMRMLNode* node = this->GetOutgoingMRMLNode(i);
    const char* pushOnConnect = node->GetAttribute("OpenIGTLinkIF.pushOnConnect");
    if (pushOnConnect && strcmp(pushOnConnect, "true") == 0)
      {
      this->PushNode(node);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents(vtkObject *caller, unsigned long event, void *callData)
{
  igtlioDevice* modifiedDevice = igtlioDevice::SafeDownCast(caller);
  if (modifiedDevice == NULL)
    {
    return;
    }

  int mrmlEvent = -1;
  if (event == igtlioDevice::AboutToReceiveEvent)
    {
    this->Internal->DeviceAboutToReceiveEvent(modifiedDevice);
    }
  else if(event == modifiedDevice->GetDeviceContentModifiedEvent())
    {
    mrmlEvent = DeviceModifiedEvent;
    if (modifiedDevice->MessageDirectionIsIn())
      {
      this->Internal->ProcessIncomingDeviceModifiedEvent(caller, event, modifiedDevice);
      }
    else if (modifiedDevice->MessageDirectionIsOut())
      {
      this->Internal->ProcessOutgoingDeviceModifiedEvent(caller, event, modifiedDevice);
      }
    }

  if (mrmlEvent > 0)
    {
    this->InvokeEvent(mrmlEvent, modifiedDevice);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents(vtkObject *caller, unsigned long event, void *callData)
{
  igtlioConnector* connector = reinterpret_cast<igtlioConnector*>(caller);
  if (connector == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }

  int mrmlEvent = -1;
  switch (event)
  {
  case igtlioCommand::CommandReceivedEvent: mrmlEvent = CommandReceivedEvent; break;  // COMMAND received
  case igtlioCommand::CommandResponseEvent: mrmlEvent = CommandResponseReceivedEvent; break;  // RTS_COMMAND received
  case igtlioCommand::CommandCompletedEvent: mrmlEvent = CommandCompletedEvent; break;  // Sent COMMAND did not receive a response before timeout
  }

  igtlioCommand* command = static_cast<igtlioCommand*>(callData);
  if (command)
    {
    this->InvokeEvent(mrmlEvent, command);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::WriteXML(ostream& of, int nIndent)
{

  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  switch (this->Internal->IOConnector->GetType())
    {
      case igtlioConnector::TYPE_SERVER:
      of << " connectorType=\"" << "SERVER" << "\" ";
      break;
    case igtlioConnector::TYPE_CLIENT:
      of << " connectorType=\"" << "CLIENT" << "\" ";
      of << " serverHostname=\"" << this->Internal->IOConnector->GetServerHostname() << "\" ";
      break;
    default:
      of << " connectorType=\"" << "NOT_DEFINED" << "\" ";
      break;
    }

  of << " serverPort=\"" << this->Internal->IOConnector->GetServerPort() << "\" ";
  of << " persistent=\"" << this->Internal->IOConnector->GetPersistent() << "\" ";
  of << " checkCRC=\"" << this->Internal->IOConnector->GetCheckCRC() << "\" ";
  of << " state=\"" << this->Internal->IOConnector->GetState() <<"\"";
  of << " restrictDeviceName=\"" << this->Internal->IOConnector->GetRestrictDeviceName() << "\" ";

}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;

  const char* serverHostname = "";
  int port = 0;
  int type = -1;
  int restrictDeviceName = 0;
  int state = igtlioConnector::STATE_OFF;
  int persistent = igtlioConnector::PERSISTENT_OFF;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "connectorType"))
      {
      if (!strcmp(attValue, "SERVER"))
        {
        type = igtlioConnector::TYPE_SERVER;
        }
      else if (!strcmp(attValue, "CLIENT"))
        {
        type = igtlioConnector::TYPE_CLIENT;
        }
      else
        {
        type = igtlioConnector::TYPE_NOT_DEFINED;
        }
      }
    if (!strcmp(attName, "serverHostname"))
      {
      serverHostname = attValue;
      }
    if (!strcmp(attName, "serverPort"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> port;
      }
    if (!strcmp(attName, "restrictDeviceName"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> restrictDeviceName;;
      }
    if (!strcmp(attName, "persistent"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> persistent;
      }
    if (!strcmp(attName, "checkCRC"))
      {
      std::stringstream ss;
      ss << attValue;
      bool checkCRC = true;
      ss >> checkCRC;
      this->SetCheckCRC(checkCRC);
      }
    if (!strcmp(attName, "state"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> state;
      }

  switch(type)
    {
    case igtlioConnector::TYPE_SERVER:
      this->Internal->IOConnector->SetTypeServer(port);
      this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    case igtlioConnector::TYPE_CLIENT:
      this->Internal->IOConnector->SetTypeClient(serverHostname, port);
      this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    default: // not defined
      // do nothing
      break;
    }

  if (persistent == igtlioConnector::PERSISTENT_ON)
    {
    this->Internal->IOConnector->SetPersistent(igtlioConnector::PERSISTENT_ON);
    // At this point not all the nodes are loaded so we cannot start
    // the processing thread (if we activate the connection then it may immediately
    // start receiving messages, create corresponding MRML nodes, while the same
    // MRML nodes are being loaded from the scene; this would result in duplicate
    // MRML nodes).
    // All the nodes will be activated by the module logic when the scene import is finished.
    this->Internal->IOConnector->SetState(state);
    this->Modified();
    }
  }
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIGTLConnectorNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLIGTLConnectorNode *node = (vtkMRMLIGTLConnectorNode *) anode;

  int type = node->Internal->IOConnector->GetType();

  switch(type)
    {
      case igtlioConnector::TYPE_SERVER:
      this->Internal->IOConnector->SetType(igtlioConnector::TYPE_SERVER);
      this->Internal->IOConnector->SetTypeServer(node->Internal->IOConnector->GetServerPort());
      this->Internal->IOConnector->SetRestrictDeviceName(node->Internal->IOConnector->GetRestrictDeviceName());
      break;
    case igtlioConnector::TYPE_CLIENT:
      this->Internal->IOConnector->SetType(igtlioConnector::TYPE_CLIENT);
      this->Internal->IOConnector->SetTypeClient(node->Internal->IOConnector->GetServerHostname(), node->Internal->IOConnector->GetServerPort());
      this->Internal->IOConnector->SetRestrictDeviceName(node->Internal->IOConnector->GetRestrictDeviceName());
      break;
    default: // not defined
      // do nothing
        this->Internal->IOConnector->SetType(igtlioConnector::TYPE_NOT_DEFINED);
      break;
    }
  this->Internal->IOConnector->SetState(node->Internal->IOConnector->GetState());
  this->Internal->IOConnector->SetPersistent(node->Internal->IOConnector->GetPersistent());
  this->SetCheckCRC(node->GetCheckCRC());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->Internal->IOConnector->GetType() == igtlioConnector::TYPE_SERVER)
    {
    os << indent << "Connector Type : SERVER\n";
    os << indent << "Listening Port #: " << this->Internal->IOConnector->GetServerPort() << "\n";
    }
  else if (this->Internal->IOConnector->GetType() == igtlioConnector::TYPE_CLIENT)
    {
    os << indent << "Connector Type: CLIENT\n";
    os << indent << "Server Hostname: " << this->Internal->IOConnector->GetServerHostname() << "\n";
    os << indent << "Server Port #: " << this->Internal->IOConnector->GetServerPort() << "\n";
    }

  switch (this->Internal->IOConnector->GetState())
    {
    case igtlioConnector::STATE_OFF:
      os << indent << "State: OFF\n";
      break;
    case igtlioConnector::STATE_WAIT_CONNECTION:
      os << indent << "State: WAIT FOR CONNECTION\n";
      break;
    case igtlioConnector::STATE_CONNECTED:
      os << indent << "State: CONNECTED\n";
      break;
    }
  os << indent << "Persistent: " << this->Internal->IOConnector->GetPersistent() << "\n";
  os << indent << "Restrict Device Name: " << this->Internal->IOConnector->GetRestrictDeviceName() << "\n";
  os << indent << "Push Outgoing Message Flag: " << this->Internal->IOConnector->GetPushOutgoingMessageFlag() << "\n";
  os << indent << "Check CRC: " << this->GetCheckCRC()<< "\n";
  os << indent << "Number of outgoing nodes: " << this->GetNumberOfOutgoingMRMLNodes() << "\n";
  os << indent << "Number of incoming nodes: " << this->GetNumberOfIncomingMRMLNodes() << "\n";
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceAdded(vtkMRMLNodeReference *reference)
{
  vtkMRMLScene* scene = this->GetScene();
  if (!scene)
  {
    return;
  }

  vtkMRMLNode* node = scene->GetNodeByID(reference->GetReferencedNodeID());
  if (!node)
  {
    return;
  }

  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
  {
    // Add new NodeInfoType structure
    igtlioConnector::NodeInfoType nodeInfo;
    //nodeInfo.node = node;
    nodeInfo.lock = 0;
    nodeInfo.second = 0;
    nodeInfo.nanosecond = 0;
    this->Internal->IncomingMRMLNodeInfoMap[node->GetID()] = nodeInfo;
  }
  else
  {
    // Find a converter for the node
    igtlioDevicePointer device = NULL;
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(node->GetID());
    if (citer == this->Internal->OutgoingMRMLIDToDeviceMap.end())
      {
      igtlioDeviceKeyType key;
      key.name = node->GetName();
      std::vector<std::string> deviceTypes = GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
      for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
        {
        key.type = deviceTypes[typeIndex];
        device = this->Internal->IOConnector->GetDevice(key);
        if (device != NULL)
          {
          break;
          }
        }
      if(device == NULL)
        {
        //If no device is found, add a device using the first device type, such as prefer "IMAGE" over "VIDEO" for ScalarVolumeNode.
        device = this->Internal->IOConnector->GetDeviceFactory()->create(deviceTypes[0], key.name);
        device->SetMessageDirection(igtlioDevice::MESSAGE_DIRECTION_OUT);
        if (device)
          {
          this->Internal->OutgoingMRMLIDToDeviceMap[node->GetID()] = device;
          this->Internal->IOConnector->AddDevice(device);
          }
        }
      }
    else
      {
      device = this->Internal->OutgoingMRMLIDToDeviceMap[node->GetID()];
      }
    if (!device)
      {
    // TODO: Remove the reference ID?
    return;
      }
    device->SetMessageDirection(igtlioDevice::MESSAGE_DIRECTION_OUT);
    unsigned int NodeModifiedEvent = this->Internal->AssignOutGoingNodeToDevice(node, device);
    node->AddObserver(NodeModifiedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);

    // Need to update the events here because observed events are not saved in the scene
    // for each reference and therefore only the role-default event observers are added.
    // Get the correct list of events to observe from the converter and update the reference
    // with that.
    // Without doing this, outgoing transforms are not updated if connectors are set up from
    // a saved scene.
    int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
    for (int i = 0; i < n; i ++)
    {
      const char* id = GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      if (strcmp(node->GetID(), id) == 0)
      {
        vtkIntArray* nodeEvents;
        nodeEvents = vtkIntArray::New();
        nodeEvents->InsertNextValue(NodeModifiedEvent);
        this->SetAndObserveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i,
                                              node->GetID(),nodeEvents );
        nodeEvents->Delete();
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceRemoved(vtkMRMLNodeReference *reference)
{
  const char* nodeID = reference->GetReferencedNodeID();
  if (!nodeID)
    {
    return;
    }
  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
    {
    // Check if the node has already been reagistered.
    // TODO: MRMLNodeListType can be reimplemented as a std::list
    // so that the converter can be removed by 'remove()' method.
    vtkInternal::NodeInfoMapType::iterator iter;
    iter = this->Internal->IncomingMRMLNodeInfoMap.find(nodeID);
    if (iter != this->Internal->IncomingMRMLNodeInfoMap.end())
      {
      this->Internal->IncomingMRMLNodeInfoMap.erase(iter);
      }
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->IncomingMRMLIDToDeviceMap.find(nodeID);
    if (citer != this->Internal->IncomingMRMLIDToDeviceMap.end())
      {
      this->Internal->IncomingMRMLIDToDeviceMap.erase(citer);
      }
    else
      {
      vtkErrorMacro("Node is not found in IncomingMRMLIDToDeviceMap: "<<nodeID);
      }
    }
  else
    {
    // Search converter from OutgoingMRMLIDToDeviceMap
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(nodeID);
    if (citer != this->Internal->OutgoingMRMLIDToDeviceMap.end())
      {
      vtkSmartPointer<igtlioDevice> device = citer->second;
      device->RemoveObserver(device->GetDeviceContentModifiedEvent());
      this->Internal->IOConnector->RemoveDevice(device);
      this->Internal->OutgoingMRMLIDToDeviceMap.erase(citer);
      }
    else
      {
      vtkErrorMacro("Node is not found in OutgoingMRMLIDToDeviceMap: "<<nodeID);
      }
    }
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceModified(vtkMRMLNodeReference *reference)
{
  vtkMRMLScene* scene = this->GetScene();
  if (!scene)
  {
    return;
  }

  vtkMRMLNode* node = scene->GetNodeByID(reference->GetReferencedNodeID());
  if (!node)
  {
    return;
  }
  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
  {
  }
  else
  {
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node, IGTLDevicePointer device)
{

  if (!node)
  {
    return false;
  }
  igtlioDevice* igtlDevice = static_cast< igtlioDevice* >(device);

  // Check if the node has already been registered.
  if (this->HasNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID()))
  {
    // the node has been already registered.
    return false;
  }
  else
  {
    this->Internal->IncomingMRMLIDToDeviceMap[node->GetID()] = igtlDevice;
    this->AddAndObserveNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID());
    this->Modified();
  }

  return true;

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnregisterIncomingMRMLNode(vtkMRMLNode* node)
{

  if (!node)
  {
    return;
  }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole());
  for (int i = 0; i < n; i ++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i);
      vtkInternal::NodeInfoMapType::iterator iter;
      iter = this->Internal->IncomingMRMLNodeInfoMap.find(id);
      if (iter != this->Internal->IncomingMRMLNodeInfoMap.end())
      {
        this->Internal->IncomingMRMLNodeInfoMap.erase(iter);
      }
      vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->IncomingMRMLIDToDeviceMap.find(node->GetID());
      if (citer != this->Internal->IncomingMRMLIDToDeviceMap.end())
        {
        this->Internal->IncomingMRMLIDToDeviceMap.erase(citer);
        }
      else
        {
        vtkErrorMacro("Node is not found in IncomingMRMLIDToDeviceMap: "<<node->GetID());
        }
      this->Modified();
      break;
    }
  }

}

//---------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::GetNumberOfOutgoingMRMLNodes()
{
  //return this->OutgoingMRMLNodeList.size();
  return this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetOutgoingMRMLNode(unsigned int i)
{
  if (i < (unsigned int)this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole()))
    {
    vtkMRMLScene* scene = this->GetScene();
    if (!scene)
      return NULL;
    vtkMRMLNode* node = scene->GetNodeByID(this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i));
    return node;
    }
  else
    {
    return NULL;
    }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType)
{

  if (!node)
    {
    return 0;
    }

  // TODO: Need to check the existing device type?
  if (node->GetAttribute("OpenIGTLinkIF.out.type") == NULL)
    {
    node->SetAttribute("OpenIGTLinkIF.out.type", devType);
    }
  if (node->GetAttribute("OpenIGTLinkIF.out.name") == NULL)
    {
    node->SetAttribute("OpenIGTLinkIF.out.name", node->GetName());
    }



  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i ++)
    {
    const char* id = GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
      {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      break;
      }
    }

  if (this->AddAndObserveNodeReferenceID(this->GetOutgoingNodeReferenceRole(), node->GetID()))
    {
    igtlioDevicePointer device = NULL;
    igtlioDeviceKeyType key;
    key.name = node->GetName();
    std::vector<std::string> deviceTypes = GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
    for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
      {
      key.type = deviceTypes[typeIndex];
      device = this->Internal->IOConnector->GetDevice(key);
      if (device != NULL)
        {
        break;
        }
      }
    /*--------
    // device should be added in the OnNodeReferenceAdded function already,
    // delete the following lines in the future
    this->OutgoingMRMLIDToDeviceMap[node->GetName()] = device;
    if (!device)
      {
      device = this->Internal->IOConnector->GetDeviceFactory()->create(key.type, key.name);
      device->SetMessageDirection(igtlioDevice::MESSAGE_DIRECTION_OUT);
      this->Internal->IOConnector->AddDevice(device);
      }
    */

    this->Modified();

    return 1;
    }
  else // If the reference node wasn't associated with any device, delete the reference
    {
    int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
    for (int i = 0; i < n; i ++)
    {
      const char* id = GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      if (strcmp(node->GetID(), id) == 0)
      {
        // Alredy on the list. Remove it.
        this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
        break;
      }
    }
    return 0;
    }

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnregisterOutgoingMRMLNode(vtkMRMLNode* node)
{
  if (!node)
  {
    return;
  }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i ++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      this->Modified();
      break;
    }
  }

}

//---------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::GetNumberOfIncomingMRMLNodes()
{
  //return this->IncomingMRMLNodeInfoMap.size();
  return this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole());
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetIncomingMRMLNode(unsigned int i)
{
  if (i < (unsigned int)this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole()))
    {
    vtkMRMLScene* scene = this->GetScene();
    if (!scene)
      return NULL;
    vtkMRMLNode* node = scene->GetNodeByID(this->GetNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i));
    return node;
    }
  else
    {
    return NULL;
    }
}


//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::PushNode(vtkMRMLNode* node)
{
  if (!node)
    {
    return 0;
    }

  if (!(node->GetID()))
  {
    return 0;
  }


  vtkInternal::MessageDeviceMapType::iterator iter = this->Internal->OutgoingMRMLIDToDeviceMap.find(node->GetID());
  if (iter == this->Internal->OutgoingMRMLIDToDeviceMap.end())
    {
    vtkErrorMacro("Node is not found in OutgoingMRMLIDToDeviceMap: "<<node->GetID());
    return 0;
    }

  vtkSmartPointer<igtlioDevice> device = iter->second;
  igtlioDeviceKeyType key;
  key.name = device->GetDeviceName();
  key.type = device->GetDeviceType();
  device->ClearMetaData();
  device->SetMetaDataElement(MEMLNodeNameKey, IANA_TYPE_US_ASCII, node->GetNodeTagName());

  if (node->IsA("vtkMRMLTransformNode"))
    {
    const char* transformStatusAttribute = node->GetAttribute("TransformStatus");
    if (transformStatusAttribute)
      {
    device->SetMetaDataElement("TransformStatus", transformStatusAttribute);
      }
    else
      {
      // If no transform status is specified, the transform node is assumed to be valid.
      // Transform status should be set to "OK", rather than "UNKNOWN" to reflect this.
      device->SetMetaDataElement("TransformStatus", "OK");
      }
    }
  else
    {
    const char* statusAttribute = node->GetAttribute("Status");
    if (statusAttribute)
      {
      device->SetMetaDataElement("Status", statusAttribute);
      }
    else
      {
      // If no status is specified, the node is assumed to be valid.
      // Status should be set to "OK", rather than "UNKNOWN" to reflect this.
      device->SetMetaDataElement("Status", "OK");
      }
    }

  device->RemoveObservers(device->GetDeviceContentModifiedEvent());
  this->Internal->AssignOutGoingNodeToDevice(node, device); // update the device content
  device->AddObserver(device->GetDeviceContentModifiedEvent(),  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);

  if((strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")!=0))
    {
    this->Internal->IOConnector->SendMessage(key); //
    }
  else if(strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")==0)
    {
    this->Internal->IOConnector->SendMessage(key, device->MESSAGE_PREFIX_RTS);
    }
  return 0;
}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PushQuery(vtkMRMLIGTLQueryNode* node)
{
  if (node==NULL)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: invalid input node");
    return;
    }
  igtlioDeviceKeyType key;
  key.name = node->GetIGTLDeviceName();
  key.type = node->GetIGTLName();

  if(this->Internal->IOConnector->GetDevice(key)==NULL)
    {
    igtlioDeviceCreatorPointer deviceCreator = this->Internal->IOConnector->GetDeviceFactory()->GetCreator(key.GetBaseTypeName());
    if (deviceCreator.GetPointer())
      {
      this->Internal->IOConnector->AddDevice(deviceCreator->Create(key.name));
      }
    else
      {
      vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: Device type not supported");
      return;
      }
    }
  this->Internal->IOConnector->SendMessage(key, igtlioDevice::MESSAGE_PREFIX_RTS);
  this->QueryQueueMutex->Lock();
  node->SetTimeStamp(vtkTimerLog::GetUniversalTime());
  node->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_WAITING);
  node->SetConnectorNodeID(this->GetID());
  this->QueryWaitingQueue.push_back(node);
  this->QueryQueueMutex->Unlock();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::CancelCommand(igtlioCommandPointer command)
{
  this->Internal->IOConnector->CancelCommand(command);
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::CancelCommand(int commandId, int clientId)
{
  this->Internal->IOConnector->CancelCommand(commandId, clientId);
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::CancelQuery(vtkMRMLIGTLQueryNode* node)
{
  if (node==NULL)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::CancelQuery failed: invalid input node");
    return;
    }
  this->QueryQueueMutex->Lock();
  this->QueryWaitingQueue.remove(node);
  node->SetConnectorNodeID("");
  this->QueryQueueMutex->Unlock();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::LockIncomingMRMLNode(vtkMRMLNode* node)
{
  vtkInternal::NodeInfoMapType::iterator iter;
  for (iter = this->Internal->IncomingMRMLNodeInfoMap.begin(); iter != this->Internal->IncomingMRMLNodeInfoMap.end(); iter++)
    {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
      {
      (iter->second).lock = 1;
      }
    }
}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnlockIncomingMRMLNode(vtkMRMLNode* node)
{
  // Check if the node has already been added in the locked node list
  vtkInternal::NodeInfoMapType::iterator iter;
  for (iter = this->Internal->IncomingMRMLNodeInfoMap.begin(); iter != this->Internal->IncomingMRMLNodeInfoMap.end(); iter++)
    {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
      {
      (iter->second).lock = 0;
      return;
      }
    }
}


//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetIGTLTimeStamp(vtkMRMLNode* node, int& second, int& nanosecond)
{
  // Check if the node has already been added in the locked node list
  vtkInternal::NodeInfoMapType::iterator iter;
  for (iter = this->Internal->IncomingMRMLNodeInfoMap.begin(); iter != this->Internal->IncomingMRMLNodeInfoMap.end(); iter++)
    {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
      {
      second = (iter->second).second;
      nanosecond = (iter->second).nanosecond;
      return 1;
      }
    }
  return 0;

}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetState()
{
  if (!this->Internal->IOConnector)
  {
    return StateOff;
  }

  switch (this->Internal->IOConnector->GetState())
  {
  case igtlioConnector::STATE_OFF: return StateOff;
  case igtlioConnector::STATE_WAIT_CONNECTION: return StateWaitConnection;
  case igtlioConnector::STATE_CONNECTED: return StateConnected;
  default:
    return StateOff;
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::GetPersistent()
{
  return this->Internal->IOConnector->GetPersistent() == igtlioConnector::PERSISTENT_ON;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetPersistent(bool persistent)
{
  this->Internal->IOConnector->SetPersistent(persistent ? igtlioConnector::PERSISTENT_ON : igtlioConnector::PERSISTENT_OFF);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommand(igtlioCommandPointer command)
{
  if (!command)
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::SendCommand failed: Invalid command");
    return;
    }

  if (command->IsInProgress())
    {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::SendCommand failed: Command is already in progress!")
    return;
    }

  this->Internal->SendCommand(command);
}

//---------------------------------------------------------------------------
igtlioCommandPointer vtkMRMLIGTLConnectorNode::SendCommand(std::string name, std::string content,
  bool blocking/*=true*/, double timeout_s/*=5*/, igtl::MessageBase::MetaDataMap* metaData/*=NULL*/, int clientId/*=-1*/)
{
  igtlioCommandPointer command = igtlioCommandPointer::New();
  command->SetClientId(clientId);
  command->SetName(name);
  command->SetCommandContent(content);
  if (metaData)
    {
    command->SetCommandMetaData(*metaData);
    }
  command->SetBlocking(blocking);
  command->SetTimeoutSec(timeout_s);
  return this->Internal->SendCommand(command);
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SendCommandResponse(igtlioCommandPointer command)
{
  return this->Internal->SendCommandResponse(command);
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SetTypeServer(int port)
{
  return this->Internal->IOConnector->SetTypeServer(port);
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SetTypeClient(std::string hostname, int port)
{
  return this->Internal->IOConnector->SetTypeClient(hostname, port);
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Start()
{
  int status = this->Internal->IOConnector->Start();
  this->Modified();
  return status;
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Stop()
{
  int status = this->Internal->IOConnector->Stop();
  this->Modified();
  return status;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PeriodicProcess()
{
  qSlicerApplication* application = qSlicerApplication::application();
  if (application)
  {
    application->pauseRender();
  }

  this->Internal->IOConnector->PeriodicProcess();

  while (!this->Internal->PendingNodeModifications.empty())
    {
    vtkInternal::NodeModification wasModifying = this->Internal->PendingNodeModifications.back();
    wasModifying.Node->EndModify(wasModifying.Modifying);
    this->Internal->PendingNodeModifications.pop_back();
    }

  if (application)
  {
    application->resumeRender();
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::AddDevice(IGTLDevicePointer device)
{
  return this->Internal->IOConnector->AddDevice(static_cast<igtlioDevice*>(device));
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::RemoveDevice(IGTLDevicePointer device)
{
  return this->Internal->IOConnector->RemoveDevice(static_cast<igtlioDevice*>(device));
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetRestrictDeviceName(int restrictDeviceName)
{
  this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
}


//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::GetDeviceFromOutgoingMRMLNode(const char* outgoingNodeID)
{
  vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(outgoingNodeID);
  if (citer == this->Internal->OutgoingMRMLIDToDeviceMap.end())
  {
    return NULL;
  }
  return citer->second;
}

//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::GetDeviceFromIncomingMRMLNode(const char* incomingNodeID)
{
  vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->IncomingMRMLIDToDeviceMap.find(incomingNodeID);
  if (citer == this->Internal->IncomingMRMLIDToDeviceMap.end())
  {
    return NULL;
  }
  return citer->second;
}

/*
//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::GetDevice(const std::string& deviceType, const std::string& deviceName)
{
  igtlioDeviceKeyType key(deviceType, deviceName);
  this->Internal->IOConnector->GetDevice(key);
}
*/

//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::CreateDeviceForOutgoingMRMLNode(vtkMRMLNode* dnode)
{
  vtkSmartPointer<igtlioDevice> device = NULL;
  vtkInternal::MessageDeviceMapType::iterator iter = this->Internal->OutgoingMRMLIDToDeviceMap.find(dnode->GetID());
  if (iter == this->Internal->OutgoingMRMLIDToDeviceMap.end())
  {
    igtlioDeviceKeyType key;
    key.name = dnode->GetName();
    std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(dnode->GetNodeTagName());
    for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
    {
      key.type = deviceTypes[typeIndex];
      device = this->Internal->IOConnector->GetDevice(key);
      if (device != NULL)
      {
        break;
      }
    }
    if (device == NULL)
    {
      //If no device is found, add a device using the first device type, such as prefer "IMAGE" over "VIDEO" for ScalarVolumeNode.
      //"VIDEO" over "IMAGE" for Streaming volume node
      key.type = deviceTypes[0];
      device = this->Internal->IOConnector->GetDeviceFactory()->create(key.type, key.name);
      if (device)
      {
        this->Internal->OutgoingMRMLIDToDeviceMap[dnode->GetID()] = device;
        this->Internal->IOConnector->AddDevice(device);
      }
    }
  }
  else
  {
    device = this->Internal->OutgoingMRMLIDToDeviceMap[dnode->GetID()];
  }
  return device;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetType(int type)
{
  switch (type)
  {
  case TypeServer:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_SERVER);
    break;
  case TypeClient:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_CLIENT);
    break;
  default:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_NOT_DEFINED);
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetType()
{
  switch (this->Internal->IOConnector->GetType())
  {
  case igtlioConnector::TYPE_SERVER: return TypeServer;
  case igtlioConnector::TYPE_CLIENT: return TypeClient;
  default:
    return TypeNotDefined;
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetServerPort()
{
  return this->Internal->IOConnector->GetServerPort();
}

//---------------------------------------------------------------------------
const char* vtkMRMLIGTLConnectorNode::GetServerHostname()
{
  return this->Internal->IOConnector->GetServerHostname();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetServerHostname(std::string hostname)
{
  this->Internal->IOConnector->SetServerHostname(hostname);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetUseStreamingVolume(bool useStreamingVolume)
{
  this->UseStreamingVolume = useStreamingVolume;
}
//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::GetUseStreamingVolume()
{
  return this->UseStreamingVolume;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetServerPort(int port)
{
  this->Internal->IOConnector->SetServerPort(port);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetCheckCRC(bool check)
{
  this->Internal->IOConnector->SetCheckCRC(check ? 1 : 0);
}

//----------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::GetCheckCRC()
{
  return (this->Internal->IOConnector->GetCheckCRC() != 0);
}
