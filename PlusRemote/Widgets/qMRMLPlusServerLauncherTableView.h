/*===========/*==============================================================================

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
  and was supported through CANARIE's Research Software Program, and Cancer
  Care Ontario.

==============================================================================*/
#ifndef __qMRMLPlusServerLauncherTableView_h
#define __qMRMLPlusServerLauncherTableView_h

// PlusRemote includes
#include "qSlicerPlusRemoteModuleWidgetsExport.h"

// MRMLWidgets includes
#include "qMRMLWidget.h"

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

class qMRMLPlusServerLauncherTableViewPrivate;
class vtkMRMLNode;
class QComboBox;
class QTableWidgetItem;
class QItemSelection;
class vtkMRMLPlusServerNode;

/// \ingroup SlicerRt_QtModules_Beams_Widgets
class Q_SLICER_MODULE_PLUSREMOTE_WIDGETS_EXPORT qMRMLPlusServerLauncherTableView : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qMRMLWidget Superclass;
  /// Constructor
  explicit qMRMLPlusServerLauncherTableView(QWidget* parent = nullptr);
  /// Destructor
  ~qMRMLPlusServerLauncherTableView() override;

  /// Get server launcher MRML node
  Q_INVOKABLE vtkMRMLNode* launcherNode();

public slots:
  /// Set plan MRML node
  Q_INVOKABLE void setLauncherNode(vtkMRMLNode* node);

  /// Called when beam is added in an observed plan node
  void onServerAdded(vtkObject* caller, void* callData);

  /// Called when beam is removed in an observed plan node
  void onServerRemoved(vtkObject* caller, void* callData);

  vtkMRMLPlusServerNode* getSelectedServer();

  /// Handle changing of values in a cell
  void updateSelectedServerPanel();

  int getRowForServer(vtkMRMLPlusServerNode* serverNode) const;
  vtkMRMLPlusServerNode* getServerForRow(int row) const;

signals:
  /// Emitted if selection changes
  void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected slots:


  /// Update table according to the node
  void updateWidgetFromLauncherMRML();
  void updateWidgetFromServerMRML(vtkObject* caller, void* callData=nullptr);
  void updateServerMRMLFromWidget();


  /// To prevent accidentally moving out of the widget when pressing up/down arrows
  bool eventFilter(QObject* target, QEvent* event) override;

  void onAddServer();
  void onRemoveServer();
  void onStartStopServer();

protected:
  QScopedPointer<qMRMLPlusServerLauncherTableViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLPlusServerLauncherTableView);
  Q_DISABLE_COPY(qMRMLPlusServerLauncherTableView);
};

#endif
