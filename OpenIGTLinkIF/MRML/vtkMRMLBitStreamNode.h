#ifndef __vtkMRMLBitStreamNode_h
#define __vtkMRMLBitStreamNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkMRMLIGTLConnectorNode.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include "vtkMRMLVectorVolumeDisplayNode.h"
#include "vtkMRMLVectorVolumeNode.h"
#include "vtkMRMLVolumeArchetypeStorageNode.h"

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
  
  
  
  void SetUpVideoDeviceByName(const char* name);
  
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
         
  void SetIsCopied(bool copied){this->IsCopied = copied;};
  vtkGetMacro(IsCopied, bool);
   
  void SetMessageValid(bool value)
  {
  MessageBufferValid = value;
  };
  
  bool GetMessageValid()
  {
  return MessageBufferValid;
  };
  
  /// Return a default file extension for writting
  //virtual const char* GetDefaultWriteFileExtension();
  
  std::string GetCodecName(){return this->codecName;};
  
  void SetCodecName(std::string name){ this->codecName=name;};

  IGTLDevicePointer GetVideoMessageDevice();

  int ObserveOutsideVideoDevice(IGTLDevicePointer devicePtr);
  
protected:
  vtkMRMLBitStreamNode();
  ~vtkMRMLBitStreamNode();
  vtkMRMLBitStreamNode(const vtkMRMLBitStreamNode&);
  void operator=(const vtkMRMLBitStreamNode&);
    
  bool isKeyFrameReceived;
  
  bool isKeyFrameUpdated;
  
  bool isKeyFrameDecoded;
  
  bool IsCopied;
  
  std::string codecName;

  bool MessageBufferValid;

private:
  class vtkInternal;
  vtkInternal * Internal;
};

#endif