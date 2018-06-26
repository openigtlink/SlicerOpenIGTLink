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

#ifndef __vtkMRMLPlusRemoteNode_h
#define __vtkMRMLPlusRemoteNode_h

#include "vtkMRMLPlusRemoteNode.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLAnnotationROINode.h>
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkCommand.h>

// Slicer includes
#include <vtkAddonSetGet.h>

// STD includes
#include <vector>
#include <map>

// PlusRemote includes
#include "vtkSlicerPlusRemoteModuleMRMLExport.h"

class vtkMRMLIGTLConnectorNode;

class VTK_SLICER_PLUSREMOTE_MODULE_MRML_EXPORT vtkMRMLPlusRemoteNode : public vtkMRMLNode
{
public:
  static vtkMRMLPlusRemoteNode *New();
  vtkTypeMacro(vtkMRMLPlusRemoteNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Create instance of a GAD node.
  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  /// Set node attributes from name/value pairs
  virtual void ReadXMLAttributes(const char** atts) VTK_OVERRIDE;

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  /// Get unique node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE{ return "PlusRemote"; };

  static const char* CONNECTOR_REFERENCE_ROLE;
  static const char* ROI_REFERENCE_ROLE;

  enum PlusRemoteStatus
  {
    PLUS_REMOTE_IDLE,
    PLUS_REMOTE_RECORDING,
    PLUS_REMOTE_RECONSTRUCTING,
    PLUS_REMOTE_IN_PROGRESS,
    PLUS_REMOTE_FAILED,
    PLUS_REMOTE_SUCCESS,
    PLUS_REMOTE_STATUS_LAST,
  };
  const char* GetPlusRemoteStatusAsString(int id);
  int GetPlusRemoteStatusFromString(const char* name);

  enum
  {
    CaptureIdsReceivedEvent                       = vtkCommand::UserEvent + 300,
    VolumeReconstructorIdsReceivedEvent           = vtkCommand::UserEvent + 301,
    RecordingStartedEvent                         = vtkCommand::UserEvent + 302,
    RecordingCompletedEvent                       = vtkCommand::UserEvent + 303,
    OfflineReconstructionStartedEvent             = vtkCommand::UserEvent + 304,
    OfflineReconstructionCompletedEvent           = vtkCommand::UserEvent + 305,
    ScoutScanStartedEvent                         = vtkCommand::UserEvent + 306,
    ScoutScanCompletedEvent                       = vtkCommand::UserEvent + 307,
    LiveVolumeReconstructionStartedEvent          = vtkCommand::UserEvent + 308,
    LiveVolumeReconstructionSnapshotReceivedEvent = vtkCommand::UserEvent + 309,
    LiveVolumeReconstructionCompletedEvent        = vtkCommand::UserEvent + 310,
  };

protected:
  vtkMRMLPlusRemoteNode();
  ~vtkMRMLPlusRemoteNode();

  PlusRemoteStatus Status;

  // Device parameters
  std::vector<std::string>    CaptureIDs;
  std::string                 CurrentCaptureID;
  std::vector<std::string>    VolumeReconstructorIDs;
  std::string                 CurrentVolumeReconstructorID;

  // Recording parameters
  int                         RecordingStatus;
  std::string                 RecordingMessage;
  std::string                 RecordingFilename;
  bool                        RecordingFilenameCompletion;
  bool                        RecordingEnableCompression;
  std::vector<std::string>    RecordedVolumes;

  // Offline volume reconstruction parameters
  int                         OfflineReconstructionStatus;
  std::string                 OfflineReconstructionMessage;
  double                      OfflineReconstructionSpacing;
  std::string                 OfflineReconstructionDevice;
  std::string                 OfflineReconstructionInputFilename;
  std::string                 OfflineReconstructionOutputFilename;
  bool                        OfflineReconstructionShowResultOnCompletion;

  // Scout scan reconstruction parameters
  int                         ScoutScanStatus;
  std::string                 ScoutScanMessage;
  double                      ScoutScanSpacing;
  std::string                 ScoutScanFilename;
  bool                        ScoutScanFilenameCompletion;
  bool                        ScoutScanShowResultOnCompletion;
  std::string                 ScoutLastRecordedVolume;

  // Live volume reconstrution parameters
  int                         LiveReconstructionStatus;
  std::string                 LiveReconstructionMessage;
  bool                        LiveReconstructionApplyHoleFilling;
  bool                        LiveReconstructionDisplayROI;
  double                      LiveReconstructionSpacing;
  std::string                 LiveReconstructionDevice;
  std::string                 LiveReconstructionFilename;
  bool                        LiveReconstructionFilenameCompletion;
  int                         LiveReconstructionROIDimensions[3];
  double                      LiveReconstructionSnapshotsIntervalSec;
  bool                        LiveReconstructionShowResultOnCompletion;
  double                      LiveReconstructionLastSnapshotTimestampSec;

  // Config file parameters
  std::string                 ConfigFilename;

  vtkSmartPointer<vtkCallbackCommand> ROINodeModifiedCallback;

  std::string ResponseText;

public:

  //////////////////////////
  // Device parameter setters/getters
  vtkSetStdVectorMacro(CaptureIDs, std::vector<std::string>);
  vtkGetStdVectorMacro(CaptureIDs, std::vector<std::string>);
  vtkSetMacro(CurrentCaptureID, std::string);
  vtkGetMacro(CurrentCaptureID, std::string);

  vtkSetStdVectorMacro(VolumeReconstructorIDs, std::vector<std::string>);
  vtkGetStdVectorMacro(VolumeReconstructorIDs, std::vector<std::string>);
  vtkSetMacro(CurrentVolumeReconstructorID, std::string);
  vtkGetMacro(CurrentVolumeReconstructorID, std::string);

  //////////////////////////
  // Recording parameter setters/getters
  vtkSetMacro(RecordingStatus, int);
  vtkGetMacro(RecordingStatus, int);
  const char* GetRecordingStatusAsString(int id) { return this->GetPlusRemoteStatusAsString(id); };
  int GetRecordingStatusFromString(const char* name) { return this->GetPlusRemoteStatusFromString(name); };

  vtkSetMacro(RecordingMessage, std::string);
  vtkGetMacro(RecordingMessage, std::string);

  vtkSetMacro(RecordingFilename, std::string);
  vtkGetMacro(RecordingFilename, std::string);

  vtkSetMacro(RecordingFilenameCompletion, bool);
  vtkGetMacro(RecordingFilenameCompletion, bool);
  vtkBooleanMacro(RecordingFilenameCompletion, bool);

  vtkSetMacro(RecordingEnableCompression, bool);
  vtkGetMacro(RecordingEnableCompression, bool);
  vtkBooleanMacro(RecordingEnableCompression, bool);

  vtkSetStdVectorMacro(RecordedVolumes, std::vector<std::string>);
  vtkGetStdVectorMacro(RecordedVolumes, std::vector<std::string>);
  void ClearRecordedVolumes() { this->RecordedVolumes.clear(); };
  bool AddRecordedVolume(std::string volumeName);

  //////////////////////////
  // Offline volume reconstruction parameter setters/getters
  vtkSetMacro(OfflineReconstructionStatus, int);
  vtkGetMacro(OfflineReconstructionStatus, int);
  const char* GetOfflineReconstructionStatusAsString(int id) { return this->GetPlusRemoteStatusAsString(id); };
  int GetOfflineReconstructionStatusFromString(const char* name) { return this->GetPlusRemoteStatusFromString(name); };

  vtkSetMacro(OfflineReconstructionMessage, std::string);
  vtkGetMacro(OfflineReconstructionMessage, std::string);

  vtkSetMacro(OfflineReconstructionSpacing, double);
  vtkGetMacro(OfflineReconstructionSpacing, double);

  vtkSetMacro(OfflineReconstructionDevice, std::string);
  vtkGetMacro(OfflineReconstructionDevice, std::string);

  vtkSetMacro(OfflineReconstructionInputFilename, std::string);
  vtkGetMacro(OfflineReconstructionInputFilename, std::string);

  vtkSetMacro(OfflineReconstructionOutputFilename, std::string);
  vtkGetMacro(OfflineReconstructionOutputFilename, std::string);

  vtkSetMacro(OfflineReconstructionShowResultOnCompletion, bool);
  vtkGetMacro(OfflineReconstructionShowResultOnCompletion, bool);
  vtkBooleanMacro(OfflineReconstructionShowResultOnCompletion, bool);

  //////////////////////////
  // Scout scan reconstruction parameter setters/getters
  vtkSetMacro(ScoutScanStatus, int);
  vtkGetMacro(ScoutScanStatus, int);
  const char* GetScoutScanStatusAsString(int id) { return this->GetPlusRemoteStatusAsString(id); };
  int GetScoutScanStatusFromString(const char* name) { return this->GetPlusRemoteStatusFromString(name); };

  vtkSetMacro(ScoutScanMessage, std::string);
  vtkGetMacro(ScoutScanMessage, std::string);

  vtkSetMacro(ScoutScanSpacing, double);
  vtkGetMacro(ScoutScanSpacing, double);

  vtkSetMacro(ScoutScanFilename, std::string);
  vtkGetMacro(ScoutScanFilename, std::string);

  vtkSetMacro(ScoutScanFilenameCompletion, bool);
  vtkGetMacro(ScoutScanFilenameCompletion, bool);
  vtkBooleanMacro(ScoutScanFilenameCompletion, bool);

  vtkSetMacro(ScoutScanShowResultOnCompletion, bool);
  vtkGetMacro(ScoutScanShowResultOnCompletion, bool);
  vtkBooleanMacro(ScoutScanShowResultOnCompletion, bool);

  vtkSetMacro(ScoutLastRecordedVolume, std::string);
  vtkGetMacro(ScoutLastRecordedVolume, std::string);

  //////////////////////////
  // Live volume reconstrution parameter setters/getters
  vtkSetMacro(LiveReconstructionStatus, int);
  vtkGetMacro(LiveReconstructionStatus, int);
  const char* GetLiveReconstructionStatusAsString(int id) { return this->GetPlusRemoteStatusAsString(id); };
  int GetLiveReconstructionStatusFromString(const char* name) { return this->GetPlusRemoteStatusFromString(name); };

  vtkSetMacro(LiveReconstructionMessage, std::string);
  vtkGetMacro(LiveReconstructionMessage, std::string);

  vtkSetMacro(LiveReconstructionApplyHoleFilling, bool);
  vtkGetMacro(LiveReconstructionApplyHoleFilling, bool);

  void SetLiveReconstructionDisplayROI(bool);
  vtkGetMacro(LiveReconstructionDisplayROI, bool);
  vtkBooleanMacro(LiveReconstructionDisplayROI, bool);

  void SetLiveReconstructionSpacing(double);
  vtkGetMacro(LiveReconstructionSpacing, double);

  vtkSetMacro(LiveReconstructionDevice, std::string);
  vtkGetMacro(LiveReconstructionDevice, std::string);

  vtkSetMacro(LiveReconstructionFilename, std::string);
  vtkGetMacro(LiveReconstructionFilename, std::string);

  vtkSetMacro(LiveReconstructionFilenameCompletion, bool);
  vtkGetMacro(LiveReconstructionFilenameCompletion, bool);
  vtkBooleanMacro(LiveReconstructionFilenameCompletion, bool);

  void SetLiveReconstructionROIDimensions(int x, int y, int z);
  void SetLiveReconstructionROIDimensions(const int dimensions[3]);
  vtkGetVector3Macro(LiveReconstructionROIDimensions, int);
  void GetLiveReconstructionROIExtent(int (&extent)[6])
  {
    for (int i = 0; i < 3; ++i)
    {
      extent[2 * i] = 0;
      extent[2 * i + 1] = this->LiveReconstructionROIDimensions[i] - 1;
    }
  };

  vtkSetMacro(LiveReconstructionSnapshotsIntervalSec, double);
  vtkGetMacro(LiveReconstructionSnapshotsIntervalSec, double);

  vtkSetMacro(LiveReconstructionShowResultOnCompletion, bool);
  vtkGetMacro(LiveReconstructionShowResultOnCompletion, bool);
  vtkBooleanMacro(LiveReconstructionShowResultOnCompletion, bool);

  vtkSetMacro(LiveReconstructionLastSnapshotTimestampSec, double);
  vtkGetMacro(LiveReconstructionLastSnapshotTimestampSec, double);

  //////////////////////////
  // Config file parameter setters/getters
  vtkSetMacro(ConfigFilename, std::string);
  vtkGetMacro(ConfigFilename, std::string);

  void SetAndObserveOpenIGTLinkConnectorNode(vtkMRMLIGTLConnectorNode* connectorNode);
  vtkMRMLIGTLConnectorNode* GetOpenIGTLinkConnectorNode();

  void SetAndObserveLiveReconstructionROINode(vtkMRMLAnnotationROINode* connectorNode);
  vtkMRMLAnnotationROINode* GetLiveReconstructionROINode();

  vtkSetMacro(ResponseText, std::string);
  vtkGetMacro(ResponseText, std::string);

protected:
  // Invoked if the roi node is modified
  // Adjusts the dimensions of the live reconstruction to match
  static void OnROINodeModified(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata));

private:
  vtkMRMLPlusRemoteNode(const vtkMRMLPlusRemoteNode&);
  void operator=(const vtkMRMLPlusRemoteNode&);

};

#endif
