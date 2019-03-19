/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

==========================================================================*/

#ifndef __vtkMRMLIGTLSensorNode_h
#define __vtkMRMLIGTLSensorNode_h

// MRML includes
#include "vtkMRMLNode.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"


class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLSensorNode : public vtkMRMLNode
{
public:
  static vtkMRMLIGTLSensorNode *New();

  vtkTypeMacro(vtkMRMLIGTLSensorNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual vtkMRMLNode* CreateNodeInstance() override;

  /// 
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts) override;

  /// 
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) override;

  /// 
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() override{return "IGTLSensor";};

  /// Set length of data array
  int SetArrayLength(vtkTypeUInt8 length);

  /// Get length of data array
  vtkTypeUInt8 GetArrayLength();

  /// Set sensor status
  int SetSensorStatus(vtkTypeUInt8 status);

  /// Get sensor status
  vtkTypeUInt8 GetSensorStatus();

  /// Set data unit
  int SetDataUnit(vtkTypeUInt64 unit);

  /// Get data unit
  vtkTypeUInt64 GetDataUnit();

  /// Insert data value
  int InsertDataValue(double value);

  /// Set data value of element
  int SetDataValue(unsigned int index, double value);
  
  /// Get data value of element
  double GetDataValue(unsigned int index);

  enum
  {
    SensorModifiedEvent = 40000,
  };

protected:
  vtkMRMLIGTLSensorNode();
  ~vtkMRMLIGTLSensorNode();
  vtkMRMLIGTLSensorNode(const vtkMRMLIGTLSensorNode&);
  void operator=(const vtkMRMLIGTLSensorNode&);

private:
  
  vtkTypeUInt8 ArrayLength;
  vtkTypeUInt8 SensorStatus;
  vtkTypeUInt64  DataUnit;
  std::vector<double> Data;

};

#endif
