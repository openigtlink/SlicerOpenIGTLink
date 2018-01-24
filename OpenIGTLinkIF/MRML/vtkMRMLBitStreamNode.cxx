#include "vtkObjectFactory.h"
#include "vtkMRMLBitStreamNode.h"
#include "vtkXMLUtilities.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLNRRDStorageNode.h"
// VTK includes
#include <vtkNew.h>
#include <vtkCollection.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBitStreamNode);

//-----------------------------------------------------------------------------
vtkMRMLBitStreamNode::vtkMRMLBitStreamNode()
{
  videoDevice = NULL;
  codecName = "";
  MessageBuffer = igtl::VideoMessage::New();
  MessageBuffer->InitPack();
  MessageBufferValid = false;
  KeyFrameBuffer = igtl::VideoMessage::New();
  KeyFrameBuffer->InitPack();
  IsCopied = false;
  isKeyFrameReceived = false;
  isKeyFrameDecoded = false;
  isKeyFrameUpdated = false;
  ImageMessageBuffer = igtl::ImageMessage::New();
  ImageMessageBuffer->InitPack();
}

//-----------------------------------------------------------------------------
vtkMRMLBitStreamNode::~vtkMRMLBitStreamNode()
{
}

void vtkMRMLBitStreamNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents(caller, event, callData);
}

void vtkMRMLBitStreamNode::ProcessDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData )
{
  igtlio::VideoDevice* modifiedDevice = reinterpret_cast<igtlio::VideoDevice*>(caller);
  if (modifiedDevice == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }
  if (event != igtlio::VideoDevice::VideoModifiedEvent)
    {
    return;
    }
  igtl::VideoMessage::Pointer videoMsg = modifiedDevice->GetCompressedIGTLMessage();
  igtl_header* h = (igtl_header*) videoMsg->GetPackPointer();
  igtl_header_convert_byte_order(h);
  this->SetIsCopied(false);
  this->SetKeyFrameUpdated(false);
  this->codecName = modifiedDevice->GetCurrentCodecType();
  this->SetMessageStream(videoMsg);
  if(!this->GetKeyFrameReceivedFlag() || modifiedDevice->GetContent().keyFrameUpdated)
    {
    igtl::VideoMessage::Pointer keyFrameMsg = modifiedDevice->GetKeyFrameMessage();
    igtl_header* h = (igtl_header*) keyFrameMsg->GetPackPointer();
    igtl_header_convert_byte_order(h);
    this->SetKeyFrameStream(keyFrameMsg);
    this->SetKeyFrameReceivedFlag(true);
    this->SetKeyFrameUpdated(true);
    }
  this->Modified();
}

void vtkMRMLBitStreamNode::DecodeMessageStream(igtl::VideoMessage::Pointer videoMessage)
{
  if(this->videoDevice == NULL)
    {
    igtl::MessageHeader::Pointer headerMsg = igtl::MessageHeader::New();
    headerMsg->Copy(videoMessage);
    SetUpVideoDeviceByName(headerMsg->GetDeviceName());
    }
  if(this->GetImageData()!=this->videoDevice->GetContent().image)
    {
    this->SetAndObserveImageData(this->videoDevice->GetContent().image);
    }
  if (this->videoDevice->ReceiveIGTLMessage(static_cast<igtl::MessageBase::Pointer>(videoMessage), false))
    {
    this->Modified();
    }
}

void vtkMRMLBitStreamNode::SetUpVideoDeviceByName(const char* name)
{
  vtkMRMLScene* scene = this->GetScene();
  if(scene)
    {
    this->SetDescription("Received by OpenIGTLink");
    if (!(strcmp(name,"") == 0))
      this->SetName(name);
    //------
    //video device initialization
    videoDevice = igtlio::VideoDevice::New();
    videoDevice->SetDeviceName(this->GetName());
    igtlio::VideoConverter::ContentData contentdata = videoDevice->GetContent();
    contentdata.image =  vtkSmartPointer<vtkImageData>::New();
    videoDevice->SetContent(contentdata);
    this->SetAndObserveImageData(videoDevice->GetContent().image);
    //-------
    }
}

int vtkMRMLBitStreamNode::ObserveOutsideVideoDevice(igtlio::VideoDevice* device)
{
  if (device)
    {
    //vectorVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(node);
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLBitStreamNode::ProcessDeviceModifiedEvents);
    //------
    igtl::VideoMessage::Pointer videoMsg = device->GetCompressedIGTLMessage();
    igtl_header* h = (igtl_header*) videoMsg->GetPackPointer();
    igtl_header_convert_byte_order(h);
    this->SetMessageStream(videoMsg);
    this->codecName = device->GetCurrentCodecType();
    this->SetAndObserveImageData(device->GetContent().image);
    this->videoDevice = device; // should the interal video device point to the external video device?
    //-------
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLBitStreamNode::CreateDefaultStorageNode()
{
  return vtkMRMLNRRDStorageNode::New();
}

