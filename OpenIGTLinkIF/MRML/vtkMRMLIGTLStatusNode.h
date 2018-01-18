/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

#ifndef __vtkMRMLIGTLStatusNode_h
#define __vtkMRMLIGTLStatusNode_h

// MRML includes
#include "vtkMRMLNode.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLStatusNode : public vtkMRMLNode
{
public:
  static vtkMRMLIGTLStatusNode *New();

  vtkTypeMacro(vtkMRMLIGTLStatusNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  /// 
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  /// 
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  /// 
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "IGTLStatus";};

  /// Set a status code. Return 0 if the code is invalid.
  int SetCode(vtkTypeUInt16 code);
  
  /// Get a status code.
  vtkTypeUInt16 GetCode();  

  /// Set a sub code. Return 0 if the code is invalid.
  int SetSubCode(vtkTypeInt64 code);

  /// Get a sub code.
  vtkTypeInt64 GetSubCode();  

  /// Set an error name. Return 0 if the name is not accpeted.
  int SetErrorName(const char* name);
  
  /// Get an error name.
  const char*  GetErrorName();

  /// Set a status string. Return 0 if the string is not accepted.
  int SetStatusString(const char* name);

  /// Get a message string. 
  const char*  GetStatusString();

  /// Set all status information
  int SetStatus(vtkTypeUInt16 code, vtkTypeUInt16 subcode, const char* errorName, const char* statusString);


  /// Status codes -- see igtl_status.h
  enum {
    STATUS_INVALID             = 0,
    STATUS_OK                  = 1,
    STATUS_UNKNOWN_ERROR       = 2,
    STATUS_PANICK_MODE         = 3,  /* emergency */
    STATUS_NOT_FOUND           = 4,  /* file, configuration, device etc */
    STATUS_ACCESS_DENIED       = 5,
    STATUS_BUSY                = 6,
    STATUS_TIME_OUT            = 7,  /* Time out / Connection lost */
    STATUS_OVERFLOW            = 8,  /* Overflow / Can't be reached */
    STATUS_CHECKSUM_ERROR      = 9,  /* Checksum error */
    STATUS_CONFIG_ERROR        = 10, /* Configuration error */
    STATUS_RESOURCE_ERROR      = 11, /* Not enough resource (memory, storage etc) */
    STATUS_UNKNOWN_INSTRUCTION = 12, /* Illegal/Unknown instruction */
    STATUS_NOT_READY           = 13, /* Device not ready (starting up)*/
    STATUS_MANUAL_MODE         = 14, /* Manual mode (device does not accept commands) */
    STATUS_DISABLED            = 15, /* Device disabled */
    STATUS_NOT_PRESENT         = 16, /* Device not present */
    STATUS_UNKNOWN_VERSION     = 17, /* Device version not known */
    STATUS_HARDWARE_FAILURE    = 18, /* Hardware failure */
    STATUS_SHUT_DOWN           = 19, /* Exiting / shut down in progress */
    STATUS_NUM_TYPES           = 20,
  };


  /// Application specific API (will be moved to child class in the future)
  ///
  enum
  {
    StatusModifiedEvent  = 30000,
  };

protected:
  vtkMRMLIGTLStatusNode();
  ~vtkMRMLIGTLStatusNode();
  vtkMRMLIGTLStatusNode(const vtkMRMLIGTLStatusNode&);
  void operator=(const vtkMRMLIGTLStatusNode&);

private:

  vtkTypeUInt16 Code;
  vtkTypeInt64  SubCode;

  //BTX
  std::string ErrorName;
  std::string StatusString;
  //ETX

};

#endif
