#ifndef __vtkMRMLBitStreamNode_h
#define __vtkMRMLBitStreamNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include "vtkMRMLVectorVolumeDisplayNode.h"
#include "vtkMRMLVectorVolumeNode.h"
#include "vtkMRMLVolumeArchetypeStorageNode.h"
#include "igtlioVideoDevice.h"
//#include "vtkIGTLToMRMLImage.h"

//OpenIGTLink Include
#include "igtlImageMessage.h"

// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>

//static std::string SEQ_BITSTREAM_POSTFIX = "_BitStream";

class  VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLBitStreamNode : public vtkMRMLVectorVolumeNode
{
public:
  
  static vtkMRMLBitStreamNode *New();
  vtkTypeMacro(vtkMRMLBitStreamNode,vtkMRMLVectorVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  
  virtual void ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData ) VTK_OVERRIDE;
  
  void ProcessDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData );
  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;
  
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode() VTK_OVERRIDE;
  
  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;
  
  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;
  
  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE
  {return "BitStream";};
  
  void SetVideoMessageDevice(igtlio::VideoDevice* inDevice)
  {
  this->videoDevice = inDevice;
  };
  
  igtlio::VideoDevice* GetVideoMessageDevice()
  {
  return this->videoDevice;
  }
  
  int ObserveOutsideVideoDevice(igtlio::VideoDevice* device);
  
  void SetUpVideoDeviceByName(const char* name);
  
  void DecodeMessageStream(igtl::VideoMessage::Pointer videoMessage);
  
  void SetKeyFrameReceivedFlag(bool flag)
  {
  this->isKeyFrameReceived = flag;
  };
  
  bool GetKeyFrameReceivedFlag()
  {
  return this->isKeyFrameReceived;
  };
  
  void SetKeyFrameUpdated(bool flag)
  {
  this->isKeyFrameUpdated = flag;
  };
  
  bool GetKeyFrameUpdated()
  {
  return this->isKeyFrameUpdated;
  };
  
  void SetKeyFrameDecoded(bool flag)
  {
  this->isKeyFrameDecoded = flag;
  };
  
  bool GetKeyFrameDecoded()
  {
  return this->isKeyFrameDecoded;
  };
  
  void SetMessageStream(igtl::VideoMessage::Pointer buffer)
  {
  this->MessageBuffer->Copy(buffer);
  this->MessageBufferValid = true;
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
  
  void SetIsCopied(bool copied){this->IsCopied = copied;};
  vtkGetMacro(IsCopied, bool);
  
  igtl::ImageMessage::Pointer GetImageMessageBuffer()
  {
  return ImageMessageBuffer;
  };
  
  void SetMessageValid(bool value)
  {
  MessageBufferValid = value
  ;
  };
  
  bool GetMessageValid()
  {
  return MessageBufferValid;
  };
  
  /// Return a default file extension for writting
  //virtual const char* GetDefaultWriteFileExtension();
  
  std::string GetCodecName(){return this->codecName;};
  
  void SetCodecName(std::string name){ this->codecName=name;};
  
  
protected:
  vtkMRMLBitStreamNode();
  ~vtkMRMLBitStreamNode();
  vtkMRMLBitStreamNode(const vtkMRMLBitStreamNode&);
  void operator=(const vtkMRMLBitStreamNode&);
  
  igtl::VideoMessage::Pointer MessageBuffer;
  igtl::VideoMessage::Pointer KeyFrameBuffer;
  igtl::ImageMessage::Pointer ImageMessageBuffer;
  bool MessageBufferValid;
  
  igtlio::VideoDevice* videoDevice;
  
  bool isKeyFrameReceived;
  
  bool isKeyFrameUpdated;
  
  bool isKeyFrameDecoded;
  
  bool IsCopied;
  
  std::string codecName;
};

#endif