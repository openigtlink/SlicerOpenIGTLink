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
#include "vtkMRMLIGTLTrackingDataBundleNode.h"

// OpenIGTLink includes
#include <igtlOSUtil.h>
#include <igtlMessageBase.h>
#include <igtlMessageHeader.h>
#include <igtl_header.h>  // to define maximum length of message name

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>
#include <map>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLTrackingDataBundleNode);

//----------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataBundleNode::vtkMRMLIGTLTrackingDataBundleNode()
{
  this->TrackingDataList.clear();

  this->HideFromEditors = 1;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataBundleNode::~vtkMRMLIGTLTrackingDataBundleNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::ReadXMLAttributes(const char** atts)
{

  vtkMRMLNode::ReadXMLAttributes(atts);

}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIGTLTrackingDataBundleNode::Copy(vtkMRMLNode *anode)
{

  Superclass::Copy(anode);
  //vtkMRMLIGTLTrackingDataBundleNode *node = (vtkMRMLIGTLTrackingDataBundleNode *) anode;

}


//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

}


//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::UpdateTransformNode(const char* name, vtkMatrix4x4* matrix, int type)
{
  TrackingDataInfoMap::iterator iter = this->TrackingDataList.find(std::string(name));

  vtkMRMLLinearTransformNode* node;

  // If the tracking node does not exist in the scene
  if (iter == this->TrackingDataList.end())
    {
    node = vtkMRMLLinearTransformNode::New();
    node->SetName(name);
    node->SetDescription("Received by OpenIGTLink");
    if (this->GetScene())
      {
      this->GetScene()->AddNode(node);
      }
    TrackingDataInfo info;
    info.type = type;
    info.node = node;
    this->TrackingDataList[std::string(name)] = info;

    // TODO: register to MRML observer

    }
  else
    {
    node = iter->second.node;
    }

  node->ApplyTransformMatrix(matrix);
  node->Delete();
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::UpdateTransformNode(const char* name, igtl::Matrix4x4& matrix, int type)
{
  TrackingDataInfoMap::iterator iter = this->TrackingDataList.find(std::string(name));

  vtkMRMLLinearTransformNode* node;

  // If the tracking node does not exist in the scene
  if (iter == this->TrackingDataList.end())
    {
    node = vtkMRMLLinearTransformNode::New();
    node->SetName(name);
    node->SetDescription("Received by OpenIGTLink");

    if (this->GetScene())
      {
      this->GetScene()->AddNode(node);
      node->Delete();
      }
    TrackingDataInfo info;
    info.type = type;
    info.node = node;
    this->TrackingDataList[std::string(name)] = info;

    // TODO: register to MRML observer

    }
  else
    {
    node = iter->second.node;
    }

  vtkNew<vtkMatrix4x4> mat;
  double *vtkmat = &mat->Element[0][0];
  float *igtlmat = &matrix[0][0];
  for (int i = 0; i < 16; i++)
    {
    vtkmat[i] = igtlmat[i];
    }
  node->SetMatrixTransformToParent(mat.GetPointer());

}

//----------------------------------------------------------------------------
int vtkMRMLIGTLTrackingDataBundleNode::GetNumberOfTransformNodes()
{
  return this->TrackingDataList.size();
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLIGTLTrackingDataBundleNode::GetTransformNode(unsigned int id)
{
  if (id > this->TrackingDataList.size())
    {
    return NULL;
    }

  TrackingDataInfoMap::iterator iter;
  iter = this->TrackingDataList.begin();
  for (unsigned int i = 0; i < id; i ++)
    {
    iter ++;
    }
  if (iter != this->TrackingDataList.end())
    {
    return iter->second.node;
    }
  else
    {
    return NULL;
    }
}
