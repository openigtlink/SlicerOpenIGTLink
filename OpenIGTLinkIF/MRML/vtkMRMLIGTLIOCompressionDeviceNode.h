#ifndef __vtkMRMLIGTLIOCompressionDevice_h
#define __vtkMRMLIGTLIOCompressionDevice_h

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkMRMLBitStreamVolumeNode.h>
#include <vtkMRMLCompressionDeviceNode.h>

// OpenIGTLink and OpenIGTLinkIO include
#include "igtlioDevice.h"
#include "igtlioVideoDevice.h"
#include "igtlVideoMessage.h"

/// A Device supporting the compression codec

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLIOCompressionDeviceNode : public vtkMRMLCompressionDeviceNode
{
public:
  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE
  {return "IGTLIOCompressionDevice";};
  
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  ///å
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  void ProcessLinkedDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData );

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  virtual std::string GetDeviceType() const;
  virtual int UncompressedDataFromBitStream(std::string bitStreamData, bool checkCRC);

  virtual std::string GetCompressedBitStreamFromData();

  std::string GetBitStreamFromContentUsingDefaultDevice();

  int LinkIGTLIOVideoDevice(igtlio::Device* device);
  int LinkIGTLIOImageDevice(igtlio::Device* device);

  igtlio::Device* GetLinkedIGTLIODevice(){return this->LinkedDevice;};

  // bool flag  whether compress the image data when it gets updated.
  vtkSetMacro(CompressImageDeviceContent, bool);
  vtkGetMacro(CompressImageDeviceContent, bool);

  void SetDefaultDeviceName(std::string name) {this->DefaultVideoDevice->SetDeviceName(name);};
  std::string GetDefaultDeviceName() {return this->DefaultVideoDevice->GetDeviceName();};

public:
  static vtkMRMLIGTLIOCompressionDeviceNode *New();
  vtkTypeMacro(vtkMRMLIGTLIOCompressionDeviceNode, vtkMRMLCompressionDeviceNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
protected:
  vtkMRMLIGTLIOCompressionDeviceNode();
  ~vtkMRMLIGTLIOCompressionDeviceNode();

private:
  void CopyVideoMessageIntoKeyFrameMSG(igtl::VideoMessage::Pointer keyFrameMsg);
  void CopyVideoMessageIntoFrameMSG(igtl::VideoMessage::Pointer frameMsg);
  igtlio::DevicePointer LinkedDevice;
  igtlio::VideoDevicePointer DefaultVideoDevice;
  bool CompressImageDeviceContent;
};

#endif