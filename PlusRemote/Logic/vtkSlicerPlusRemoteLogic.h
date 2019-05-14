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
class vtkCollection;
class vtkMRMLAnnotationROINode;
class vtkMRMLDisplayableNode;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLPlusRemoteNode;
class vtkMRMLLinearTransformNode;
class vtkMRMLVolumeNode;
class vtkMRMLPlusServerLauncherNode;
class vtkMRMLPlusServerNode;

struct ParameterNodeCommands;
struct LauncherCommands;
struct ServerCommands;

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


  void ProcessMRMLSceneEvents(vtkObject *caller, unsigned long event, void *callData) override;

  /// Initialize listening to MRML events
  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  virtual void OnMRMLSceneNodeAboutToBeRemoved(vtkMRMLNode* node);

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

  /// Calls UpdateLauncher on all vtkMRMLPlusServerLauncherNode in the Scene
  /// \sa UpdateLauncher()
  void UpdateAllLaunchers();

  /// Update a vtkMRMLPlusServerLauncherNode
  /// Calls UpdateLauncherConnectorNode, SendGetRunningServersCommand and if the launcher is not connected, sets status
  /// of all servers to OFF.
  /// \sa UpdateLauncherConnectorNode()
  /// \sa SendGetRunningServersCommand()
  void UpdateLauncher(vtkMRMLPlusServerLauncherNode* launcherNode);

  /// If the launcher has no connector node, calls GetOrAddConnectorNode and then updates the launcher hostname/port
  // \sa GetOrAddConnectorNode()
  void UpdateLauncherConnectorNode(vtkMRMLPlusServerLauncherNode* launcherNode);

  /// Send a command to the connector node for the specified launcher in order to get a list of the currently running servers
  /// \sa OnGetRunningServersCompleted()
  void SendGetRunningServersCommand(vtkMRMLPlusServerLauncherNode* launcherNode);

  /// Searches the scene for a vtkMRMLIGTLConnector node matching the hostname and port specified by the launcher node and
  /// creates one if it does not exist
  vtkMRMLIGTLConnectorNode* GetOrAddConnectorNode(std::string hostname, int port, std::string baseName="");

  /// Calls UpdateServer on all vtkMRMLPlusServerNode in the scene
  /// \sa UpdateServer()
  void UpdateAllServers();

  /// If a server node is controlled locally and it's state doesn't match the desired state, then calls SendStartServerCommand
  /// or SendStopServerCommand to Start/Stop the server and match the desired state
  /// \sa SendStartServerCommand()
  /// \sa SendStopServerCommand()
  void UpdateServer(vtkMRMLPlusServerNode* serverNode);

  /// Send a command to the connector node to start the specified server
  void SendStartServerCommand(vtkMRMLPlusServerNode* serverNode);

  /// Send a command to the connector node to stop the specified server
  void SendStopServerCommand(vtkMRMLPlusServerNode* serverNode);

  void UpdatePlusOpenIGTLinkConnectors(vtkMRMLPlusServerNode* serverNode);

  /// Callback function when a response is received for a GetRunningServersCommand.
  /// Sends GetConfigFileContentsCompleted if an unknown remote server is detected
  /// \sa SendGetRunningServersCommand()
  /// \sa OnGetConfigFileContentsCompleted()
  static void OnGetRunningServersCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  /// Callback function when a response is received for GetConfigFileContentsCompleted.
  /// sa OnGetRunningServersCompleted()
  static void OnGetConfigFileContentsCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  /// Callback when a command is received on the connector node used by a launcher node
  static void OnLauncherCommandReceived(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

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
  static void OnRequestCaptureDeviceIDsCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnRequestVolumeReconstructorDeviceIDsCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnRequestDeviceIDsCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void OnRecordingStarted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnRecordingCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  //static void OnOfflineVolumeReconstructionStarted(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void OnOfflineVolumeReconstructionCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void OnScoutScanStarted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnScoutScanRecorded(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnScoutScanCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void OnLiveVolumeReconstructionStarted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnLiveVolumeSnapshotAquired(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnLiveVolumeReconstructionCompleted(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void OnPrintCommandResponseRequested(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  ///////////////////////
  // Delayed callbacks called when the required volumes in the scene are updated, invoked when the corresponding devices are modified
  static void OnOfflineVolumeReconstructedFinalize(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnScoutVolumeReconstructedFinalize(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnSnapshotAquiredFinalize(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void OnLiveVolumeReconstructedFinalized(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  double CommandTimeoutSec;
  bool CommandBlocking;
  std::string CommandIGTLIODeviceID;

  typedef std::map<vtkMRMLPlusRemoteNode*, ParameterNodeCommands> ParameterNodeMapType;
  ParameterNodeMapType NodeCommandMap;

  typedef std::map<vtkMRMLPlusServerLauncherNode*, LauncherCommands> LauncherCommandMapType;
  LauncherCommandMapType LauncherCommandMap;

  typedef std::map<vtkMRMLPlusServerNode*, ServerCommands> ServerCommandMapType;
  ServerCommandMapType ServerCommandMap;

private:
  vtkSlicerPlusRemoteLogic(const vtkSlicerPlusRemoteLogic&); // Not implemented
  void operator=(const vtkSlicerPlusRemoteLogic&); // Not implemented
};

#endif
