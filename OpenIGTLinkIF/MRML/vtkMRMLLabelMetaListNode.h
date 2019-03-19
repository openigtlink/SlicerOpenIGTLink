/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLLabelMetaListNode_h
#define __vtkMRMLLabelMetaListNode_h

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
#include <sstream>

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLLabelMetaListNode : public vtkMRMLNode
{
 public:

  //----------------------------------------------------------------
  // Constants Definitions
  //----------------------------------------------------------------

  // Events
  enum {
    NewDeviceEvent        = 118949,
  };


  typedef struct {
    std::string   Name;        /* name / description (< 64 bytes)*/
    std::string   DeviceName;  /* device name to query the LABEL and COLORT */
    std::string   Owner;       /* owner of the label (name of image node) */
    int           Size[3];     /* entire label volume size */
  } LabelMetaElement;

 public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLLabelMetaListNode *New();
  vtkTypeMacro(vtkMRMLLabelMetaListNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual vtkMRMLNode* CreateNodeInstance() override;

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts) override;

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) override;

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) override;

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() override
  {return "LabelMetaList";};

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData ) override;

  // Description:
  // Get number of label meta element stored in this class instance
  int  GetNumberOfLabelMetaElement();

  // Description:
  // Add label meta element
  void AddLabelMetaElement(LabelMetaElement element);

  // Description:
  // Get label meta element. If the element does not eists,
  // DeviceName is set to "".
  void GetLabelMetaElement(int index, LabelMetaElement* element);

  // Description:
  // Clear label meta element list
  void ClearLabelMetaElement();

 protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLLabelMetaListNode();
  ~vtkMRMLLabelMetaListNode();
  vtkMRMLLabelMetaListNode(const vtkMRMLLabelMetaListNode&);
  void operator=(const vtkMRMLLabelMetaListNode&);

 public:
  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------


 private:
  //----------------------------------------------------------------
  // Data
  //----------------------------------------------------------------

  std::vector<LabelMetaElement> LabelMetaList;

};

#endif

