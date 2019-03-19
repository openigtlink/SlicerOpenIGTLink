/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLImageMetaListNode_h
#define __vtkMRMLImageMetaListNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkObject.h>

// STD includes
#include <string>
#include <map>
#include <vector>
#include <set>

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLImageMetaListNode : public vtkMRMLNode
{
public:

  //----------------------------------------------------------------
  // Constants Definitions
  //----------------------------------------------------------------

  // Events
  enum
  {
    NewDeviceEvent        = 118949,
  };


  typedef struct
  {
    std::string   Name;        /* name / description (< 64 bytes)*/
    std::string   DeviceName;  /* device name to query the IMAGE and COLORT */
    std::string   Modality;    /* modality name (< 32 bytes) */
    std::string   PatientName; /* patient name (< 64 bytes) */
    std::string   PatientID;   /* patient ID (MRN etc.) (< 64 bytes) */
    double        TimeStamp;   /* scan time */
    int           Size[3];     /* entire image volume size */
    unsigned char ScalarType;  /* scalar type. see scalar_type in IMAGE message */
  } ImageMetaElement;

public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLImageMetaListNode* New();
  vtkTypeMacro(vtkMRMLImageMetaListNode, vtkMRMLNode);

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
  {return "ImageMetaList";};

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents(vtkObject* caller, unsigned long event, void* callData) override;

  // Description:
  // Get number of image meta element stored in this class instance
  int  GetNumberOfImageMetaElement();

  // Description:
  // Add image meta element
  void AddImageMetaElement(ImageMetaElement element);

  // Description:
  // Get image meta element. If the element does not eists,
  // DeviceName is set to "".
  void GetImageMetaElement(int index, ImageMetaElement* element);

  // Description:
  // Clear image meta element list
  void ClearImageMetaElement();

protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLImageMetaListNode();
  ~vtkMRMLImageMetaListNode();
  vtkMRMLImageMetaListNode(const vtkMRMLImageMetaListNode&);
  void operator=(const vtkMRMLImageMetaListNode&);

public:
  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------


private:
  //----------------------------------------------------------------
  // Data
  //----------------------------------------------------------------

  std::vector<ImageMetaElement> ImageMetaList;

};

#endif

