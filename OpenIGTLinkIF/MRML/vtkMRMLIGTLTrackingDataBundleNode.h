/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLIGTLTrackingDataBundleNode_h
#define __vtkMRMLIGTLTrackingDataBundleNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkObject.h>

// STD includes
#include <string>
#include <map>
#include <vector>
#include <set>

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLTrackingDataBundleNode : public vtkMRMLNode
{
 public:

  //----------------------------------------------------------------
  // Constants Definitions
  //----------------------------------------------------------------

  // Events
  enum {
    ResponseEvent        = 128940,
  };

  enum {
    TYPE_NOT_DEFINED,
    TYPE_GET,
    TYPE_START,
    TYPE_STOP,
    NUM_TYPE,
  };

  enum {
    STATUS_NOT_DEFINED,
    STATUS_PREPARED,     // Ready to query
    STATUS_WAITING,      // Waiting for response from server
    STATUS_SUCCESS,      // Server accepted query successfuly
    STATUS_ERROR,        // Server failed to accept query
    NUM_STATUS,
  };

 public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLIGTLTrackingDataBundleNode *New();
  vtkTypeMacro(vtkMRMLIGTLTrackingDataBundleNode,vtkMRMLNode);

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
  { return "IGTLTrackingDataSplitter"; };

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData ) override;

  // Description:
  // Update Transform nodes. If new data is specified, create a new Transform node.
  // default type is 1 (igtl::TrackingDataMessage::TYPE_TRACKER)
  virtual void UpdateTransformNode(const char* name, vtkMatrix4x4* matrix, int type = 1);

  // Description:
  // Get the number of linear transform nodes in the bundle
  virtual int GetNumberOfTransformNodes();

  // Description:
  // Get the N-th linear transform node (id == N)
  virtual vtkMRMLLinearTransformNode* GetTransformNode(unsigned int id);


 protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLIGTLTrackingDataBundleNode();
  ~vtkMRMLIGTLTrackingDataBundleNode();
  vtkMRMLIGTLTrackingDataBundleNode(const vtkMRMLIGTLTrackingDataBundleNode&);
  void operator=(const vtkMRMLIGTLTrackingDataBundleNode&);

 public:
  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------


 private:
  //----------------------------------------------------------------
  // Data
  //----------------------------------------------------------------

  typedef struct {
    int                         type;
    vtkMRMLLinearTransformNode* node;
  } TrackingDataInfo;
  typedef std::map<std::string, TrackingDataInfo> TrackingDataInfoMap;

  TrackingDataInfoMap TrackingDataList;

  class vtkInternal;
  vtkInternal * Internal;
};

#endif
