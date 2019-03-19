/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

  =========================================================================auto=*/

// VTK includes
#include <vtkCommand.h>
#include <vtkIntArray.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLIGTLStatusNode.h"

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLStatusNode);

//----------------------------------------------------------------------------
vtkMRMLIGTLStatusNode::vtkMRMLIGTLStatusNode()
{
  this->HideFromEditors = 1;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLStatusNode::~vtkMRMLIGTLStatusNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLStatusNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLStatusNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLStatusNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLStatusNode::SetCode(vtkTypeUInt16 code)
{
  if (code < STATUS_NUM_TYPES)
  {
    this->Code = code;
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
vtkTypeUInt16 vtkMRMLIGTLStatusNode::GetCode()
{
  return this->Code;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLStatusNode::SetSubCode(vtkTypeInt64 code)
{
  this->SubCode = code;
  this->InvokeEvent(StatusModifiedEvent, NULL);
  return 1;
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkMRMLIGTLStatusNode::GetSubCode()
{
  return this->SubCode;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLStatusNode::SetErrorName(const char* name)
{
  if (strlen(name) <= 20)
  {
    this->ErrorName = name;
    this->InvokeEvent(StatusModifiedEvent, NULL);
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLStatusNode::GetErrorName()
{
  return this->ErrorName.c_str();
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLStatusNode::SetStatusString(const char* name)
{

  this->StatusString = name;
  this->InvokeEvent(StatusModifiedEvent, NULL);
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLStatusNode::GetStatusString()
{
  return this->StatusString.c_str();
}


//----------------------------------------------------------------------------
int vtkMRMLIGTLStatusNode::SetStatus(vtkTypeUInt16 code, vtkTypeUInt16 subcode, const char* errorName, const char* statusString)
{
  if (code < STATUS_NUM_TYPES &&
      strlen(errorName) <= 20)
  {
    this->Code = code;
    this->SubCode = subcode;
    this->ErrorName = errorName;
    this->StatusString = statusString;
    this->InvokeEvent(StatusModifiedEvent, NULL);
    return 1;
  }
  else
  {
    return 0;
  }

}
