/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGradientAnisotropicDiffusionFilterNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLink includes
#include <igtlOSUtil.h>
#include <igtlMessageBase.h>
#include <igtlMessageHeader.h>
#include <igtl_header.h>  // to define maximum length of message name

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkTimerLog.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>
#include <map>

static const char ResponseDataNodeRole[] = "responseData";
static const char ConnectorNodeRole[] = "connector";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLQueryNode);

//----------------------------------------------------------------------------
vtkMRMLIGTLQueryNode::vtkMRMLIGTLQueryNode()
{
  this->QueryStatus = 0;
  this->QueryType   = 0;

  this->TimeStamp = 0.0;
  this->TimeOut   = 0.0;

  this->AddNodeReferenceRole(ResponseDataNodeRole);
  this->AddNodeReferenceRole(ConnectorNodeRole);
}

//----------------------------------------------------------------------------
vtkMRMLIGTLQueryNode::~vtkMRMLIGTLQueryNode()
{
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLQueryNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

}


//----------------------------------------------------------------------------
void vtkMRMLIGTLQueryNode::ReadXMLAttributes(const char** atts)
{

  vtkMRMLNode::ReadXMLAttributes(atts);

}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIGTLQueryNode::Copy(vtkMRMLNode* anode)
{

  Superclass::Copy(anode);

  vtkMRMLIGTLQueryNode* node = vtkMRMLIGTLQueryNode::SafeDownCast(anode);
  if (node == NULL)
  {
    vtkErrorMacro("vtkMRMLIGTLQueryNode::Copy failed: unrelated input node type");
    return;
  }

  this->IGTLName = node->IGTLName;
  this->IGTLDeviceName = node->IGTLDeviceName;
  this->QueryStatus = node->QueryStatus;
  this->QueryType = node->QueryType;
  this->TimeStamp = node->TimeStamp;
  this->TimeOut = node->TimeOut;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLQueryNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "IGTLName: " << this->IGTLName << "\n";
  os << indent << "IGTLDeviceName: " << this->IGTLDeviceName << "\n";
  os << indent << "QueryType: " << vtkMRMLIGTLQueryNode::QueryTypeToString(this->QueryType) << "\n";
  os << indent << "QueryStatus: " << vtkMRMLIGTLQueryNode::QueryStatusToString(this->QueryStatus) << "\n";
  if (this->TimeOut > 0 && this->QueryStatus == STATUS_WAITING)
  {
    double remainingTime = this->TimeOut - (vtkTimerLog::GetUniversalTime() - this->TimeStamp);
    os << indent << indent << "Remaining time: " << remainingTime << "\n";
  }
  os << indent << "TimeStamp: " << this->TimeStamp << "\n";
  os << indent << "TimeOut: " << this->TimeOut << "\n";
  os << indent << "ConnectorNodeID: " << (this->GetConnectorNodeID() ? this->GetConnectorNodeID() : "(none)") << "\n";
  os << indent << "DataNodeID: " << (this->GetResponseDataNodeID() ? this->GetResponseDataNodeID() : "(none)") << "\n";
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLQueryNode::SetIGTLName(const char* name)
{
  char buf[IGTL_HEADER_TYPE_SIZE + 1];
  buf[IGTL_HEADER_TYPE_SIZE] = '\0';
  strncpy(buf, name, IGTL_HEADER_TYPE_SIZE);
  this->IGTLName = buf;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLQueryNode::SetIGTLDeviceName(const char* name)
{
  char buf[IGTL_HEADER_NAME_SIZE + 1];
  buf[IGTL_HEADER_NAME_SIZE] = '\0';
  strncpy(buf, name, IGTL_HEADER_NAME_SIZE);
  this->IGTLDeviceName = buf;
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLQueryNode::GetErrorString()
{
  return "";
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLQueryNode::QueryStatusToString(int queryStatus)
{
  switch (queryStatus)
  {
    case STATUS_NOT_DEFINED:
      return "NOT_DEFINED";
    case STATUS_PREPARED:
      return "PREPARED";
    case STATUS_WAITING:
      return "WAITING";
    case STATUS_SUCCESS:
      return "SUCCESS";
    case STATUS_EXPIRED:
      return "EXPIRED";
    default:
      return "INVALID";
  }
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLQueryNode::QueryTypeToString(int queryType)
{
  switch (queryType)
  {
    case TYPE_NOT_DEFINED:
      return "NOT_DEFINED";
    case TYPE_GET:
      return "GET";
    case TYPE_START:
      return "START";
    case TYPE_STOP:
      return "STOP";
    default:
      return "INVALID";
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLQueryNode::SetResponseDataNodeID(const char* id)
{
  this->SetNodeReferenceID(ResponseDataNodeRole, id);
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLQueryNode::GetResponseDataNodeID()
{
  return GetNodeReferenceID(ResponseDataNodeRole);
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLQueryNode::GetResponseDataNode()
{
  return GetNodeReference(ResponseDataNodeRole);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLQueryNode::SetConnectorNodeID(const char* id)
{
  this->SetNodeReferenceID(ConnectorNodeRole, id);
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLQueryNode::GetConnectorNodeID()
{
  return GetNodeReferenceID(ConnectorNodeRole);
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkMRMLIGTLQueryNode::GetConnectorNode()
{
  return vtkMRMLIGTLConnectorNode::SafeDownCast(GetNodeReference(ConnectorNodeRole));
}
