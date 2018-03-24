#ifndef __vtkMRMLIGTLIOCompressionDevice_h
#define __vtkMRMLIGTLIOCompressionDevice_h

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkMRMLStreamingVolumeNode.h>
#include <vtkStreamingVolumeCodec.h>

// OpenIGTLink and OpenIGTLinkIO include
#include "igtlioDevice.h"
#include "igtlioVideoDevice.h"
#include "igtlVideoMessage.h"

/// A Device supporting the compression codec

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLStreamingVolumeCodec : public vtkStreamingVolumeCodec
{
public:

  void ProcessLinkedDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData );

  virtual std::string GetDeviceType() const;
  virtual int UncompressedDataFromStream(vtkSmartPointer<vtkUnsignedCharArray> bitStreamData, bool checkCRC);

  virtual vtkSmartPointer<vtkUnsignedCharArray> GetCompressedStreamFromData();

  vtkSmartPointer<vtkUnsignedCharArray> GetStreamFromContentUsingDefaultDevice();

  int LinkIGTLIOVideoDevice(igtlio::DevicePointer device);
  int LinkIGTLIOImageDevice(igtlio::DevicePointer device);

  igtlio::Device* GetLinkedIGTLIODevice(){return this->LinkedDevice;};

  // bool flag  whether compress the image data when it gets updated.
  vtkSetMacro(CompressImageDeviceContent, bool);
  vtkGetMacro(CompressImageDeviceContent, bool);

  void SetDefaultDeviceName(std::string name) {this->DefaultVideoDevice->SetDeviceName(name);};
  std::string GetDefaultDeviceName() {return this->DefaultVideoDevice->GetDeviceName();};

public:
  static vtkIGTLStreamingVolumeCodec *New();
  vtkTypeMacro(vtkIGTLStreamingVolumeCodec, vtkStreamingVolumeCodec);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
protected:
  vtkIGTLStreamingVolumeCodec();
  ~vtkIGTLStreamingVolumeCodec();

private:
  void CopyVideoMessageIntoKeyFrameMSG(igtl::VideoMessage::Pointer keyFrameMsg);
  void CopyVideoMessageIntoFrameMSG(igtl::VideoMessage::Pointer frameMsg);
  igtlio::DevicePointer LinkedDevice;
  igtlio::VideoDevicePointer DefaultVideoDevice;
  bool CompressImageDeviceContent;
};

#endif