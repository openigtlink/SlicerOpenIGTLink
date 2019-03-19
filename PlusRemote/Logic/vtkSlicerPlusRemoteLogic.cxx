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
#include "vtkSlicerPlusRemoteLogic.h"
#include "vtkMRMLPlusRemoteNode.h"
#include "vtkMRMLPlusServerLauncherRemoteNode.h"

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkTimerLog.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

// Slicer includes
#include <qSlicerAbstractCoreModule.h>
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <vtkSlicerVolumeRenderingLogic.h>

// MRML includes
#include <vtkMRMLAnnotationROINode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLVolumeRenderingDisplayNode.h>

// OpenIGTLinkIF includes
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include "igtlioDevice.h"

//----------------------------------------------------------------------------
class vtkPlusRemoteLogicCallbackCommand : public vtkCallbackCommand
{
public:
  static vtkPlusRemoteLogicCallbackCommand *New()
  {
    return new vtkPlusRemoteLogicCallbackCommand;
  }

  ///
  vtkWeakPointer<vtkSlicerPlusRemoteLogic> Logic;
  vtkWeakPointer<vtkMRMLPlusRemoteNode> ParameterNode;

};

//---------------------------------------------------------------------------
struct ParameterNodeCommand
{
  igtlioCommandPointer Command;
  vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand> Callback;
  ParameterNodeCommand()
    : Command(igtlioCommandPointer::New())
    , Callback(vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand>::New())
  {
    this->Command->AddObserver(igtlioCommand::CommandCompletedEvent, this->Callback);
    this->Callback->SetClientData(this->Callback.GetPointer());
  }
};

//---------------------------------------------------------------------------
struct ParameterNodeCommands
{
  double CommandTimeoutSec;
  bool CommandBlocking;
  ParameterNodeCommand CmdGetCaptureDeviceIDs;
  ParameterNodeCommand CmdGetReconstructorDeviceIDs;
  ParameterNodeCommand CmdGetDeviceIDs;
  ParameterNodeCommand CmdStartVolumeReconstruction;
  ParameterNodeCommand CmdStopVolumeReconstruction;
  ParameterNodeCommand CmdReconstructRecorded;
  ParameterNodeCommand CmdStartRecording;
  ParameterNodeCommand CmdStopRecording;
  ParameterNodeCommand CmdGetVolumeReconstructionSnapshot;
  ParameterNodeCommand CmdUpdateTransform;
  ParameterNodeCommand CmdSaveConfig;
  vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand> OfflineReconstructionCompletionCallback;
  vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand> ScoutScanCompletionCallback;
  vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand> LiveVolumeReconstructionSnapshotCallback;
  vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand> LiveVolumeReconstructionCompletionCallback;
  ParameterNodeCommands()
    : CommandTimeoutSec(30.0)
    , CommandBlocking(false)
    , CmdGetCaptureDeviceIDs(ParameterNodeCommand())
    , CmdGetReconstructorDeviceIDs(ParameterNodeCommand())
    , CmdGetDeviceIDs(ParameterNodeCommand())
    , CmdStartVolumeReconstruction(ParameterNodeCommand())
    , CmdStopVolumeReconstruction(ParameterNodeCommand())
    , CmdReconstructRecorded(ParameterNodeCommand())
    , CmdStartRecording(ParameterNodeCommand())
    , CmdStopRecording(ParameterNodeCommand())
    , CmdGetVolumeReconstructionSnapshot(ParameterNodeCommand())
    , CmdUpdateTransform(ParameterNodeCommand())
    , CmdSaveConfig(ParameterNodeCommand())
    , OfflineReconstructionCompletionCallback(vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand>::New())
    , ScoutScanCompletionCallback(vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand>::New())
    , LiveVolumeReconstructionSnapshotCallback(vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand>::New())
    , LiveVolumeReconstructionCompletionCallback(vtkSmartPointer<vtkPlusRemoteLogicCallbackCommand>::New())
  {
    this->CmdGetCaptureDeviceIDs.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdGetCaptureDeviceIDs.Command->SetName("RequestDeviceIds");
    this->CmdGetCaptureDeviceIDs.Command->SetBlocking(this->CommandBlocking);

    this->CmdGetReconstructorDeviceIDs.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdGetReconstructorDeviceIDs.Command->SetName("RequestDeviceIds");
    this->CmdGetReconstructorDeviceIDs.Command->SetBlocking(this->CommandBlocking);

    this->CmdGetDeviceIDs.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdGetDeviceIDs.Command->SetName("RequestDeviceIds");
    this->CmdGetDeviceIDs.Command->SetBlocking(this->CommandBlocking);

    this->CmdStartVolumeReconstruction.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdStartVolumeReconstruction.Command->SetName("StartVolumeReconstruction");
    this->CmdStartVolumeReconstruction.Command->SetBlocking(this->CommandBlocking);

    this->CmdStopVolumeReconstruction.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdStopVolumeReconstruction.Command->SetName("StopVolumeReconstruction");
    this->CmdStopVolumeReconstruction.Command->SetBlocking(this->CommandBlocking);

    this->CmdReconstructRecorded.Command->SetTimeoutSec(this->CommandTimeoutSec * 2.0);  // Potentially requires more time. Expiry is doubled
    this->CmdReconstructRecorded.Command->SetName("ReconstructVolume");
    this->CmdReconstructRecorded.Command->SetBlocking(this->CommandBlocking);

    this->CmdStartRecording.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdStartRecording.Command->SetName("StartRecording");
    this->CmdStartRecording.Command->SetBlocking(this->CommandBlocking);

    this->CmdStopRecording.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdStopRecording.Command->SetName("StopRecording");
    this->CmdStopRecording.Command->SetBlocking(this->CommandBlocking);

    this->CmdGetVolumeReconstructionSnapshot.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdGetVolumeReconstructionSnapshot.Command->SetName("GetVolumeReconstructionSnapshot");
    this->CmdGetVolumeReconstructionSnapshot.Command->SetBlocking(this->CommandBlocking);

    this->CmdUpdateTransform.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdUpdateTransform.Command->SetName("UpdateTransform");
    this->CmdUpdateTransform.Command->SetBlocking(this->CommandBlocking);

    this->CmdSaveConfig.Command->SetTimeoutSec(this->CommandTimeoutSec);
    this->CmdSaveConfig.Command->SetName("SaveConfig");
    this->CmdSaveConfig.Command->SetBlocking(this->CommandBlocking);

    this->OfflineReconstructionCompletionCallback->SetClientData(this->OfflineReconstructionCompletionCallback.GetPointer());
    this->ScoutScanCompletionCallback->SetClientData(this->ScoutScanCompletionCallback.GetPointer());
    this->LiveVolumeReconstructionSnapshotCallback->SetClientData(this->LiveVolumeReconstructionSnapshotCallback.GetPointer());
    this->LiveVolumeReconstructionCompletionCallback->SetClientData(this->LiveVolumeReconstructionCompletionCallback.GetPointer());
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlusRemoteLogic);

//----------------------------------------------------------------------------
vtkSlicerPlusRemoteLogic::vtkSlicerPlusRemoteLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerPlusRemoteLogic::~vtkSlicerPlusRemoteLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("vtkSlicerPlusRemoteLogic::RegisterNodes failed: MRML scene is invalid");
    return;
  }

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLPlusRemoteNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLPlusServerLauncherRemoteNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node)
  {
    return;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::CreateDefaultParameterSet()
{
  if (!this->GetMRMLScene())
  {
    return;
  }

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLPlusRemoteNode", nodes);

  if (nodes.empty())
  {
    this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLPlusRemoteNode");
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::RequestCaptureDeviceIDs(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdGetCaptureDeviceIDs.Command->GetName().c_str());
  commandElement->SetAttribute("DeviceType", "VirtualCapture");
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdGetCaptureDeviceIDs.Command->SetCommandContent(ss.str());
  commands->CmdGetCaptureDeviceIDs.Callback->Logic = this;
  commands->CmdGetCaptureDeviceIDs.Callback->ParameterNode = parameterNode;
  commands->CmdGetCaptureDeviceIDs.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onRequestCaptureDeviceIDsCompleted);
  connectorNode->SendCommand(commands->CmdGetCaptureDeviceIDs.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::RequestVolumeReconstructorDeviceIDs(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdGetReconstructorDeviceIDs.Command->GetName().c_str());
  commandElement->SetAttribute("DeviceType", "VirtualVolumeReconstructor");
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdGetReconstructorDeviceIDs.Command->SetCommandContent(ss.str());
  commands->CmdGetReconstructorDeviceIDs.Callback->Logic = this;
  commands->CmdGetReconstructorDeviceIDs.Callback->ParameterNode = parameterNode;
  commands->CmdGetReconstructorDeviceIDs.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onRequestVolumeReconstructorDeviceIDsCompleted);
  connectorNode->SendCommand(commands->CmdGetReconstructorDeviceIDs.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::RequestDeviceIDs(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdGetReconstructorDeviceIDs.Command->GetName().c_str());
  if (parameterNode->GetDeviceIDType() != "")
  {
    commandElement->SetAttribute("DeviceType", parameterNode->GetDeviceIDType().c_str());
  }
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdGetDeviceIDs.Command->SetCommandContent(ss.str());
  commands->CmdGetDeviceIDs.Callback->Logic = this;
  commands->CmdGetDeviceIDs.Callback->ParameterNode = parameterNode;
  commands->CmdGetDeviceIDs.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onRequestDeviceIDsCompleted);
  connectorNode->SendCommand(commands->CmdGetDeviceIDs.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::StartRecording(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  std::string filename = parameterNode->GetRecordingFilename();
  if (parameterNode->GetRecordingFilenameCompletion())
  {
    filename = vtkSlicerPlusRemoteLogic::AddTimestampToFilename(filename);
  }

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdStartRecording.Command->GetName().c_str());
  commandElement->SetAttribute("CaptureDeviceId", parameterNode->GetCurrentCaptureID().c_str());
  commandElement->SetAttribute("OutputFilename", filename.c_str());
  commandElement->SetAttribute("EnableCompression", parameterNode->GetRecordingEnableCompression() ? "True" : "False");
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdStartRecording.Command->SetCommandContent(ss.str());
  commands->CmdStartRecording.Callback->Logic = this;
  commands->CmdStartRecording.Callback->ParameterNode = parameterNode;
  commands->CmdStartRecording.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onRecordingStarted);
  connectorNode->SendCommand(commands->CmdStartRecording.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::StopRecording(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  parameterNode->SetRecordingMessage("Recording is being stopped");

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdStopRecording.Command->GetName().c_str());
  commandElement->SetAttribute("CaptureDeviceId", parameterNode->GetCurrentCaptureID().c_str());
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdStopRecording.Command->SetCommandContent(ss.str());
  commands->CmdStopRecording.Callback->Logic = this;
  commands->CmdStopRecording.Callback->ParameterNode = parameterNode;
  commands->CmdStopRecording.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onRecordingCompleted);
  connectorNode->SendCommand(commands->CmdStopRecording.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::StartOfflineReconstruction(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // Convert numeric value to string
  std::stringstream spacingSS;

  double spacing = parameterNode->GetOfflineReconstructionSpacing();
  spacingSS << spacing << " " << spacing << " " << spacing;
  std::string spacingString = spacingSS.str();

  parameterNode->SetOfflineReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECONSTRUCTING);
  parameterNode->SetOfflineReconstructionMessage("Offline reconstruction in progress");

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdReconstructRecorded.Command->GetName().c_str());
  commandElement->SetAttribute("VolumeReconstructorDeviceId", parameterNode->GetCurrentVolumeReconstructorID().c_str());
  commandElement->SetAttribute("InputSeqFilename", parameterNode->GetOfflineReconstructionInputFilename().c_str());
  commandElement->SetAttribute("OutputSpacing", spacingString.c_str());
  commandElement->SetAttribute("OutputVolFilename", parameterNode->GetOfflineReconstructionOutputFilename().c_str());
  commandElement->SetAttribute("OutputVolDeviceName", parameterNode->GetOfflineReconstructionDevice().c_str());
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdReconstructRecorded.Command->SetCommandContent(ss.str());
  commands->CmdReconstructRecorded.Callback->Logic = this;
  commands->CmdReconstructRecorded.Callback->ParameterNode = parameterNode;
  commands->CmdReconstructRecorded.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onOfflineVolumeReconstructionCompleted);
  connectorNode->SendCommand(commands->CmdReconstructRecorded.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::StartScoutScan(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  std::string filename = parameterNode->GetScoutScanFilename();
  if (parameterNode->GetScoutScanFilenameCompletion())
  {
    filename = vtkSlicerPlusRemoteLogic::AddTimestampToFilename(filename);
  }
  parameterNode->SetScoutLastRecordedVolume(filename);

  parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECORDING);
  parameterNode->SetOfflineReconstructionMessage("Scout scan recording in progress");

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdStartRecording.Command->GetName().c_str());
  commandElement->SetAttribute("CaptureDeviceId", parameterNode->GetCurrentCaptureID().c_str());
  commandElement->SetAttribute("OutputFilename", filename.c_str());
  commandElement->SetAttribute("EnableCompression", "False");
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdStartRecording.Command->SetCommandContent(ss.str());
  commands->CmdStartRecording.Callback->Logic = this;
  commands->CmdStartRecording.Callback->ParameterNode = parameterNode;
  commands->CmdStartRecording.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onScoutScanStarted);
  connectorNode->SendCommand(commands->CmdStartRecording.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::StopScoutScan(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdStopRecording.Command->GetName().c_str());
  commandElement->SetAttribute("CaptureDeviceId", parameterNode->GetCurrentCaptureID().c_str());
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdStopRecording.Command->SetCommandContent(ss.str());
  commands->CmdStopRecording.Callback->Logic = this;
  commands->CmdStopRecording.Callback->ParameterNode = parameterNode;
  commands->CmdStopRecording.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onScoutScanRecorded);
  connectorNode->SendCommand(commands->CmdStopRecording.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::StartLiveVolumeReconstruction(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  std::string filename = parameterNode->GetLiveReconstructionFilename();
  if (parameterNode->GetLiveReconstructionFilenameCompletion())
  {
    filename = vtkSlicerPlusRemoteLogic::AddTimestampToFilename(filename);
  }

  vtkMRMLAnnotationROINode* roiNode = parameterNode->GetLiveReconstructionROINode();

  double roiCenter[3] = { 0.0, 0.0, 0.0 };
  roiNode->GetXYZ(roiCenter);
  double roiRadius[3] = { 0.0, 0.0, 0.0 };
  roiNode->GetRadiusXYZ(roiRadius);
  double roiOrigin[3] = { 0.0, 0.0, 0.0 };
  for (int i = 0; i < 3; ++i)
  {
    roiOrigin[i] = roiCenter[i] - roiRadius[i];
  }

  // Convert numeric values to string
  std::stringstream spacingSS;
  double spacing = parameterNode->GetLiveReconstructionSpacing();
  spacingSS << spacing << " " << spacing << " " << spacing;
  std::string spacingString = spacingSS.str();

  std::stringstream originSS;
  originSS << roiOrigin[0] << " " << roiOrigin[1] << " " << roiOrigin[2];
  std::string originString = originSS.str();

  int extent[6] = { 0, 0, 0, 0, 0, 0 };
  parameterNode->GetLiveReconstructionROIExtent(extent);
  std::stringstream extentSS;
  extentSS << extent[0] << " " << extent[1] << " " << extent[2] << " " << extent[3] << " " << extent[4] << " " << extent[5];
  std::string extentString = extentSS.str();

  // Update command attributes
  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdStartVolumeReconstruction.Command->GetName().c_str());
  commandElement->SetAttribute("VolumeReconstructorDeviceId", parameterNode->GetCurrentVolumeReconstructorID().c_str());
  commandElement->SetAttribute("OutputSpacing", spacingString.c_str());
  commandElement->SetAttribute("OutputOrigin", originString.c_str());
  commandElement->SetAttribute("OutputExtent", extentString.c_str());
  commandElement->SetAttribute("OutputVolFilename", filename.c_str());
  commandElement->SetAttribute("OutputVolDeviceName", parameterNode->GetLiveReconstructionDevice().c_str());
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdStartVolumeReconstruction.Command->SetCommandContent(ss.str());
  commands->CmdStartVolumeReconstruction.Callback->Logic = this;
  commands->CmdStartVolumeReconstruction.Callback->ParameterNode = parameterNode;
  commands->CmdStartVolumeReconstruction.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onLiveVolumeReconstructionStarted);
  connectorNode->SendCommand(commands->CmdStartVolumeReconstruction.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::GetLiveVolumeReconstructionSnapshot(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // Update command attributes
  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdGetVolumeReconstructionSnapshot.Command->GetName().c_str());
  commandElement->SetAttribute("VolumeReconstructorDeviceId", parameterNode->GetCurrentVolumeReconstructorID().c_str());
  commandElement->SetAttribute("OutputVolFilename", parameterNode->GetLiveReconstructionFilename().c_str());
  commandElement->SetAttribute("OutputVolDeviceName", parameterNode->GetLiveReconstructionDevice().c_str());
  commandElement->SetAttribute("ApplyHoleFilling", parameterNode->GetLiveReconstructionApplyHoleFilling() ? "True" : "False");
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdGetVolumeReconstructionSnapshot.Command->SetCommandContent(ss.str());
  commands->CmdGetVolumeReconstructionSnapshot.Callback->Logic = this;
  commands->CmdGetVolumeReconstructionSnapshot.Callback->ParameterNode = parameterNode;
  commands->CmdGetVolumeReconstructionSnapshot.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onLiveVolumeSnapshotAquired);
  connectorNode->SendCommand(commands->CmdGetVolumeReconstructionSnapshot.Command);

  parameterNode->SetLiveReconstructionLastSnapshotTimestampSec(vtkTimerLog::GetUniversalTime());
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::StopLiveVolumeReconstruction(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // Update command attributes
  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdStopVolumeReconstruction.Command->GetName().c_str());
  commandElement->SetAttribute("VolumeReconstructorDeviceId", parameterNode->GetCurrentVolumeReconstructorID().c_str());
  commandElement->SetAttribute("OutputVolFilename", parameterNode->GetLiveReconstructionFilename().c_str());
  commandElement->SetAttribute("OutputVolDeviceName", parameterNode->GetLiveReconstructionDevice().c_str());
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdStopVolumeReconstruction.Command->SetCommandContent(ss.str());
  commands->CmdStopVolumeReconstruction.Callback->Logic = this;
  commands->CmdStopVolumeReconstruction.Callback->ParameterNode = parameterNode;
  commands->CmdStopVolumeReconstruction.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onLiveVolumeReconstructionCompleted);
  connectorNode->SendCommand(commands->CmdStopVolumeReconstruction.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::SaveConfigFile(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // Update command attributes
  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdSaveConfig.Command->GetName().c_str());
  commandElement->SetAttribute("Filename", parameterNode->GetConfigFilename().c_str());
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdSaveConfig.Command->SetCommandContent(ss.str());
  commands->CmdSaveConfig.Callback->Logic = this;
  commands->CmdSaveConfig.Callback->ParameterNode = parameterNode;
  commands->CmdSaveConfig.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onPrintCommandResponseRequested);
  connectorNode->SendCommand(commands->CmdSaveConfig.Command);

}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::UpdateTransform(vtkMRMLPlusRemoteNode* parameterNode)
{
  if (!parameterNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  vtkMRMLLinearTransformNode* transformNode = parameterNode->GetUpdatedTransformNode();
  if (!connectorNode || !transformNode)
  {
    return;
  }

  // Get transform matrix as string
  vtkMatrix4x4* transformMatrix = transformNode->GetMatrixTransformToParent();

  std::stringstream transformSS;

  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      transformSS << transformMatrix->GetElement(i, j);
      if (i != 3 || j != 3)
      {
        transformSS << " ";
      }
    }
  }

  // Get transform date as string
  std::string transformDate = this->GetTimestampString("%Y-%m-%d %H:%M:%S.f");
  std::string transformValue = transformSS.str();

  ParameterNodeCommands* commands = &this->NodeCommandMap[parameterNode];

  vtkNew<vtkXMLDataElement> commandElement;
  commandElement->SetName("Command");
  commandElement->SetAttribute("Name", commands->CmdUpdateTransform.Command->GetName().c_str());
  commandElement->SetAttribute("TransformName", transformNode->GetName());
  commandElement->SetAttribute("TransformValue", transformValue.c_str());
  commandElement->SetAttribute("TransformDate", transformDate.c_str());
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(commandElement, ss);

  commands->CmdUpdateTransform.Command->SetCommandContent(ss.str());
  commands->CmdUpdateTransform.Callback->Logic = this;
  commands->CmdUpdateTransform.Callback->ParameterNode = parameterNode;
  commands->CmdUpdateTransform.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onPrintCommandResponseRequested);

  // Send command
  connectorNode->SendCommand(commands->CmdUpdateTransform.Command);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::ShowVolumeInSliceViewers(vtkMRMLVolumeNode* volumeNode, std::vector<std::string> sliceWidgetNames)
{
  // Displays volumeNode in the selected slice viewers as background volume
  // Existing background volume is pushed to foreground, existing foreground volume will not be shown anymore
  // sliceWidgetNames is a list of slice view names, such as["Yellow", "Green"]
  if (!volumeNode)
  {
    return;
  }
  std::string newVolumeNodeID = volumeNode->GetID();

  for (std::vector<std::string>::iterator sliceNameIt = sliceWidgetNames.begin();
    sliceNameIt != sliceWidgetNames.end(); ++sliceNameIt)
  {
    std::string sliceName = *sliceNameIt;
    vtkMRMLSliceLogic* sliceLogic = this->GetApplicationLogic()->GetSliceLogicByLayoutName(sliceName.c_str());
    vtkMRMLSliceCompositeNode* compositeNode = sliceLogic->GetSliceCompositeNode();
    const char* foregroundVolumeNodeID = compositeNode->GetForegroundVolumeID();
    const char* backgroundVolumeNodeID = compositeNode->GetBackgroundVolumeID();

    if (foregroundVolumeNodeID || backgroundVolumeNodeID)
    {
      // A volume is shown already, so we just need to make foreground semi-transparent to make sure the new volume will be visible
      sliceLogic->GetSliceCompositeNode()->SetForegroundOpacity(0.5);
      if ((foregroundVolumeNodeID && strcmp(foregroundVolumeNodeID, newVolumeNodeID.c_str()) == 0) ||
        (backgroundVolumeNodeID && strcmp(backgroundVolumeNodeID, newVolumeNodeID.c_str()) == 0))
      {
        continue;
      }
    }
    if (backgroundVolumeNodeID)
    {
      // There is a background volume, push it to the foreground because we will replace the background volume
      sliceLogic->GetSliceCompositeNode()->SetForegroundVolumeID(backgroundVolumeNodeID);
    }
    // Show the new volume as background
    sliceLogic->GetSliceCompositeNode()->SetBackgroundVolumeID(newVolumeNodeID.c_str());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::ShowVolumeRendering(vtkMRMLVolumeNode* volumeNode)
{
  // Display reconstructed volume in Slicer 3D view
  if (!volumeNode)
  {
    return;
  }

  vtkSmartPointer<vtkMRMLVolumeRenderingDisplayNode> volumeRenderingDisplayNode = NULL;

  // Find existing VR display node
  for (int i = 0; i<volumeNode->GetNumberOfDisplayNodes(); ++i)
  {
    vtkSmartPointer<vtkMRMLDisplayNode> displayNode = volumeNode->GetNthDisplayNode(i);
    if (displayNode->IsA("vtkMRMLVolumeRenderingDisplayNode"))
    {
      // Found existing VR display node
      volumeRenderingDisplayNode = vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(displayNode);
      break;
    }
  }
  // Create new volume rendering display node if not exist yet
  if (!volumeRenderingDisplayNode)
  {
    qSlicerAbstractCoreModule* volumeRenderingModule =
      qSlicerCoreApplication::application()->moduleManager()->module("VolumeRendering");

    vtkSlicerVolumeRenderingLogic* volumeRenderingLogic =
      volumeRenderingModule ? vtkSlicerVolumeRenderingLogic::SafeDownCast(volumeRenderingModule->logic()) : 0;

    if (volumeRenderingLogic)
    {
      vtkMRMLVolumeRenderingDisplayNode* displayNode =
        volumeRenderingLogic->CreateVolumeRenderingDisplayNode();
      this->GetMRMLScene()->AddNode(displayNode);
      displayNode->Delete();
      volumeRenderingLogic->UpdateDisplayNodeFromVolumeNode(displayNode, volumeNode);
      volumeNode->AddAndObserveDisplayNodeID(displayNode->GetID());

      vtkMRMLAnnotationROINode* roiNode = displayNode->GetROINode();
      if (roiNode)
      {
        roiNode->SetDisplayVisibility(false);
      }
    }
  }
}

//---------------------------------------------------------------------------
std::string vtkSlicerPlusRemoteLogic::GetTimestampString(std::string format/*="%Y%m%d_%H%M%S"*/)
{
  int bytes = 50;
  char date[100];
  std::time_t currentTime;
  struct tm *ts;
  currentTime = std::time(NULL);
  ts = localtime(&currentTime);
  std::strftime(date, bytes, format.c_str(), ts);
  return std::string(date);
}

//---------------------------------------------------------------------------
std::string vtkSlicerPlusRemoteLogic::AddTimestampToFilename(std::string filename)
{
  std::string outputFilename = "";
  std::string timestampString = vtkSlicerPlusRemoteLogic::GetTimestampString();
  std::string prefix = filename;
  std::string suffix = "";
  size_t index = filename.find_last_of('.');
  if (index != std::string::npos && filename.length() > 1)
  {
    prefix = filename.substr(0, index);
    suffix = filename.substr(index, filename.length());
  }
  return prefix + timestampString + suffix;
}

//---------------------------------------------------------------------------
std::string vtkSlicerPlusRemoteLogic::ParseFilenameFromMessage(std::string message)
{
  std::string volumeToReconstructFileName = "";
  size_t index = message.find_last_of('\\/');
  if (index != std::string::npos)
  {
    volumeToReconstructFileName = message.substr(index + 1, message.length());
  }
  return volumeToReconstructFileName;
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::InitializeROIFromVolume(vtkMRMLAnnotationROINode* roiNode, vtkMRMLVolumeNode* volumeNode)
{
  // ROI is initialized to fit scout scan reconstructed volume
  if (!roiNode || !volumeNode)
  {
    return;
  }

  vtkImageData* imageData = volumeNode->GetImageData();
  if (!imageData)
  {
    return;
  }

  // Get volume bounds
  int volumeExtent[6] = { 0, 0, 0, 0, 0, 0 };
  imageData->GetExtent(volumeExtent);

  vtkSmartPointer<vtkMatrix4x4> ijkToRas = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASMatrix(ijkToRas);

  double ijkPosition[4] = { (double)volumeExtent[0], (double)volumeExtent[2], (double)volumeExtent[4], 1.0 };
  double rasPosition[4] = { 0, 0, 0, 1 };
  ijkToRas->MultiplyPoint(ijkPosition, rasPosition);

  double volumeBounds[6] = { rasPosition[0], rasPosition[0], rasPosition[1], rasPosition[1], rasPosition[2], rasPosition[2] };
  for (int kCoord = 4; kCoord <= 5; ++kCoord)
  {
    for (int jCoord = 2; jCoord <= 3; ++jCoord)
    {
      for (int iCoord = 0; iCoord <= 1; ++iCoord)
      {
        double ijkPosition[4] = { (double)volumeExtent[iCoord], (double)volumeExtent[jCoord], (double)volumeExtent[kCoord], 1.0 };
        ijkToRas->MultiplyPoint(ijkPosition, rasPosition);

        for (int component = 0; component < 3; ++component)
        {
          volumeBounds[component * 2] = std::min(volumeBounds[component * 2], rasPosition[component]);
          volumeBounds[component * 2 + 1] = std::max(volumeBounds[component * 2 + 1], rasPosition[component]);
        }
      }
    }
  }

  // Set ROI position and size
  double roiCenter[3] = { 0.0, 0.0, 0.0 };
  double roiRadius[3] = { 0.0, 0.0, 0.0 };
  for (int i = 0; i < 3; ++i)
  {
    roiCenter[i] = (volumeBounds[i * 2 + 1] + volumeBounds[i * 2]) / 2.0;
    roiRadius[i] = (volumeBounds[i * 2 + 1] - volumeBounds[i * 2]) / 2.0;
  }

  roiNode->SetXYZ(roiCenter[0], roiCenter[1], roiCenter[2]);
  roiNode->SetRadiusXYZ(roiRadius[0], roiRadius[1], roiRadius[2]);
  roiNode->SetAndObserveTransformNodeID(volumeNode->GetTransformNodeID());
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onRequestCaptureDeviceIDsCompleted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);

  if (command->GetStatus() != igtlioCommandStatus::CommandResponseReceived)
  {
    return;
  }

  vtkSmartPointer<vtkXMLDataElement> responseXML = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(responseContent.c_str()));
  if (!responseXML)
  {
    return;
  }
  std::string captureDeviceIDsListString = responseXML->GetAttribute("Message") ? responseXML->GetAttribute("Message") : "";

  std::vector<std::string> captureDevicesIDs;
  // If the command was not successful, then there are no matching device ids and captureDevicesIDs should remain empty
  if (!captureDeviceIDsListString.empty() && command->GetSuccessful())
  {
    std::istringstream ss(captureDeviceIDsListString);
    std::string captureDeviceID;
    while (std::getline(ss, captureDeviceID, ','))
    {
      captureDevicesIDs.push_back(captureDeviceID);
    }
  }

  parameterNode->SetCaptureIDs(captureDevicesIDs);
  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::CaptureIdsReceivedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onRequestVolumeReconstructorDeviceIDsCompleted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);

  if (command->GetStatus() != igtlioCommandStatus::CommandResponseReceived)
  {
    return;
  }

  std::string volumeReconstructorDeviceIDsListString = "";
  vtkSmartPointer<vtkXMLDataElement> responseXML = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(responseContent.c_str()));
  if (responseXML)
  {
    volumeReconstructorDeviceIDsListString = responseXML->GetAttribute("Message") ? responseXML->GetAttribute("Message") : "";
  }

  std::vector<std::string> volumeReconstructorDeviceIDs;
  // If the command was not successful, then there are no matching device ids and volumeReconstructorDeviceIDs should remain empty
  if (!volumeReconstructorDeviceIDsListString.empty() && command->GetSuccessful())
  {
    std::istringstream ss(volumeReconstructorDeviceIDsListString);
    std::string volumeReconstructorDeviceID;
    while (std::getline(ss, volumeReconstructorDeviceID, ','))
    {
      volumeReconstructorDeviceIDs.push_back(volumeReconstructorDeviceID);
    }
  }

  parameterNode->SetVolumeReconstructorIDs(volumeReconstructorDeviceIDs);
  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::VolumeReconstructorIdsReceivedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onRequestDeviceIDsCompleted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command || !command->GetSuccessful())
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);

  if (command->GetStatus() != igtlioCommandStatus::CommandResponseReceived)
  {
    return;
  }

  std::string deviceIDsListString = "";
  vtkSmartPointer<vtkXMLDataElement> responseXML = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(responseContent.c_str()));
  if (responseXML)
  {
    deviceIDsListString = responseXML->GetAttribute("Message") ? responseXML->GetAttribute("Message") : "";
  }

  std::vector<std::string> deviceIDs;
  // If the command was not successful, then there are no matching device ids and deviceIDs should remain empty
  if (!deviceIDsListString.empty() && command->GetSuccessful())
  {
    std::istringstream ss(deviceIDsListString);
    std::string deviceID;
    while (std::getline(ss, deviceID, ','))
    {
      deviceIDs.push_back(deviceID);
    }
  }

  parameterNode->SetDeviceIDs(deviceIDs);
  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::DeviceIdsReceivedEvent);
}


//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onRecordingStarted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);
  if (command->GetSuccessful())
  {
    parameterNode->SetRecordingMessage("Recording in progress");
    parameterNode->SetRecordingStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECORDING);
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::RecordingStartedEvent);
  }
  else
  {
    parameterNode->SetRecordingMessage(command->GetErrorMessage());
    parameterNode->SetRecordingStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::RecordingCompletedEvent);
  }  
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onRecordingCompleted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);

  vtkSmartPointer<vtkXMLDataElement> responseXML = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(responseContent.c_str()));

  int status = command->GetStatus();
  if (status == igtlioCommandStatus::CommandExpired)
  {
    parameterNode->SetRecordingStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetRecordingMessage("Timeout while attempting to stop recording");
  }
  else if (status != igtlioCommandStatus::CommandResponseReceived)
  {
    std::string errorString = command->GetErrorMessage();
    parameterNode->SetRecordingMessage(errorString);
    parameterNode->SetRecordingStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
  }
  else
  {
    if (responseXML)
    {
      std::string messageString = responseXML->GetAttribute("Message") ? responseXML->GetAttribute("Message") : "";
      std::string volumeToReconstructFileName = vtkSlicerPlusRemoteLogic::ParseFilenameFromMessage(messageString);
      parameterNode->AddRecordedVolume(volumeToReconstructFileName);
      parameterNode->SetRecordingMessage("Recording completed, saved as " + volumeToReconstructFileName);
    }
    parameterNode->SetRecordingStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_SUCCESS);
  }
  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::RecordingCompletedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onOfflineVolumeReconstructionCompleted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);

  if (command->GetStatus() == igtlioCommandStatus::CommandExpired)
  {
    // Volume reconstruction command timed out
    parameterNode->SetOfflineReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetOfflineReconstructionMessage("Timeout while waiting for volume reconstruction result");
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::OfflineReconstructionCompletedEvent);
    return;
  }
  else if (command->GetStatus() != igtlioCommandStatus::CommandResponseReceived || !command->GetSuccessful())
  {
    std::string errorString = command->GetErrorMessage();
    parameterNode->SetOfflineReconstructionMessage(errorString);
    parameterNode->SetOfflineReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::OfflineReconstructionCompletedEvent);
    return;
  }

  vtkSmartPointer<vtkXMLDataElement> responseXML = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(responseContent.c_str()));
  if (responseXML)
  {
    std::string messageString = responseXML->GetAttribute("Message") ? responseXML->GetAttribute("Message") : "";
    parameterNode->SetOfflineReconstructionMessage(messageString);
  }

  // Order of OpenIGTLink message receiving and processing is not guaranteed to be the same
  // therefore we wait a bit to make sure the image message is processed as well
  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // Order of OpenIGTLink message receiving and processing is not guaranteed to be the same
  // therefore we wait a bit to make sure the image message is processed as well
  ParameterNodeCommands* commands = &self->NodeCommandMap[parameterNode];
  commands->OfflineReconstructionCompletionCallback->SetCallback(vtkSlicerPlusRemoteLogic::onOfflineVolumeReconstructedFinalize);
  commands->OfflineReconstructionCompletionCallback->Logic = self;
  commands->OfflineReconstructionCompletionCallback->ParameterNode = parameterNode;
  connectorNode->AddObserver(vtkMRMLIGTLConnectorNode::DeviceModifiedEvent, commands->OfflineReconstructionCompletionCallback);

  parameterNode->SetOfflineReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_SUCCESS);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onOfflineVolumeReconstructedFinalize(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void* calldata)
{
  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;
  vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);

  igtlioDevicePointer device = static_cast<igtlioDevice*>(calldata);
  if (!device)
  {
    return;
  }

  std::string nodeName = parameterNode->GetOfflineReconstructionDevice();
  if (device->GetDeviceName() != nodeName)
  {
    return;
  }
  connectorNode->RemoveObserver(callback);

  if (parameterNode->GetOfflineReconstructionShowResultOnCompletion())
  {
    vtkSmartPointer<vtkMRMLVolumeNode> offlineVolume = vtkMRMLVolumeNode::SafeDownCast(self->GetMRMLScene()->GetFirstNodeByName(nodeName.c_str()));
    if (!offlineVolume)
    {
      return;
    }

    std::vector<std::string> sliceViews = { "Yellow", "Green" };
    self->ShowVolumeInSliceViewers(offlineVolume, sliceViews);
    self->GetMRMLApplicationLogic()->FitSliceToAll();
    self->ShowVolumeRendering(offlineVolume);
  }

  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::OfflineReconstructionCompletedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onScoutScanStarted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);
  parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECORDING);
  parameterNode->SetScoutScanMessage("Recording in progress");
  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::ScoutScanStartedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onScoutScanRecorded(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);

  if (command->GetStatus() == igtlioCommandStatus::CommandExpired)
  {
    parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetScoutScanMessage("Timeout while waiting for volume reconstruction result");
  }
  else if (command->GetStatus() != igtlioCommandStatus::CommandResponseReceived || !command->GetSuccessful())
  {
    parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetScoutScanMessage(command->GetErrorMessage());
  }
  else if (command->GetStatus() == igtlioCommandStatus::CommandResponseReceived)
  {
    parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECONSTRUCTING);
    parameterNode->SetScoutScanMessage("Recording completed. Reconstruction in progress");

    double outputSpacingValue = parameterNode->GetScoutScanSpacing();
    std::stringstream spacingSS;
    spacingSS << outputSpacingValue << " " << outputSpacingValue << " " << outputSpacingValue;
    std::string spacingString = spacingSS.str();

    vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
    if (!connectorNode)
    {
      return;
    }

    // Update command attributes
    ParameterNodeCommands* commands = &self->NodeCommandMap[parameterNode];

    std::string outputScoutScanFilename = vtkSlicerPlusRemoteLogic::GetScoutScanOutputFilename();
    std::string scoutScanDeviceName = vtkSlicerPlusRemoteLogic::GetScoutScanDeviceName();

    vtkNew<vtkXMLDataElement> commandElement;
    commandElement->SetName("Command");
    commandElement->SetAttribute("Name", commands->CmdReconstructRecorded.Command->GetName().c_str());
    commandElement->SetAttribute("VolumeReconstructorDeviceId", parameterNode->GetCurrentVolumeReconstructorID().c_str());
    commandElement->SetAttribute("InputSeqFilename", parameterNode->GetScoutLastRecordedVolume().c_str());
    commandElement->SetAttribute("OutputSpacing", spacingString.c_str());
    commandElement->SetAttribute("OutputVolFilename", outputScoutScanFilename.c_str());
    commandElement->SetAttribute("OutputVolDeviceName", scoutScanDeviceName.c_str());
    std::stringstream ss;
    vtkXMLUtilities::FlattenElement(commandElement, ss);

    commands->CmdReconstructRecorded.Command->SetCommandContent(ss.str());
    commands->CmdReconstructRecorded.Callback->SetCallback(vtkSlicerPlusRemoteLogic::onScoutScanCompleted);
    commands->CmdReconstructRecorded.Callback->Logic = self;
    commands->CmdReconstructRecorded.Callback->ParameterNode = parameterNode;
    connectorNode->SendCommand(commands->CmdReconstructRecorded.Command);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onScoutScanCompleted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  if (command->GetStatus() == igtlioCommandStatus::CommandExpired)
  {
    parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetScoutScanMessage("Timeout while waiting for volume recording result");
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::ScoutScanCompletedEvent);
  }
  else if (command->GetStatus() != igtlioCommandStatus::CommandResponseReceived || !command->GetSuccessful())
  {
    parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetScoutScanMessage(command->GetErrorMessage());
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::ScoutScanCompletedEvent);
  }
  else if (command->GetStatus() == igtlioCommandStatus::CommandResponseReceived)
  {
    parameterNode->SetScoutScanMessage("Scout scan completed");
    parameterNode->SetScoutScanStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_SUCCESS);

    vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
    if (!connectorNode)
    {
      return;
    }

    // Order of OpenIGTLink message receiving and processing is not guaranteed to be the same
    // therefore we wait a bit to make sure the image message is processed as well
    ParameterNodeCommands* commands = &self->NodeCommandMap[parameterNode];
    commands->ScoutScanCompletionCallback->SetCallback(vtkSlicerPlusRemoteLogic::onScoutVolumeReconstructedFinalize);
    commands->ScoutScanCompletionCallback->Logic = self;
    commands->ScoutScanCompletionCallback->ParameterNode = parameterNode;
    connectorNode->AddObserver(vtkMRMLIGTLConnectorNode::DeviceModifiedEvent, commands->ScoutScanCompletionCallback);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onScoutVolumeReconstructedFinalize(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void* calldata)
{
  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;
  vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);

  igtlioDevicePointer device = static_cast<igtlioDevice*>(calldata);
  if (!device)
  {
    return;
  }

  std::string nodeName = self->GetScoutScanDeviceName();
  if (device->GetDeviceName() != nodeName)
  {
    return;
  }
  connectorNode->RemoveObserver(callback);

  vtkSmartPointer<vtkMRMLAnnotationROINode> roiNode = parameterNode->GetLiveReconstructionROINode();
  if (!roiNode)
  {
    roiNode = vtkMRMLAnnotationROINode::SafeDownCast(self->GetMRMLScene()->AddNewNodeByClass("vtkMRMLAnnotationROINode"));
    roiNode->SetDisplayVisibility(parameterNode->GetLiveReconstructionDisplayROI());
    parameterNode->SetAndObserveLiveReconstructionROINode(roiNode);
  }

  vtkSmartPointer<vtkMRMLVolumeNode> scoutScanVolume = vtkMRMLVolumeNode::SafeDownCast(self->GetMRMLScene()->GetFirstNodeByName(nodeName.c_str()));
  if (!scoutScanVolume)
  {
    return;
  }

  // Create and initialize ROI after scout scan because low resolution scout scan is used to set a smaller ROI for the live high resolution reconstruction
  self->InitializeROIFromVolume(roiNode, scoutScanVolume);
  if (parameterNode->GetScoutScanShowResultOnCompletion())
  {
    std::vector<std::string> sliceViews = { "Yellow", "Green" };
    self->ShowVolumeInSliceViewers(scoutScanVolume, sliceViews);
    self->GetMRMLApplicationLogic()->FitSliceToAll();
    self->ShowVolumeRendering(scoutScanVolume);
  }

  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::ScoutScanCompletedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onLiveVolumeReconstructionStarted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);
  parameterNode->SetLiveReconstructionLastSnapshotTimestampSec(vtkTimerLog::GetUniversalTime());
  parameterNode->SetLiveReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_IN_PROGRESS);
  parameterNode->SetLiveReconstructionMessage("Live volume reconstruction in progress");
  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::LiveVolumeReconstructionStartedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onLiveVolumeSnapshotAquired(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);
  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::LiveVolumeReconstructionSnapshotReceivedEvent);
  parameterNode->SetLiveReconstructionMessage("Live volume reconstruction in progress\nSnapshot received");

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // Order of OpenIGTLink message receiving and processing is not guaranteed to be the same
  // therefore we wait a bit to make sure the image message is processed as well
  ParameterNodeCommands* commands = &self->NodeCommandMap[parameterNode];
  commands->LiveVolumeReconstructionSnapshotCallback->SetCallback(vtkSlicerPlusRemoteLogic::onSnapshotAquiredFinalize);
  commands->LiveVolumeReconstructionSnapshotCallback->Logic = self;
  commands->LiveVolumeReconstructionSnapshotCallback->ParameterNode = parameterNode;
  connectorNode->AddObserver(vtkMRMLIGTLConnectorNode::DeviceModifiedEvent, commands->LiveVolumeReconstructionSnapshotCallback);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onSnapshotAquiredFinalize(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void* calldata)
{
  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;
  vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);

  igtlioDevicePointer device = static_cast<igtlioDevice*>(calldata);
  if (!device)
  {
    return;
  }

  std::string nodeName = parameterNode->GetLiveReconstructionDevice();
  if (device->GetDeviceName() != nodeName)
  {
    return;
  }
  connectorNode->RemoveObserver(callback);

  if (parameterNode->GetLiveReconstructionShowResultOnCompletion())
  {
    vtkSmartPointer<vtkMRMLVolumeNode> snapshotVolumeNode = vtkMRMLVolumeNode::SafeDownCast(self->GetMRMLScene()->GetFirstNodeByName(nodeName.c_str()));
    if (!snapshotVolumeNode)
    {
      return;
    }
    std::vector<std::string> sliceViews = { "Yellow", "Green" };
    self->ShowVolumeInSliceViewers(snapshotVolumeNode, sliceViews);
    self->ShowVolumeRendering(snapshotVolumeNode);
  }

  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::LiveVolumeReconstructionSnapshotReceivedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onLiveVolumeReconstructionCompleted(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *vtkNotUsed(calldata))
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);

  if (command->GetStatus() == igtlioCommandStatus::CommandExpired)
  {
    parameterNode->SetLiveReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetLiveReconstructionMessage("Failed to stop volume reconstruction");
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::LiveVolumeReconstructionCompletedEvent);
    return;
  }
  else if (command->GetStatus() != igtlioCommandStatus::CommandResponseReceived || !command->GetSuccessful())
  {
    parameterNode->SetLiveReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
    parameterNode->SetLiveReconstructionMessage(command->GetErrorMessage());
    parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::LiveVolumeReconstructionCompletedEvent);
    return;
  }
  parameterNode->SetLiveReconstructionMessage("Live reconstruction completed");

  // Order of OpenIGTLink message receiving and processing is not guaranteed to be the same
  // therefore we wait a bit to make sure the image message is processed as well

  vtkMRMLIGTLConnectorNode* connectorNode = parameterNode->GetOpenIGTLinkConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // Order of OpenIGTLink message receiving and processing is not guaranteed to be the same
  // therefore we wait a bit to make sure the image message is processed as well
  ParameterNodeCommands* commands = &self->NodeCommandMap[parameterNode];
  commands->LiveVolumeReconstructionCompletionCallback->SetCallback(vtkSlicerPlusRemoteLogic::onLiveVolumeReconstructedFinalized);
  commands->LiveVolumeReconstructionCompletionCallback->Logic = self;
  commands->LiveVolumeReconstructionCompletionCallback->ParameterNode = parameterNode;
  connectorNode->AddObserver(vtkMRMLIGTLConnectorNode::DeviceModifiedEvent, commands->LiveVolumeReconstructionCompletionCallback);

  parameterNode->SetLiveReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_SUCCESS);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onLiveVolumeReconstructedFinalized(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *calldata)
{
  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;
  vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);

  igtlioDevicePointer device = static_cast<igtlioDevice*>(calldata);
  if (!device)
  {
    return;
  }

  std::string nodeName = parameterNode->GetLiveReconstructionDevice();
  if (device->GetDeviceName() != nodeName)
  {
    return;
  }
  connectorNode->RemoveObserver(callback);

  if (parameterNode->GetLiveReconstructionShowResultOnCompletion())
  {
    vtkSmartPointer<vtkMRMLVolumeNode> liveVolume = vtkMRMLVolumeNode::SafeDownCast(self->GetMRMLScene()->GetFirstNodeByName(nodeName.c_str()));
    if (!liveVolume)
    {
      return;
    }
    std::vector<std::string> sliceViews = { "Yellow", "Green" };
    self->ShowVolumeInSliceViewers(liveVolume, sliceViews);
    self->GetMRMLApplicationLogic()->FitSliceToAll();
    self->ShowVolumeRendering(liveVolume);
  }

  parameterNode->InvokeEvent(vtkMRMLPlusRemoteNode::LiveVolumeReconstructionCompletedEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::onPrintCommandResponseRequested(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void *calldata)
{
  igtlioCommandPointer command = igtlioCommand::SafeDownCast(caller);
  if (!command)
  {
    return;
  }

  vtkPlusRemoteLogicCallbackCommand* callback = static_cast<vtkPlusRemoteLogicCallbackCommand*>(clientdata);
  vtkSlicerPlusRemoteLogic* self = callback->Logic;
  vtkMRMLPlusRemoteNode* parameterNode = callback->ParameterNode;

  std::string responseContent = command->GetResponseContent();
  parameterNode->SetResponseText(responseContent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlusRemoteLogic::UpdateAllPlusRemoteNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (scene == NULL)
  {
    return;
  }

  vtkSmartPointer<vtkCollection> plusRemoteNodes = vtkSmartPointer<vtkCollection>::Take(scene->GetNodesByClass("vtkMRMLPlusRemoteNode"));
  vtkSmartPointer<vtkCollectionIterator> plusRemoteNodeIt = vtkSmartPointer<vtkCollectionIterator>::New();
  plusRemoteNodeIt->SetCollection(plusRemoteNodes);
  for (plusRemoteNodeIt->InitTraversal(); !plusRemoteNodeIt->IsDoneWithTraversal(); plusRemoteNodeIt->GoToNextItem())
  {
    vtkMRMLPlusRemoteNode* plusRemoteNode = vtkMRMLPlusRemoteNode::SafeDownCast(plusRemoteNodeIt->GetCurrentObject());
    if (plusRemoteNode == NULL ||
        plusRemoteNode->GetLiveReconstructionStatus() != vtkMRMLPlusRemoteNode::PLUS_REMOTE_IN_PROGRESS ||
        plusRemoteNode->GetLiveReconstructionSnapshotsIntervalSec() <= 0)
    {
      continue;
    }
    vtkTimeStamp timestamp;
    timestamp.Modified();

    double lastSnapshotTimestamp = plusRemoteNode->GetLiveReconstructionLastSnapshotTimestampSec();
    double timeElapsed = vtkTimerLog::GetUniversalTime() - lastSnapshotTimestamp;
    if (timeElapsed > plusRemoteNode->GetLiveReconstructionSnapshotsIntervalSec())
    {
      this->GetLiveVolumeReconstructionSnapshot(plusRemoteNode);
    }
  }
}
