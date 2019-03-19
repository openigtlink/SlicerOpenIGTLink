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

#ifndef __vtkIGTLVP9VolumeCodec_h
#define __vtkIGTLVP9VolumeCodec_h

// vtkAddon includes
#include "vtkStreamingVolumeCodec.h"

// MRML includes
#include "vtkMRML.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkObject.h>
#include <vtkUnsignedCharArray.h>

// OpenIGTLink includes
#include <igtlVP9Encoder.h>
#include <igtlVP9Decoder.h>

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLVP9VolumeCodec : public vtkStreamingVolumeCodec
{
public:
  static vtkIGTLVP9VolumeCodec *New();
  virtual vtkStreamingVolumeCodec* CreateCodecInstance();
  vtkTypeMacro(vtkIGTLVP9VolumeCodec, vtkStreamingVolumeCodec);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum VideoFrameType
  {
    FrameKey,
    FrameInterpolated,
  };

  // Supported codec parameters
  std::string GetLosslessEncodingParameter() { return "losslessEncoding"; };
  std::string GetKeyFrameDistanceParameter() { return "keyFrameDistance"; };
  std::string GetBitRateParameter() { return "bitRate"; };
  virtual std::string GetParameterDescription(std::string parameterName);

  virtual std::string GetFourCC() { return "VP90"; };
  

protected:

  igtl::VP9Decoder::Pointer Decoder;
  igtl::VP9Encoder::Pointer Encoder;
  vtkSmartPointer<vtkImageData> YUVImage;

protected:
  vtkIGTLVP9VolumeCodec();
  ~vtkIGTLVP9VolumeCodec();

  virtual bool DecodeFrameInternal(vtkStreamingVolumeFrame* inputFrame, vtkImageData* outputImageData, bool saveDecodedImage = true);
  virtual bool EncodeImageDataInternal(vtkImageData* outputImageData, vtkStreamingVolumeFrame* inputFrame, bool forceKeyFrame);
  virtual bool UpdateParameterInternal(std::string parameterValue, std::string parameterName);

  vtkSmartPointer<vtkStreamingVolumeFrame> LastEncodedFrame;

private:
  vtkIGTLVP9VolumeCodec(const vtkIGTLVP9VolumeCodec&);
  void operator=(const vtkIGTLVP9VolumeCodec&);
};

#endif
