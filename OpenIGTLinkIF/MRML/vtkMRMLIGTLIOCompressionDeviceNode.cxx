#include "vtkMRMLIGTLIOCompressionDeviceNode.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLNRRDStorageNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkXMLUtilities.h>

// OpenIGTLinkIO includes
#include "igtlioVideoDevice.h"
#include "igtlioImageDevice.h"

//---------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLIOCompressionDeviceNode);
//---------------------------------------------------------------------------
vtkMRMLIGTLIOCompressionDeviceNode::vtkMRMLIGTLIOCompressionDeviceNode()
{
  this->defaultVideoDevice = igtlio::VideoDevice::New();
  this->linkedDevice = NULL;
  this->CompressImageDeviceContent = false;
}

//---------------------------------------------------------------------------
vtkMRMLIGTLIOCompressionDeviceNode::~vtkMRMLIGTLIOCompressionDeviceNode()
{
}
//---------------------------------------------------------------------------
std::string vtkMRMLIGTLIOCompressionDeviceNode::GetDeviceType() const
{
  return "igtlioVideoDevice";
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLIOCompressionDeviceNode::ProcessLinkedDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData )
{
  igtlio::Device* modifiedDevice = reinterpret_cast<igtlio::Device*>(caller);
  if (modifiedDevice->GetDeviceType().compare(igtlio::VideoConverter::GetIGTLTypeName()) == 0)
   {
    igtlio::VideoDevice* videoDevice = reinterpret_cast<igtlio::VideoDevice*>(caller);
    this->CopyVideoMessageIntoFrameMSG(videoDevice->GetCompressedIGTLMessage());
    if(videoDevice->GetContent().keyFrameUpdated)
      {
      this->CopyVideoMessageIntoKeyFrameMSG(videoDevice->GetKeyFrameMessage());
      Content->keyFrameUpdated = true;
      }
    }
  else if(modifiedDevice->GetDeviceType().compare(igtlio::ImageConverter::GetIGTLTypeName()) == 0 )
    {
    if (CompressImageDeviceContent)
      {
      igtlio::ImageDevice* imageDevice = reinterpret_cast<igtlio::ImageDevice*>(caller);
      this->Content->deviceName = std::string(imageDevice->GetDeviceName());
      this->Content->image = imageDevice->GetContent().image;
      this->GetBitStreamFromContentUsingDefaultDevice(); // inside this line FrameMSG is copied.
      if(defaultVideoDevice->GetContent().keyFrameUpdated)
        {
        this->CopyVideoMessageIntoKeyFrameMSG(defaultVideoDevice->GetKeyFrameMessage());
        Content->keyFrameUpdated = true;
        }
      }
    }
    this->InvokeEvent(DeviceModifiedEvent);
}

void vtkMRMLIGTLIOCompressionDeviceNode::CopyVideoMessageIntoKeyFrameMSG(igtl::VideoMessage::Pointer keyFrameMsg)
{
  igtl_header* h_key = (igtl_header*) keyFrameMsg->GetPackPointer();
  igtl_header_convert_byte_order(h_key);
  char * messageString = new char[keyFrameMsg->GetPackSize()];
  memcpy(messageString, (char*)h_key, IGTL_HEADER_SIZE);
  memcpy(messageString+IGTL_HEADER_SIZE, keyFrameMsg->GetPackBodyPointer(), keyFrameMsg->GetPackSize() - IGTL_HEADER_SIZE);
  Content->keyFrameMessage.resize(keyFrameMsg->GetPackSize());
  Content->keyFrameMessage.assign(messageString, keyFrameMsg->GetPackSize());
}

void vtkMRMLIGTLIOCompressionDeviceNode::CopyVideoMessageIntoFrameMSG(igtl::VideoMessage::Pointer frameMsg)
{
  igtl_header* h_key = (igtl_header*) frameMsg->GetPackPointer();
  igtl_header_convert_byte_order(h_key);
  char * messageString = new char[frameMsg->GetPackSize()];
  memcpy(messageString, (char*)h_key, IGTL_HEADER_SIZE);
  memcpy(messageString+IGTL_HEADER_SIZE, frameMsg->GetPackBodyPointer(), frameMsg->GetPackSize() - IGTL_HEADER_SIZE);
  Content->frameMessage.resize(frameMsg->GetPackSize());
  Content->frameMessage.assign(messageString, frameMsg->GetPackSize());
}

int vtkMRMLIGTLIOCompressionDeviceNode::LinkIGTLIOVideoDevice(igtlio::Device* device)
{
  igtlio::VideoDevice* modifiedDevice = reinterpret_cast<igtlio::VideoDevice*>(device);
  this->Content->codecName = std::string(modifiedDevice->GetContent().videoMessage->GetCodecType());
  this->Content->deviceName = std::string(modifiedDevice->GetDeviceName());
  this->Content->keyFrameUpdated = modifiedDevice->GetContent().keyFrameUpdated;
  this->Content->image = modifiedDevice->GetContent().image;
  this->CopyVideoMessageIntoFrameMSG(modifiedDevice->GetCompressedIGTLMessage());
  if(modifiedDevice->GetContent().keyFrameUpdated)
    {
    this->CopyVideoMessageIntoKeyFrameMSG(modifiedDevice->GetKeyFrameMessage());
    Content->keyFrameUpdated = true;
    }
  modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(), this, &vtkMRMLIGTLIOCompressionDeviceNode::ProcessLinkedDeviceModifiedEvents);
  this->linkedDevice = device;
  return 0;
}

int vtkMRMLIGTLIOCompressionDeviceNode::LinkIGTLIOImageDevice(igtlio::Device* device)
{
  igtlio::ImageDevice* modifiedDevice = reinterpret_cast<igtlio::ImageDevice*>(device);
  
  this->Content->codecName = defaultVideoDevice->GetContent().codecName;
  this->Content->deviceName = std::string(modifiedDevice->GetDeviceName());
  this->defaultVideoDevice->GetContent().image = modifiedDevice->GetContent().image;
  this->Content->image = modifiedDevice->GetContent().image;
  std::string* frameMessage = this->GetBitStreamFromContentUsingDefaultDevice(); // in this line FrameMSG is updated.
  this->Content->keyFrameMessage.resize(frameMessage->length());
  this->Content->keyFrameMessage.assign(*frameMessage, frameMessage->length());
  Content->keyFrameUpdated = true;
  modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(), this, &vtkMRMLIGTLIOCompressionDeviceNode::ProcessLinkedDeviceModifiedEvents);
  this->linkedDevice = device;
  return 0;
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLIOCompressionDeviceNode::UncompressedDataFromBitStream(std::string bitStreamData, bool checkCRC)
{
  //To do : use the buffer to update Content.image
   if (this->linkedDevice == NULL && this->defaultVideoDevice == NULL)
    {
      vtkWarningMacro("Video Devices are NULL, message not generated.")
      return 0;
    }
  if (bitStreamData.size()<=0)
    {
    vtkWarningMacro("message size equal to zero.")
    return 0;
    }
  igtl::MessageHeader::Pointer headerMsg = igtl::MessageHeader::New();
  headerMsg->InitPack();
  memcpy(headerMsg->GetPackPointer(), bitStreamData.c_str(), headerMsg->GetPackSize());
  headerMsg->Unpack();
  igtl::MessageBase::Pointer buffer = igtl::MessageBase::New();
  buffer->SetMessageHeader(headerMsg);
  buffer->AllocatePack();
  memcpy(buffer->GetPackBodyPointer(), bitStreamData.c_str()+IGTL_HEADER_SIZE, buffer->GetPackBodySize());
  if (strcmp(headerMsg->GetDeviceType(), igtlio::ImageConverter::GetIGTLTypeName()) == 0 ||
      this->linkedDevice == NULL ||
      this->linkedDevice->GetDeviceType().compare(igtlio::ImageConverter::GetIGTLTypeName()) == 0)
    {
    if (this->defaultVideoDevice->ReceiveIGTLMessage(buffer, checkCRC))
      {
      this->Content->image = defaultVideoDevice->GetContent().image;
      this->Modified();
      return 1;
      }
    }
  else if (strcmp(headerMsg->GetDeviceType(), igtlio::VideoConverter::GetIGTLTypeName()) == 0)
    {
    igtlio::VideoDevice* videoDevice = reinterpret_cast<igtlio::VideoDevice*>(this->linkedDevice.GetPointer());
    if (videoDevice)
      {
      if (this->linkedDevice->ReceiveIGTLMessage(buffer, checkCRC))
        {
        this->Content->image = videoDevice->GetContent().image;
        this->Modified();
        return 1;
        }
      }
    }
  return 0;
}


//---------------------------------------------------------------------------
std::string* vtkMRMLIGTLIOCompressionDeviceNode::GetBitStreamFromContentUsingDefaultDevice()
{
if (!Content->image)
  {
  vtkWarningMacro("Video is NULL, message not generated.")
  return NULL;
  }
  defaultVideoDevice->GetContent().image = Content->image;
  igtl::VideoMessage::Pointer videoMessage = dynamic_pointer_cast<igtl::VideoMessage>(defaultVideoDevice->GetIGTLMessage());
  this->CopyVideoMessageIntoFrameMSG(videoMessage);
  std::string* compressedBitStream = new std::string(Content->frameMessage);
  return compressedBitStream;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLIOCompressionDeviceNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Video:\t" <<"\n";
  Content->image->PrintSelf(os, indent.GetNextIndent());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLIOCompressionDeviceNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLIOCompressionDeviceNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLIOCompressionDeviceNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->EndModify(disabledModify);
}



