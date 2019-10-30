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
#include <igtlMath.h>
#include <igtlMessageBase.h>
#include <igtlMessageHeader.h>
#include <igtl_header.h>  // to define maximum length of message name

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkCommand.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>
#include <map>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLTrackingDataBundleNode);

//---------------------------------------------------------------------------
class vtkMRMLIGTLTrackingDataBundleNode::vtkInternal
{
public:
  //---------------------------------------------------------------------------
  vtkInternal(vtkMRMLIGTLTrackingDataBundleNode* external);
  ~vtkInternal();

  // Update Transform nodes. If new data is specified, create a new Transform node.
  // default type is 1 (igtl::TrackingDataMessage::TYPE_TRACKER)
  void UpdateTransformNode(const char* name, igtl::Matrix4x4& matrix, int type = 1);

  vtkMRMLIGTLTrackingDataBundleNode* External;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataBundleNode::vtkInternal::vtkInternal(vtkMRMLIGTLTrackingDataBundleNode* external)
  : External(external)
{
}


//---------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataBundleNode::vtkInternal::~vtkInternal()
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::vtkInternal::UpdateTransformNode(const char* name, igtl::Matrix4x4& matrix, int type)
{
  TrackingDataInfoMap::iterator iter = this->External->TrackingDataList.find(std::string(name));

  vtkMRMLLinearTransformNode* node;

  // If the tracking node does not exist in the scene
  if (iter == this->External->TrackingDataList.end())
  {
    node = vtkMRMLLinearTransformNode::New();
    node->SetName(name);
    node->SetDescription("Received by OpenIGTLink");

    if (this->External->GetScene())
    {
      this->External->GetScene()->AddNode(node);
      node->Delete();
    }
    TrackingDataInfo info;
    info.type = type;
    info.node = node;
    this->External->TrackingDataList[std::string(name)] = info;

    // TODO: register to MRML observer

  }
  else
  {
    node = iter->second.node;
  }

  vtkNew<vtkMatrix4x4> mat;
  double* vtkmat = &mat->Element[0][0];
  float* igtlmat = &matrix[0][0];
  for (int i = 0; i < 16; i++)
  {
    vtkmat[i] = igtlmat[i];
  }
  node->SetMatrixTransformToParent(mat.GetPointer());
  this->External->InvokeEvent(vtkCommand::ModifiedEvent);
}

//----------------------------------------------------------------------------
// vtkMRMLIGTLTrackingDataBundleNode methods

//----------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataBundleNode::vtkMRMLIGTLTrackingDataBundleNode()
{
  this->Internal = new vtkInternal(this);

  this->HideFromEditors = 1;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLTrackingDataBundleNode::~vtkMRMLIGTLTrackingDataBundleNode()
{
  delete this->Internal;
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
void vtkMRMLIGTLTrackingDataBundleNode::Copy(vtkMRMLNode* anode)
{

  Superclass::Copy(anode);
  //vtkMRMLIGTLTrackingDataBundleNode *node = (vtkMRMLIGTLTrackingDataBundleNode *) anode;

}


//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::ProcessMRMLEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

}


//----------------------------------------------------------------------------
void vtkMRMLIGTLTrackingDataBundleNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os, indent);
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

  node->SetMatrixTransformToParent(matrix);
  node->Delete();
  this->InvokeEvent(vtkCommand::ModifiedEvent);
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
