/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLConnectorNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

// OpenIGTLink includes
#include "igtlOSUtil.h"

// OpenIGTLinkIO include
#include <igtlioCommand.h>
#include <igtlioConnector.h>
#include <igtlioDeviceFactory.h>
#include <igtlioImageDevice.h>
#include <igtlioImageMetaDevice.h>
#include <igtlioLabelMetaDevice.h>
#include <igtlioPolyDataDevice.h>
#include <igtlioPointDevice.h>
#include <igtlioStatusDevice.h>
#include <igtlioStringDevice.h>
#include <igtlioTransformDevice.h>
#include <igtlioTrackingDataDevice.h>
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
#include <igtlioVideoDevice.h>
#endif

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLIGTLStatusNode.h"
#include "vtkMRMLImageMetaListNode.h"
#include "vtkMRMLLabelMetaListNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLTextNode.h"
#include "vtkMRMLIGTLTrackingDataBundleNode.h"
#include "vtkSlicerOpenIGTLinkCommand.h"

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
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkImageData.h>
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkMutexLock.h>
#include <vtkPolyData.h>
#include <vtkTimerLog.h>
#include <vtkWeakPointer.h>

// vtksys includes
#include <vtksys/SystemTools.hxx>

// SlicerQt includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>

#define MRMLNodeNameKey "MRMLNodeName"

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

  void ProcessNewDeviceEvent(vtkObject* caller, unsigned long event, void* callData);

  void ProcessIncomingDeviceModifiedEvent(vtkObject* caller, unsigned long event, igtlioDevice* modifiedDevice);

  void ProcessOutgoingDeviceModifiedEvent(vtkObject* caller, unsigned long event, igtlioDevice* modifiedDevice);

  void DeviceAboutToReceiveEvent(igtlioDevice* modifiedDevice);

  /// Called when command responses are received.
  /// Handles igtlioCommandPointer related details on command response.
  /// If there is a corresponding igtlioCommandPointer, an event is invoked on it.
  void ReceiveCommandResponse(igtlioCommandPointer commandDevice);

  vtkMRMLNode* GetMRMLNodeforDevice(igtlioDevice* device);

  /// Find the first relevant query node for a given device in the list of pending queries
  vtkMRMLIGTLQueryNode* GetPendingQueryNodeForDevice(igtlioDevice* device);
  /// Remove the query node from the list of pending queries
  bool RemovePendingQueryNode(vtkMRMLIGTLQueryNode* queryNode);
  /// Remove queries that have timed out from the list of pending queries
  void RemoveExpiredQueries();

public:
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

  typedef std::map<std::string, int> IncomingNodeClientIDMapType;
  IncomingNodeClientIDMapType IncomingNodeClientIDMap;

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
  this->IOConnector->Delete();
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
    vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(node);
    transformNode->GetMatrixTransformToParent(mat);
    igtlioTransformConverter::ContentData content = { mat, transformNode->GetName(), "", "" };
    transformDevice->SetContent(content);
    modifiedEvent = vtkMRMLTransformNode::TransformModifiedEvent;
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
    std::string text = textNode->GetText();
    igtlioStringConverter::ContentData content = { static_cast<unsigned int>(textNode->GetEncoding()), text };
    stringDevice->SetContent(content);
    modifiedEvent = vtkMRMLTextNode::TextModifiedEvent;
  }
  else if (device->GetDeviceType().compare("POINT") == 0)
  {
    igtlioPointDevice* pointDevice = static_cast<igtlioPointDevice*>(device.GetPointer());
    vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);
    igtlioPointConverter::ContentData content;
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetDisplayNode());
    for (int controlPointIndex = 0; controlPointIndex < markupsNode->GetNumberOfControlPoints(); controlPointIndex++)
    {
      igtlioPointConverter::PointElement point;
      point.Name = markupsNode->GetNthControlPointLabel(controlPointIndex);

      bool pointSelected = markupsNode->GetNthControlPointSelected(controlPointIndex);
      point.GroupName = (pointSelected ? "Selected" : "Unselected");
      if (displayNode)
      {
        double* color = (pointSelected ? displayNode->GetSelectedColor() : displayNode->GetColor());
        point.RGBA[0] = int(color[0] * 255);
        point.RGBA[1] = int(color[1] * 255);
        point.RGBA[2] = int(color[2] * 255);
      }
      else
      {
        // No display node, use default generic display colors
        if (pointSelected)
        {
          // red
          point.RGBA[0] = 255;
          point.RGBA[1] = 0;
          point.RGBA[2] = 0;
        }
        else
        {
          // yellow
          point.RGBA[0] = 230;
          point.RGBA[1] = 230;
          point.RGBA[2] = 77;
        }
      }
      point.RGBA[3] = markupsNode->GetNthControlPointVisibility(controlPointIndex) ? 255 : 0;

      double position[3] = { 0.0 };
      markupsNode->GetNthControlPointPosition(controlPointIndex, position);
      point.Position[0] = position[0];
      point.Position[1] = position[1];
      point.Position[2] = position[2];
      point.Radius = 0.0; // TODO: ream from measurement array
      // point.Owner; TODO: set device name of the ower image
      content.PointElements.push_back(point);
    }
    pointDevice->SetContent(content);
    modifiedEvent = vtkMRMLMarkupsNode::PointModifiedEvent;
  }
  else if (device->GetDeviceType().compare("TDATA") == 0)
  {
    igtlioTrackingDataDevice* tdataDevice = static_cast<igtlioTrackingDataDevice*>(device.GetPointer());
    igtlioTrackingDataConverter::ContentData content = tdataDevice->GetContent();
    vtkMRMLIGTLTrackingDataBundleNode* tBundleNode = vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(node);
    if (tBundleNode)
    {
      int nElements = tBundleNode->GetNumberOfTransformNodes();
      for (int i = 0; i < nElements; i++)
      {
        vtkMRMLLinearTransformNode* transformNode = tBundleNode->GetTransformNode(i);
        vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
        transformNode->GetMatrixTransformToParent(mat);
        bool found(false);
        for (auto iter = content.trackingDataElements.begin(); iter != content.trackingDataElements.end(); ++iter)
        {
          if (iter->second.deviceName.compare(transformNode->GetName()) == 0)
          {
            // already exists, update transform
            found = true;
            iter->second.transform = mat;
            break;
          }
        }
        if (!found)
        {
          content.trackingDataElements[static_cast<int>(content.trackingDataElements.size())] = igtlioTrackingDataConverter::ContentEntry(mat, transformNode->GetName(), transformNode->GetName());
        }
      }
    }

    tdataDevice->SetContent(content);
    modifiedEvent = vtkCommand::ModifiedEvent;
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
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), void* vtkNotUsed(callData))
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessOutgoingDeviceModifiedEvent(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlioDevice* modifiedDevice)
{
  this->IOConnector->SendMessage(igtlioDeviceKeyType::CreateDeviceKey(modifiedDevice), modifiedDevice->MESSAGE_PREFIX_NOT_DEFINED);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessIncomingDeviceModifiedEvent(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlioDevice* modifiedDevice)
{
  vtkMRMLNode* modifiedNode = this->GetMRMLNodeforDevice(modifiedDevice);
  if (!modifiedNode)
  {
    // Could not find or add node.
    return;
  }

  int wasModifyingNode = modifiedNode->StartModify();

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
          volumeNode->GetImageData()->SetSpacing(1.0, 1.0, 1.0); // IGTL device sets spacing in image data, do not duplicate with IJKtoRAS spacing
          volumeNode->GetImageData()->SetOrigin(0.0, 0.0, 0.0); // IGTL device sets origin in image data, do not duplicate with IJKtoRAS origin
#if VTK_MAJOR_VERSION >= 9
          vtkSmartPointer<vtkMatrix3x3> mat = vtkSmartPointer<vtkMatrix3x3>::New();
          mat->Identity();
          volumeNode->GetImageData()->SetDirectionMatrix(mat); // IGTL device sets directions in image data, do not duplicate with IJKtoRAS directions
#endif
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
        vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(modifiedNode);
        vtkSmartPointer<vtkMatrix4x4> transfromMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
        transfromMatrix->DeepCopy(transformDevice->GetContent().transform);
        transformNode->SetMatrixTransformToParent(transfromMatrix.GetPointer());
        transformNode->Modified();
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
    else if (strcmp(deviceType.c_str(), "POINT") == 0)
    {
      igtlioPointDevice* pointDevice = reinterpret_cast<igtlioPointDevice*>(modifiedDevice);
      double selectedColor[3] = { 1.0, 1.0, 1.0 };
      double unselectedColor[3] = { 1.0, 1.0, 1.0 };
      bool selectedColorDefined = false;
      bool unselectedColorDefined = false;
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(modifiedNode);
        MRMLNodeModifyBlocker blocker(markupsNode);

        const igtlioPointConverter::PointList& points = pointDevice->GetContent().PointElements;
        // Remove unneeded existing points
        int numberOfPointsToRemove = markupsNode->GetNumberOfControlPoints() - points.size();
        for (int i = 0; i < numberOfPointsToRemove; i++)
        {
          markupsNode->RemoveNthControlPoint(markupsNode->GetNumberOfControlPoints() - 1);
        }
        // Add/update other control points
        for (int controlPointIndex = 0; controlPointIndex < points.size(); controlPointIndex++)
        {
          const igtlioPointConverter::PointElement& point = points[controlPointIndex];
          bool selected = (point.GroupName != "Unselected");

          // Use the first encountered selected/unselected color and opacity for all points
          if (selected && !selectedColorDefined)
          {
            selectedColor[0] = point.RGBA[0] / 255.0;
            selectedColor[1] = point.RGBA[1] / 255.0;
            selectedColor[2] = point.RGBA[2] / 255.0;
            selectedColorDefined = true;
          }
          else if (!selected && !unselectedColorDefined)
          {
            unselectedColor[0] = point.RGBA[0] / 255.0;
            unselectedColor[1] = point.RGBA[1] / 255.0;
            unselectedColor[2] = point.RGBA[2] / 255.0;
            unselectedColorDefined = true;
          }

          if (markupsNode->GetNumberOfControlPoints() <= controlPointIndex)
          {
            markupsNode->AddControlPoint(vtkVector3d(point.Position[0], point.Position[1], point.Position[2]), point.Name);
          }
          else
          {
            markupsNode->SetNthControlPointLabel(controlPointIndex, point.Name);
            markupsNode->SetNthControlPointPosition(controlPointIndex, point.Position[0], point.Position[1], point.Position[2]);
          }
          markupsNode->SetNthControlPointSelected(controlPointIndex, selected);
          markupsNode->SetNthControlPointVisibility(controlPointIndex, point.RGBA[3]>0);
          // Note: we currently do not preserve point.Radius and point.Owner information
        }

        vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetDisplayNode());
        if (displayNode)
        {
          MRMLNodeModifyBlocker blocker(displayNode);
          if (selectedColorDefined)
          {
            displayNode->SetSelectedColor(selectedColor);
          }
          if (unselectedColorDefined)
          {
            displayNode->SetColor(unselectedColor);
          }
        }
      }
    }
    else if (strcmp(deviceType.c_str(), "IMGMETA") == 0)
    {
      igtlioImageMetaDevice* imageMetaDevice = reinterpret_cast<igtlioImageMetaDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLImageMetaListNode* imageMetaNode = vtkMRMLImageMetaListNode::SafeDownCast(modifiedNode);
        imageMetaNode->ClearImageMetaElement();
        igtlioImageMetaConverter::ImageMetaDataList imageMetaList = imageMetaDevice->GetContent().ImageMetaDataElements;
        for (igtlioImageMetaConverter::ImageMetaDataList::iterator imageMetaIt = imageMetaList.begin(); imageMetaIt != imageMetaList.end(); ++imageMetaIt)
        {
          vtkMRMLImageMetaElement imageMetaElement;
          imageMetaElement.DeviceName = imageMetaIt->DeviceName;
          imageMetaElement.Modality = imageMetaIt->Modality;
          imageMetaElement.Name = imageMetaIt->Name;
          imageMetaElement.PatientID = imageMetaIt->PatientID;
          imageMetaElement.PatientName = imageMetaIt->PatientName;
          imageMetaElement.ScalarType = imageMetaIt->ScalarType;
          for (int i = 0; i < 3; ++i)
          {
            imageMetaElement.Size[i] = imageMetaIt->Size[i];
          }
          imageMetaElement.TimeStamp = imageMetaIt->Timestamp;
          imageMetaNode->AddImageMetaElement(imageMetaElement);
        }
      }
    }
    else if (strcmp(deviceType.c_str(), "LBMETA") == 0)
    {
      igtlioLabelMetaDevice* labelMetaDevice = reinterpret_cast<igtlioLabelMetaDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLLabelMetaListNode* labelMetaNode = vtkMRMLLabelMetaListNode::SafeDownCast(modifiedNode);
        labelMetaNode->ClearLabelMetaElement();
        igtlioLabelMetaConverter::LabelMetaDataList labelMetaList = labelMetaDevice->GetContent().LabelMetaDataElements;
        for (igtlioLabelMetaConverter::LabelMetaDataList::iterator labelMetaIt = labelMetaList.begin(); labelMetaIt != labelMetaList.end(); ++labelMetaIt)
        {
          vtkMRMLLabelMetaListNode::LabelMetaElement labelMetaElement;
          labelMetaElement.DeviceName = labelMetaIt->DeviceName;
          labelMetaElement.Label = labelMetaIt->Label;
          labelMetaElement.Name = labelMetaIt->Name;
          labelMetaElement.Owner = labelMetaIt->Owner;
          for (int i = 0; i < 4; ++i)
          {
            labelMetaElement.RGBA[i] = labelMetaIt->RGBA[i];
          }
          for (int i = 0; i < 3; ++i)
          {
            labelMetaElement.Size[i] = labelMetaIt->Size[i];
          }
          labelMetaNode->AddLabelMetaElement(labelMetaElement);
        }
      }
    }
    else if (strcmp(deviceType.c_str(), "TDATA") == 0)
    {
      igtlioTrackingDataDevice* tdataDevice = reinterpret_cast<igtlioTrackingDataDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
      {
        vtkMRMLIGTLTrackingDataBundleNode* tBundleNode = vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(modifiedNode);
        if (tBundleNode)
        {
          int nElements = tBundleNode->GetNumberOfTransformNodes();
          igtlioTrackingDataConverter::ContentData content = tdataDevice->GetContent();
          for (auto iter = content.trackingDataElements.begin(); iter != content.trackingDataElements.end(); ++iter)
          {
            bool found(false);
            vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();

            for (int i = 0; i < nElements; i++)
            {
              vtkMRMLLinearTransformNode* transformNode = tBundleNode->GetTransformNode(i);
              if (iter->second.deviceName.compare(transformNode->GetName()) == 0)
              {
                // already exists, update transform
                found = true;
                mat->DeepCopy(iter->second.transform);
                transformNode->SetMatrixTransformToParent(mat.GetPointer());
                transformNode->Modified();
                break;
              }
            }
            if (!found)
            {
              tBundleNode->UpdateTransformNode(iter->second.deviceName.c_str(), mat, iter->second.type);
            }
          }
        }
      }
    }
  }

  vtkMRMLIGTLQueryNode* queryNode = this->GetPendingQueryNodeForDevice(modifiedDevice);
  if (queryNode && modifiedNode)
  {
    this->RemovePendingQueryNode(queryNode);
    queryNode->SetResponseDataNodeID(modifiedNode->GetID());
    queryNode->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_SUCCESS);
    queryNode->InvokeEvent(vtkMRMLIGTLQueryNode::ResponseEvent);
  }

  // Copy all device metadata to node attributes
  for (igtl::MessageBase::MetaDataMap::const_iterator iter = modifiedDevice->GetMetaData().begin(); iter != modifiedDevice->GetMetaData().end(); ++iter)
  {
    modifiedNode->SetAttribute(iter->first.c_str(), iter->second.second.c_str());
  }

  this->IncomingNodeClientIDMap[modifiedNode->GetName()] = modifiedDevice->GetClientID();
  modifiedNode->EndModify(wasModifyingNode);
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
  if (!this->External->GetScene())
  {
    // No scene to add nodes to.
    return NULL;
  }

  const std::string deviceType = device->GetDeviceType();
  const std::string deviceName = device->GetDeviceName();

  // Found the node and return the node;
  NodeInfoMapType::iterator inIter;
  for (inIter = this->IncomingMRMLNodeInfoMap.begin();
    inIter != this->IncomingMRMLNodeInfoMap.end();
    inIter++)
  {
    vtkMRMLNode* node = this->External->GetScene()->GetNodeByID((inIter->first));
    if (node)
    {
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
    igtlioImageDevice* imageDevice = reinterpret_cast<igtlioImageDevice*>(device);
    igtlioImageConverter::ContentData content = imageDevice->GetContent();
    if (!content.image)
    {
      // Image data has not been set yet
      return NULL;
    }
    numberOfComponents = content.image->GetNumberOfScalarComponents(); //to improve the io module to be able to cope with video data
    image = content.image;
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
      if (numberOfComponents > 1)
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
    if (numberOfComponents > 1)
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
    vtkSmartPointer<vtkMRMLTransformNode> transformNode =
      vtkMRMLTransformNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLTransformNode"));
    if (transformNode)
    {
      this->External->RegisterIncomingMRMLNode(transformNode, device);
      return transformNode;
    }

    transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    transformNode->SetName(deviceName.c_str());
    transformNode->SetDescription("Received by OpenIGTLink");
    vtkNew<vtkMatrix4x4> transform;
    transformNode->SetMatrixTransformToParent(transform);
    this->External->GetScene()->AddNode(transformNode);
    this->External->RegisterIncomingMRMLNode(transformNode, device);
    return transformNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "POLYDATA") == 0)
  {
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
    if (device->GetMetaDataElement(MRMLNodeNameKey, mrmlNodeTagName))
    {
      std::string className = this->External->GetScene()->GetClassNameByTag(mrmlNodeTagName.c_str());
      vtkMRMLNode* createdNode = this->External->GetScene()->CreateNodeByClass(className.c_str());
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
  else if (strcmp(device->GetDeviceType().c_str(), "POINT") == 0)
  {
    vtkSmartPointer<vtkMRMLMarkupsFiducialNode> markupsNode =
      vtkMRMLMarkupsFiducialNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLMarkupsFiducialNode"));
    if (markupsNode)
    {
      this->External->RegisterIncomingMRMLNode(markupsNode, device);
      return markupsNode;
    }
    markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(this->External->GetScene()->AddNewNodeByClass("vtkMRMLMarkupsFiducialNode"));
    markupsNode->SetName(deviceName.c_str());
    markupsNode->SetDescription("Received by OpenIGTLink");
    markupsNode->CreateDefaultDisplayNodes();
    // its contents will be updated later
    this->External->RegisterIncomingMRMLNode(markupsNode, device);
    return markupsNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "IMGMETA") == 0)
  {
    vtkSmartPointer<vtkMRMLImageMetaListNode> imageMetaNode =
      vtkMRMLImageMetaListNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLImageMetaListNode"));
    if (imageMetaNode)
    {
      this->External->RegisterIncomingMRMLNode(imageMetaNode, device);
      return imageMetaNode;
    }

    igtlioImageMetaDevice* imageMetaDevice = reinterpret_cast<igtlioImageMetaDevice*>(device);
    imageMetaNode = vtkSmartPointer<vtkMRMLImageMetaListNode>::New();
    imageMetaNode->SetName(deviceName.c_str());
    imageMetaNode->SetDescription("Received by OpenIGTLink");
    igtlioImageMetaConverter::ImageMetaDataList imageMetaList = imageMetaDevice->GetContent().ImageMetaDataElements;
    for (igtlioImageMetaConverter::ImageMetaDataList::iterator imageMetaIt = imageMetaList.begin(); imageMetaIt != imageMetaList.end(); ++imageMetaIt)
    {
      vtkMRMLImageMetaElement imageMetaElement;
      imageMetaElement.DeviceName = imageMetaIt->DeviceName;
      imageMetaElement.Modality = imageMetaIt->Modality;
      imageMetaElement.Name = imageMetaIt->Name;
      imageMetaElement.PatientID = imageMetaIt->PatientID;
      imageMetaElement.PatientName = imageMetaIt->PatientName;
      imageMetaElement.ScalarType = imageMetaIt->ScalarType;
      for (int i = 0; i < 3; ++i)
      {
        imageMetaElement.Size[i] = imageMetaIt->Size[i];
      }
      imageMetaElement.TimeStamp = imageMetaIt->Timestamp;
      imageMetaNode->AddImageMetaElement(imageMetaElement);
    }
    this->External->GetScene()->AddNode(imageMetaNode);
    this->External->RegisterIncomingMRMLNode(imageMetaNode, device);
    return imageMetaNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "LBMETA") == 0)
  {
    vtkSmartPointer<vtkMRMLLabelMetaListNode> labelMetaNode =
      vtkMRMLLabelMetaListNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLImageMetaListNode"));
    if (labelMetaNode)
    {
      this->External->RegisterIncomingMRMLNode(labelMetaNode, device);
      return labelMetaNode;
    }

    igtlioLabelMetaDevice* labelMetaDevice = reinterpret_cast<igtlioLabelMetaDevice*>(device);
    labelMetaNode = vtkSmartPointer<vtkMRMLLabelMetaListNode>::New();
    labelMetaNode->SetName(deviceName.c_str());
    labelMetaNode->SetDescription("Received by OpenIGTLink");
    igtlioLabelMetaConverter::LabelMetaDataList labelMetaList = labelMetaDevice->GetContent().LabelMetaDataElements;
    for (igtlioLabelMetaConverter::LabelMetaDataList::iterator labelMetaIt = labelMetaList.begin(); labelMetaIt != labelMetaList.end(); ++labelMetaIt)
    {
      vtkMRMLLabelMetaListNode::LabelMetaElement labelMetaElement;
      labelMetaElement.DeviceName = labelMetaIt->DeviceName;
      labelMetaElement.Label = labelMetaIt->Label;
      labelMetaElement.Name = labelMetaIt->Name;
      labelMetaElement.Owner = labelMetaIt->Owner;
      for (int i = 0; i < 4; ++i)
      {
        labelMetaElement.RGBA[i] = labelMetaIt->RGBA[i];
      }
      for (int i = 0; i < 3; ++i)
      {
        labelMetaElement.Size[i] = labelMetaIt->Size[i];
      }
      labelMetaNode->AddLabelMetaElement(labelMetaElement);
    }
    this->External->GetScene()->AddNode(labelMetaNode);
    this->External->RegisterIncomingMRMLNode(labelMetaNode, device);
    return labelMetaNode;
  }
  else if (strcmp(device->GetDeviceType().c_str(), "TDATA") == 0)
  {
    igtlioTrackingDataDevice* tdata = dynamic_cast<igtlioTrackingDataDevice*>(device);
    if (tdata == nullptr)
    {
      vtkErrorWithObjectMacro(this->External, "TDATA message type but not a TDATA device. Cannot process.");
      return NULL;
    }
    auto contentCopy = tdata->GetContent();

    vtkSmartPointer<vtkMRMLIGTLTrackingDataBundleNode> tdatanode =
      vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(this->External->GetScene()->GetFirstNode(deviceName.c_str(), "vtkMRMLIGTLTrackingDataBundleNode"));
    if (tdatanode)
    {
      this->External->RegisterIncomingMRMLNode(tdatanode, device);
      return tdatanode;
    }
    tdatanode = vtkMRMLIGTLTrackingDataBundleNode::New();
    tdatanode->SetName(deviceName.c_str());
    tdatanode->SetDescription("Received by OpenIGTLink");
    for (auto iter = contentCopy.trackingDataElements.cbegin(); iter != contentCopy.trackingDataElements.cend(); ++iter)
    {
      tdatanode->UpdateTransformNode(iter->second.deviceName.c_str(), iter->second.transform, iter->second.type);
    }
    this->External->GetScene()->AddNode(tdatanode);
    this->External->RegisterIncomingMRMLNode(tdatanode, device);
    return tdatanode;
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
vtkMRMLIGTLQueryNode* vtkMRMLIGTLConnectorNode::vtkInternal::GetPendingQueryNodeForDevice(igtlioDevice* device)
{
  vtkMRMLIGTLQueryNode* queryNode = nullptr;
  this->External->QueryQueueMutex->Lock();
  for (QueryListType::iterator igtlQueryIt = this->External->QueryWaitingQueue.begin();
    igtlQueryIt != this->External->QueryWaitingQueue.end(); ++igtlQueryIt)
  {
    vtkMRMLIGTLQueryNode* currentQueryNode = *igtlQueryIt;
    if (currentQueryNode && currentQueryNode->GetIGTLName() == device->GetDeviceType() &&
      (!currentQueryNode->GetIGTLDeviceName() || !currentQueryNode->GetIGTLDeviceName()[0] || currentQueryNode->GetIGTLDeviceName() == device->GetDeviceName()))
    {
      queryNode = currentQueryNode;
      break;
    }
  }
  this->External->QueryQueueMutex->Unlock();

  return queryNode;
}

//----------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::vtkInternal::RemovePendingQueryNode(vtkMRMLIGTLQueryNode* queryNode)
{
  this->External->QueryQueueMutex->Lock();
  QueryListType::iterator queryIt = std::find(this->External->QueryWaitingQueue.begin(), this->External->QueryWaitingQueue.end(), queryNode);
  if (queryIt == this->External->QueryWaitingQueue.end())
  {
    // Could not find query to remove
    return false;
  }
  this->External->QueryWaitingQueue.erase(queryIt);
  this->External->QueryQueueMutex->Unlock();
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::RemoveExpiredQueries()
{
  QueryListType expiredQueries;
  this->External->QueryQueueMutex->Lock();
  expiredQueries.remove(nullptr); // Weak pointers could be set to nullptr if references are removed
  for (QueryListType::iterator queryIt = this->External->QueryWaitingQueue.begin(); queryIt != this->External->QueryWaitingQueue.end(); ++queryIt)
  {
    vtkMRMLIGTLQueryNode* queryNode = *queryIt;
    if (!queryNode)
    {
      continue;
    }

    if (queryNode->GetQueryStatus() != vtkMRMLIGTLQueryNode::STATUS_WAITING)
    {
      // Query is not pending
      expiredQueries.push_back(queryNode);
      continue;
    }

    double timeout = queryNode->GetTimeOut();
    if (timeout <= 0.0)
    {
      // Query has no timeout
      continue;
    }

    double timeElapsed = vtkTimerLog::GetUniversalTime() - queryNode->GetTimeStamp();
    if (timeElapsed > timeout)
    {
      // Query is expired
      queryNode->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_EXPIRED);
      expiredQueries.push_back(queryNode);
    }
  }
  this->External->QueryQueueMutex->Unlock();

  for (QueryListType::iterator queryIt = expiredQueries.begin(); queryIt != expiredQueries.end(); ++queryIt)
  {
    this->RemovePendingQueryNode(*queryIt);
  }
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

  this->IncomingNodeReferenceRole = NULL;
  this->IncomingNodeReferenceMRMLAttributeName = NULL;
  this->OutgoingNodeReferenceRole = NULL;
  this->OutgoingNodeReferenceMRMLAttributeName = NULL;

  this->SetIncomingNodeReferenceRole("incoming");
  this->SetIncomingNodeReferenceMRMLAttributeName("incomingNodeRef");
  this->AddNodeReferenceRole(this->GetIncomingNodeReferenceRole(),
    this->GetIncomingNodeReferenceMRMLAttributeName());

  this->SetOutgoingNodeReferenceRole("outgoing");
  this->SetOutgoingNodeReferenceMRMLAttributeName("outgoingNodeRef");
  this->AddNodeReferenceRole(this->GetOutgoingNodeReferenceRole(),
    this->GetOutgoingNodeReferenceMRMLAttributeName());

  this->OutgoingMessageHeaderVersionMaximum = -1;

  this->Internal->DeviceTypeToNodeTagMap.clear();
  std::string volumeTags[] = { "Volume", "VectorVolume", "StreamingVolume" };
  this->Internal->DeviceTypeToNodeTagMap["IMAGE"] = std::vector<std::string>(volumeTags, volumeTags + 3);
  this->Internal->DeviceTypeToNodeTagMap["VIDEO"] = std::vector<std::string>(1, "StreamingVolume");
  this->Internal->DeviceTypeToNodeTagMap["STATUS"] = std::vector<std::string>(1, "IGTLStatus");
  this->Internal->DeviceTypeToNodeTagMap["TRANSFORM"] = std::vector<std::string>(1, "LinearTransform");
  std::string modelTags[] = { "Model", "FiberBundle" };
  this->Internal->DeviceTypeToNodeTagMap["POLYDATA"] = std::vector<std::string>(modelTags, modelTags + 2);
  this->Internal->DeviceTypeToNodeTagMap["STRING"] = std::vector<std::string>(1, "Text");
  this->Internal->DeviceTypeToNodeTagMap["POINT"] = std::vector<std::string>(1, "MarkupsFiducial");
  this->Internal->DeviceTypeToNodeTagMap["IMGMETA"] = std::vector<std::string>(1, "ImageMetaList");
  this->Internal->DeviceTypeToNodeTagMap["LBMETA"] = std::vector<std::string>(1, "LabelMetaList");
  this->Internal->DeviceTypeToNodeTagMap["TDATA"] = std::vector<std::string>(1, "IGTLTrackingDataSplitter");
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
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DisconnectedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->ActivatedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DeactivatedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->NewDeviceEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->RemovedDeviceEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);

  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandReceivedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandResponseEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandCompletedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkMRMLIGTLConnectorNode::GetNodeTagFromDeviceType(const char* deviceType)
{
  vtkInternal::DeviceTypeToNodeTagMapType::iterator iter;
  if (this->Internal->DeviceTypeToNodeTagMap.find(std::string(deviceType)) != this->Internal->DeviceTypeToNodeTagMap.end())
  {
    return this->Internal->DeviceTypeToNodeTagMap.find(std::string(deviceType))->second;
  }
  return std::vector<std::string>(0);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkMRMLIGTLConnectorNode::GetDeviceTypeFromMRMLNodeType(const char* nodeTag)
{
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  if (strcmp(nodeTag, "StreamingVolume") == 0)
  {
    std::vector<std::string> deviceTypes;
    deviceTypes.push_back("VIDEO");
    deviceTypes.push_back("IMAGE");
    return deviceTypes;
  }
#endif
  if (strcmp(nodeTag, "Volume") == 0 || strcmp(nodeTag, "VectorVolume") == 0)
  {
    std::vector<std::string> deviceTypes;
    deviceTypes.push_back("IMAGE");
    return deviceTypes;
  }
  if (strcmp(nodeTag, "IGTLStatus") == 0)
  {
    return std::vector<std::string>(1, "STATUS");
  }
  if (strcmp(nodeTag, "LinearTransform") == 0)
  {
    return std::vector<std::string>(1, "TRANSFORM");
  }
  if (strcmp(nodeTag, "Model") == 0 || strcmp(nodeTag, "FiberBundle") == 0)
  {
    return std::vector<std::string>(1, "POLYDATA");
  }
  if (strcmp(nodeTag, "Text") == 0)
  {
    return std::vector<std::string>(1, "STRING");
  }
  if (strcmp(nodeTag, "MarkupsFiducial") == 0)
  {
    return std::vector<std::string>(1, "POINT");
  }

  return std::vector<std::string>(0);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessMRMLEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(caller);
  if (!node)
  {
    return;
  }

  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      this->PushNode(node);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents(vtkObject* caller, unsigned long event, void* callData)
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
  case igtlioConnector::ConnectedEvent:
    mrmlEvent = ConnectedEvent;
    break;
  case igtlioConnector::DisconnectedEvent:
    mrmlEvent = DisconnectedEvent;
    break;
  case igtlioConnector::ActivatedEvent:
    mrmlEvent = ActivatedEvent;
    break;
  case igtlioConnector::DeactivatedEvent:
    mrmlEvent = DeactivatedEvent;
    break;
  case igtlioConnector::NewDeviceEvent:
    mrmlEvent = NewDeviceEvent;
    break;
  case igtlioConnector::RemovedDeviceEvent:
    mrmlEvent = DeviceModifiedEvent;
    break;
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

    if (this->OutgoingMessageHeaderVersionMaximum < 0 || this->OutgoingMessageHeaderVersionMaximum >= IGTL_HEADER_VERSION_2)
    {
      statusDevice->SetMetaDataElement("dummy", "dummy"); // existence of metadata makes the IO connector send a header v2 message
    }
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
      modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(), this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);
      modifiedDevice->AddObserver(igtlioDevice::AboutToReceiveEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);
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
    if (!node)
    {
      vtkErrorMacro("PushOnConnect: Could not find outgoing node!");
      continue;
    }
    const char* pushOnConnect = node->GetAttribute("OpenIGTLinkIF.pushOnConnect");
    if (pushOnConnect && strcmp(pushOnConnect, "true") == 0)
    {
      this->PushNode(node);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents(vtkObject* caller, unsigned long event, void* callData)
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
  else if (event == modifiedDevice->GetDeviceContentModifiedEvent())
  {
    mrmlEvent = DeviceModifiedEvent;
    if (modifiedDevice->MessageDirectionIsIn())
    {
      this->Internal->ProcessIncomingDeviceModifiedEvent(caller, event, modifiedDevice);
    }
  }

  if (mrmlEvent > 0)
  {
    this->InvokeEvent(mrmlEvent, modifiedDevice);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents(vtkObject* caller, unsigned long event, void* callData)
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
  case igtlioCommand::CommandReceivedEvent:
    mrmlEvent = CommandReceivedEvent;
    break;  // COMMAND received
  case igtlioCommand::CommandResponseEvent:
    mrmlEvent = CommandResponseReceivedEvent;
    break;  // RTS_COMMAND received
  case igtlioCommand::CommandCompletedEvent:
    mrmlEvent = CommandCompletedEvent;
    break;  // Sent COMMAND did not receive a response before timeout
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
  of << " state=\"" << this->Internal->IOConnector->GetState() << "\"";
  of << " restrictDeviceName=\"" << this->Internal->IOConnector->GetRestrictDeviceName() << "\" ";
  if (this->OutgoingMessageHeaderVersionMaximum > 0)
  {
    of << " outgoingMessageHeaderVersionMaximum=\"" << this->GetOutgoingMessageHeaderVersionMaximum() << "\" ";
  }
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
    if (!strcmp(attName, "outgoingMessageHeaderVersionMaximum"))
    {
      std::stringstream ss;
      ss << attValue;
      int outgoingMessageHeaderVersionMaximum = -1;
      ss >> outgoingMessageHeaderVersionMaximum;
      this->SetOutgoingMessageHeaderVersionMaximum(outgoingMessageHeaderVersionMaximum);
    }

    switch (type)
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
void vtkMRMLIGTLConnectorNode::Copy(vtkMRMLNode* anode)
{
  Superclass::Copy(anode);
  vtkMRMLIGTLConnectorNode* node = (vtkMRMLIGTLConnectorNode*)anode;

  int type = node->Internal->IOConnector->GetType();

  switch (type)
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
  this->Internal->IOConnector->SetPersistent(node->Internal->IOConnector->GetPersistent());
  this->SetCheckCRC(node->GetCheckCRC());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

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
  os << indent << "Check CRC: " << this->GetCheckCRC() << "\n";
  os << indent << "Number of outgoing nodes: " << this->GetNumberOfOutgoingMRMLNodes() << "\n";
  os << indent << "Number of incoming nodes: " << this->GetNumberOfIncomingMRMLNodes() << "\n";
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceAdded(vtkMRMLNodeReference* reference)
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
  else if (strcmp(reference->GetReferenceRole(), this->GetOutgoingNodeReferenceRole()) == 0)
  {
    // Find a converter for the node
    igtlioDevicePointer device = NULL;
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(node->GetID());
    if (citer == this->Internal->OutgoingMRMLIDToDeviceMap.end())
    {
      igtlioDeviceKeyType key;
      key.name = node->GetName();
      std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
      if (deviceTypes.empty())
      {
        vtkErrorMacro("Outgoing node reference is not supported for node type: " << node->GetClassName());
      }

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
        igtlioDeviceFactoryPointer factory = this->Internal->IOConnector->GetDeviceFactory();
        if (factory && !deviceTypes.empty())
        {
          device = factory->create(deviceTypes[0], key.name);
        }
        if (device)
        {
          device->SetMessageDirection(igtlioDevice::MESSAGE_DIRECTION_OUT);
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
      // Could not create outgoing node. Remove the node reference
      int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
      for (int i = 0; i < n; i++)
      {
        const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
        if (strcmp(node->GetID(), id) == 0)
        {
          this->RemoveNthNodeReferenceID(this->OutgoingNodeReferenceRole, i);
          break;
        }
      }
      return;
    }
    device->SetMessageDirection(igtlioDevice::MESSAGE_DIRECTION_OUT);
    unsigned int nodeModifiedEvent = this->Internal->AssignOutGoingNodeToDevice(node, device);
    node->AddObserver(nodeModifiedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);

    // Need to update the events here because observed events are not saved in the scene
    // for each reference and therefore only the role-default event observers are added.
    // Get the correct list of events to observe from the converter and update the reference
    // with that.
    // Without doing this, outgoing transforms are not updated if connectors are set up from
    // a saved scene.
    int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
    for (int i = 0; i < n; i++)
    {
      const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      if (strcmp(node->GetID(), id) == 0)
      {
        vtkSmartPointer<vtkIntArray> nodeEvents = vtkSmartPointer<vtkIntArray>::New();
        nodeEvents->InsertNextValue(nodeModifiedEvent);
        this->SetAndObserveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i,
          node->GetID(), nodeEvents);
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceRemoved(vtkMRMLNodeReference* reference)
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
      vtkErrorMacro("Node is not found in IncomingMRMLIDToDeviceMap: " << nodeID);
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
      vtkErrorMacro("Node is not found in OutgoingMRMLIDToDeviceMap: " << nodeID);
    }
  }
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceModified(vtkMRMLNodeReference* reference)
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
bool vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node)
{
  if (!node)
  {
    vtkErrorMacro("Error registering incoming node. Incoming node is NULL");
    return false;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering incoming node. Node scene is NULL");
    return false;
  }

  igtlioDevicePointer device = NULL;
  igtlioDeviceKeyType key;
  key.name = node->GetName();
  std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
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
  }

  return this->RegisterIncomingMRMLNode(node, device);;
}

//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node, IGTLDevicePointer device)
{
  if (!node)
  {
    vtkErrorMacro("Error registering incoming node. Incoming node is NULL");
    return false;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering incoming node. Node scene is NULL");
    return false;
  }

  igtlioDevice* igtlDevice = static_cast<igtlioDevice*>(device);

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
    vtkErrorMacro("Error registering incoming node. Incoming node is NULL");
    return;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering incoming node. Node scene is NULL");
    return;
  }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole());
  for (int i = 0; i < n; i++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      // Already on the list. Remove it.
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
        vtkErrorMacro("Node is not found in IncomingMRMLIDToDeviceMap: " << node->GetID());
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
    {
      return NULL;
    }
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
    vtkErrorMacro("Error registering outgoing node. Incoming node is NULL");
    return 0;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering outgoing node. Node scene is NULL");
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
  for (int i = 0; i < n; i++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
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
    std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
    for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
    {
      key.type = deviceTypes[typeIndex];
      device = this->Internal->IOConnector->GetDevice(key);
      if (device != NULL)
      {
        break;
      }
    }
    this->Modified();

    return 1;
  }
  else // If the reference node wasn't associated with any device, delete the reference
  {
    int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
    for (int i = 0; i < n; i++)
    {
      const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
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
  for (int i = 0; i < n; i++)
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
    {
      return NULL;
    }
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

  if (node->IsA("vtkMRMLIGTLQueryNode"))
  {
    return this->PushQuery(vtkMRMLIGTLQueryNode::SafeDownCast(node));
  }

  vtkInternal::MessageDeviceMapType::iterator iter = this->Internal->OutgoingMRMLIDToDeviceMap.find(node->GetID());
  if (iter == this->Internal->OutgoingMRMLIDToDeviceMap.end())
  {
    vtkErrorMacro("Node is not found in OutgoingMRMLIDToDeviceMap: " << node->GetID());
    return 0;
  }

  vtkSmartPointer<igtlioDevice> device = iter->second;
  igtlioDeviceKeyType key;
  key.name = device->GetDeviceName();
  key.type = device->GetDeviceType();
  device->ClearMetaData();
  if (this->OutgoingMessageHeaderVersionMaximum < 0 || this->OutgoingMessageHeaderVersionMaximum >= IGTL_HEADER_VERSION_2)
  {
    device->SetMetaDataElement(MRMLNodeNameKey, IANA_TYPE_US_ASCII, node->GetNodeTagName());
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
  }

  device->RemoveObservers(device->GetDeviceContentModifiedEvent());
  this->Internal->AssignOutGoingNodeToDevice(node, device); // update the device content
  device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);


  int incomingClientID = -1;
  vtkInternal::IncomingNodeClientIDMapType::iterator incomingClientIDIt = this->Internal->IncomingNodeClientIDMap.find(node->GetName());
  if (incomingClientIDIt != this->Internal->IncomingNodeClientIDMap.end())
  {
    incomingClientID = incomingClientIDIt->second;
  }

  // Send the node to all connected clients
  std::vector<int> clientIDs = this->Internal->IOConnector->GetClientIds();
  for (std::vector<int>::iterator clientIDIt = clientIDs.begin(); clientIDIt != clientIDs.end(); ++clientIDIt)
  {
    int clientID = *clientIDIt;
    if (clientID == incomingClientID)
    {
      // The message was originally received from this client.
      // We don't need to send it back.
      continue;
    }

    if ((strcmp(node->GetClassName(), "vtkMRMLIGTLQueryNode") != 0))
    {
      this->Internal->IOConnector->SendMessage(key, igtlioDevice::MESSAGE_PREFIX_NOT_DEFINED, clientID);
    }
    else if (strcmp(node->GetClassName(), "vtkMRMLIGTLQueryNode") == 0)
    {
      this->Internal->IOConnector->SendMessage(key, device->MESSAGE_PREFIX_RTS, clientID);
    }
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::PushQuery(vtkMRMLIGTLQueryNode* node)
{
  if (node == NULL)
  {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: invalid input node");
    return 1;
  }

  igtlioDeviceKeyType key;
  key.name = node->GetIGTLDeviceName();
  key.type = node->GetIGTLName();

  if (this->Internal->IOConnector->GetDevice(key) == NULL)
  {
    igtlioDeviceCreatorPointer deviceCreator = this->Internal->IOConnector->GetDeviceFactory()->GetCreator(key.GetBaseTypeName());
    if (deviceCreator.GetPointer())
    {
      this->Internal->IOConnector->AddDevice(deviceCreator->Create(key.name));
    }
    else
    {
      vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: Device type not supported");
      return 1;
    }
  }

  igtlioDevice::MESSAGE_PREFIX prefix = igtlioDevice::MESSAGE_PREFIX_NOT_DEFINED;
  int nodeQueryType = node->GetQueryType();
  if (nodeQueryType == vtkMRMLIGTLQueryNode::TYPE_GET)
  {
    prefix = igtlioDevice::MESSAGE_PREFIX_GET;
  }
  else if (nodeQueryType == vtkMRMLIGTLQueryNode::TYPE_START)
  {
    prefix = igtlioDevice::MESSAGE_PREFIX_START;
  }
  else if (nodeQueryType == vtkMRMLIGTLQueryNode::TYPE_STOP)
  {
    prefix = igtlioDevice::MESSAGE_PREFIX_STOP;
  }

  this->Internal->IOConnector->SendMessage(key, prefix);
  this->QueryQueueMutex->Lock();
  node->SetTimeStamp(vtkTimerLog::GetUniversalTime());
  node->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_WAITING);
  node->SetConnectorNodeID(this->GetID());
  this->QueryWaitingQueue.push_back(node);
  this->QueryQueueMutex->Unlock();
  return 0;
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
  if (node == NULL)
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
  case igtlioConnector::STATE_OFF:
    return StateOff;
  case igtlioConnector::STATE_WAIT_CONNECTION:
    return StateWaitConnection;
  case igtlioConnector::STATE_CONNECTED:
    return StateConnected;
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
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::SendCommand failed: Command is already in progress!");
    return;
  }

  this->Internal->SendCommand(command);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommand(vtkSlicerOpenIGTLinkCommand* command)
{
  this->SendCommand(command->GetCommand());
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

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommandResponse(vtkSlicerOpenIGTLinkCommand* command)
{
  this->SendCommandResponse(command->GetCommand());
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
    if (wasModifying.Node->GetName())
    {
      //this->Internal->IncomingNodeClientIDMap[wasModifying.Node->GetName()] = wasModifying.Node->;
    }
    wasModifying.Node->EndModify(wasModifying.Modifying);
    if (wasModifying.Node->GetName())
    {
      this->Internal->IncomingNodeClientIDMap[wasModifying.Node->GetName()] = -1;
    }
    this->Internal->PendingNodeModifications.pop_back();
  }

  this->Internal->RemoveExpiredQueries();

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
  case igtlioConnector::TYPE_SERVER:
    return TypeServer;
  case igtlioConnector::TYPE_CLIENT:
    return TypeClient;
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
