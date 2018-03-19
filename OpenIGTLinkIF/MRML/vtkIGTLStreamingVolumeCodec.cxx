#include "vtkIGTLStreamingVolumeCodec.h"

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
vtkStandardNewMacro(vtkIGTLStreamingVolumeCodec);
//---------------------------------------------------------------------------
vtkIGTLStreamingVolumeCodec::vtkIGTLStreamingVolumeCodec()
{
  this->DefaultVideoDevice = igtlio::VideoDevice::New();
  this->LinkedDevice = NULL;
  this->CompressImageDeviceContent = false;
}

//---------------------------------------------------------------------------
vtkIGTLStreamingVolumeCodec::~vtkIGTLStreamingVolumeCodec()
{
}
//---------------------------------------------------------------------------
std::string vtkIGTLStreamingVolumeCodec::GetDeviceType() const
{
  return "igtlioVideoDevice";
}


//----------------------------------------------------------------------------
void vtkIGTLStreamingVolumeCodec::ProcessLinkedDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData )
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
      this->GetStreamFromContentUsingDefaultDevice(); // inside this line FrameMSG is copied.
      if(DefaultVideoDevice->GetContent().keyFrameUpdated)
        {
        this->CopyVideoMessageIntoKeyFrameMSG(DefaultVideoDevice->GetKeyFrameMessage());
        Content->keyFrameUpdated = true;
        }
      }
    }
    this->InvokeEvent(DeviceModifiedEvent);
}

void vtkIGTLStreamingVolumeCodec::CopyVideoMessageIntoKeyFrameMSG(igtl::VideoMessage::Pointer keyFrameMsg)
{
  igtl_header* h_key = (igtl_header*) keyFrameMsg->GetPackPointer();
  igtl_header_convert_byte_order(h_key);
  char * messageString = new char[keyFrameMsg->GetPackSize()];
  memcpy(messageString, (char*)h_key, IGTL_HEADER_SIZE);
  memcpy(messageString+IGTL_HEADER_SIZE, keyFrameMsg->GetPackBodyPointer(), keyFrameMsg->GetPackSize() - IGTL_HEADER_SIZE);
  Content->keyFrame.resize(keyFrameMsg->GetPackSize());
  Content->keyFrame.assign(messageString, keyFrameMsg->GetPackSize());
}

void vtkIGTLStreamingVolumeCodec::CopyVideoMessageIntoFrameMSG(igtl::VideoMessage::Pointer frameMsg)
{
  igtl_header* h_key = (igtl_header*) frameMsg->GetPackPointer();
  igtl_header_convert_byte_order(h_key);
  char * messageString = new char[frameMsg->GetPackSize()];
  memcpy(messageString, (char*)h_key, IGTL_HEADER_SIZE);
  memcpy(messageString+IGTL_HEADER_SIZE, frameMsg->GetPackBodyPointer(), frameMsg->GetPackSize() - IGTL_HEADER_SIZE);
  Content->frame.resize(frameMsg->GetPackSize());
  Content->frame.assign(messageString, frameMsg->GetPackSize());
}

int vtkIGTLStreamingVolumeCodec::LinkIGTLIOVideoDevice(igtlio::Device* device)
{
  igtlio::VideoDevice* modifiedDevice = reinterpret_cast<igtlio::VideoDevice*>(device);
  this->Content->codecType= std::string(modifiedDevice->GetContent().videoMessage->GetCodecType());
  this->Content->deviceName = std::string(modifiedDevice->GetDeviceName());
  this->Content->keyFrameUpdated = modifiedDevice->GetContent().keyFrameUpdated;
  this->Content->image = modifiedDevice->GetContent().image;
  this->CopyVideoMessageIntoFrameMSG(modifiedDevice->GetCompressedIGTLMessage());
  if(modifiedDevice->GetContent().keyFrameUpdated)
    {
    this->CopyVideoMessageIntoKeyFrameMSG(modifiedDevice->GetKeyFrameMessage());
    Content->keyFrameUpdated = true;
    }
  modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(), this, &vtkIGTLStreamingVolumeCodec::ProcessLinkedDeviceModifiedEvents);
  this->LinkedDevice = device;
  return 0;
}

int vtkIGTLStreamingVolumeCodec::LinkIGTLIOImageDevice(igtlio::Device* device)
{
  igtlio::ImageDevice* modifiedDevice = reinterpret_cast<igtlio::ImageDevice*>(device);
  
  this->Content->codecType = DefaultVideoDevice->GetContent().codecName;
  this->Content->deviceName = std::string(modifiedDevice->GetDeviceName());
  igtlio::VideoConverter::ContentData deviceContent = DefaultVideoDevice->GetContent();
  deviceContent.image = modifiedDevice->GetContent().image;
  DefaultVideoDevice->SetContent(deviceContent);
  this->Content->image = modifiedDevice->GetContent().image;
  std::string frameMessage(this->GetStreamFromContentUsingDefaultDevice()); // in this line FrameMSG is updated.
  this->Content->keyFrame.resize(frameMessage.length());
  this->Content->keyFrame.assign(frameMessage, frameMessage.length());
  Content->keyFrameUpdated = true;
  modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(), this, &vtkIGTLStreamingVolumeCodec::ProcessLinkedDeviceModifiedEvents);
  this->LinkedDevice = device;
  return 0;
}

//---------------------------------------------------------------------------
int vtkIGTLStreamingVolumeCodec::UncompressedDataFromStream(std::string bitStreamData, bool checkCRC)
{
  //To do : use the buffer to update Content.image
   if (this->LinkedDevice == NULL && this->DefaultVideoDevice == NULL)
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
      this->LinkedDevice == NULL ||
      this->LinkedDevice->GetDeviceType().compare(igtlio::ImageConverter::GetIGTLTypeName()) == 0)
    {
    if (this->DefaultVideoDevice->ReceiveIGTLMessage(buffer, checkCRC))
      {
      this->Content->image = DefaultVideoDevice->GetContent().image;
      this->Modified();
      return 1;
      }
    }
  else if (strcmp(headerMsg->GetDeviceType(), igtlio::VideoConverter::GetIGTLTypeName()) == 0)
    {
    igtlio::VideoDevice* videoDevice = reinterpret_cast<igtlio::VideoDevice*>(this->LinkedDevice.GetPointer());
    if (videoDevice)
      {
      if (this->LinkedDevice->ReceiveIGTLMessage(buffer, checkCRC))
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

std::string vtkIGTLStreamingVolumeCodec::GetCompressedStreamFromData()
{
  return this->GetStreamFromContentUsingDefaultDevice();
}

std::string vtkIGTLStreamingVolumeCodec::GetStreamFromContentUsingDefaultDevice()
{
  if (!Content->image)
    {
    vtkWarningMacro("Image is NULL, message not generated.")
    return "";
    }
  igtlio::VideoConverter::ContentData deviceContent = DefaultVideoDevice->GetContent();
  deviceContent.image = Content->image;
  DefaultVideoDevice->SetContent(deviceContent);
  igtl::VideoMessage::Pointer videoMessage = dynamic_pointer_cast<igtl::VideoMessage>(DefaultVideoDevice->GetIGTLMessage());
  if (videoMessage.GetPointer() == NULL)
    {
    vtkWarningMacro("Encoding failed, message not generated.")
    return "";
    }
  this->CopyVideoMessageIntoFrameMSG(videoMessage);
  std::string compressedBitStream(Content->frame);
  return compressedBitStream;
}

//---------------------------------------------------------------------------
void vtkIGTLStreamingVolumeCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Video:\t" <<"\n";
  Content->image->PrintSelf(os, indent.GetNextIndent());
}

