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

#include "qSlicerPlusRemoteModuleWidget.h"

// Qt includes
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QtGui>

// UI includes
#include "ui_qSlicerPlusRemoteModuleWidget.h"

// MRML includes
#include <vtkMRMLAnnotationROINode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLVolumeNode.h>

// OpenIGTLinkIF includes
#include "vtkMRMLIGTLConnectorNode.h"

// PlusRemote includes
#include "vtkMRMLPlusRemoteNode.h"
#include "vtkMRMLPlusServerLauncherRemoteNode.h"
#include "vtkSlicerPlusRemoteLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ToolPlusRemote
class qSlicerPlusRemoteModuleWidgetPrivate : public Ui_qSlicerPlusRemoteModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerPlusRemoteModuleWidget);

protected:
  qSlicerPlusRemoteModuleWidget* const q_ptr;
public:
  qSlicerPlusRemoteModuleWidgetPrivate(qSlicerPlusRemoteModuleWidget& object);
  vtkSlicerPlusRemoteLogic* logic() const;

private:
  QLabel*                 RecordingStatus;
  QLabel*                 OfflineReconstructionStatus;
  QLabel*                 ScoutScanStatus;
  QLabel*                 LiveReconstructionStatus;

  QSize                   StatusPixmapSize = QSize(60, 60);
  QPixmap                 InformationPixmap;
  QPixmap                 WarningPixmap;
  QPixmap                 CriticalPixmap;

  QIcon                   IconRecord;
  QIcon                   IconStop;
  QIcon                   IconVisibleOn;
  QIcon                   IconVisibleOff;

  vtkMRMLPlusRemoteNode*  ParameterNode;
};

//-----------------------------------------------------------------------------
// qSlicerPlusRemoteModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPlusRemoteModuleWidgetPrivate::qSlicerPlusRemoteModuleWidgetPrivate(qSlicerPlusRemoteModuleWidget& object)
  : q_ptr(&object)
  , RecordingStatus(NULL)
  , OfflineReconstructionStatus(NULL)
  , ScoutScanStatus(NULL)
  , LiveReconstructionStatus(NULL)

  , InformationPixmap(object.style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(this->StatusPixmapSize))
  , WarningPixmap(object.style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(this->StatusPixmapSize))
  , CriticalPixmap(object.style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(this->StatusPixmapSize))

  , IconRecord(QIcon(":/Icons/icon_Record.png"))
  , IconStop(QIcon(":/Icons/icon_Stop.png"))
  , IconVisibleOn(QIcon(":Icons/VisibleOn.png"))
  , IconVisibleOff(QIcon(":Icons/VisibleOff.png"))

  , ParameterNode(NULL)
{
}

//-----------------------------------------------------------------------------
vtkSlicerPlusRemoteLogic* qSlicerPlusRemoteModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerPlusRemoteModuleWidget);
  return vtkSlicerPlusRemoteLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerPlusRemoteModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPlusRemoteModuleWidget::qSlicerPlusRemoteModuleWidget(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlusRemoteModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerPlusRemoteModuleWidget::~qSlicerPlusRemoteModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::setup()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Add message boxes
  d->RecordingStatus = new QLabel(this);
  d->RecordingStatus->setToolTip("Recording status");
  d->RecordingStatus->setPixmap(d->InformationPixmap);
  d->RecordingStatus->setEnabled(false);
  d->RecordingLayout->addWidget(d->RecordingStatus);

  d->OfflineReconstructionStatus = new QLabel(this);
  d->OfflineReconstructionStatus->setToolTip("Offline reconstruction status");
  d->OfflineReconstructionStatus->setPixmap(d->InformationPixmap);
  d->OfflineReconstructionStatus->setEnabled(false);
  d->OfflineReconstructionLayout->addWidget(d->OfflineReconstructionStatus);

  d->ScoutScanStatus = new QLabel(this);
  d->ScoutScanStatus->setToolTip("Scout scan status");
  d->ScoutScanStatus->setPixmap(d->InformationPixmap);
  d->ScoutScanStatus->setEnabled(false);
  d->ScoutScanLayout->addWidget(d->ScoutScanStatus);

  d->LiveReconstructionStatus = new QLabel(this);
  d->LiveReconstructionStatus->setToolTip("Live reconstruction status");
  d->LiveReconstructionStatus->setPixmap(d->InformationPixmap);
  d->LiveReconstructionStatus->setEnabled(false);
  d->LiveReconstructionLayout->addWidget(d->LiveReconstructionStatus);

  d->RecordingSettingsButton->setChecked(false);
  d->OfflineReconstructionSettingsButton->setChecked(false);
  d->ScoutScanSettingsButton->setChecked(false);
  d->LiveReconstructionSettingsButton->setChecked(false);

  connect(d->ParameterNodeSelector, SIGNAL(nodeActivated(vtkMRMLNode*)), this, SLOT(onParameterSetSelected(vtkMRMLNode*)));
  connect(d->ParameterNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onParameterSetSelected(vtkMRMLNode*)));

  connect(d->OpenIGTLinkConnectorNodeSelector, SIGNAL(nodeActivated(vtkMRMLNode*)), this, SLOT(updateParameterNodeFromGui(vtkMRMLNode*)));
  connect(d->OpenIGTLinkConnectorNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(updateParameterNodeFromGui()));

  connect(d->CaptureIDSelector, SIGNAL(currentTextChanged(QString)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->VolumeReconstructorIDSelector, SIGNAL(currentTextChanged(QString)), this, SLOT(updateParameterNodeFromGui()));

  connect(d->RecordingStartStopButton, SIGNAL(clicked(bool)), this, SLOT(onRecordingStartStopButtonClicked()));
  connect(d->OfflineReconstructionStartButton, SIGNAL(clicked(bool)), this, SLOT(onOfflineReconstructionButtonClicked()));
  connect(d->ScoutScanStartStopButton, SIGNAL(clicked(bool)), this, SLOT(onScoutScanStartStopButtonClicked()));
  connect(d->LiveReconstructionStartStopButton, SIGNAL(clicked(bool)), this, SLOT(onLiveReconstructionStartStopButtonClicked()));
  connect(d->ConfigUpdateTransformButton, SIGNAL(clicked(bool)), this, SLOT(onUpdateTransformButtonClicked()));
  connect(d->ConfigSaveButton, SIGNAL(clicked(bool)), this, SLOT(onSaveConfigFileButtonClicked()));

  connect(d->RecordingFilenameLineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->RecordingAddTimestampToFilenameCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->RecordingEnableCompressionCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateParameterNodeFromGui()));

  connect(d->OfflineReconstructionSpacingSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->OfflineReconstructionDeviceLineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->OfflineReconstructionVolumeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->OfflineReconstructionShowResultOnCompletionCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateParameterNodeFromGui()));

  connect(d->ScoutScanSpacingSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->ScoutScanFilenameLineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->ScoutScanAddTimestampToFilenameCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->ScoutScanShowResultOnCompletionCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateParameterNodeFromGui()));

  connect(d->LiveReconstructionShowHideROIButton, SIGNAL(toggled(bool)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionSpacingSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionDeviceLineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionFilenameLineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionAddTimestampToFilenameCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionExtentXSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionExtentYSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionExtentZSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionSnapshotIntervalSecSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateParameterNodeFromGui()));
  connect(d->LiveReconstructionShowResultOnCompletionCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateParameterNodeFromGui()));

  connect(d->ConfigFilenameLineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateParameterNodeFromGui()));

  d->logic()->CreateDefaultParameterSet();
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::enter()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  this->Superclass::enter();

  if (this->mrmlScene() == NULL)
  {
    qCritical() << "Invalid scene!";
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  this->Superclass::setMRMLScene(scene);
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onSceneImportedEvent()
{
  this->enter();

}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  vtkMRMLPlusRemoteNode* selectedParameterNode = vtkMRMLPlusRemoteNode::SafeDownCast(node);
  if (selectedParameterNode == d->ParameterNode)
  {
    return;
  }

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->ParameterNodeSelector->setCurrentNode(selectedParameterNode);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(d->ParameterNode, selectedParameterNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  d->ParameterNode = selectedParameterNode;

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  if (!d->ParameterNode)
  {
    return;
  }

  bool wasBlocked = false;

  // Remember button status for specific operations
  this->updateButtonsFromStatus();

  /////////////////////////
  /// Connection parameters
  vtkMRMLIGTLConnectorNode* openIGTLinkConnectorNode = d->ParameterNode->GetOpenIGTLinkConnectorNode();
  wasBlocked = d->OpenIGTLinkConnectorNodeSelector->blockSignals(true);
  d->OpenIGTLinkConnectorNodeSelector->setCurrentNode(openIGTLinkConnectorNode);
  d->OpenIGTLinkConnectorNodeSelector->blockSignals(wasBlocked);

  bool connected = openIGTLinkConnectorNode &&
                   openIGTLinkConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::StateConnected;
  if (!connected)
  {
    d->ReplyTextEdit->setPlainText("IGTLConnector not connected or missing");
  }

  /////////////////////////
  // Disable the buttons and status icons if there is no connector node or if it is not connected
  d->CaptureIDSelector->setEnabled(connected);
  d->VolumeReconstructorIDSelector->setEnabled(connected);

  d->RecordingStartStopButton->setEnabled(d->RecordingStartStopButton->isEnabled() && connected);
  d->RecordingStatus->setEnabled(connected);

  d->OfflineReconstructionStartButton->setEnabled(d->OfflineReconstructionStartButton->isEnabled() && connected);
  d->OfflineReconstructionStatus->setEnabled(connected);

  d->ScoutScanStartStopButton->setEnabled(d->ScoutScanStartStopButton->isEnabled() && connected);
  d->ScoutScanStatus->setEnabled(connected);

  d->LiveReconstructionStartStopButton->setEnabled(d->LiveReconstructionStartStopButton->isEnabled() && connected);
  d->LiveReconstructionStatus->setEnabled(connected);
  d->LiveReconstructionShowHideROIButton->setEnabled(d->ParameterNode->GetLiveReconstructionROINode());

  /////////////////////////
  // Device parameters

  // Update the list of capture device IDs
  wasBlocked = d->CaptureIDSelector->blockSignals(true);
  d->CaptureIDSelector->clear();
  std::vector<std::string> captureDeviceIDs = d->ParameterNode->GetCaptureIDs();
  for (std::vector<std::string>::iterator captureDeviceIDIter = captureDeviceIDs.begin();
    captureDeviceIDIter != captureDeviceIDs.end(); ++captureDeviceIDIter)
  {
    d->CaptureIDSelector->addItem(QString::fromStdString(*captureDeviceIDIter));
  }

  // Set the current capture ID in the combobox
  // If the there are capture IDs and one of them is not selected, update the parameter node to select the first one
  int captureIndex = -1;
  std::string captureID = d->ParameterNode->GetCurrentCaptureID();
  for (int i = 0; i < d->CaptureIDSelector->count(); ++i)
  {
    if (d->CaptureIDSelector->itemText(i).toStdString() == captureID)
    {
    captureIndex = i;
    break;
    }
  }
  if (captureIndex == -1 && d->CaptureIDSelector->count() > 0)
  {
    captureIndex = 0;
    int previousDisabledModified = d->ParameterNode->GetDisableModifiedEvent();
    d->ParameterNode->SetCurrentCaptureID(d->CaptureIDSelector->itemText(0).toStdString());
    d->ParameterNode->SetDisableModifiedEvent(previousDisabledModified);
  }
  d->CaptureIDSelector->setCurrentIndex(captureIndex);
  d->CaptureIDSelector->blockSignals(wasBlocked);

  // Update the list of volume reconstructor device IDs
  wasBlocked = d->VolumeReconstructorIDSelector->blockSignals(true);
  std::vector<std::string> volumeReconstructorIDs = d->ParameterNode->GetVolumeReconstructorIDs();
  d->VolumeReconstructorIDSelector->clear();
  for (std::vector<std::string>::iterator reconstructIDIter = volumeReconstructorIDs.begin();
    reconstructIDIter != volumeReconstructorIDs.end(); ++reconstructIDIter)
  {
    d->VolumeReconstructorIDSelector->addItem(QString::fromStdString(*reconstructIDIter));
  }

  // Set the current volume reconstructor ID in the combobox
  // If the there are reconstructor IDs and one of them is not selected, update the parameter node to select the first one
  captureIndex = -1;
  std::string volumeReconstructorID = d->ParameterNode->GetCurrentVolumeReconstructorID();
  for (int i = 0; i < d->VolumeReconstructorIDSelector->count(); ++i)
  {
    if (d->VolumeReconstructorIDSelector->itemText(i).toStdString() == volumeReconstructorID)
    {
      captureIndex = i;
      break;
    }
  }
  if (captureIndex == -1 && d->VolumeReconstructorIDSelector->count() > 0)
  {
    captureIndex = 0;
    int previousDisabledModified = d->ParameterNode->GetDisableModifiedEvent();
    d->ParameterNode->SetCurrentVolumeReconstructorID(d->VolumeReconstructorIDSelector->itemText(0).toStdString());
    d->ParameterNode->SetDisableModifiedEvent(previousDisabledModified);
  }
  d->VolumeReconstructorIDSelector->setCurrentIndex(captureIndex);
  d->VolumeReconstructorIDSelector->blockSignals(wasBlocked);

  /////////////////////////
  /// Recording parameters

  std::string recordingFilename = d->ParameterNode->GetRecordingFilename();
  wasBlocked = d->RecordingFilenameLineEdit->blockSignals(true);
  d->RecordingFilenameLineEdit->setText(QString::fromStdString(recordingFilename));
  d->RecordingFilenameLineEdit->blockSignals(wasBlocked);

  bool recordingFilenameCompletion = d->ParameterNode->GetRecordingFilenameCompletion();
  wasBlocked = d->RecordingAddTimestampToFilenameCheckBox->blockSignals(true);
  d->RecordingAddTimestampToFilenameCheckBox->setChecked(recordingFilenameCompletion);
  d->RecordingAddTimestampToFilenameCheckBox->blockSignals(wasBlocked);

  bool recordingEnableCompression = d->ParameterNode->GetRecordingEnableCompression();
  wasBlocked = d->RecordingEnableCompressionCheckBox->blockSignals(true);
  d->RecordingEnableCompressionCheckBox->setChecked(recordingEnableCompression);
  d->RecordingEnableCompressionCheckBox->blockSignals(wasBlocked);

  /////////////////////////
  /// Offline reconstruction parameters

  double offlineVolumeSpacing = d->ParameterNode->GetOfflineReconstructionSpacing();
  wasBlocked = d->OfflineReconstructionSpacingSpinBox->blockSignals(true);
  d->OfflineReconstructionSpacingSpinBox->setValue(offlineVolumeSpacing);
  d->OfflineReconstructionSpacingSpinBox->blockSignals(wasBlocked);

  std::string offlineOutputVolumeDevice = d->ParameterNode->GetOfflineReconstructionDevice();
  wasBlocked = d->OfflineReconstructionDeviceLineEdit->blockSignals(true);
  d->OfflineReconstructionDeviceLineEdit->setText(QString::fromStdString(offlineOutputVolumeDevice));
  d->OfflineReconstructionDeviceLineEdit->blockSignals(wasBlocked);

  wasBlocked = d->OfflineReconstructionVolumeSelector->blockSignals(true);
  d->OfflineReconstructionVolumeSelector->clear();
  std::vector<std::string> recordedVolumes = d->ParameterNode->GetRecordedVolumes();
  for (std::vector<std::string>::iterator recordedVolumeIt = recordedVolumes.begin(); recordedVolumeIt != recordedVolumes.end(); ++recordedVolumeIt)
  {
    d->OfflineReconstructionVolumeSelector->addItem(QString::fromStdString(*recordedVolumeIt));
  }

  // If the input volume has not been set, but one is selected, set the input volume from the currently selected item
  std::string offlineVolumeInputFilename = d->ParameterNode->GetOfflineReconstructionInputFilename();
  int index = d->OfflineReconstructionVolumeSelector->findText(QString::fromStdString(offlineVolumeInputFilename));
  if (index != -1)
  {
    d->OfflineReconstructionVolumeSelector->setCurrentIndex(index);
  }
  else
  {
    offlineVolumeInputFilename = "";
  }

  if (offlineVolumeInputFilename == "" && offlineVolumeInputFilename != d->OfflineReconstructionVolumeSelector->currentText().toStdString())
  {
    d->ParameterNode->SetOfflineReconstructionInputFilename(d->OfflineReconstructionVolumeSelector->currentText().toStdString());
  }
  d->OfflineReconstructionVolumeSelector->blockSignals(wasBlocked);

  bool offlineShowResultOnCompletion = d->ParameterNode->GetOfflineReconstructionShowResultOnCompletion();
  wasBlocked = d->OfflineReconstructionShowResultOnCompletionCheckBox->blockSignals(true);
  d->OfflineReconstructionShowResultOnCompletionCheckBox->setChecked(offlineShowResultOnCompletion);
  d->OfflineReconstructionShowResultOnCompletionCheckBox->blockSignals(wasBlocked);

  /////////////////////////
  /// Scout scan parameters

  double scoutScanSpacing = d->ParameterNode->GetScoutScanSpacing();
  wasBlocked = d->ScoutScanSpacingSpinBox->blockSignals(true);
  d->ScoutScanSpacingSpinBox->setValue(scoutScanSpacing);
  d->ScoutScanSpacingSpinBox->blockSignals(wasBlocked);

  std::string scoutScanFilename = d->ParameterNode->GetScoutScanFilename();
  wasBlocked = d->ScoutScanFilenameLineEdit->blockSignals(true);
  d->ScoutScanFilenameLineEdit->setText(QString::fromStdString(scoutScanFilename));
  d->ScoutScanFilenameLineEdit->blockSignals(wasBlocked);

  bool scoutScanFilenameCompletion = d->ParameterNode->GetScoutScanFilenameCompletion();
  wasBlocked = d->ScoutScanAddTimestampToFilenameCheckBox->blockSignals(true);
  d->ScoutScanAddTimestampToFilenameCheckBox->setChecked(scoutScanFilenameCompletion);
  d->ScoutScanAddTimestampToFilenameCheckBox->blockSignals(wasBlocked);

  bool scoutScanShowResultOnCompletion = d->ParameterNode->GetScoutScanShowResultOnCompletion();
  wasBlocked = d->ScoutScanShowResultOnCompletionCheckBox->blockSignals(true);
  d->ScoutScanShowResultOnCompletionCheckBox->setChecked(scoutScanShowResultOnCompletion);
  d->ScoutScanShowResultOnCompletionCheckBox->blockSignals(wasBlocked);

  /////////////////////////
  /// Live reconstruction parameters
  bool liveReconstructionDisplayROI = d->ParameterNode->GetLiveReconstructionDisplayROI();
  wasBlocked = d->LiveReconstructionShowHideROIButton->blockSignals(true);
  d->LiveReconstructionShowHideROIButton->setChecked(liveReconstructionDisplayROI);
  if (liveReconstructionDisplayROI)
  {
    d->LiveReconstructionShowHideROIButton->setToolTip("If clicked, hide ROI");
  }
  else
  {
    d->LiveReconstructionShowHideROIButton->setToolTip("If clicked, display ROI");
  }
  d->LiveReconstructionShowHideROIButton->blockSignals(wasBlocked);

  double liveReconstructionSpacing = d->ParameterNode->GetLiveReconstructionSpacing();
  wasBlocked = d->LiveReconstructionSpacingSpinBox->blockSignals(true);
  d->LiveReconstructionSpacingSpinBox->setValue(liveReconstructionSpacing);
  d->LiveReconstructionSpacingSpinBox->blockSignals(wasBlocked);

  std::string liveReconstructionDevice = d->ParameterNode->GetLiveReconstructionDevice();
  wasBlocked = d->LiveReconstructionDeviceLineEdit->blockSignals(true);
  d->LiveReconstructionDeviceLineEdit->setText(QString::fromStdString(liveReconstructionDevice));
  d->LiveReconstructionDeviceLineEdit->blockSignals(wasBlocked);

  std::string liveReconstructionFilename = d->ParameterNode->GetLiveReconstructionFilename();
  wasBlocked = d->LiveReconstructionFilenameLineEdit->blockSignals(true);
  d->LiveReconstructionFilenameLineEdit->setText(QString::fromStdString(liveReconstructionFilename));
  d->LiveReconstructionFilenameLineEdit->blockSignals(wasBlocked);

  double liveReconstructionSnapshotsIntervalSec = d->ParameterNode->GetLiveReconstructionSnapshotsIntervalSec();
  wasBlocked = d->LiveReconstructionSnapshotIntervalSecSpinBox->blockSignals(true);
  d->LiveReconstructionSnapshotIntervalSecSpinBox->setValue(liveReconstructionSnapshotsIntervalSec);
  d->LiveReconstructionSnapshotIntervalSecSpinBox->blockSignals(wasBlocked);

  int liveROIDimensions[3] = { 0, 0, 0 };
  d->ParameterNode->GetLiveReconstructionROIDimensions(liveROIDimensions);
  QDoubleSpinBox* outputExtentBoxes[3] = {
    d->LiveReconstructionExtentXSpinBox,
    d->LiveReconstructionExtentYSpinBox,
    d->LiveReconstructionExtentZSpinBox,
  };
  for (int i = 0; i<3; ++i)
  {
    wasBlocked = outputExtentBoxes[i]->blockSignals(true);
    outputExtentBoxes[i]->setValue(liveROIDimensions[i]);
    outputExtentBoxes[i]->blockSignals(wasBlocked);
  }

  bool liveReconstructionFilenameCompletion = d->ParameterNode->GetLiveReconstructionFilenameCompletion();
  wasBlocked = d->LiveReconstructionAddTimestampToFilenameCheckBox->blockSignals(true);
  d->LiveReconstructionAddTimestampToFilenameCheckBox->setChecked(liveReconstructionFilenameCompletion);
  d->LiveReconstructionAddTimestampToFilenameCheckBox->blockSignals(wasBlocked);

  bool liveReconstructionShowResultOnCompletion = d->ParameterNode->GetLiveReconstructionShowResultOnCompletion();
  wasBlocked = d->LiveReconstructionShowResultOnCompletionCheckBox->blockSignals(true);
  d->LiveReconstructionShowResultOnCompletionCheckBox->setChecked(liveReconstructionShowResultOnCompletion);
  d->LiveReconstructionShowResultOnCompletionCheckBox->blockSignals(wasBlocked);

  /////////////////////////
  /// Config file parameter
  std::string configFileName = d->ParameterNode->GetConfigFilename();
  wasBlocked = d->ConfigFilenameLineEdit->blockSignals(true);
  d->ConfigFilenameLineEdit->setText(QString::fromStdString(configFileName));
  d->ConfigFilenameLineEdit->blockSignals(wasBlocked);

  /////////////////////////
  /// Reply text
  std::string responseText = d->ParameterNode->GetResponseText();
  d->ReplyTextEdit->setPlainText(QString::fromStdString(responseText));

}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::updateButtonsFromStatus()
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  // Recording
  d->RecordingStatus->setToolTip(QString::fromStdString(d->ParameterNode->GetRecordingMessage()));
  switch (d->ParameterNode->GetRecordingStatus())
  {
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECORDING:
    d->RecordingStartStopButton->setIcon(d->IconStop);
    d->RecordingStartStopButton->setText("  Stop recording");
    d->RecordingStartStopButton->setToolTip("If clicked, stop recording");
    d->RecordingStartStopButton->setEnabled(true);
    d->RecordingStartStopButton->setChecked(true);
    d->RecordingStatus->setPixmap(d->InformationPixmap);
    break;
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED:
    d->RecordingStartStopButton->setIcon(d->IconRecord);
    d->RecordingStartStopButton->setText("  Start recording");
    d->RecordingStartStopButton->setToolTip("If clicked, start recording");
    d->RecordingStartStopButton->setEnabled(true);
    d->RecordingStartStopButton->setChecked(false);
    d->RecordingStatus->setPixmap(d->CriticalPixmap);
    break;
  default:
    d->RecordingStartStopButton->setIcon(d->IconRecord);
    d->RecordingStartStopButton->setText("  Start recording");
    d->RecordingStartStopButton->setToolTip("If clicked, start recording");
    d->RecordingStartStopButton->setEnabled(true);
    d->RecordingStartStopButton->setChecked(false);
    d->RecordingStatus->setPixmap(d->InformationPixmap);
    break;
  }

  // Offline reconstruction
  d->OfflineReconstructionStatus->setToolTip(QString::fromStdString(d->ParameterNode->GetOfflineReconstructionMessage()));
  switch (d->ParameterNode->GetOfflineReconstructionStatus())
  {
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECONSTRUCTING:
    d->OfflineReconstructionStartButton->setIcon(QIcon(":/Icons/icon_Wait.png"));
    d->OfflineReconstructionStartButton->setText("  Offline reconstruction in progress ...");
    d->OfflineReconstructionStartButton->setToolTip("If clicked, stop recording");
    d->OfflineReconstructionStartButton->setEnabled(false);
    d->OfflineReconstructionStartButton->setChecked(true);
    d->OfflineReconstructionStatus->setPixmap(d->InformationPixmap);
    break;
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED:
    d->OfflineReconstructionStartButton->setIcon(d->IconRecord);
    d->OfflineReconstructionStartButton->setText("  Offline reconstruction");
    d->OfflineReconstructionStartButton->setEnabled(true);
    d->OfflineReconstructionStartButton->setChecked(false);
    d->OfflineReconstructionStatus->setPixmap(d->CriticalPixmap);
    break;
  default:
    d->OfflineReconstructionStartButton->setIcon(d->IconRecord);
    d->OfflineReconstructionStartButton->setText("  Offline reconstruction");
    d->OfflineReconstructionStartButton->setEnabled(true);
    d->OfflineReconstructionStartButton->setChecked(false);
    d->OfflineReconstructionStatus->setPixmap(d->InformationPixmap);
    break;
  }

  // Scout scan
  d->ScoutScanStatus->setToolTip(QString::fromStdString(d->ParameterNode->GetScoutScanMessage()));
  switch (d->ParameterNode->GetScoutScanStatus())
  {
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECORDING:
    d->ScoutScanStartStopButton->setText("  Stop scout scan and reconstruct volume");
    d->ScoutScanStartStopButton->setIcon(d->IconStop);
    d->ScoutScanStartStopButton->setToolTip("If clicked, stop recording and reconstruct recorded volume");
    d->ScoutScanStartStopButton->setEnabled(true);
    d->ScoutScanStartStopButton->setChecked(true);
    d->ScoutScanStatus->setPixmap(d->InformationPixmap);
    break;
  case  vtkMRMLPlusRemoteNode::PLUS_REMOTE_RECONSTRUCTING:
    d->ScoutScanStartStopButton->setIcon(QIcon(":/Icons/icon_Wait.png"));
    d->ScoutScanStartStopButton->setText("  Scout scan reconstruction in progress...");
    d->ScoutScanStartStopButton->setEnabled(false);
    d->ScoutScanStartStopButton->setChecked(true);
    d->ScoutScanStatus->setPixmap(d->InformationPixmap);
    break;
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED:
    d->ScoutScanStartStopButton->setText("  Start scout scan");
    d->ScoutScanStartStopButton->setIcon(d->IconRecord);
    d->ScoutScanStartStopButton->setToolTip("If clicked, start recording");
    d->ScoutScanStartStopButton->setEnabled(true);
    d->ScoutScanStartStopButton->setChecked(false);
    d->ScoutScanStatus->setPixmap(d->CriticalPixmap);
    break;
  default:
    d->ScoutScanStartStopButton->setText("  Start scout scan");
    d->ScoutScanStartStopButton->setIcon(d->IconRecord);
    d->ScoutScanStartStopButton->setToolTip("If clicked, start recording");
    d->ScoutScanStartStopButton->setEnabled(true);
    d->ScoutScanStartStopButton->setChecked(false);
    d->ScoutScanStatus->setPixmap(d->InformationPixmap);
    break;
  }

  // Live volume reconstruction
  d->LiveReconstructionStatus->setToolTip(QString::fromStdString(d->ParameterNode->GetLiveReconstructionMessage()));
  switch (d->ParameterNode->GetLiveReconstructionStatus())
  {
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_IN_PROGRESS:
    d->LiveReconstructionStartStopButton->setIcon(d->IconStop);
    d->LiveReconstructionStartStopButton->setText("  Stop live reconstruction");
    d->LiveReconstructionStartStopButton->setToolTip("If clicked, stop live reconstruction");
    d->LiveReconstructionStartStopButton->setEnabled(true);
    d->LiveReconstructionStartStopButton->setChecked(true);
    d->LiveReconstructionStatus->setPixmap(d->InformationPixmap);
    break;
  case vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED:
    d->LiveReconstructionStartStopButton->setText("  Start live reconstruction");
    d->LiveReconstructionStartStopButton->setIcon(d->IconRecord);
    d->LiveReconstructionStartStopButton->setToolTip("If clicked, start live reconstruction");
    d->LiveReconstructionStartStopButton->setEnabled(true);
    d->LiveReconstructionStartStopButton->setChecked(false);
    d->LiveReconstructionStatus->setPixmap(d->CriticalPixmap);
    break;
  default:
    d->LiveReconstructionStartStopButton->setText("  Start live reconstruction");
    d->LiveReconstructionStartStopButton->setIcon(d->IconRecord);
    d->LiveReconstructionStartStopButton->setToolTip("If clicked, start live reconstruction");
    d->LiveReconstructionStartStopButton->setEnabled(true);
    d->LiveReconstructionStartStopButton->setChecked(false);
    d->LiveReconstructionStatus->setPixmap(d->InformationPixmap);
    break;
  }

}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::updateParameterNodeFromGui()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  if (!d->ParameterNode)
  {
    // Block events so that onParameterSetSelected is not called,
    // becuase it would update the GUI from the node.
    d->ParameterNodeSelector->addNode();
    d->ParameterNode = vtkMRMLPlusRemoteNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
    if (!d->ParameterNode)
    {
      QString errorMessage("Failed to create PlusRemote parameter node");
      qCritical() << Q_FUNC_INFO << ": " << errorMessage;
      return;
    }
  }

  int previousModify = d->ParameterNode->StartModify();

  vtkSmartPointer<vtkMRMLIGTLConnectorNode> oldConnectorNode = d->ParameterNode->GetOpenIGTLinkConnectorNode();
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> newConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(d->OpenIGTLinkConnectorNodeSelector->currentNode());
  if (oldConnectorNode != newConnectorNode)
  {
    qvtkReconnect(oldConnectorNode, newConnectorNode, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(onConnectorNodeConnected()));
    qvtkReconnect(oldConnectorNode, newConnectorNode, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(onConnectorNodeDisconnected()));
    d->ParameterNode->SetAndObserveOpenIGTLinkConnectorNode(newConnectorNode);
    if (newConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::StateConnected)
    {
      this->onConnectorNodeConnected();
    }
    else
    {
      this->onConnectorNodeDisconnected();
    }
  }

  d->ParameterNode->SetAndObserveOpenIGTLinkConnectorNode(vtkMRMLIGTLConnectorNode::SafeDownCast(d->OpenIGTLinkConnectorNodeSelector->currentNode()));
  d->ParameterNode->SetCurrentCaptureID(d->CaptureIDSelector->currentText().toStdString());
  d->ParameterNode->SetCurrentVolumeReconstructorID(d->VolumeReconstructorIDSelector->currentText().toStdString());

  /// Recording
  d->ParameterNode->SetRecordingFilename(d->RecordingFilenameLineEdit->text().toStdString());
  d->ParameterNode->SetRecordingFilenameCompletion(d->RecordingAddTimestampToFilenameCheckBox->isChecked());
  d->ParameterNode->SetRecordingEnableCompression(d->RecordingEnableCompressionCheckBox->isChecked());

  // Offline volume reconstruction
  d->ParameterNode->SetOfflineReconstructionSpacing(d->OfflineReconstructionSpacingSpinBox->value());
  d->ParameterNode->SetOfflineReconstructionDevice(d->OfflineReconstructionDeviceLineEdit->text().toStdString());
  d->ParameterNode->AddRecordedVolume(d->OfflineReconstructionVolumeSelector->currentText().toStdString());
  d->ParameterNode->SetOfflineReconstructionInputFilename(d->OfflineReconstructionVolumeSelector->currentText().toStdString());
  d->ParameterNode->SetOfflineReconstructionShowResultOnCompletion(d->OfflineReconstructionShowResultOnCompletionCheckBox->isChecked());

  // Scout scan reconstruction
  d->ParameterNode->SetScoutScanSpacing(d->ScoutScanSpacingSpinBox->value());
  d->ParameterNode->SetScoutScanFilename(d->ScoutScanFilenameLineEdit->text().toStdString());
  d->ParameterNode->SetScoutScanFilenameCompletion(d->ScoutScanAddTimestampToFilenameCheckBox->isChecked());
  d->ParameterNode->SetScoutScanShowResultOnCompletion(d->ScoutScanShowResultOnCompletionCheckBox->isChecked());

  // Live volume reconstrution
  d->ParameterNode->SetLiveReconstructionDisplayROI(d->LiveReconstructionShowHideROIButton->isChecked());
  d->ParameterNode->SetLiveReconstructionROIDimensions(
    d->LiveReconstructionExtentXSpinBox->value(),
    d->LiveReconstructionExtentYSpinBox->value(),
    d->LiveReconstructionExtentZSpinBox->value());
  d->ParameterNode->SetLiveReconstructionSpacing(d->LiveReconstructionSpacingSpinBox->value());
  d->ParameterNode->SetLiveReconstructionDevice(d->LiveReconstructionDeviceLineEdit->text().toStdString());
  d->ParameterNode->SetLiveReconstructionFilename(d->LiveReconstructionFilenameLineEdit->text().toStdString());
  d->ParameterNode->SetLiveReconstructionFilenameCompletion(d->LiveReconstructionAddTimestampToFilenameCheckBox->isChecked());

  d->ParameterNode->SetLiveReconstructionSnapshotsIntervalSec(d->LiveReconstructionSnapshotIntervalSecSpinBox->value());
  d->ParameterNode->SetLiveReconstructionShowResultOnCompletion(d->LiveReconstructionShowResultOnCompletionCheckBox->isChecked());

  // Config file
  d->ParameterNode->SetConfigFilename(d->ConfigFilenameLineEdit->text().toStdString());

  d->ParameterNode->EndModify(previousModify);
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onParameterSetSelected(vtkMRMLNode* node)
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  vtkSmartPointer<vtkMRMLPlusRemoteNode> newNode = vtkMRMLPlusRemoteNode::SafeDownCast(node);
  this->setParameterNode(node);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onConnectorNodeConnected()
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  if (!d->ParameterNode)
  {
    return;
  }

  d->logic()->RequestCaptureDeviceIDs(d->ParameterNode);
  d->logic()->RequestVolumeReconstructorDeviceIDs(d->ParameterNode);
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onConnectorNodeDisconnected()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  if (!d->ParameterNode)
  {
    return;
  }
  d->ParameterNode->Modified();
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onRecordingStartStopButtonClicked()
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  if (!d->ParameterNode)
  {
    return;
  }
  d->RecordingStatus->setPixmap(d->InformationPixmap);
  if (d->RecordingStartStopButton->isChecked())
  {
    d->logic()->StartRecording(d->ParameterNode);
  }
  else
  {
    d->logic()->StopRecording(d->ParameterNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onOfflineReconstructionButtonClicked()
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  if (!d->ParameterNode)
  {
    return;
  }

  d->logic()->StartOfflineReconstruction(d->ParameterNode);
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onScoutScanStartStopButtonClicked()
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  if (d->ScoutScanStartStopButton->isChecked())
  {
    d->logic()->StartScoutScan(d->ParameterNode);
  }
  else
  {
    d->logic()->StopScoutScan(d->ParameterNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onLiveReconstructionStartStopButtonClicked()
{
  Q_D(qSlicerPlusRemoteModuleWidget);

  if (!d->ParameterNode)
  {
    return;
  }

  if (d->LiveReconstructionStartStopButton->isChecked())
  {
    vtkMRMLAnnotationROINode* roiNode = d->ParameterNode->GetLiveReconstructionROINode();
    if (roiNode)
    {
      d->LiveReconstructionStatus->setPixmap(d->InformationPixmap);
      d->LiveReconstructionStatus->setToolTip("Live reconstruction in progress");

      d->logic()->StartLiveVolumeReconstruction(d->ParameterNode);
    }
    else
    {
      d->LiveReconstructionStartStopButton->setChecked(false);
      d->ParameterNode->SetLiveReconstructionStatus(vtkMRMLPlusRemoteNode::PLUS_REMOTE_FAILED);
      d->ParameterNode->SetLiveReconstructionMessage("No ROI has been set yet. Start with scout scan");
    }

  }
  else
  {
    d->LiveReconstructionStatus->setPixmap(d->InformationPixmap);
    d->LiveReconstructionStatus->setToolTip("Live reconstruction is being stopped");
    d->logic()->StopLiveVolumeReconstruction(d->ParameterNode);
    d->LiveReconstructionStartStopButton->setEnabled(false);
    d->LiveReconstructionStartStopButton->setChecked(true);
  }

}

//---------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onLiveReconstructionShowHideROIButtonClicked()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  d->ParameterNode->SetLiveReconstructionDisplayROI(d->LiveReconstructionShowHideROIButton->isChecked());
}

//---------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onUpdateTransformButtonClicked()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  vtkSmartPointer<vtkMRMLLinearTransformNode> updatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(d->ConfigUpdateTransformSelector->currentNode());
  if (!updatedTransformNode)
  {
    return;
  }
  d->ParameterNode->SetAndObserveUpdatedTransformNode(updatedTransformNode);
  d->logic()->UpdateTransform(d->ParameterNode);
}

//---------------------------------------------------------------------------
void qSlicerPlusRemoteModuleWidget::onSaveConfigFileButtonClicked()
{
  Q_D(qSlicerPlusRemoteModuleWidget);
  d->logic()->SaveConfigFile(d->ParameterNode);
}
