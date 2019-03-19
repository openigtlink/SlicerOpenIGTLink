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

#ifndef __qSlicerPlusRemoteModuleWidget_h
#define __qSlicerPlusRemoteModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"
#include "qSlicerPlusRemoteModuleExport.h"

class qSlicerPlusRemoteModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_ToolPlusRemote
class Q_SLICER_QTMODULES_PLUSREMOTE_EXPORT qSlicerPlusRemoteModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPlusRemoteModuleWidget(QWidget* parent = 0);
  virtual ~qSlicerPlusRemoteModuleWidget();

public slots:

  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene* scene);
  /// Process loaded scene, updates the table after a scene is set
  void onSceneImportedEvent();

public:
  // Set the current parameter node
  void setParameterNode(vtkMRMLNode* node);

protected:
  QScopedPointer<qSlicerPlusRemoteModuleWidgetPrivate> d_ptr;

  /// Connects the gui signals
  virtual void setup();
  /// Set up the GUI from mrml when entering
  virtual void enter();

protected slots:

  // Updates the current state of the UI from the parameters
  virtual void updateWidgetFromMRML();

  // Updates the text/icon/state of the main buttons based on the current state stored in the parameter node
  virtual void updateButtonsFromStatus();

  // Changes the values in the parameter node to match the GUI
  virtual void updateParameterNodeFromGui();

  virtual void onParameterSetSelected(vtkMRMLNode* node);

  virtual void onConnectorNodeConnected();
  virtual void onConnectorNodeDisconnected();

  virtual void onRecordingStartStopButtonClicked();
  virtual void onOfflineReconstructionButtonClicked();
  virtual void onScoutScanStartStopButtonClicked();
  virtual void onLiveReconstructionStartStopButtonClicked();
  virtual void onLiveReconstructionShowHideROIButtonClicked();
  virtual void onSaveConfigFileButtonClicked();
  virtual void onUpdateTransformButtonClicked();

private:
  Q_DECLARE_PRIVATE(qSlicerPlusRemoteModuleWidget);
  Q_DISABLE_COPY(qSlicerPlusRemoteModuleWidget);

};

#endif
