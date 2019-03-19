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
#include <vtkMRMLImageMetaListNode.h>
#include <vtkMRMLScene.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>
#include <map>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLImageMetaListNode);

//----------------------------------------------------------------------------
vtkMRMLImageMetaListNode::vtkMRMLImageMetaListNode()
{
  this->ImageMetaList.clear();
}

//----------------------------------------------------------------------------
vtkMRMLImageMetaListNode::~vtkMRMLImageMetaListNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLImageMetaListNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

}

//----------------------------------------------------------------------------
void vtkMRMLImageMetaListNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLImageMetaListNode::Copy(vtkMRMLNode* anode)
{

  Superclass::Copy(anode);
  /*
  vtkMRMLImageMetaListNode *node = (vtkMRMLImageMetaListNode *) anode;
  */

}

//----------------------------------------------------------------------------
void vtkMRMLImageMetaListNode::ProcessMRMLEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

}

//----------------------------------------------------------------------------
void vtkMRMLImageMetaListNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
int  vtkMRMLImageMetaListNode::GetNumberOfImageMetaElement()
{
  return this->ImageMetaList.size();
}

//----------------------------------------------------------------------------
void vtkMRMLImageMetaListNode::AddImageMetaElement(ImageMetaElement element)
{
  this->ImageMetaList.push_back(element);
}

//----------------------------------------------------------------------------
void vtkMRMLImageMetaListNode::GetImageMetaElement(int index, ImageMetaElement* element)
{
  if (index >= 0 && index < (int) this->ImageMetaList.size())
  {
    *element = this->ImageMetaList[index];
  }
  else
  {
    element->DeviceName = "";
  }
}

//----------------------------------------------------------------------------
void vtkMRMLImageMetaListNode::ClearImageMetaElement()
{
  this->ImageMetaList.clear();
}
