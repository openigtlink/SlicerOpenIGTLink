/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGradientAnisotropicDiffusionFilterNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/
// OpenIGTLinkIO include
#include "igtlioConnector.h"
#include "igtlioDeviceFactory.h"
#include "igtlioImageDevice.h"
#include "igtlioStatusDevice.h"
#include "igtlioTransformDevice.h"
#include "igtlioCommandDevice.h"
#include "igtlioPolyDataDevice.h"
#include "igtlioStringDevice.h"
#include "igtlOSUtil.h"

#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  #include "igtlioVideoDevice.h"
  #include <vtkMRMLStreamingVolumeNode.h>
  #include "vtkIGTLStreamingVolumeCodec.h"
#endif
// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLVolumeNode.h"
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLTextNode.h>
#include <vtkMRMLIGTLStatusNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkCollection.h>
#include <vtkMutexLock.h>
#include <vtkTimerLog.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyData.h>

// VTK include
#include <vtksys/SystemTools.hxx>

#define MEMLNodeNameKey "MEMLNodeName"

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLConnectorNode);

//---------------------------------------------------------------------------
class vtkMRMLIGTLConnectorNode::vtkInternal:public vtkObject
{
public:
  //---------------------------------------------------------------------------
  vtkInternal(vtkMRMLIGTLConnectorNode* external);
  ~vtkInternal();

  ///  Send the given command from the given device.
  /// - If using BLOCKING, the call blocks until a response appears or timeout. Return response.
  /// - If using ASYNCHRONOUS, wait for the CommandResponseReceivedEvent event. Return device.
  ///
  igtlio::CommandDevicePointer SendCommand(std::string device_id, std::string command, std::string content, igtlio::SYNCHRONIZATION_TYPE synchronized = igtlio::BLOCKING, double timeout_s = 5);

  /// Send a command response from the given device. Asynchronous.
  /// Precondition: The given device has received a query that is not yet responded to.
  /// Return device.
  igtlio::CommandDevicePointer SendCommandResponse(std::string device_id, std::string command, std::string content);

  unsigned int AssignOutGoingNodeToDevice(vtkMRMLNode* node, igtlio::DevicePointer device);

  void ProcessNewDeviceEvent(vtkObject *caller, unsigned long event, void *callData);

  void ProcessIncomingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlio::Device * modifiedDevice);

  void ProcessOutgoingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlio::Device * modifiedDevice);

  vtkMRMLNode* GetOrAddMRMLNodeforDevice(igtlio::Device* device);

  vtkMRMLIGTLConnectorNode* External;
  igtlio::ConnectorPointer IOConnector;

  typedef std::map<std::string, igtlio::Connector::NodeInfoType>   NodeInfoMapType;
  typedef std::map<std::string, vtkSmartPointer <igtlio::Device> > MessageDeviceMapType;
  typedef std::map<std::string, std::vector<std::string> > DeviceTypeToNodeTagMapType;

  NodeInfoMapType IncomingMRMLNodeInfoMap;
  MessageDeviceMapType  OutgoingMRMLIDToDeviceMap;
  MessageDeviceMapType  IncomingMRMLIDToDeviceMap;
  DeviceTypeToNodeTagMapType DeviceTypeToNodeTagMap;

};

//----------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkInternal::vtkInternal(vtkMRMLIGTLConnectorNode* external)
  : External(external)
{
  this->IOConnector = igtlio::Connector::New();
}


//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkInternal::~vtkInternal()
{
  IOConnector->Delete();
}

//----------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::vtkInternal::AssignOutGoingNodeToDevice(vtkMRMLNode* node, igtlio::DevicePointer device)
{
  this->OutgoingMRMLIDToDeviceMap[node->GetID()] = device;
  unsigned int modifiedEvent = 0;
  if (device->GetDeviceType().compare("IMAGE") == 0)
  {
    igtlio::ImageDevice* imageDevice = static_cast<igtlio::ImageDevice*>(device.GetPointer());
    vtkMRMLVolumeNode* imageNode = vtkMRMLVolumeNode::SafeDownCast(node);
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    imageNode->GetIJKToRASMatrix(mat);
    igtlio::ImageConverter::ContentData content = { imageNode->GetImageData(), mat };
    imageDevice->SetContent(content);
    modifiedEvent = vtkMRMLVolumeNode::ImageDataModifiedEvent;
  }
  else if (device->GetDeviceType().compare("STATUS") == 0)
  {
    igtlio::StatusDevice* statusDevice = static_cast<igtlio::StatusDevice*>(device.GetPointer());

    vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(node);
    igtlio::StatusConverter::ContentData content = { static_cast<int>(statusNode->GetCode()), static_cast<int>(statusNode->GetSubCode()), statusNode->GetErrorName(), statusNode->GetStatusString() };
    statusDevice->SetContent(content);
    modifiedEvent = vtkMRMLIGTLStatusNode::StatusModifiedEvent;
  }
  else if (device->GetDeviceType().compare("TRANSFORM") == 0)
  {
    igtlio::TransformDevice* transformDevice = static_cast<igtlio::TransformDevice*>(device.GetPointer());
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
    transformNode->GetMatrixTransformToParent(mat);
    igtlio::TransformConverter::ContentData content = { mat, transformNode->GetName(),"",""};
    transformDevice->SetContent(content);
    modifiedEvent = vtkMRMLLinearTransformNode::TransformModifiedEvent;
  }
  else if (device->GetDeviceType().compare("POLYDATA") == 0)
  {
    igtlio::PolyDataDevice* polyDevice = static_cast<igtlio::PolyDataDevice*>(device.GetPointer());
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
    igtlio::PolyDataConverter::ContentData content = { modelNode->GetPolyData(), modelNode->GetName() };
    polyDevice->SetContent(content);
    modifiedEvent = vtkMRMLModelNode::MeshModifiedEvent;
  }
  else if (device->GetDeviceType().compare("STRING") == 0)
  {
    igtlio::StringDevice* stringDevice = static_cast<igtlio::StringDevice*>(device.GetPointer());
    vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(node);
    igtlio::StringConverter::ContentData content = { static_cast<unsigned int>(textNode->GetEncoding()), textNode->GetText() };
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
    igtlio::VideoDevice* videoDevice = static_cast<igtlio::VideoDevice*>(device.GetPointer());
    vtkMRMLStreamingVolumeNode* bitStreamNode = vtkMRMLStreamingVolumeNode::SafeDownCast(node);
    igtlio::VideoConverter::ContentData content;
    content.image = bitStreamNode->GetImageData();
    content.frameType = FrameTypeUnKnown;
    strncpy(content.codecName, videoDevice->GetCurrentCodecType().c_str(), IGTL_VIDEO_CODEC_NAME_SIZE);
    content.keyFrameMessage = NULL;
    content.keyFrameUpdated = false;
    content.videoMessage = NULL;
    videoDevice->SetContent(content);
    modifiedEvent = vtkMRMLStreamingVolumeNode::ImageDataModifiedEvent;
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
  vtkObject * vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlio::Device * modifiedDevice)
{
  this->IOConnector->SendMessage(CreateDeviceKey(modifiedDevice), modifiedDevice->MESSAGE_PREFIX_NOT_DEFINED);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessIncomingDeviceModifiedEvent(
  vtkObject * vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlio::Device * modifiedDevice)
{
  vtkMRMLNode* modifiedNode = this->GetOrAddMRMLNodeforDevice(modifiedDevice);
  const std::string deviceType = modifiedDevice->GetDeviceType();
  const std::string deviceName = modifiedDevice->GetDeviceName();
  if (this->External->GetNodeTagFromDeviceType(deviceType.c_str()).size() > 0)
  {
    if (strcmp(deviceType.c_str(), "IMAGE") == 0)
    {
      igtlio::ImageDevice* imageDevice = reinterpret_cast<igtlio::ImageDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        if (strcmp(modifiedNode->GetNodeTagName(), "Volume") == 0 ||
            strcmp(modifiedNode->GetNodeTagName(), "VectorVolume") == 0
            )
        {
          vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(modifiedNode);
          volumeNode->SetIJKToRASMatrix(imageDevice->GetContent().transform);
          volumeNode->SetAndObserveImageData(imageDevice->GetContent().image);
          volumeNode->Modified();
        }
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
        else if (strcmp(modifiedNode->GetNodeTagName(), "StreamingVolume") == 0)
        {
          vtkMRMLStreamingVolumeNode* bitStreamNode = vtkMRMLStreamingVolumeNode::SafeDownCast(modifiedNode);
          bitStreamNode->SetAndObserveImageData(imageDevice->GetContent().image);
          bitStreamNode->SetIJKToRASMatrix(imageDevice->GetContent().transform);
          bitStreamNode->Modified();
          vtkIGTLStreamingVolumeCodec* device = static_cast<vtkIGTLStreamingVolumeCodec*> (bitStreamNode->GetCompressionCodec());
          //igtlio::VideoDevice* device = static_cast<igtlio::VideoDevice*>(bitStreamNode->GetVideoMessageDevice());
          vtkImageData* srcImageData = imageDevice->GetContent().image;
          int numComponents = srcImageData->GetNumberOfScalarComponents();
          int size[3];
          srcImageData->GetDimensions(size);
          igtl::ImageMessage::Pointer temImageMsg = igtl::ImageMessage::New();
          int scalarTypeSize = temImageMsg->GetScalarSize(srcImageData->GetScalarType());
          long dataSize = size[0] * size[1] * size[2] * scalarTypeSize*numComponents;
          memcpy(device->GetContent()->image->GetScalarPointer(), imageDevice->GetContent().image->GetScalarPointer(), dataSize);
          device->GetStreamFromContentUsingDefaultDevice();
        }
#endif
      }
    }
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
    else if (strcmp(deviceType.c_str(), "VIDEO") == 0)
    {
      igtlio::VideoDevice* videoDevice = reinterpret_cast<igtlio::VideoDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLStreamingVolumeNode* bitStreamNode = vtkMRMLStreamingVolumeNode::SafeDownCast(modifiedNode);
        bitStreamNode->SetAndObserveImageData(videoDevice->GetContent().image);
      }
      // The BitstreamNode has its own handling of the device modified event
    }
#endif
    else if (strcmp(deviceType.c_str(), "STATUS") == 0)
    {
      igtlio::StatusDevice* statusDevice = reinterpret_cast<igtlio::StatusDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(modifiedNode);
        statusNode->SetStatus(statusDevice->GetContent().code, statusDevice->GetContent().subcode, statusDevice->GetContent().errorname.c_str(), statusDevice->GetContent().statusstring.c_str());
        statusNode->Modified();
      }
    }
    else if (strcmp(deviceType.c_str(), "TRANSFORM") == 0)
    {
      igtlio::TransformDevice* transformDevice = reinterpret_cast<igtlio::TransformDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(modifiedNode);
        vtkSmartPointer<vtkMatrix4x4> transfromMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
        transfromMatrix->DeepCopy(transformDevice->GetContent().transform);
        transformNode->SetMatrixTransformToParent(transfromMatrix.GetPointer());
        transformNode->Modified();
      }
    }
    else if (strcmp(deviceType.c_str(), "POLYDATA") == 0)
    {
      igtlio::PolyDataDevice* polyDevice = reinterpret_cast<igtlio::PolyDataDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(modifiedNode);
        modelNode->SetAndObservePolyData(polyDevice->GetContent().polydata);
        modelNode->Modified();
      }
    }
    else if (strcmp(deviceType.c_str(), "STRING") == 0)
    {
      igtlio::StringDevice* stringDevice = reinterpret_cast<igtlio::StringDevice*>(modifiedDevice);
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
vtkMRMLNode* vtkMRMLIGTLConnectorNode::vtkInternal::GetOrAddMRMLNodeforDevice(igtlio::Device* device)
{
  if (device)
  {
    if (device->GetMessageDirection() != igtlio::Device::MESSAGE_DIRECTION_IN)
    {
      // we are only interested in incomming devices
      return NULL;
    }
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

  // Node not found and add the node
  if (strcmp(device->GetDeviceType().c_str(), "IMAGE") == 0)
  {
    vtkSmartPointer<vtkMRMLVolumeNode> volumeNode;
    int numberOfComponents = 1;
    vtkSmartPointer<vtkImageData> image;
    std::string deviceName = "";
    igtlio::ImageDevice* imageDevice = reinterpret_cast<igtlio::ImageDevice*>(device);
    igtlio::ImageConverter::ContentData content = imageDevice->GetContent();
    numberOfComponents = content.image->GetNumberOfScalarComponents(); //to improve the io module to be able to cope with video data
    image = content.image;
    deviceName = imageDevice->GetDeviceName().c_str();
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
    volumeNode = vtkSmartPointer<vtkMRMLStreamingVolumeNode>::New();
#else
    if (numberOfComponents>1)
    {
      volumeNode = vtkSmartPointer<vtkMRMLVectorVolumeNode>::New();
    }
    else
    {
      volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    }

#endif
    volumeNode->SetAndObserveImageData(image);
    volumeNode->SetName(deviceName.c_str());
    this->External->GetScene()->SaveStateForUndo();
    volumeNode->SetDescription("Received by OpenIGTLink");
    vtkDebugMacro("Name vol node " << volumeNode->GetClassName());
    this->External->GetScene()->AddNode(volumeNode);
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
    vtkMRMLStreamingVolumeNode * tempNode = vtkMRMLStreamingVolumeNode::SafeDownCast(volumeNode);
    vtkIGTLStreamingVolumeCodec* compressionDevice = vtkIGTLStreamingVolumeCodec::New();
    compressionDevice->LinkIGTLIOImageDevice(reinterpret_cast<igtlio::Device*>(imageDevice));
    tempNode->ObserveOutsideCompressionCodec(compressionDevice);
    vtkImageData* imageData = compressionDevice->GetContent()->image.GetPointer();
    vtkImageData* srcImageData = imageDevice->GetContent().image;
    int size[3];
    srcImageData->GetDimensions(size);
    imageData->SetDimensions(size[0], size[1], size[2]);
    imageData->SetExtent(0, size[0] - 1, 0, size[1] - 1, 0, size[2] - 1);
    imageData->SetOrigin(0.0, 0.0, 0.0);
    imageData->SetSpacing(1.0, 1.0, 1.0);
    int numComponents = srcImageData->GetNumberOfScalarComponents();
#if (VTK_MAJOR_VERSION <= 5)
    imageData->SetNumberOfScalarComponents(numComponents);
    imageData->SetScalarType(srcImageData->GetScalarType());
    imageData->AllocateScalars();
#else
    imageData->AllocateScalars(srcImageData->GetScalarType(), numComponents);
#endif
    igtl::ImageMessage::Pointer temImageMsg = igtl::ImageMessage::New();
    int scalarTypeSize = temImageMsg->GetScalarSize(srcImageData->GetScalarType());
    long dataSize = size[0] * size[1] * size[2] * scalarTypeSize*numComponents;
    memcpy(imageData->GetScalarPointer(), srcImageData->GetScalarPointer(), dataSize);
    //videoDevice->GetContent().image = imageData;
#endif
    vtkDebugMacro("Set basic display info");
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
    igtlio::VideoDevice* videoDevice = reinterpret_cast<igtlio::VideoDevice*>(device);
    igtlio::VideoConverter::ContentData content = videoDevice->GetContent();
    int numberOfComponents = content.image->GetNumberOfScalarComponents(); //to improve the io module to be able to cope with video data
    vtkSmartPointer<vtkImageData> image = content.image;
    std::string deviceName = videoDevice->GetDeviceName().c_str();
    this->External->GetScene()->SaveStateForUndo();
    bool scalarDisplayNodeRequired = (numberOfComponents == 1);
    vtkSmartPointer<vtkMRMLStreamingVolumeNode> bitStreamNode = vtkSmartPointer<vtkMRMLStreamingVolumeNode>::New();
    bitStreamNode->SetName(deviceName.c_str());
    bitStreamNode->SetDescription("Received by OpenIGTLink");
    this->External->GetScene()->AddNode(bitStreamNode);
    vtkIGTLStreamingVolumeCodec* compressionDevice = vtkIGTLStreamingVolumeCodec::New();
    compressionDevice->LinkIGTLIOVideoDevice(videoDevice);
    bitStreamNode->ObserveOutsideCompressionCodec(compressionDevice);
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
    bitStreamNode->SetAndObserveDisplayNodeID(displayNode->GetID());
    this->External->RegisterIncomingMRMLNode(bitStreamNode, device);
    return bitStreamNode;
  }
#endif
  else if (strcmp(device->GetDeviceType().c_str(), "STATUS") == 0)
  {
    vtkSmartPointer<vtkMRMLIGTLStatusNode> statusNode;
    statusNode = vtkSmartPointer<vtkMRMLIGTLStatusNode>::New();
    statusNode->SetName(device->GetDeviceName().c_str());
    statusNode->SetDescription("Received by OpenIGTLink");
    this->External->GetScene()->AddNode(statusNode);
    this->External->RegisterIncomingMRMLNode(statusNode, device);
    return statusNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "TRANSFORM") == 0)
  {
    vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode;
    transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    transformNode->SetName(device->GetDeviceName().c_str());
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
    igtlio::PolyDataDevice* polyDevice = reinterpret_cast<igtlio::PolyDataDevice*>(device);
    igtlio::PolyDataConverter::ContentData content = polyDevice->GetContent();

    vtkSmartPointer<vtkMRMLModelNode> modelNode = NULL;
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
    modelNode->SetName(device->GetDeviceName().c_str());
    modelNode->SetDescription("Received by OpenIGTLink");
    this->External->GetScene()->AddNode(modelNode);
    modelNode->SetAndObservePolyData(content.polydata);
    modelNode->CreateDefaultDisplayNodes();
    this->External->RegisterIncomingMRMLNode(modelNode, device);
    return modelNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "STRING") == 0)
  {
    igtlio::StringDevice* modifiedDevice = reinterpret_cast<igtlio::StringDevice*>(device);
    vtkSmartPointer<vtkMRMLTextNode> textNode = vtkSmartPointer<vtkMRMLTextNode>::New();
    textNode->SetEncoding(modifiedDevice->GetContent().encoding);
    textNode->SetText(modifiedDevice->GetContent().string_msg.c_str());
    textNode->SetName(device->GetDeviceName().c_str());
    this->External->RegisterIncomingMRMLNode(textNode, device);
    return textNode;
  }
  return NULL;
}

igtlio::CommandDevicePointer vtkMRMLIGTLConnectorNode::vtkInternal::SendCommand(std::string device_id, std::string command, std::string content, igtlio::SYNCHRONIZATION_TYPE synchronized, double timeout_s)
{
  igtlio::CommandDevicePointer device = this->IOConnector->SendCommand(device_id, command, content);

  if (synchronized == igtlio::BLOCKING)
  {
    double starttime = vtkTimerLog::GetUniversalTime();
    while (vtkTimerLog::GetUniversalTime() - starttime < timeout_s)
    {
      this->IOConnector->PeriodicProcess();
      vtksys::SystemTools::Delay(5);

      igtlio::CommandDevicePointer response = device->GetResponseFromCommandID(device->GetContent().id);

      if (response.GetPointer())
      {
        return response;
      }
    }
  }
  else
  {
    return device;
  }
  return vtkSmartPointer<igtlio::CommandDevice>();
}

//----------------------------------------------------------------------------
igtlio::CommandDevicePointer vtkMRMLIGTLConnectorNode::vtkInternal::SendCommandResponse(std::string device_id, std::string command, std::string content)
{
  igtlio::DeviceKeyType key(igtlio::CommandConverter::GetIGTLTypeName(), device_id);
  igtlio::CommandDevicePointer device = igtlio::CommandDevice::SafeDownCast(this->IOConnector->GetDevice(key));

  igtlio::CommandConverter::ContentData contentdata = device->GetContent();

  if (command != contentdata.name)
  {
    vtkGenericWarningMacro("Requested command response " << command << " does not match the existing query: " << contentdata.name);
    return igtlio::CommandDevicePointer();
  }

  contentdata.name = command;
  contentdata.content = content;
  device->SetContent(contentdata);

  this->IOConnector->SendMessage(CreateDeviceKey(device), igtlio::Device::MESSAGE_PREFIX_RTS);
  return device;
}


//----------------------------------------------------------------------------
// vtkMRMLIGTLConnectorNode method

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkMRMLIGTLConnectorNode()
{
  this->Internal = new vtkInternal(this);
  this->HideFromEditors = false;
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

void vtkMRMLIGTLConnectorNode::ConnectEvents()
{
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->ConnectedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DisconnectedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->ActivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DeactivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->NewDeviceEvent, this,  &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->RemovedDeviceEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
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


void vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents(vtkObject *caller, unsigned long event, void *callData )
{
  igtlio::Connector* connector = reinterpret_cast<igtlio::Connector*>(caller);
  igtlio::Device* modifiedDevice = reinterpret_cast<igtlio::Device*>(callData);
  if (connector == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }
  int mrmlEvent = -1;
  switch (event)
    {
    case igtlio::Connector::ConnectedEvent: mrmlEvent = ConnectedEvent; break;
    case igtlio::Connector::DisconnectedEvent: mrmlEvent = DisconnectedEvent; break;
    case igtlio::Connector::ActivatedEvent: mrmlEvent = ActivatedEvent; break;
    case igtlio::Connector::DeactivatedEvent: mrmlEvent = DeactivatedEvent; break;
    case 118948: mrmlEvent = ReceiveEvent; break; // deprecated. it was for query response, OpenIGTLinkIO doesn't support query event. it is replaced with command message.
    case igtlio::Connector::NewDeviceEvent: mrmlEvent = NewDeviceEvent; break;
    case igtlio::Connector::DeviceContentModifiedEvent: mrmlEvent = DeviceModifiedEvent; break;
    case igtlio::Connector::RemovedDeviceEvent: mrmlEvent = DeviceModifiedEvent; break;
    case igtlio::Device::CommandReceivedEvent: mrmlEvent = CommandReceivedEvent; break;  // COMMAND device got a query, COMMAND received
    case igtlio::Device::CommandResponseReceivedEvent: mrmlEvent = CommandResponseReceivedEvent; break;  // COMMAND device got a response, RTS_COMMAND received
    }
  if( modifiedDevice != NULL)
    {
    if(static_cast<int>(event)==this->Internal->IOConnector->NewDeviceEvent)
      {
      // no action perform at this stage, wait until the message content in the device is unpacked,
      // As we need the message content data to create mrmlnode.
      // Also the newly added device could also be a outgoing message from IF module
      //this->ProcessNewDeviceEvent(caller, event, callData );
      //modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(),  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
      if (modifiedDevice->MessageDirectionIsIn())
        {
        modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(),  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
        if (modifiedDevice->GetDeviceType().compare(igtlio::CommandConverter::GetIGTLTypeName()) == 0)
          {
          modifiedDevice->AddObserver(modifiedDevice->CommandReceivedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
          modifiedDevice->AddObserver(modifiedDevice->CommandResponseReceivedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
          }
        }
      }
    if(event==modifiedDevice->GetDeviceContentModifiedEvent())
      {
      if (modifiedDevice->MessageDirectionIsIn())
        {
        this->Internal->ProcessIncomingDeviceModifiedEvent(caller, event, modifiedDevice);
        }
      else if (modifiedDevice->MessageDirectionIsOut())
        {
        this->Internal->ProcessOutgoingDeviceModifiedEvent(caller, event, modifiedDevice);
        }
      }
    if(event==modifiedDevice->CommandReceivedEvent || event==modifiedDevice->CommandResponseReceivedEvent)
      {
      this->InvokeEvent(mrmlEvent, modifiedDevice);
      return;
      }
    }
  //propagate the event to the connector property and treeview widgets
  this->InvokeEvent(mrmlEvent);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::WriteXML(ostream& of, int nIndent)
{

  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  switch (this->Internal->IOConnector->GetType())
    {
      case igtlio::Connector::TYPE_SERVER:
      of << " connectorType=\"" << "SERVER" << "\" ";
      break;
    case igtlio::Connector::TYPE_CLIENT:
      of << " connectorType=\"" << "CLIENT" << "\" ";
      of << " serverHostname=\"" << this->Internal->IOConnector->GetServerHostname() << "\" ";
      break;
    default:
      of << " connectorType=\"" << "NOT_DEFINED" << "\" ";
      break;
    }

  of << " serverPort=\"" << this->Internal->IOConnector->GetServerPort() << "\" ";
  of << " persistent=\"" << this->Internal->IOConnector->GetPersistent() << "\" ";
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
  int state = igtlio::Connector::STATE_OFF;
  int persistent = igtlio::Connector::PERSISTENT_OFF;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "connectorType"))
      {
      if (!strcmp(attValue, "SERVER"))
        {
        type = igtlio::Connector::TYPE_SERVER;
        }
      else if (!strcmp(attValue, "CLIENT"))
        {
        type = igtlio::Connector::TYPE_CLIENT;
        }
      else
        {
        type = igtlio::Connector::TYPE_NOT_DEFINED;
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
    if (!strcmp(attName, "state"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> state;
      }
    /*if (!strcmp(attName, "logErrorIfServerConnectionFailed"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> LogErrorIfServerConnectionFailed;
      }
    }*/

  switch(type)
    {
    case igtlio::Connector::TYPE_SERVER:
      this->Internal->IOConnector->SetTypeServer(port);
      this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    case igtlio::Connector::TYPE_CLIENT:
      this->Internal->IOConnector->SetTypeClient(serverHostname, port);
      this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    default: // not defined
      // do nothing
      break;
    }

  if (persistent == igtlio::Connector::PERSISTENT_ON)
    {
    this->Internal->IOConnector->SetPersistent(igtlio::Connector::PERSISTENT_ON);
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
      case igtlio::Connector::TYPE_SERVER:
      this->Internal->IOConnector->SetType(igtlio::Connector::TYPE_SERVER);
      this->Internal->IOConnector->SetTypeServer(node->Internal->IOConnector->GetServerPort());
      this->Internal->IOConnector->SetRestrictDeviceName(node->Internal->IOConnector->GetRestrictDeviceName());
      break;
    case igtlio::Connector::TYPE_CLIENT:
      this->Internal->IOConnector->SetType(igtlio::Connector::TYPE_CLIENT);
      this->Internal->IOConnector->SetTypeClient(node->Internal->IOConnector->GetServerHostname(), node->Internal->IOConnector->GetServerPort());
      this->Internal->IOConnector->SetRestrictDeviceName(node->Internal->IOConnector->GetRestrictDeviceName());
      break;
    default: // not defined
      // do nothing
        this->Internal->IOConnector->SetType(igtlio::Connector::TYPE_NOT_DEFINED);
      break;
    }
  this->Internal->IOConnector->SetState(node->Internal->IOConnector->GetState());
  this->Internal->IOConnector->SetPersistent(node->Internal->IOConnector->GetPersistent());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->Internal->IOConnector->GetType() == igtlio::Connector::TYPE_SERVER)
    {
    os << indent << "Connector Type : SERVER\n";
    os << indent << "Listening Port #: " << this->Internal->IOConnector->GetServerPort() << "\n";
    }
  else if (this->Internal->IOConnector->GetType() == igtlio::Connector::TYPE_CLIENT)
    {
    os << indent << "Connector Type: CLIENT\n";
    os << indent << "Server Hostname: " << this->Internal->IOConnector->GetServerHostname() << "\n";
    os << indent << "Server Port #: " << this->Internal->IOConnector->GetServerPort() << "\n";
    }

  switch (this->Internal->IOConnector->GetState())
    {
    case igtlio::Connector::STATE_OFF:
      os << indent << "State: OFF\n";
      break;
    case igtlio::Connector::STATE_WAIT_CONNECTION:
      os << indent << "State: WAIT FOR CONNECTION\n";
      break;
    case igtlio::Connector::STATE_CONNECTED:
      os << indent << "State: CONNECTED\n";
      break;
    }
  os << indent << "Persistent: " << this->Internal->IOConnector->GetPersistent() << "\n";
  os << indent << "Restrict Device Name: " << this->Internal->IOConnector->GetRestrictDeviceName() << "\n";
  os << indent << "Push Outgoing Message Flag: " << this->Internal->IOConnector->GetPushOutgoingMessageFlag() << "\n";
  os << indent << "Check CRC: " << this->Internal->IOConnector->GetCheckCRC()<< "\n";
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
    igtlio::Connector::NodeInfoType nodeInfo;
    //nodeInfo.node = node;
    nodeInfo.lock = 0;
    nodeInfo.second = 0;
    nodeInfo.nanosecond = 0;
    this->Internal->IncomingMRMLNodeInfoMap[node->GetID()] = nodeInfo;
  }
  else
  {
    // Find a converter for the node
    igtlio::DevicePointer device = NULL;
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(node->GetID());
    if (citer == this->Internal->OutgoingMRMLIDToDeviceMap.end())
      {
      igtlio::DeviceKeyType key;
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
        //If no device is found, add a device using the first device type, such as prefer "IMAGE" over "VIDEO".
        device = this->Internal->IOConnector->GetDeviceFactory()->create(deviceTypes[0], key.name);
        device->SetMessageDirection(igtlio::Device::MESSAGE_DIRECTION_OUT);
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
    device->SetMessageDirection(igtlio::Device::MESSAGE_DIRECTION_OUT);
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
      vtkSmartPointer<igtlio::Device> device = citer->second;
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
  igtlio::Device* igtlDevice = static_cast< igtlio::Device* >(device);
  
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
  
  return &this->Internal->IncomingMRMLNodeInfoMap[node->GetID()];
  
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
    igtlio::DevicePointer device = NULL;
    igtlio::DeviceKeyType key;
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
      device->SetMessageDirection(igtlio::Device::MESSAGE_DIRECTION_OUT);
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

  vtkSmartPointer<igtlio::Device> device = iter->second;
  igtlio::DeviceKeyType key;
  key.name = device->GetDeviceName();
  key.type = device->GetDeviceType();
  device->ClearMetaData();
  device->SetMetaDataElement(MEMLNodeNameKey, IANA_TYPE_US_ASCII, node->GetNodeTagName());
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
  igtlio::DeviceKeyType key;
  key.name = node->GetIGTLDeviceName();
  key.type = node->GetIGTLName();
  
  if(this->Internal->IOConnector->GetDevice(key)==NULL)
    {
    igtlio::DeviceCreatorPointer deviceCreator = this->Internal->IOConnector->GetDeviceFactory()->GetCreator(key.GetBaseTypeName());
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
  this->Internal->IOConnector->SendMessage(key, igtlio::Device::MESSAGE_PREFIX_RTS);
  this->QueryQueueMutex->Lock();
  node->SetTimeStamp(vtkTimerLog::GetUniversalTime());
  node->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_WAITING);
  node->SetConnectorNodeID(this->GetID());
  this->QueryWaitingQueue.push_back(node);
  this->QueryQueueMutex->Unlock();
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
  switch (this->Internal->IOConnector->GetState())
  {
  case igtlio::Connector::STATE_OFF: return StateOff;
  case igtlio::Connector::STATE_WAIT_CONNECTION: return StateWaitConnection;
  case igtlio::Connector::STATE_CONNECTED: return StateConnected;
  default:
    return StateOff;
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::GetPersistent()
{
  return this->Internal->IOConnector->GetPersistent() == igtlio::Connector::PERSISTENT_ON;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetPersistent(bool persistent)
{
  this->Internal->IOConnector->SetPersistent(persistent ? igtlio::Connector::PERSISTENT_ON : igtlio::Connector::PERSISTENT_OFF);
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommand(std::string device_id, std::string command, std::string content, bool blocking /*=true*/, double timeout_s /*=5*/)
{
  this->Internal->SendCommand(device_id, command, content, blocking ? igtlio::BLOCKING : igtlio::ASYNCHRONOUS, timeout_s);
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommandResponse(std::string device_id, std::string command, std::string content)
{
  this->Internal->SendCommandResponse(device_id, command, content);
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
  return this->Internal->IOConnector->Start();
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Stop()
{
  return this->Internal->IOConnector->Stop();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PeriodicProcess()
{
  this->Internal->IOConnector->PeriodicProcess();
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::AddDevice(IGTLDevicePointer device)
{
  return this->Internal->IOConnector->AddDevice(static_cast<igtlio::Device*>(device));
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::RemoveDevice(IGTLDevicePointer device)
{
  return this->Internal->IOConnector->RemoveDevice(static_cast<igtlio::Device*>(device));
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
  igtlio::DeviceKeyType key(deviceType, deviceName);
  this->Internal->IOConnector->GetDevice(key);
}
*/

//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::CreateDeviceForOutgoingMRMLNode(vtkMRMLNode* dnode)
{
  vtkSmartPointer<igtlio::Device> device = NULL;
  vtkInternal::MessageDeviceMapType::iterator iter = this->Internal->OutgoingMRMLIDToDeviceMap.find(dnode->GetID());
  if (iter == this->Internal->OutgoingMRMLIDToDeviceMap.end())
  {
    igtlio::DeviceKeyType key;
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
    this->Internal->IOConnector->SetType(igtlio::Connector::TYPE_SERVER);
    break;
  case TypeClient:
    this->Internal->IOConnector->SetType(igtlio::Connector::TYPE_CLIENT); 
    break;
  default:
    this->Internal->IOConnector->SetType(igtlio::Connector::TYPE_NOT_DEFINED);
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetType()
{
  switch (this->Internal->IOConnector->GetType())
  {
  case igtlio::Connector::TYPE_SERVER: return TypeServer;
  case igtlio::Connector::TYPE_CLIENT: return TypeClient;
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
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetServerPort(int port)
{
  this->Internal->IOConnector->SetServerPort(port);
}
