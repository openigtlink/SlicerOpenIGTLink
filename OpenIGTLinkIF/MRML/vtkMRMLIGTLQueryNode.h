/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLIGTLQueryNode_h
#define __vtkMRMLIGTLQueryNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

// STD includes
#include <string>

class vtkMRMLIGTLConnectorNode;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLQueryNode : public vtkMRMLNode
{
public:

  //----------------------------------------------------------------
  // Constants Definitions
  //----------------------------------------------------------------

  // Events
  enum
  {
    ResponseEvent        = 128940,
  };

  enum
  {
    TYPE_NOT_DEFINED,
    TYPE_GET,
    TYPE_START,
    TYPE_STOP,
    NUM_TYPE,
  };

  enum
  {
    STATUS_NOT_DEFINED,
    STATUS_PREPARED,     // Ready to query
    STATUS_WAITING,      // Waiting for response from server
    STATUS_SUCCESS,      // Server accepted query successfuly
    STATUS_ERROR,        // Server failed to accept query
    STATUS_EXPIRED,      // Server did not respond in the maximum allowed time
    NUM_STATUS,
  };

public:

  //----------------------------------------------------------------
  // Access functions
  //----------------------------------------------------------------
  vtkGetMacro(QueryStatus, int);
  vtkSetMacro(QueryStatus, int);
  vtkGetMacro(QueryType, int);
  vtkSetMacro(QueryType, int);

  // Description:
  // Get OpenIGTLink type name. If the query node is for IMAGE, "IMAGE" is returned.
  virtual void SetIGTLName(const char* name);
  virtual const char* GetIGTLName() { return IGTLName.c_str(); };

  // Description:
  // Get OpenIGTLink device name. If it is empty then the query will look for any device name with a matching type (IGTLName).
  virtual void SetIGTLDeviceName(const char* name);
  virtual const char* GetIGTLDeviceName() { return IGTLDeviceName.c_str(); };

  // Description:
  // Time when the query issued in Universal Time in seconds (see vtkTimerLog::GetUniversalTime())
  vtkGetMacro(TimeStamp, double);
  vtkSetMacro(TimeStamp, double);

  // Description:
  // Timeout of the query in seconds.
  // If TimeOut>0 then it means that QueryStatus has to be changed to STATUS_TIMEOUT if the status
  // is still STATUS_WAITING and more than TimeOut time elapsed since the TimeStamp.
  // If TimeOut==0 then there is no limit on the amount of time waiting for a query response.
  vtkGetMacro(TimeOut, double);
  vtkSetMacro(TimeOut, double);

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLIGTLQueryNode* New();
  vtkTypeMacro(vtkMRMLIGTLQueryNode, vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual vtkMRMLNode* CreateNodeInstance() override;

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes(const char** atts) override;

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) override;

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode* node) override;

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() override
  { return "IGTLQuery"; };

  // Description:
  // Return error message after receiving requested message.
  virtual const char* GetErrorString();

  // Description:
  // Node that holds received data as response to the query.
  virtual void SetResponseDataNodeID(const char* id);
  virtual const char* GetResponseDataNodeID();
  vtkMRMLNode* GetResponseDataNode();

  // Description:
  // Connector node that currently has this query node in its queue (pushed into the queue, waiting for a response).
  virtual void SetConnectorNodeID(const char* id);
  virtual const char* GetConnectorNodeID();
  vtkMRMLIGTLConnectorNode* GetConnectorNode();

  static const char* QueryStatusToString(int queryStatus);
  static const char* QueryTypeToString(int queryType);

protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLIGTLQueryNode();
  ~vtkMRMLIGTLQueryNode();
  vtkMRMLIGTLQueryNode(const vtkMRMLIGTLQueryNode&);
  void operator=(const vtkMRMLIGTLQueryNode&);

public:
  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------


private:
  //----------------------------------------------------------------
  // Data
  //----------------------------------------------------------------

  std::string IGTLName;
  std::string IGTLDeviceName;

  int QueryStatus;
  int QueryType;

  double TimeStamp;
  double TimeOut;

};

#endif

