/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerPlusRemoteLogic - slicer plus remote logic class for displayable nodes (tools)
// .SECTION Description
// This class manages the logic associated with displayable nodes plus remote. The watched nodes last time stamp is
// stored and compared with the current one every timer shot. If the last and current time stamps are different it
// has an updated status.

#ifndef __vtkSlicerPlusRemoteLogic_h
#define __vtkSlicerPlusRemoteLogic_h

// VTK includes
#include <vtkCallbackCommand.h>

// Slicer includes
#include <vtkSlicerModuleLogic.h>

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>

// OpenIGTLinkIF includes
#include "igtlioCommand.h"

// For referencing own MRML node
class vtkMRMLAnnotationROINode;
class vtkMRMLDisplayableNode;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLPlusRemoteNode;
class vtkMRMLLinearTransformNode;
class vtkMRMLVolumeNode;
struct ParameterNodeCommands;

#include "vtkSlicerPlusRemoteModuleLogicExport.h"

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLUSREMOTE_MODULE_LOGIC_EXPORT vtkSlicerPlusRemoteLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerPlusRemoteLogic* New();
  vtkTypeMacro(vtkSlicerPlusRemoteLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerPlusRemoteLogic();
  virtual ~vtkSlicerPlusRemoteLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  /// Initialize listening to MRML events
  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);

public:
  void CreateDefaultParameterSet();

  /// Send a command to get the list of capture devices from the Plus
  void RequestCaptureDeviceIDs(vtkMRMLPlusRemoteNode*);
  /// Send a command to get the list of volume reconstructor devices from the Plus
  void RequestVolumeReconstructorDeviceIDs(vtkMRMLPlusRemoteNode*);
  /// Send a command to get a list of device ids from Plus
  void RequestDeviceIDs(vtkMRMLPlusRemoteNode*);

  /// Sends a command to Plus to start recording with the specified parameters
  void StartRecording(vtkMRMLPlusRemoteNode*);
  /// Sends a command to Plus to stop recording with the specified capture device
  void StopRecording(vtkMRMLPlusRemoteNode*);

  /// Sends a command to Plus to reconstruct a volume with the specified parmeters
  void StartOfflineReconstruction(vtkMRMLPlusRemoteNode*);

  /// Sends a command to Plus to start scout scan volume reconstruction with the specified parameters
  void StartScoutScan(vtkMRMLPlusRemoteNode*);
  /// Sends a command to Plus to stop scout scan volume reconstruction with the specified parameters
  void StopScoutScan(vtkMRMLPlusRemoteNode*);

  /// Sends a command to Plus to start live volume reconstruction with the specified parameters
  void StartLiveVolumeReconstruction(vtkMRMLPlusRemoteNode*);
  /// Sends a command to Plus aquire a snapshot of the currentlive volume reconstruction
  void GetLiveVolumeReconstructionSnapshot(vtkMRMLPlusRemoteNode*);
  /// Sends a command to Plus to stop live volume reconstruction
  void StopLiveVolumeReconstruction(vtkMRMLPlusRemoteNode*);

  /// Sends a command to Plus to update a static transform that specified by the config file
  void UpdateTransform(vtkMRMLPlusRemoteNode*);
  /// Sends a command to Plus to save the current config file, including modified parameters such as updated transforms
  void SaveConfigFile(vtkMRMLPlusRemoteNode*);
  /// Displays the specified volume node in the views contained in sliceWidgetNames
  void ShowVolumeInSliceViewers(vtkMRMLVolumeNode* volumeNode, std::vector<std::string> sliceWidgetNames);
  /// Enables volume rendering for the specified volume node
  void ShowVolumeRendering(vtkMRMLVolumeNode* volumeNode);

  /// Sends GetLiveVolumeReconstructionSnapshot commands to all PlusRemote nodes that are constructing live volume reconstruction with a
  /// snapshot timer > 0
  void UpdateAllPlusRemoteNodes();

  ///////////////////////
  //Static methods

  /// Parses the command reply to get the filename of the reconstructed volume. (Assumes the filename is the section of the string after the last "/" character)
  static std::string ParseFilenameFromMessage(std::string filename);
  /// Returns the current date and time as a string using the specified format
  static std::string GetTimestampString(std::string format = "%Y%m%d_%H%M%S");
  /// Returns the specified filename, modified to include the timestamp (Ex: filename.ext -> filename_YmD_HMS.ext
  static std::string AddTimestampToFilename(std::string filename);
  /// Sets the bounds of the ROI node to the bounds of the volume node
  static void InitializeROIFromVolume(vtkMRMLAnnotationROINode* roiNode, vtkMRMLVolumeNode* volumeNode);

protected:

  static std::string GetScoutScanOutputFilename() { return "ScoutScan.mha"; };
  static std::string GetScoutScanDeviceName() { return "ScoutScan"; };

  ///////////////////////
  // Callback functions for when commands are received
  static void onRequestCaptureDeviceIDsCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onRequestVolumeReconstructorDeviceIDsCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onRequestDeviceIDsCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void onRecordingStarted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onRecordingCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  //static void onOfflineVolumeReconstructionStarted(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void onOfflineVolumeReconstructionCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void onScoutScanStarted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onScoutScanRecorded(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onScoutScanCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void onLiveVolumeReconstructionStarted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onLiveVolumeSnapshotAquired(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onLiveVolumeReconstructionCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void onPrintCommandResponseRequested(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  ///////////////////////
  // Delayed callbacks called when the required volumes in the scene are updated, invoked when the corresponding devices are modified
  static void onOfflineVolumeReconstructedFinalize(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onScoutVolumeReconstructedFinalize(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onSnapshotAquiredFinalize(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void onLiveVolumeReconstructedFinalized(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  double CommandTimeoutSec;
  bool CommandBlocking;
  std::string CommandIGTLIODeviceID;

  typedef std::map<vtkMRMLPlusRemoteNode*, ParameterNodeCommands> ParameterNodeMapType;
  ParameterNodeMapType NodeCommandMap;

private:
  vtkSlicerPlusRemoteLogic(const vtkSlicerPlusRemoteLogic&); // Not implemented
  void operator=(const vtkSlicerPlusRemoteLogic&); // Not implemented
};

#endif
