/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/OpenIGTLinkIF/vtkSlicerOpenIGTLinkIFLogic.h $
  Date:      $Date: 2010-06-10 11:05:22 -0400 (Thu, 10 Jun 2010) $
  Version:   $Revision: 13728 $

==========================================================================*/

/// This class manages the logic associated with tracking device for IGT.

#ifndef __vtkSlicerOpenIGTLinkIFLogic_h
#define __vtkSlicerOpenIGTLinkIFLogic_h

#include "vtkSlicerOpenIGTLinkIFModuleLogicExport.h"

// Slicer includes
#include <vtkSlicerBaseLogic.h>
#include <vtkSlicerModuleLogic.h>

// MRML includes
#include <vtkMRMLFiducialListNode.h>
#include <vtkMRMLScene.h>

// OpenIGTLinkIF includes
#include "vtkMRMLIGTLConnectorNode.h"

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkMultiThreader.h>

// STD includes
#include <vector>
#include <string.h>

class vtkMRMLIGTLConnectorNode;

/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class VTK_SLICER_OPENIGTLINKIF_MODULE_LOGIC_EXPORT vtkSlicerOpenIGTLinkIFLogic : public vtkSlicerModuleLogic
{
public:

  enum    // Events
  {
    StatusUpdateEvent       = 50001,
    //SliceUpdateEvent        = 50002,
  };

  typedef struct
  {
    std::string name;
    std::string type;
    int io;
    std::string nodeID;
  } IGTLMrmlNodeInfoType;

  typedef std::vector<IGTLMrmlNodeInfoType>         IGTLMrmlNodeListType;

  // Work phase keywords used in NaviTrack (defined in BRPTPRInterface.h)

public:

  static vtkSlicerOpenIGTLinkIFLogic* New();
  vtkTypeMacro(vtkSlicerOpenIGTLinkIFLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream&, vtkIndent) override;

  /// The selected transform node is observed for TransformModified events and the transform
  /// data is copied to the slice nodes depending on the current mode

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;

  virtual void RegisterNodes() override;

  //----------------------------------------------------------------
  // Events
  //----------------------------------------------------------------

  virtual void OnMRMLSceneEndImport() override;

  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* /*node*/) override;

  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* /*node*/) override;

  virtual void OnMRMLNodeModified(vtkMRMLNode* /*node*/) override {}

  //----------------------------------------------------------------
  // Connector and device Management
  //----------------------------------------------------------------

  // Access connectors
  vtkMRMLIGTLConnectorNode* GetConnector(const char* conID);

  // Call timer-driven routines for each connector
  void                      CallConnectorTimerHander();

  // Device Name management
  int  SetRestrictDeviceName(int f);

  int  RegisterMessageDevice(IGTLDevicePointer device);
  int  UnregisterMessageDevice(IGTLDevicePointer device);

  unsigned int       GetNumberOfDevices();
  IGTLDevicePointer GetDevice(unsigned int i);
  IGTLDevicePointer GetDeviceByMRMLTag(const char* mrmlTag);
  IGTLDevicePointer GetDeviceByDeviceType(const char* deviceType);

  //----------------------------------------------------------------
  // MRML Management
  //----------------------------------------------------------------

  virtual void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData) override;
  //virtual void ProcessLogicEvents(vtkObject * caller, unsigned long event, void * callData);

  void ProcCommand(const char* nodeName, int size, unsigned char* data);

  void GetDeviceNamesFromMrml(IGTLMrmlNodeListType& list);
  void GetDeviceNamesFromMrml(IGTLMrmlNodeListType& list, const char* mrmlTagName);
  //void GetDeviceTypes(std::vector<char*> &list);

  // Transform locator model node reference role
  vtkGetStringMacro(LocatorModelReferenceRole);

protected:

  //----------------------------------------------------------------
  // Constructor, destructor etc.
  //----------------------------------------------------------------

  vtkSlicerOpenIGTLinkIFLogic();
  virtual ~vtkSlicerOpenIGTLinkIFLogic();

  static void DataCallback(vtkObject*, unsigned long, void*, void*);

  void AddMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode* connectorNode);
  void RemoveMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode* connectorNode);

  void RegisterMessageDevices(vtkMRMLIGTLConnectorNode* connectorNode);

  void UpdateAll();
  void UpdateSliceDisplay();
  vtkCallbackCommand* DataCallbackCommand;

private:

  int Initialized;

  //----------------------------------------------------------------
  // Connector Management
  //----------------------------------------------------------------

  //ConnectorMapType              ConnectorMap;

  //int LastConnectorID;
  int RestrictDeviceName;


  char* LocatorModelReferenceRole;

  // Transform locator model node reference role
  vtkSetStringMacro(LocatorModelReferenceRole);

private:
  class vtkInternal;
  vtkInternal* Internal;

  vtkSlicerOpenIGTLinkIFLogic(const vtkSlicerOpenIGTLinkIFLogic&); // Not implemented
  void operator=(const vtkSlicerOpenIGTLinkIFLogic&);               // Not implemented
};

#endif
