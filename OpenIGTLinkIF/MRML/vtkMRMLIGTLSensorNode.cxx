/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

// VTK includes
#include <vtkCommand.h>
#include <vtkIntArray.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLIGTLSensorNode.h"

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLSensorNode);

//----------------------------------------------------------------------------
vtkMRMLIGTLSensorNode::vtkMRMLIGTLSensorNode()
{
  this->HideFromEditors = 0;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLSensorNode::~vtkMRMLIGTLSensorNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLSensorNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLSensorNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLSensorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLSensorNode::SetArrayLength(vtkTypeUInt8 length)
{
  if (this->Data.size() != length)
  {
    this->Data.resize(length);
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkTypeUInt8 vtkMRMLIGTLSensorNode::GetArrayLength()
{
  return this->Data.size();
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLSensorNode::SetSensorStatus(vtkTypeUInt8 status)
{
  this->SensorStatus = status;
  this->InvokeEvent(SensorModifiedEvent, NULL);
  return 1;
}

//----------------------------------------------------------------------------
vtkTypeUInt8 vtkMRMLIGTLSensorNode::GetSensorStatus()
{
  return this->SensorStatus;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLSensorNode::SetDataUnit(vtkTypeUInt64 unit)
{
  this->DataUnit = unit;
  this->InvokeEvent(SensorModifiedEvent, NULL);
  return 1;
}

//----------------------------------------------------------------------------
vtkTypeUInt64 vtkMRMLIGTLSensorNode::GetDataUnit()
{
  return this->DataUnit;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLSensorNode::InsertDataValue(double value)
{
  this->Data.push_back(value);
  this->InvokeEvent(SensorModifiedEvent, NULL);
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLSensorNode::SetDataValue(unsigned int index, double value)
{
  if (index < this->Data.size())
  {
    this->Data[index] = value;
  }
  else
  {
    this->InsertDataValue(value);
  }
  this->InvokeEvent(SensorModifiedEvent, NULL);
  return 1;
}

//----------------------------------------------------------------------------
double vtkMRMLIGTLSensorNode::GetDataValue(unsigned int index)
{
  if (index < this->Data.size())
  {
    return this->Data[index];
  }
  return 0;
}

