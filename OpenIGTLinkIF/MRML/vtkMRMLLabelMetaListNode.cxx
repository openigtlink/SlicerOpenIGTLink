/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLGradientAnisotropicDiffusionFilterNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

// OpenIGTLink includes
#include <igtlOSUtil.h>
#include <igtlMessageBase.h>
#include <igtlMessageHeader.h>

// VTK includes
#include <vtkObjectFactory.h>

// MRML includes
#include <vtkMRMLLabelMetaListNode.h>
#include <vtkMRMLScene.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>
#include <map>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLLabelMetaListNode);

//----------------------------------------------------------------------------
vtkMRMLLabelMetaListNode::vtkMRMLLabelMetaListNode()
{
  this->LabelMetaList.clear();
}

//----------------------------------------------------------------------------
vtkMRMLLabelMetaListNode::~vtkMRMLLabelMetaListNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLLabelMetaListNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  //of << " serverPort=\"" << this->ServerPort << "\" ";
  //of << " restrictDeviceName=\"" << this->RestrictDeviceName << "\" ";
}

//----------------------------------------------------------------------------
void vtkMRMLLabelMetaListNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLLabelMetaListNode::Copy(vtkMRMLNode *anode)
{

  Superclass::Copy(anode);
  /*
  vtkMRMLLabelMetaListNode *node = (vtkMRMLLabelMetaListNode *) anode;
  */

}

//----------------------------------------------------------------------------
void vtkMRMLLabelMetaListNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{

  Superclass::ProcessMRMLEvents(caller, event, callData);

}

//----------------------------------------------------------------------------
void vtkMRMLLabelMetaListNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
int  vtkMRMLLabelMetaListNode::GetNumberOfLabelMetaElement()
{
  return this->LabelMetaList.size();
}

//----------------------------------------------------------------------------
void vtkMRMLLabelMetaListNode::AddLabelMetaElement(LabelMetaElement element)
{
  this->LabelMetaList.push_back(element);
}

//----------------------------------------------------------------------------
void vtkMRMLLabelMetaListNode::GetLabelMetaElement(int index, LabelMetaElement* element)
{
  if (index >= 0 && index < (int) this->LabelMetaList.size())
    {
    *element = this->LabelMetaList[index];
    }
  else
    {
    element->DeviceName = "";
    }
}

//----------------------------------------------------------------------------
void vtkMRMLLabelMetaListNode::ClearLabelMetaElement()
{
  this->LabelMetaList.clear();
}
