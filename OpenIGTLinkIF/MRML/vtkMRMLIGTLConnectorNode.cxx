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
#include "igtlioImageDevice.h"
#include "igtlioStatusDevice.h"
#include "igtlioTransformDevice.h"
#include "igtlioCommandDevice.h"
#include "igtlioPolyDataDevice.h"
#include "igtlioStringDevice.h"
#include "igtlOSUtil.h"

#if OpenIGTLink_ENABLE_VIDEOSTREAMING
  #include "igtlioVideoDevice.h"
#endif
// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLVolumeNode.h"
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLBitStreamNode.h>
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


//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkMRMLIGTLConnectorNode()
{
  this->HideFromEditors = false;
  this->QueryQueueMutex = vtkMutexLock::New();
  this->MRMLIDToDeviceMap.clear();
  this->IncomingMRMLNodeInfoMap.clear();
  IOConnector = igtlio::Connector::New();
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
  
  this->DeviceTypeToNodeTagMap.clear();
  std::string volumeTags[] = {"VectorVolume", "Volume"};
  this->DeviceTypeToNodeTagMap["IMAGE"] = std::vector<std::string>(volumeTags, volumeTags+2);
  this->DeviceTypeToNodeTagMap["VIDEO"] = std::vector<std::string>(1,"BitStream");
  this->DeviceTypeToNodeTagMap["STATUS"] = std::vector<std::string>(1,"IGTLStatus");
  this->DeviceTypeToNodeTagMap["TRANSFORM"] = std::vector<std::string>(1,"LinearTransform");
  std::string modelTags[] = {"Model", "FiberBundle"};
  this->DeviceTypeToNodeTagMap["POLYDATA"] = std::vector<std::string>(modelTags, modelTags+2);
  this->DeviceTypeToNodeTagMap["STRING"] = std::vector<std::string>(1,"Text");
  
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::~vtkMRMLIGTLConnectorNode()
{
}

void vtkMRMLIGTLConnectorNode::ConnectEvents()
{
  IOConnector->AddObserver(IOConnector->ConnectedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  IOConnector->AddObserver(IOConnector->DisconnectedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  IOConnector->AddObserver(IOConnector->ActivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  IOConnector->AddObserver(IOConnector->DeactivatedEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  IOConnector->AddObserver(IOConnector->NewDeviceEvent, this,  &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  IOConnector->AddObserver(IOConnector->RemovedDeviceEvent,  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
}

std::vector<std::string> vtkMRMLIGTLConnectorNode::GetNodeTagFromDeviceType(const char * deviceType)
{
  DeviceTypeToNodeTagMapType::iterator iter;
  if(this->DeviceTypeToNodeTagMap.find(std::string(deviceType)) != this->DeviceTypeToNodeTagMap.end())
    {
    return this->DeviceTypeToNodeTagMap.find(std::string(deviceType))->second;
    }
  return std::vector<std::string>(0);
}

std::vector<std::string> vtkMRMLIGTLConnectorNode::GetDeviceTypeFromMRMLNodeType(const char* NodeTag)
{
  if(strcmp(NodeTag, "Volume")==0 || strcmp(NodeTag, "VectorVolume")==0)
    {
    std::vector<std::string> DeviceTypes;
    DeviceTypes.push_back("IMAGE");
    DeviceTypes.push_back("VIDEO");
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

vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetOrAddMRMLNodeforDevice(igtlio::Device* device)
{
  if(device)
    {
    if(device->GetMessageDirection() != igtlio::Device::MESSAGE_DIRECTION_IN)
      {
      // we are only interested in incomming devices
      return NULL;
      }
    }
  // Found the node and return the node;
  NodeInfoMapType::iterator inIter;
  for (inIter = this->IncomingMRMLNodeInfoMap.begin();
       inIter != this->IncomingMRMLNodeInfoMap.end();
       inIter ++)
    {
    vtkMRMLNode* node = this->GetScene()->GetNodeByID((inIter->first));
    if(node)
      {
      const std::string deviceType = device->GetDeviceType();
      const std::string deviceName = device->GetDeviceName();
      bool typeMatched = false;
      for (unsigned int i = 0; i < this->GetNodeTagFromDeviceType(deviceType.c_str()).size(); i++)
        {
        const std::string nodeTag = this->GetNodeTagFromDeviceType(deviceType.c_str())[i];
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
  if(strcmp(device->GetDeviceType().c_str(), "IMAGE")==0 )
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
    if (numberOfComponents == 1)
      {
      volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
      }
    else if (numberOfComponents > 1)
      {
      volumeNode = vtkSmartPointer<vtkMRMLVectorVolumeNode>::New();
      }
    volumeNode->SetAndObserveImageData(image);
    volumeNode->SetName(deviceName.c_str());
    Scene->SaveStateForUndo();
    volumeNode->SetDescription("Received by OpenIGTLink");
    vtkDebugMacro("Name vol node "<<volumeNode->GetClassName());
    this->GetScene()->AddNode(volumeNode);
    vtkDebugMacro("Set basic display info");
    bool scalarDisplayNodeRequired = (numberOfComponents==1);
    vtkSmartPointer<vtkMRMLVolumeDisplayNode> displayNode;
    if (scalarDisplayNodeRequired)
      {
      displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
      }
    else
      {
      displayNode = vtkSmartPointer<vtkMRMLVectorVolumeDisplayNode>::New();
      }
    
    this->GetScene()->AddNode(displayNode);
    
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
    this->RegisterIncomingMRMLNode(volumeNode);
    return volumeNode;
    }
#if OpenIGTLink_ENABLE_VIDEOSTREAMING
  else if(strcmp(device->GetDeviceType().c_str(), "VIDEO")==0)
    {
    igtlio::VideoDevice* videoDevice = reinterpret_cast<igtlio::VideoDevice*>(device);
    igtlio::VideoConverter::ContentData content = videoDevice->GetContent();
    int numberOfComponents = content.image->GetNumberOfScalarComponents(); //to improve the io module to be able to cope with video data
    vtkSmartPointer<vtkImageData> image = content.image;
    std::string deviceName = videoDevice->GetDeviceName().c_str();
    Scene->SaveStateForUndo();
    bool scalarDisplayNodeRequired = (numberOfComponents==1);
    vtkMRMLBitStreamNode* bitStreamNode = vtkMRMLBitStreamNode::New();
    bitStreamNode->SetName(deviceName.c_str());
    bitStreamNode->SetDescription("Received by OpenIGTLink");
    Scene->AddNode(bitStreamNode);
    bitStreamNode->ObserveOutsideVideoDevice(reinterpret_cast<igtlio::VideoDevice*>(device));
    vtkSmartPointer<vtkMRMLVolumeDisplayNode> displayNode;
    if (scalarDisplayNodeRequired)
      {
      displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
      }
    else
      {
      displayNode = vtkSmartPointer<vtkMRMLVectorVolumeDisplayNode>::New();
      }
    this->GetScene()->AddNode(displayNode);
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
    this->RegisterIncomingMRMLNode(bitStreamNode);
    return bitStreamNode;
#endif
    }
  else if(strcmp(device->GetDeviceType().c_str(),"STATUS")==0)
    {
    vtkMRMLIGTLStatusNode* statusNode;
    statusNode = vtkMRMLIGTLStatusNode::New();
    statusNode->SetName(device->GetDeviceName().c_str());
    statusNode->SetDescription("Received by OpenIGTLink");
    this->GetScene()->AddNode(statusNode);
    this->RegisterIncomingMRMLNode(statusNode);
    return statusNode;
    }
  else if(strcmp(device->GetDeviceType().c_str(),"TRANSFORM")==0)
    {
    vtkMRMLLinearTransformNode* transformNode;
    transformNode = vtkMRMLLinearTransformNode::New();
    transformNode->SetName(device->GetDeviceName().c_str());
    transformNode->SetDescription("Received by OpenIGTLink");
    
    vtkMatrix4x4* transform = vtkMatrix4x4::New();
    transform->Identity();
    transformNode->ApplyTransformMatrix(transform);
    transform->Delete();
    this->GetScene()->AddNode(transformNode);
    this->RegisterIncomingMRMLNode(transformNode);
    return transformNode;
    }
  else if(strcmp(device->GetDeviceType().c_str(),"POLYDATA")==0)
    {
    igtlio::PolyDataDevice* polyDevice = reinterpret_cast<igtlio::PolyDataDevice*>(device);
    igtlio::PolyDataConverter::ContentData content = polyDevice->GetContent();
    
    vtkMRMLModelNode* modelNode = NULL;    
    std::string mrmlNodeTagName = "";
    if(device->GetMetaDataElement(MEMLNodeNameKey, mrmlNodeTagName))
      {
      std::string className = this->GetScene()->GetClassNameByTag(mrmlNodeTagName.c_str());
      vtkMRMLNode * createdNode = this->GetScene()->CreateNodeByClass(className.c_str());
      if (createdNode)
        {
        modelNode = vtkMRMLModelNode::SafeDownCast(createdNode);
        }
      else
        {
        modelNode = vtkMRMLModelNode::New();
        }
      }
    else
      {
      modelNode = vtkMRMLModelNode::New();
      }
      modelNode->SetName(device->GetDeviceName().c_str());
      modelNode->SetDescription("Received by OpenIGTLink");
      this->GetScene()->AddNode(modelNode);
      modelNode->SetAndObservePolyData(content.polydata);
      modelNode->CreateDefaultDisplayNodes();
      this->RegisterIncomingMRMLNode(modelNode);
      return modelNode;
    }
  else if (strcmp(device->GetDeviceType().c_str(),"STRING")==0)
    {
      igtlio::StringDevice* modifiedDevice = reinterpret_cast<igtlio::StringDevice*>(device);
      vtkMRMLTextNode* textNode =  vtkMRMLTextNode::New();
      textNode->SetEncoding(modifiedDevice->GetContent().encoding);
      textNode->SetText(modifiedDevice->GetContent().string_msg.c_str());
      textNode->SetName(device->GetDeviceName().c_str());
      this->RegisterIncomingMRMLNode(textNode);
      return textNode;
    }
  return NULL;
}

void vtkMRMLIGTLConnectorNode::ProcessNewDeviceEvent(vtkObject *caller, unsigned long event, void *callData )
{
}


void vtkMRMLIGTLConnectorNode::ProcessOutgoingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlio::Device * modifiedDevice)
{
  this->IOConnector->SendMessage(CreateDeviceKey(modifiedDevice), modifiedDevice->MESSAGE_PREFIX_NOT_DEFINED);
}

void vtkMRMLIGTLConnectorNode::ProcessIncomingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlio::Device * modifiedDevice)
{
  vtkMRMLNode* modifiedNode = this->GetOrAddMRMLNodeforDevice(modifiedDevice);
  const std::string deviceType = modifiedDevice->GetDeviceType();
  const std::string deviceName = modifiedDevice->GetDeviceName();
  if (this->GetNodeTagFromDeviceType(deviceType.c_str()).size() > 0)
    {
    if (strcmp(deviceType.c_str(), "IMAGE")==0)
      {
      igtlio::ImageDevice* imageDevice = reinterpret_cast<igtlio::ImageDevice*>(modifiedDevice);
      if (strcmp(modifiedNode->GetName(), deviceName.c_str()) == 0)
        {
        vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(modifiedNode);
        volumeNode->SetIJKToRASMatrix(imageDevice->GetContent().transform);
        volumeNode->SetAndObserveImageData(imageDevice->GetContent().image);
        volumeNode->Modified();
        }
      }
#if OpenIGTLink_ENABLE_VIDEOSTREAMING
    else if (strcmp(deviceType.c_str(), "VIDEO")==0)
      {
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
        vtkSmartPointer<vtkMatrix4x4> transfromMatrix = vtkMatrix4x4::New();
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
  if( modifiedDevice != NULL)
    {
    if(event==IOConnector->NewDeviceEvent)
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
        this->ProcessIncomingDeviceModifiedEvent(caller, event, modifiedDevice);
        }
      else if (modifiedDevice->MessageDirectionIsOut())
        {
        this->ProcessOutgoingDeviceModifiedEvent(caller, event, modifiedDevice);
        }
      }
    if(event==modifiedDevice->CommandReceivedEvent || event==modifiedDevice->CommandResponseReceivedEvent)
      {
      this->InvokeEvent(event, modifiedDevice);
      return;
      }
    }
  //propagate the event to the connector property and treeview widgets
  this->InvokeEvent(event);
}

igtlio::CommandDevicePointer vtkMRMLIGTLConnectorNode::SendCommand(std::string device_id, std::string command, std::string content, igtlio::SYNCHRONIZATION_TYPE synchronized, double timeout_s)
{
  igtlio::CommandDevicePointer device = IOConnector->SendCommand( device_id, command, content );

  if (synchronized==igtlio::BLOCKING)
  {
    double starttime = vtkTimerLog::GetUniversalTime();
    while (vtkTimerLog::GetUniversalTime() - starttime < timeout_s)
    {
      IOConnector->PeriodicProcess();
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


igtlio::CommandDevicePointer vtkMRMLIGTLConnectorNode::SendCommandResponse(std::string device_id, std::string command, std::string content)
{
  igtlio::DeviceKeyType key(igtlio::CommandConverter::GetIGTLTypeName(), device_id);
  igtlio::CommandDevicePointer device = igtlio::CommandDevice::SafeDownCast(IOConnector->GetDevice(key));

  igtlio::CommandConverter::ContentData contentdata = device->GetContent();

  if (command != contentdata.name)
  {
    vtkErrorMacro("Requested command response " << command << " does not match the existing query: " << contentdata.name);
    return igtlio::CommandDevicePointer();
  }

  contentdata.name = command;
  contentdata.content = content;
  device->SetContent(contentdata);

  IOConnector->SendMessage(CreateDeviceKey(device), igtlio::Device::MESSAGE_PREFIX_RTS);
  return device;
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::WriteXML(ostream& of, int nIndent)
{

  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  switch (IOConnector->GetType())
    {
      case igtlio::Connector::TYPE_SERVER:
      of << " connectorType=\"" << "SERVER" << "\" ";
      break;
    case igtlio::Connector::TYPE_CLIENT:
      of << " connectorType=\"" << "CLIENT" << "\" ";
      of << " serverHostname=\"" << IOConnector->GetServerHostname() << "\" ";
      break;
    default:
      of << " connectorType=\"" << "NOT_DEFINED" << "\" ";
      break;
    }

  of << " serverPort=\"" << IOConnector->GetServerPort() << "\" ";
  of << " persistent=\"" << IOConnector->GetPersistent() << "\" ";
  of << " state=\"" << IOConnector->GetState() <<"\"";
  of << " restrictDeviceName=\"" << IOConnector->GetRestrictDeviceName() << "\" ";

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
      IOConnector->SetTypeServer(port);
      IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    case igtlio::Connector::TYPE_CLIENT:
      IOConnector->SetTypeClient(serverHostname, port);
      IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    default: // not defined
      // do nothing
      break;
    }

  if (persistent == igtlio::Connector::PERSISTENT_ON)
    {
    IOConnector->SetPersistent(igtlio::Connector::PERSISTENT_ON);
    // At this point not all the nodes are loaded so we cannot start
    // the processing thread (if we activate the connection then it may immediately
    // start receiving messages, create corresponding MRML nodes, while the same
    // MRML nodes are being loaded from the scene; this would result in duplicate
    // MRML nodes).
    // All the nodes will be activated by the module logic when the scene import is finished.
    IOConnector->SetState(state);
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

  int type = node->IOConnector->GetType();

  switch(type)
    {
      case igtlio::Connector::TYPE_SERVER:
        this->IOConnector->SetType(igtlio::Connector::TYPE_SERVER);
      this->IOConnector->SetTypeServer(node->IOConnector->GetServerPort());
      this->IOConnector->SetRestrictDeviceName(node->IOConnector->GetRestrictDeviceName());
      break;
    case igtlio::Connector::TYPE_CLIENT:
      this->IOConnector->SetType(igtlio::Connector::TYPE_CLIENT);
      this->IOConnector->SetTypeClient(node->IOConnector->GetServerHostname(), node->IOConnector->GetServerPort());
      this->IOConnector->SetRestrictDeviceName(node->IOConnector->GetRestrictDeviceName());
      break;
    default: // not defined
      // do nothing
        this->IOConnector->SetType(igtlio::Connector::TYPE_NOT_DEFINED);
      break;
    }
  this->IOConnector->SetState(node->IOConnector->GetState());
  this->IOConnector->SetPersistent(node->IOConnector->GetPersistent());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  if (this->IOConnector->GetType() == igtlio::Connector::TYPE_SERVER)
    {
    os << indent << "Connector Type : SERVER\n";
    os << indent << "Listening Port #: " << this->IOConnector->GetServerPort() << "\n";
    }
  else if (this->IOConnector->GetType() == igtlio::Connector::TYPE_CLIENT)
    {
    os << indent << "Connector Type: CLIENT\n";
    os << indent << "Server Hostname: " << this->IOConnector->GetServerHostname() << "\n";
    os << indent << "Server Port #: " << this->IOConnector->GetServerPort() << "\n";
    }

  switch (this->IOConnector->GetState())
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
  os << indent << "Persistent: " << this->IOConnector->GetPersistent() << "\n";
  os << indent << "Restrict Device Name: " << this->IOConnector->GetRestrictDeviceName() << "\n";
  os << indent << "Push Outgoing Message Flag: " << this->IOConnector->GetPushOutgoingMessageFlag() << "\n";
  os << indent << "Check CRC: " << this->IOConnector->GetCheckCRC()<< "\n";
  os << indent << "Number of outgoing nodes: " << this->GetNumberOfOutgoingMRMLNodes() << "\n";
  os << indent << "Number of incoming nodes: " << this->GetNumberOfIncomingMRMLNodes() << "\n";
}

//----------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::AssignNodeToDevice(vtkMRMLNode* node, igtlio::DevicePointer device)
{
  unsigned int modifiedEvent = 0;
  if(device->GetDeviceType().compare("IMAGE")==0)
    {
    igtlio::ImageDevice* imageDevice = static_cast<igtlio::ImageDevice*>(device.GetPointer());
    vtkMRMLVolumeNode* imageNode = vtkMRMLVolumeNode::SafeDownCast(node);
    vtkMatrix4x4 *mat = vtkMatrix4x4::New();
    imageNode->GetIJKToRASMatrix(mat);
    igtlio::ImageConverter::ContentData content = {imageNode->GetImageData(), mat};
    imageDevice->SetContent(content);
    modifiedEvent = vtkMRMLVolumeNode::ImageDataModifiedEvent;
    }
  else if(device->GetDeviceType().compare("STATUS")==0)
    {
    igtlio::StatusDevice* statusDevice = static_cast<igtlio::StatusDevice*>(device.GetPointer());

    vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(node);
    igtlio::StatusConverter::ContentData content = {statusNode->GetCode(), statusNode->GetSubCode(), statusNode->GetErrorName(), statusNode->GetStatusString()};
    statusDevice->SetContent(content);
    modifiedEvent = vtkMRMLIGTLStatusNode::StatusModifiedEvent;
    }
  else if(device->GetDeviceType().compare("TRANSFORM")==0)
    {
    igtlio::TransformDevice* transformDevice = static_cast<igtlio::TransformDevice*>(device.GetPointer());
    vtkMatrix4x4 *mat = vtkMatrix4x4::New();
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
    transformNode->GetMatrixTransformToParent(mat);
    igtlio::TransformConverter::ContentData content = {mat, transformNode->GetName()};
    transformDevice->SetContent(content);
    modifiedEvent = vtkMRMLLinearTransformNode::TransformModifiedEvent;
    }
  else if(device->GetDeviceType().compare("POLYDATA") == 0)
    {
    igtlio::PolyDataDevice* polyDevice = static_cast<igtlio::PolyDataDevice*>(device.GetPointer());
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
    igtlio::PolyDataConverter::ContentData content = {modelNode->GetPolyData(), modelNode->GetName()};
    polyDevice->SetContent(content);
    modifiedEvent = vtkMRMLModelNode::MeshModifiedEvent;
    }
  else if(device->GetDeviceType().compare("STRING") == 0)
    {
    igtlio::StringDevice* stringDevice = static_cast<igtlio::StringDevice*>(device.GetPointer());
    vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(node);
    igtlio::StringConverter::ContentData content = {textNode->GetEncoding(), textNode->GetText()};
    stringDevice->SetContent(content);
    modifiedEvent = vtkMRMLTextNode::TextModifiedEvent;
    }
  else if (device->GetDeviceType().compare("COMMAND")==0)
    {
      return 0;
      // Process the modified event from command device.
    }
  return modifiedEvent;
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
    this->IncomingMRMLNodeInfoMap[node->GetID()] = nodeInfo;
  }
  else
  {
    // Find a converter for the node
    igtlio::DevicePointer device = NULL;
    MessageDeviceMapType::iterator citer = this->MRMLIDToDeviceMap.find(node->GetID());
    if (citer == this->MRMLIDToDeviceMap.end())
      {
      igtlio::DeviceKeyType key;
      key.name = node->GetName();
      std::vector<std::string> deviceTypes = GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
      for (int typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
        {
        key.type = deviceTypes[typeIndex];
        device = IOConnector->GetDevice(key);
        if (device != NULL)
          {
          break;
          }
        }
      if(device == NULL)
        {
        //If no device is found, add a device using the first device type, such as prefer "IMAGE" over "VIDEO".
        device = IOConnector->GetDeviceFactory()->create(deviceTypes[0], key.name);
        device->SetMessageDirection(igtlio::Device::MESSAGE_DIRECTION_OUT);
        if (device)
          {
          this->MRMLIDToDeviceMap[node->GetID()] = device;
          IOConnector->AddDevice(device);
          }
        }
      }
    else
      {
      device = this->MRMLIDToDeviceMap[node->GetID()];
      }
    if (!device)
      {
    // TODO: Remove the reference ID?
    return;
      }
    device->SetMessageDirection(igtlio::Device::MESSAGE_DIRECTION_OUT);
    unsigned int NodeModifiedEvent = this->AssignNodeToDevice(node, device);
    device->AddObserver(device->GetDeviceContentModifiedEvent(),  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
        
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
    NodeInfoMapType::iterator iter;
    iter = this->IncomingMRMLNodeInfoMap.find(nodeID);
    if (iter != this->IncomingMRMLNodeInfoMap.end())
      {
      this->IncomingMRMLNodeInfoMap.erase(iter);
      }
    }
  else
    {
    // Search converter from MRMLIDToDeviceMap
    MessageDeviceMapType::iterator citer = this->MRMLIDToDeviceMap.find(nodeID);
    if (citer != this->MRMLIDToDeviceMap.end())
      {
      vtkSmartPointer<igtlio::Device> device = citer->second;
      device->RemoveObserver(device->GetDeviceContentModifiedEvent());
      this->IOConnector->RemoveDevice(device);
      this->MRMLIDToDeviceMap.erase(citer);
      }
    else
      {
      vtkErrorMacro("Node is not found in MRMLIDToDeviceMap: "<<nodeID);
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
igtlio::Connector::NodeInfoType* vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node)
{
  
  if (!node)
  {
    return NULL;
  }
  
  // Check if the node has already been registered.
  if (this->HasNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID()))
  {
    // the node has been already registered.
  }
  else
  {
    this->AddAndObserveNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID());
    this->Modified();
  }
  
  return &this->IncomingMRMLNodeInfoMap[node->GetID()];
  
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
      NodeInfoMapType::iterator iter;
      iter = this->IncomingMRMLNodeInfoMap.find(id);
      if (iter != this->IncomingMRMLNodeInfoMap.end())
      {
        this->IncomingMRMLNodeInfoMap.erase(iter);
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
    for (int typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
      {
      key.type = deviceTypes[typeIndex];
      device = IOConnector->GetDevice(key);
      if (device != NULL)
        {
        break;
        }
      }
    /*--------
    // device should be added in the OnNodeReferenceAdded function already,
    // delete the following lines in the future
    this->MRMLIDToDeviceMap[node->GetName()] = device;
    if (!device)
      {
      device = this->IOConnector->GetDeviceFactory()->create(key.type, key.name);
      device->SetMessageDirection(igtlio::Device::MESSAGE_DIRECTION_OUT);
      this->IOConnector->AddDevice(device);
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
  
  
  MessageDeviceMapType::iterator iter = this->MRMLIDToDeviceMap.find(node->GetID());
  if (iter == this->MRMLIDToDeviceMap.end())
    {
    vtkErrorMacro("Node is not found in MRMLIDToDeviceMap: "<<node->GetID());
    return 0;
    }

  vtkSmartPointer<igtlio::Device> device = iter->second;
  igtlio::DeviceKeyType key;
  key.name = device->GetDeviceName();
  key.type = device->GetDeviceType();
  device->ClearMetaData();
  device->SetMetaDataElement(MEMLNodeNameKey, IANA_TYPE_US_ASCII, node->GetNodeTagName());
  device->RemoveObservers(device->GetDeviceContentModifiedEvent());
  this->AssignNodeToDevice(node, device); // update the device content
  device->AddObserver(device->GetDeviceContentModifiedEvent(),  this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  
  if((strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")!=0))
    {
    this->IOConnector->SendMessage(key); //
    }
  else if(strcmp(node->GetClassName(),"vtkMRMLIGTLQueryNode")==0)
    {
    this->IOConnector->SendMessage(key, device->MESSAGE_PREFIX_RTS);
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
  
  if(this->IOConnector->GetDevice(key)==NULL)
  {
    vtkSmartPointer<igtlio::DeviceCreator> deviceCreator = IOConnector->GetDeviceFactory()->GetCreator(key.GetBaseTypeName());
    this->IOConnector->AddDevice(deviceCreator->Create(key.name));
  }
  this->IOConnector->SendMessage(key, igtlio::Device::MESSAGE_PREFIX_RTS);
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
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: invalid input node");
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
  NodeInfoMapType::iterator iter;
  for (iter = this->IncomingMRMLNodeInfoMap.begin(); iter != this->IncomingMRMLNodeInfoMap.end(); iter++)
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
  NodeInfoMapType::iterator iter;
  for (iter = this->IncomingMRMLNodeInfoMap.begin(); iter != this->IncomingMRMLNodeInfoMap.end(); iter++)
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
  NodeInfoMapType::iterator iter;
  for (iter = this->IncomingMRMLNodeInfoMap.begin(); iter != this->IncomingMRMLNodeInfoMap.end(); iter++)
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


