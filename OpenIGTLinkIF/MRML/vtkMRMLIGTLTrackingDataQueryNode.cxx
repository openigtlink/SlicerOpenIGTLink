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
#include <vtkMRMLIGTLTrackingDataQueryNode.h>

// OpenIGTLink includes
#include <igtlOSUtil.h>
#include <igtlMessageBase.h>
#include <igtlMessageHeader.h>
#include <igtl_header.h>  // to define maximum length of message name

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>
#include <map>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLTrackingDataQueryNode);

//----------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataQueryNode::vtkMRMLIGTLTrackingDataQueryNode()
{
  this->QueryStatus = 0;
  this->QueryType   = 0;

  this->ConnectorNodeID = "";

  this->TimeStamp = 0.0;
  this->TimeOut   = 0.0;

  this->HideFromEditors = 1;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataQueryNode::~vtkMRMLIGTLTrackingDataQueryNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataQueryNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  //of << " serverPort=\"" << this->ServerPort << "\" ";
  //of << " restrictDeviceName=\"" << this->RestrictDeviceName << "\" ";
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataQueryNode::ReadXMLAttributes(const char** atts)
{

  vtkMRMLNode::ReadXMLAttributes(atts);

}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIGTLTrackingDataQueryNode::Copy(vtkMRMLNode *anode)
{

  Superclass::Copy(anode);
  //vtkMRMLIGTLTrackingDataQueryNode *node = (vtkMRMLIGTLTrackingDataQueryNode *) anode;

}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataQueryNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{

  Superclass::ProcessMRMLEvents(caller, event, callData);

}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataQueryNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataQueryNode::SetIGTLName(const char* name)
{
  char buf[IGTL_HEADER_DEVSIZE+1];
  buf[IGTL_HEADER_DEVSIZE] = '\0';
  strncpy(buf, name, IGTL_HEADER_DEVSIZE);
  this->IGTLName = buf;
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLTrackingDataQueryNode::GetErrorString()
{
  return "";
}
