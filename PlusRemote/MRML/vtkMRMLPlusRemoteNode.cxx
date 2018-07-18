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

// PlusRemote includes
#include "vtkMRMLPlusRemoteNode.h"

// OpenIGTLinkIF includes
#include "vtkMRMLIGTLConnectorNode.h"

// Slicer includes
#include <vtkMRMLLinearTransformNode.h>

// std includes
#include <sstream>

// VTK include
#include <vtkStringArray.h>

// vtksys includes
#include <vtksys/SystemTools.hxx>

//------------------------------------------------------------------------------
const char* vtkMRMLPlusRemoteNode::CONNECTOR_REFERENCE_ROLE = "OpenIGTLinkConnectorNode";
const char* vtkMRMLPlusRemoteNode::ROI_REFERENCE_ROLE = "LiveReconstructionROINode";
const char* vtkMRMLPlusRemoteNode::UPDATE_TRANSFORM_REFERENCE_ROLE = "UpdateTransformNode";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusRemoteNode);

//----------------------------------------------------------------------------
vtkMRMLPlusRemoteNode::vtkMRMLPlusRemoteNode()
  : RecordingStatus(PLUS_REMOTE_IDLE)
  , RecordingFilename("Recording.mha")
  , RecordingFilenameCompletion(false)
  , RecordingEnableCompression(false)

  , OfflineReconstructionStatus(PLUS_REMOTE_IDLE)
  , OfflineReconstructionSpacing(3.0)
  , OfflineReconstructionDevice("RecVol_Reference")
  , OfflineReconstructionOutputFilename("RecVol_Reference.mha")
  , OfflineReconstructionShowResultOnCompletion(true)

  , ScoutScanStatus(PLUS_REMOTE_IDLE)
  , ScoutScanSpacing(3.0)
  , ScoutScanFilename("ScoutScanRecording.mha")
  , ScoutScanFilenameCompletion(false)
  , ScoutScanShowResultOnCompletion(true)

  , LiveReconstructionStatus(PLUS_REMOTE_IDLE)
  , LiveReconstructionApplyHoleFilling(false)
  , LiveReconstructionDisplayROI(false)
  , LiveReconstructionSpacing(1.0)
  , LiveReconstructionDevice("liveReconstruction")
  , LiveReconstructionFilename("LiveReconstructedVolume.mha")
  , LiveReconstructionFilenameCompletion(false)
  , LiveReconstructionSnapshotsIntervalSec(3)
  , LiveReconstructionShowResultOnCompletion(true)
  , LiveReconstructionLastSnapshotTimestampSec(0.0)

  , ROINodeModifiedCallback(vtkSmartPointer<vtkCallbackCommand>::New())
{
  this->LiveReconstructionROIDimensions[0] = 0.0;
  this->LiveReconstructionROIDimensions[1] = 0.0;
  this->LiveReconstructionROIDimensions[2] = 0.0;

  this->ROINodeModifiedCallback->SetCallback(vtkMRMLPlusRemoteNode::OnROINodeModified);
  this->ROINodeModifiedCallback->SetClientData(this);
}

//----------------------------------------------------------------------------
vtkMRMLPlusRemoteNode::~vtkMRMLPlusRemoteNode()
{
}

//-----------------------------------------------------------
const char* vtkMRMLPlusRemoteNode::GetPlusRemoteStatusAsString(int id)
{
  switch (id)
  {
  case PLUS_REMOTE_FAILED: return "Failed";
  case PLUS_REMOTE_IDLE: return "Idle";
  case PLUS_REMOTE_IN_PROGRESS: return "InProgress";
  case PLUS_REMOTE_RECONSTRUCTING: return "Reconstructing";
  case PLUS_REMOTE_RECORDING: return "Recording";
  case PLUS_REMOTE_SUCCESS: return "Success";
  default:
    // invalid id
    return "";
  }
}

//-----------------------------------------------------------
int vtkMRMLPlusRemoteNode::GetPlusRemoteStatusFromString(const char* name)
{
  if (name == NULL)
  {
    // invalid name
    return -1;
  }
  for (int i = 0; i < vtkMRMLPlusRemoteNode::PLUS_REMOTE_STATUS_LAST; i++)
  {
    if (strcmp(name, this->GetPlusRemoteStatusAsString(i)) == 0)
    {
      // found a matching name
      return i;
    }
  }
  // unknown name
  return -1;
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::SetAndObserveOpenIGTLinkConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }
  this->SetNodeReferenceID(CONNECTOR_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkMRMLPlusRemoteNode::GetOpenIGTLinkConnectorNode()
{
  return vtkMRMLIGTLConnectorNode::SafeDownCast(this->GetNodeReference(CONNECTOR_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::SetAndObserveUpdatedTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }
  this->SetNodeReferenceID(UPDATE_TRANSFORM_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLPlusRemoteNode::GetUpdatedTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(UPDATE_TRANSFORM_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::GetDeviceIDs(vtkStringArray* stringArray)
{
  if (!stringArray)
  {
    return;
  }

  for (std::vector<std::string>::iterator deviceIDIt = this->DeviceIDs.begin(); deviceIDIt != this->DeviceIDs.end(); ++deviceIDIt)
  {
    stringArray->InsertNextValue((*deviceIDIt).c_str());
  }
}

//----------------------------------------------------------------------------
bool vtkMRMLPlusRemoteNode::AddRecordedVolume(std::string volumeName)
{
  std::vector<std::string>::iterator it = std::find(this->RecordedVolumes.begin(), this->RecordedVolumes.end(), volumeName);
  if (it == this->RecordedVolumes.end())
  {
    this->RecordedVolumes.push_back(volumeName);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
vtkMRMLAnnotationROINode* vtkMRMLPlusRemoteNode::GetLiveReconstructionROINode()
{
  if (!this->Scene)
  {
    vtkErrorMacro("GetLiveReconstructionROINode: Invalid MRML scene!");
    return NULL;
  }

  vtkMRMLAnnotationROINode* roiNode = vtkMRMLAnnotationROINode::SafeDownCast(this->GetNodeReference(ROI_REFERENCE_ROLE));
  return roiNode;
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::SetAndObserveLiveReconstructionROINode(vtkMRMLAnnotationROINode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }

  vtkMRMLAnnotationROINode* oldNode = vtkMRMLAnnotationROINode::SafeDownCast(this->GetNodeReference(ROI_REFERENCE_ROLE));

  if (node == oldNode)
  {
    return;
  }

  if (oldNode)
  {
    oldNode->RemoveObserver(this->ROINodeModifiedCallback);
  }
  this->SetNodeReferenceID(ROI_REFERENCE_ROLE, (node ? node->GetID() : NULL));

  if (node)
  {
    node->AddObserver(vtkCommand::ModifiedEvent, this->ROINodeModifiedCallback);
    node->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::OnROINodeModified(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  vtkSmartPointer<vtkMRMLAnnotationROINode> roiNode = vtkMRMLAnnotationROINode::SafeDownCast(caller);

  vtkMRMLPlusRemoteNode* self = reinterpret_cast<vtkMRMLPlusRemoteNode*>(clientdata);

  double roiCenter[3] = { 0.0, 0.0, 0.0 };
  roiNode->GetXYZ(roiCenter);

  double roiRadius[3] = { 0.0, 0.0, 0.0 };
  roiNode->GetRadiusXYZ(roiRadius);

  double roiOrigin[3] = { 0.0, 0.0, 0.0 };
  for (int i = 0; i < 3; ++i)
  {
    roiOrigin[i] = roiCenter[i] - roiRadius[i];
  }

  double outputSpacing = self->GetLiveReconstructionSpacing();
  int outputDimensions[3] = {
    (int)std::ceil((2 * roiRadius[0]) / outputSpacing),
    (int)std::ceil((2 * roiRadius[1]) / outputSpacing),
    (int)std::ceil((2 * roiRadius[2]) / outputSpacing)
  };

  self->SetLiveReconstructionROIDimensions(outputDimensions);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::SetLiveReconstructionDisplayROI(bool displayROI)
{
  vtkMRMLAnnotationROINode* roiNode = this->GetLiveReconstructionROINode();
  if (roiNode)
  {
    roiNode->SetDisplayVisibility(displayROI);
  }

  bool oldVisibility = this->LiveReconstructionDisplayROI;
  this->LiveReconstructionDisplayROI = displayROI;
  if (oldVisibility != displayROI)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::SetLiveReconstructionSpacing(double spacing)
{
  double oldSpacing = this->LiveReconstructionSpacing;
  if (oldSpacing == spacing)
  {
    return;
  }
  int wasModifying = this->StartModify();

  this->LiveReconstructionSpacing = spacing;

  int oldDimensions[3] = { 0, 0, 0 };
  this->GetLiveReconstructionROIDimensions(oldDimensions);

  int newDimensions[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; ++i)
  {
    newDimensions[i] = std::ceil((oldSpacing*oldDimensions[i]) / spacing);
  }
  this->SetLiveReconstructionROIDimensions(newDimensions);
  this->Modified();
  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::SetLiveReconstructionROIDimensions(int x, int y, int z)
{
  int dimensions[3] = { x, y, z };
  this->SetLiveReconstructionROIDimensions(dimensions);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::SetLiveReconstructionROIDimensions(const int dimensions[3])
{
  bool modified = false;
  for (int i = 0; i < 3; ++i)
  {
    if (dimensions[i] != this->LiveReconstructionROIDimensions[i])
    {
      modified = true;
      break;
    }
  }

  if (!modified)
  {
    return;
  }

  int wasModifying = this->StartModify();

  for (int i = 0; i < 3; ++i)
  {
    this->LiveReconstructionROIDimensions[i] = dimensions[i];
  }

  vtkMRMLAnnotationROINode* roiNode = this->GetLiveReconstructionROINode();
  if (roiNode)
  {
    double spacing = this->LiveReconstructionSpacing;
    double radius[3] = { 0.0, 0.0, 0.0 };
    for (int i = 0; i < 3; ++i)
    {
      radius[i] = (dimensions[i] * spacing) / 2;
    }
    roiNode->SetRadiusXYZ(radius);
  }
  this->Modified();
  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);

  vtkMRMLWriteXMLStdStringVectorMacro(captureIDs, CaptureIDs, std::vector);
  vtkMRMLWriteXMLStdStringMacro(currentCaptureID, CurrentCaptureID);

  vtkMRMLWriteXMLStdStringVectorMacro(volumeReconstructorIDs, VolumeReconstructorIDs, std::vector);
  vtkMRMLWriteXMLStdStringMacro(currentVolumeReconstructorID, CurrentVolumeReconstructorID);

  vtkMRMLWriteXMLStdStringVectorMacro(deviceIDs, DeviceIDs, std::vector);
  vtkMRMLWriteXMLStdStringMacro(currentDeviceID, CurrentDeviceID);
  vtkMRMLWriteXMLStdStringMacro(deviceIDType, DeviceIDType);

  vtkMRMLWriteXMLEnumMacro(recordingStatus, RecordingStatus);
  vtkMRMLWriteXMLStdStringMacro(recordingMessage, RecordingMessage);
  vtkMRMLWriteXMLStdStringMacro(recordingFilename, RecordingFilename);
  vtkMRMLWriteXMLBooleanMacro(recordingFilenameCompletion, RecordingFilenameCompletion);
  vtkMRMLWriteXMLBooleanMacro(recordingEnableCompression, RecordingEnableCompression);
  vtkMRMLWriteXMLStdStringVectorMacro(recordedVolumes, RecordedVolumes, std::vector);

  vtkMRMLWriteXMLEnumMacro(offlineReconstructionStatus, OfflineReconstructionStatus);
  vtkMRMLWriteXMLStdStringMacro(offlineReconstructionMessage, OfflineReconstructionMessage);
  vtkMRMLWriteXMLFloatMacro(offlineReconstructionSpacing, OfflineReconstructionSpacing);
  vtkMRMLWriteXMLStdStringMacro(offlineReconstructionDevice, OfflineReconstructionDevice);
  vtkMRMLWriteXMLIntMacro(offlineReconstructionInputFilename, OfflineReconstructionInputFilename);
  vtkMRMLWriteXMLIntMacro(offlineReconstructionOutputFilename, OfflineReconstructionOutputFilename);
  vtkMRMLWriteXMLBooleanMacro(offlineReconstructionShowResultOnCompletion, OfflineReconstructionShowResultOnCompletion);

  vtkMRMLWriteXMLEnumMacro(scoutScanStatus, ScoutScanStatus);
  vtkMRMLWriteXMLStdStringMacro(scoutScanMessage, ScoutScanMessage);
  vtkMRMLWriteXMLFloatMacro(scoutScanSpacing, ScoutScanSpacing);
  vtkMRMLWriteXMLStdStringMacro(scoutScanFilename, ScoutScanFilename);
  vtkMRMLWriteXMLBooleanMacro(scoutScanFilenameCompletion, ScoutScanFilenameCompletion);
  vtkMRMLWriteXMLBooleanMacro(scoutScanShowResultOnCompletion, ScoutScanShowResultOnCompletion);
  vtkMRMLWriteXMLStdStringMacro(scoutLastRecordedVolume, ScoutLastRecordedVolume);

  vtkMRMLWriteXMLEnumMacro(liveReconstructionStatus, LiveReconstructionStatus);
  vtkMRMLWriteXMLStdStringMacro(LiveReconstructionMessage, LiveReconstructionMessage);
  vtkMRMLWriteXMLBooleanMacro(liveReconstructionApplyHoleFilling, LiveReconstructionApplyHoleFilling);
  vtkMRMLWriteXMLBooleanMacro(liveReconstructionDisplayROI, LiveReconstructionDisplayROI);
  vtkMRMLWriteXMLFloatMacro(liveReconstructionSpacing, LiveReconstructionSpacing);
  vtkMRMLWriteXMLStdStringMacro(liveReconstructionDevice, LiveReconstructionDevice);
  vtkMRMLWriteXMLStdStringMacro(liveReconstructionFilename, LiveReconstructionFilename);
  vtkMRMLWriteXMLBooleanMacro(liveReconstructionFilenameCompletion, LiveReconstructionFilenameCompletion);
  vtkMRMLWriteXMLVectorMacro(liveReconstructionROIDimensions, LiveReconstructionROIDimensions, int, 3);
  vtkMRMLWriteXMLFloatMacro(liveReconstructionSnapshotsIntervalSec, LiveReconstructionSnapshotsIntervalSec);
  vtkMRMLWriteXMLBooleanMacro(liveReconstructionShowResultOnCompletion, LiveReconstructionShowResultOnCompletion);
  vtkMRMLWriteXMLFloatMacro(LiveReconstructionLastSnapshotTimestampSec, LiveReconstructionLastSnapshotTimestampSec);

  vtkMRMLWriteXMLStdStringMacro(configFilename, ConfigFilename);

  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts);

  vtkMRMLReadXMLStdStringVectorMacro(captureIDs, CaptureIDs, std::vector);
  vtkMRMLReadXMLStdStringMacro(currentCaptureID, CurrentCaptureID);

  vtkMRMLReadXMLStdStringVectorMacro(volumeReconstructorIDs, VolumeReconstructorIDs, std::vector);
  vtkMRMLReadXMLStdStringMacro(currentVolumeReconstructorID, CurrentVolumeReconstructorID);

  vtkMRMLReadXMLStdStringVectorMacro(deviceIDs, DeviceIDs, std::vector);
  vtkMRMLReadXMLStdStringMacro(currentDeviceID, CurrentDeviceID);
  vtkMRMLReadXMLStdStringMacro(deviceIDType, DeviceIDType);

  vtkMRMLReadXMLEnumMacro(recordingStatus, RecordingStatus);
  vtkMRMLReadXMLStdStringMacro(recordingMessage, RecordingMessage);
  vtkMRMLReadXMLStdStringMacro(recordingFilename, RecordingFilename);
  vtkMRMLReadXMLBooleanMacro(recordingFilenameCompletion, RecordingFilenameCompletion);
  vtkMRMLReadXMLBooleanMacro(recordingEnableCompression, RecordingEnableCompression);
  vtkMRMLReadXMLStdStringVectorMacro(recordedVolumes, RecordedVolumes, std::vector);

  vtkMRMLReadXMLEnumMacro(offlineReconstructionStatus, OfflineReconstructionStatus);
  vtkMRMLReadXMLStdStringMacro(offlineReconstructionMessage, OfflineReconstructionMessage);
  vtkMRMLReadXMLFloatMacro(offlineReconstructionSpacing, OfflineReconstructionSpacing);
  vtkMRMLReadXMLStdStringMacro(offlineReconstructionDevice, OfflineReconstructionDevice);
  vtkMRMLReadXMLStdStringMacro(offlineReconstructionInputFilename, OfflineReconstructionInputFilename);
  vtkMRMLReadXMLStdStringMacro(offlineReconstructionOutputFilename, OfflineReconstructionOutputFilename);
  vtkMRMLReadXMLBooleanMacro(offlineReconstructionShowResultOnCompletion, OfflineReconstructionShowResultOnCompletion);

  vtkMRMLReadXMLEnumMacro(scoutScanStatus, ScoutScanStatus);
  vtkMRMLReadXMLStdStringMacro(scoutScanMessage, ScoutScanMessage);
  vtkMRMLReadXMLFloatMacro(scoutScanSpacing, ScoutScanSpacing);
  vtkMRMLReadXMLStdStringMacro(scoutScanFilename, ScoutScanFilename);
  vtkMRMLReadXMLBooleanMacro(scoutScanFilenameCompletion, ScoutScanFilenameCompletion);
  vtkMRMLReadXMLBooleanMacro(scoutScanShowResultOnCompletion, ScoutScanShowResultOnCompletion);

  vtkMRMLReadXMLEnumMacro(liveReconstructionStatus, LiveReconstructionStatus);
  vtkMRMLReadXMLStdStringMacro(liveReconstructionMessage, LiveReconstructionMessage);
  vtkMRMLReadXMLBooleanMacro(liveReconstructionApplyHoleFilling, LiveReconstructionApplyHoleFilling);
  vtkMRMLReadXMLBooleanMacro(liveReconstructionDisplayROI, LiveReconstructionDisplayROI);
  vtkMRMLReadXMLFloatMacro(liveReconstructionSpacing, LiveReconstructionSpacing);
  vtkMRMLReadXMLStdStringMacro(liveReconstructionDevice, LiveReconstructionDevice);
  vtkMRMLReadXMLStdStringMacro(liveReconstructionFilename, LiveReconstructionFilename);
  vtkMRMLReadXMLBooleanMacro(liveReconstructionFilenameCompletion, LiveReconstructionFilenameCompletion);
  vtkMRMLReadXMLVectorMacro(liveReconstructionROIDimensions, LiveReconstructionROIDimensions, int, 3);
  vtkMRMLReadXMLFloatMacro(liveReconstructionSnapshotsIntervalSec, LiveReconstructionSnapshotsIntervalSec);
  vtkMRMLReadXMLBooleanMacro(liveReconstructionShowResultOnCompletion, LiveReconstructionShowResultOnCompletion);

  vtkMRMLReadXMLStdStringMacro(configFilename, ConfigFilename);
  vtkMRMLReadXMLEndMacro();
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLPlusRemoteNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLCopyBeginMacro(anode);

  vtkMRMLCopyStdStringMacro(CurrentCaptureID);
  vtkMRMLCopyStdStringVectorMacroMacro(CaptureIDs);

  vtkMRMLCopyStdStringMacro(CurrentVolumeReconstructorID);
  vtkMRMLCopyStdStringVectorMacroMacro(VolumeReconstructorIDs);

  vtkMRMLCopyStdStringVectorMacroMacro(DeviceIDs);
  vtkMRMLCopyStdStringMacro(CurrentDeviceID);
  vtkMRMLCopyStdStringMacro(DeviceIDType);

  vtkMRMLCopyEnumMacro(RecordingStatus);
  vtkMRMLCopyStdStringMacro(RecordingMessage);
  vtkMRMLCopyStdStringMacro(RecordingFilename);
  vtkMRMLCopyBooleanMacro(RecordingFilenameCompletion);
  vtkMRMLCopyBooleanMacro(RecordingEnableCompression);
  vtkMRMLCopyStdStringVectorMacroMacro(RecordedVolumes);

  vtkMRMLCopyEnumMacro(OfflineReconstructionStatus);
  vtkMRMLCopyStdStringMacro(OfflineReconstructionMessage);
  vtkMRMLCopyFloatMacro(OfflineReconstructionSpacing);
  vtkMRMLCopyStdStringMacro(OfflineReconstructionDevice);
  vtkMRMLCopyStdStringMacro(OfflineReconstructionInputFilename);
  vtkMRMLCopyStdStringMacro(OfflineReconstructionOutputFilename);
  vtkMRMLCopyBooleanMacro(OfflineReconstructionShowResultOnCompletion);

  vtkMRMLCopyEnumMacro(ScoutScanStatus);
  vtkMRMLCopyStdStringMacro(ScoutScanMessage);
  vtkMRMLCopyFloatMacro(ScoutScanSpacing);
  vtkMRMLCopyStdStringMacro(ScoutScanFilename);
  vtkMRMLCopyBooleanMacro(ScoutScanFilenameCompletion);
  vtkMRMLCopyBooleanMacro(ScoutScanShowResultOnCompletion);

  vtkMRMLCopyEnumMacro(LiveReconstructionStatus);
  vtkMRMLCopyStdStringMacro(LiveReconstructionMessage);
  vtkMRMLCopyBooleanMacro(LiveReconstructionApplyHoleFilling);
  vtkMRMLCopyBooleanMacro(LiveReconstructionDisplayROI);
  vtkMRMLCopyFloatMacro(LiveReconstructionSpacing);
  vtkMRMLCopyStdStringMacro(LiveReconstructionDevice);
  vtkMRMLCopyStdStringMacro(LiveReconstructionFilename);
  vtkMRMLCopyBooleanMacro(LiveReconstructionFilenameCompletion);
  vtkMRMLCopyVectorMacro(LiveReconstructionROIDimensions, int, 3);
  vtkMRMLCopyFloatMacro(LiveReconstructionSnapshotsIntervalSec);
  vtkMRMLCopyBooleanMacro(LiveReconstructionShowResultOnCompletion);

  vtkMRMLCopyStdStringMacro(ConfigFilename);

  vtkMRMLCopyEndMacro();

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  vtkMRMLPrintBeginMacro(os, indent);

  vtkMRMLPrintStdStringMacro(CurrentCaptureID);
  vtkMRMLPrintStdStringVectorMacro(CaptureIDs, std::vector);

  vtkMRMLPrintStdStringMacro(CurrentVolumeReconstructorID);
  vtkMRMLPrintStdStringVectorMacro(VolumeReconstructorIDs, std::vector);

  vtkMRMLPrintStdStringVectorMacro(DeviceIDs, std::vector);
  vtkMRMLPrintStdStringMacro(CurrentDeviceID);
  vtkMRMLPrintStdStringMacro(DeviceIDType);

  vtkMRMLPrintEnumMacro(RecordingStatus);
  vtkMRMLPrintStdStringMacro(RecordingMessage);
  vtkMRMLPrintStdStringMacro(RecordingFilename);
  vtkMRMLPrintBooleanMacro(RecordingFilenameCompletion);
  vtkMRMLPrintBooleanMacro(RecordingEnableCompression);
  vtkMRMLPrintStdStringVectorMacro(RecordedVolumes, std::vector);

  vtkMRMLPrintEnumMacro(OfflineReconstructionStatus);
  vtkMRMLPrintStdStringMacro(OfflineReconstructionMessage);
  vtkMRMLPrintFloatMacro(OfflineReconstructionSpacing);
  vtkMRMLPrintStdStringMacro(OfflineReconstructionDevice);
  vtkMRMLPrintStdStringMacro(OfflineReconstructionInputFilename);
  vtkMRMLPrintStdStringMacro(OfflineReconstructionOutputFilename);
  vtkMRMLPrintBooleanMacro(OfflineReconstructionShowResultOnCompletion);

  vtkMRMLPrintEnumMacro(ScoutScanStatus);
  vtkMRMLPrintStdStringMacro(ScoutScanMessage);
  vtkMRMLPrintFloatMacro(ScoutScanSpacing);
  vtkMRMLPrintStdStringMacro(ScoutScanFilename);
  vtkMRMLPrintBooleanMacro(ScoutScanFilenameCompletion);
  vtkMRMLPrintBooleanMacro(ScoutScanShowResultOnCompletion);

  vtkMRMLPrintEnumMacro(LiveReconstructionStatus);
  vtkMRMLPrintStdStringMacro(LiveReconstructionMessage);
  vtkMRMLPrintBooleanMacro(LiveReconstructionApplyHoleFilling);
  vtkMRMLPrintBooleanMacro(LiveReconstructionDisplayROI);
  vtkMRMLPrintFloatMacro(LiveReconstructionSpacing);
  vtkMRMLPrintStdStringMacro(LiveReconstructionDevice);
  vtkMRMLPrintStdStringMacro(LiveReconstructionFilename);
  vtkMRMLPrintBooleanMacro(LiveReconstructionFilenameCompletion);
  vtkMRMLPrintVectorMacro(LiveReconstructionROIDimensions, int, 3);
  vtkMRMLPrintFloatMacro(LiveReconstructionSnapshotsIntervalSec);
  vtkMRMLPrintBooleanMacro(LiveReconstructionShowResultOnCompletion);

  vtkMRMLPrintStdStringMacro(ConfigFilename);

  vtkMRMLPrintEndMacro();
}
