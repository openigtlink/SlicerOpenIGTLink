/*==============================================================================

Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
Queen's University, Kingston, ON, Canada. All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
and was supported through CANARIE's Research Software Program, and Cancer
Care Ontario.

==============================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLVP9VolumeCodec.h"

// OpenIGTLinkIO includes
#include <igtlioVideoConverter.h>

// VTK includes
#include <vtkMatrix4x4.h>

// vtksys includes
#include <vtksys\SystemTools.hxx>

vtkCodecNewMacro(vtkIGTLVP9VolumeCodec);

//---------------------------------------------------------------------------
vtkIGTLVP9VolumeCodec::vtkIGTLVP9VolumeCodec()
  : YUVImage(NULL)
  , LastEncodedFrame(NULL)
{
  this->Encoder = igtl::VP9Encoder::New();
  this->Decoder = igtl::VP9Decoder::New();

  this->AvailiableParameterNames.push_back(this->GetLosslessEncodingParameter());
  this->AvailiableParameterNames.push_back(this->GetKeyFrameDistanceParameter());
  this->AvailiableParameterNames.push_back(this->GetBitRateParameter());

  this->SetParameter(this->GetLosslessEncodingParameter(), "true");
  this->SetParameter(this->GetKeyFrameDistanceParameter(), "50");
}

//---------------------------------------------------------------------------
vtkIGTLVP9VolumeCodec::~vtkIGTLVP9VolumeCodec()
{
}

//---------------------------------------------------------------------------
std::string vtkIGTLVP9VolumeCodec::GetParameterDescription(std::string parameterName)
{
  if (parameterName == this->GetKeyFrameDistanceParameter())
  {
    return "Distance between key frames";
  }

  if (parameterName == this->GetBitRateParameter())
  {
    return "Encoding bitrate";
  }

  if (parameterName == this->GetLosslessEncodingParameter())
  {
    return "Lossless encoding flag";
  }

  return "";
}


//---------------------------------------------------------------------------
bool vtkIGTLVP9VolumeCodec::UpdateParameterInternal(std::string parameterName, std::string parameterValue)
{
  vtkVariant inputParameter = vtkVariant(parameterValue);
  std::string lowerParameterValue = vtksys::SystemTools::LowerCase(parameterValue);

  if (parameterName == this->GetKeyFrameDistanceParameter())
  {
    this->Encoder->SetKeyFrameDistance(inputParameter.ToInt());
    return true;
  }

  if (parameterName == this->GetBitRateParameter())
  {
    this->Encoder->SetRCTaregetBitRate(inputParameter.ToUnsignedInt());
    return true;
  }

  if (parameterName == this->GetLosslessEncodingParameter())
  {
    this->Encoder->SetLosslessLink(lowerParameterValue == "true");
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
bool vtkIGTLVP9VolumeCodec::DecodeFrameInternal(vtkStreamingVolumeFrame* inputFrame, vtkImageData* outputImageData, bool saveDecodedImage/*=true*/)
{
  if (!inputFrame || !outputImageData)
  {
    vtkErrorMacro("Incorrect arguments!");
    return false;
  }

  if (!this->YUVImage)
  {
    this->YUVImage = vtkSmartPointer<vtkImageData>::New();
  }

  int dimensions[3] = { 0,0,0 };
  inputFrame->GetDimensions(dimensions);

  unsigned int numberOfVoxels = dimensions[0] * dimensions[1] * dimensions[2];
  if (numberOfVoxels == 0)
  {
    vtkErrorMacro("Cannot decode frame, number of voxels is zero");
    return false;
  }

  vtkUnsignedCharArray* frameData = inputFrame->GetFrameData();
  void* framePointer = frameData->GetPointer(0);

  this->YUVImage->SetDimensions(dimensions[0], dimensions[1] * 3 / 2, 1); //TODO: ONLY VALID UNLESS GREYSCALE
  this->YUVImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  void* yuvPointer = this->YUVImage->GetScalarPointer();

  outputImageData->SetDimensions(dimensions);
  void* imagePointer = outputImageData->GetScalarPointer();

  igtl_uint64 size = frameData->GetSize() * frameData->GetElementComponentSize();
  igtl_uint32 frameSize[3] = { (igtl_uint32)dimensions[0], (igtl_uint32)dimensions[1], (igtl_uint32)dimensions[2] };

  // Convert compressed frame to YUV image
  this->Decoder->DecodeBitStreamIntoFrame((unsigned char*)framePointer, (igtl_uint8*)yuvPointer, frameSize, size);

  // Convert YUV image to RGB image
  if (saveDecodedImage)
    {
    this->Decoder->ConvertYUVToRGB((igtl_uint8*)yuvPointer, (igtl_uint8*)imagePointer, dimensions[1], dimensions[0]);
    }

  return true;
}

//---------------------------------------------------------------------------
bool vtkIGTLVP9VolumeCodec::EncodeImageDataInternal(vtkImageData* inputImageData, vtkStreamingVolumeFrame* outputFrame, bool forceKeyFrame)
{
  if (!inputImageData || !outputFrame)
  {
    vtkErrorMacro("Incorrect arguments!");
    return false;
  }

  igtl::VideoMessage::Pointer videoMessage = igtl::VideoMessage::New();
  igtlioVideoConverter::HeaderData headerData = igtlioVideoConverter::HeaderData();
  igtlioVideoConverter::ContentData contentData = igtlioVideoConverter::ContentData();
  contentData.videoMessage = videoMessage;
  contentData.image = inputImageData;
  contentData.transform = vtkSmartPointer<vtkMatrix4x4>::New();
  headerData.deviceName = videoMessage->GetDeviceName();

  if (!igtlioVideoConverter::toIGTL(headerData, contentData, this->Encoder))
  {
    return false;
  }

  unsigned char* framePointer = (unsigned char*)videoMessage->GetPackPointer() + IGTL_HEADER_SIZE + IGTL_VIDEO_HEADER_SIZE;
  int bitstreamSize = videoMessage->GetBitStreamSize();
  vtkSmartPointer<vtkUnsignedCharArray> frameData = vtkSmartPointer<vtkUnsignedCharArray>::New();
  frameData->Allocate(bitstreamSize);

  memcpy(frameData->GetPointer(0), framePointer, bitstreamSize);

  int dimensions[3] = { 0,0,0 };
  inputImageData->GetDimensions(dimensions);
  outputFrame->SetDimensions(dimensions);
  outputFrame->SetFrameData(frameData);
  outputFrame->SetCodecFourCC(this->GetFourCC());
  outputFrame->SetFrameType(videoMessage->GetFrameType() == FrameTypeKey ? vtkStreamingVolumeFrame::IFrame : vtkStreamingVolumeFrame::PFrame);

  if (outputFrame->IsKeyFrame())
  {
    outputFrame->SetPreviousFrame(NULL);
  }
  else
  {
    outputFrame->SetPreviousFrame(this->LastEncodedFrame);
  }
  this->LastEncodedFrame = outputFrame;

  return true;
}

//---------------------------------------------------------------------------
void vtkIGTLVP9VolumeCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}