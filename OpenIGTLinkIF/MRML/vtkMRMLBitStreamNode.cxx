#include "vtkMRMLBitStreamNode.h"

// OpenIGTLink includes
#include "igtlioVideoDevice.h"
#include "igtlImageMessage.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLNRRDStorageNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkXMLUtilities.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBitStreamNode);


//---------------------------------------------------------------------------
class vtkMRMLBitStreamNode::vtkInternal:public vtkObject
{
public:
  //---------------------------------------------------------------------------
  vtkInternal(vtkMRMLBitStreamNode* external);
  ~vtkInternal();

  void SetVideoMessageDevice(igtlio::VideoDevice* inDevice)
  {
    this->videoDevice = inDevice;
  };

  igtlio::VideoDevice* GetVideoMessageDevice()
  {
    return this->videoDevice;
  }

  void SetMessageStream(igtl::VideoMessage::Pointer buffer)
  {
    this->MessageBuffer->Copy(buffer);
    this->External->MessageBufferValid = true;
  };

  igtl::VideoMessage::Pointer GetMessageStreamBuffer()
  {
    igtl::VideoMessage::Pointer returnMSG = igtl::VideoMessage::New();
    returnMSG->Copy(this->MessageBuffer);
    return returnMSG;
  };

  void SetKeyFrameStream(igtl::VideoMessage::Pointer buffer)
  {
    this->KeyFrameBuffer->Copy(buffer);
  };

  igtl::VideoMessage::Pointer GetKeyFrameStream()
  {
    igtl::VideoMessage::Pointer returnMSG = igtl::VideoMessage::New();
    returnMSG->Copy(this->KeyFrameBuffer);
    return returnMSG;
  };

  igtl::ImageMessage::Pointer GetImageMessageBuffer()
  {
    return ImageMessageBuffer;
  };

  int ObserveOutsideVideoDevice(igtlio::VideoDevice* device);

  void DecodeMessageStream(igtl::VideoMessage::Pointer videoMessage);

  vtkMRMLBitStreamNode* External;

  igtl::VideoMessage::Pointer MessageBuffer;
  igtl::VideoMessage::Pointer KeyFrameBuffer;
  igtl::ImageMessage::Pointer ImageMessageBuffer;

  igtlio::VideoDevicePointer videoDevice;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLBitStreamNode::vtkInternal::vtkInternal(vtkMRMLBitStreamNode* external)
  : External(external)
{
  videoDevice = NULL;

  MessageBuffer = igtl::VideoMessage::New();
  MessageBuffer->InitPack();

  KeyFrameBuffer = igtl::VideoMessage::New();
  KeyFrameBuffer->InitPack();

  ImageMessageBuffer = igtl::ImageMessage::New();
  ImageMessageBuffer->InitPack();
}

//---------------------------------------------------------------------------
vtkMRMLBitStreamNode::vtkInternal::~vtkInternal()
{
}

//---------------------------------------------------------------------------
int vtkMRMLBitStreamNode::vtkInternal::ObserveOutsideVideoDevice(igtlio::VideoDevice* device)
{
  if (device)
  {
    //vectorVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(node);
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this->External, &vtkMRMLBitStreamNode::ProcessDeviceModifiedEvents);
    //------
    igtl::VideoMessage::Pointer videoMsg = device->GetCompressedIGTLMessage();
    igtl_header* h = (igtl_header*)videoMsg->GetPackPointer();
    igtl_header_convert_byte_order(h);
    this->SetMessageStream(videoMsg);
    this->External->codecName = device->GetCurrentCodecType();
    this->External->SetAndObserveImageData(device->GetContent().image);
    //this->Internal->videoDevice = device; // should the interal video device point to the external video device?
    //-------
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkMRMLBitStreamNode::vtkInternal::DecodeMessageStream(igtl::VideoMessage::Pointer videoMessage)
{
  if (this->videoDevice == NULL)
  {
    igtl::MessageHeader::Pointer headerMsg = igtl::MessageHeader::New();
    headerMsg->Copy(videoMessage);
    this->External->SetUpVideoDeviceByName(headerMsg->GetDeviceName());
  }
  if (this->External->GetImageData() != this->videoDevice->GetContent().image)
  {
    this->External->SetAndObserveImageData(this->videoDevice->GetContent().image);
  }
  if (this->videoDevice->ReceiveIGTLMessage(static_cast<igtl::MessageBase::Pointer>(videoMessage), false))
  {
    this->External->Modified();
  }
}

//----------------------------------------------------------------------------
// vtkMRMLBitStreamNode methods

//-----------------------------------------------------------------------------
vtkMRMLBitStreamNode::vtkMRMLBitStreamNode()
{
  this->Internal = new vtkInternal(this);
  IsCopied = false;
  isKeyFrameReceived = false;
  isKeyFrameDecoded = false;
  isKeyFrameUpdated = false;
  MessageBufferValid = false;
}

//-----------------------------------------------------------------------------
vtkMRMLBitStreamNode::~vtkMRMLBitStreamNode()
{
  delete this->Internal;
}

void vtkMRMLBitStreamNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents(caller, event, callData);
  this->InvokeEvent(ImageDataModifiedEvent);
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
  this->Internal->SetMessageStream(videoMsg);
  if(!this->GetKeyFrameReceivedFlag() || modifiedDevice->GetContent().keyFrameUpdated)
    {
    igtl::VideoMessage::Pointer keyFrameMsg = modifiedDevice->GetKeyFrameMessage();
    igtl_header* h = (igtl_header*) keyFrameMsg->GetPackPointer();
    igtl_header_convert_byte_order(h);
    this->Internal->SetKeyFrameStream(keyFrameMsg);
    this->SetKeyFrameReceivedFlag(true);
    this->SetKeyFrameUpdated(true);
    }
  this->Modified();
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
    this->Internal->videoDevice = igtlio::VideoDevicePointer::New();
    this->Internal->videoDevice->SetDeviceName(this->GetName());
    igtlio::VideoConverter::ContentData contentdata = this->Internal->videoDevice->GetContent();
    contentdata.image =  vtkSmartPointer<vtkImageData>::New();
    this->Internal->videoDevice->SetContent(contentdata);
    this->SetAndObserveImageData(this->Internal->videoDevice->GetContent().image);
    //-------
    }
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

//----------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLBitStreamNode::GetVideoMessageDevice()
{
  return this->Internal->GetVideoMessageDevice();
}

//----------------------------------------------------------------------------
int vtkMRMLBitStreamNode::ObserveOutsideVideoDevice(IGTLDevicePointer devicePtr)
{
  igtlio::VideoDevice* device = static_cast<igtlio::VideoDevice*>(devicePtr);
  return this->Internal->ObserveOutsideVideoDevice(device);
}