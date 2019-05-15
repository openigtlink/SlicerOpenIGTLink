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
  and was supported through CANARIE's Research Software Program, and Cancer
  Care Ontario.

==============================================================================*/

// PlusRemote includes
#include "qMRMLPlusServerLauncherTableView.h"
#include "ui_qMRMLPlusServerLauncherTableView.h"

#include "vtkMRMLPlusServerLauncherNode.h"
#include "vtkMRMLPlusServerNode.h"
#include <vtkMRMLScene.h>

#include "vtkSlicerPlusRemoteLogic.h"

#include "vtkMRMLTextNode.h"

// VTK includes
#include <vtkWeakPointer.h>

// Qt includes
#include <QDebug>
#include <QKeyEvent>
#include <QStringList>
#include <QPushButton>

// SlicerQt includes
#include "qSlicerApplication.h"

enum ServerColumns
{
  ID,
  Name,
  State,
  LastColumn
};

enum
{
  ItemTypeServerNodeId = QTableWidgetItem::UserType,
};

//-----------------------------------------------------------------------------
class qMRMLPlusServerLauncherTableViewPrivate : public Ui_qMRMLPlusServerLauncherTableView
{
  Q_DECLARE_PUBLIC(qMRMLPlusServerLauncherTableView);

protected:
  qMRMLPlusServerLauncherTableView* const q_ptr;
public:
  qMRMLPlusServerLauncherTableViewPrivate(qMRMLPlusServerLauncherTableView& object);
  void init();

public:
  /// RT plan MRML node containing shown beams
  vtkWeakPointer<vtkMRMLPlusServerLauncherNode> LauncherNode;

  QPixmap IconOn;
  QPixmap IconOff;
  QPixmap IconStarting;
  QPixmap IconStopping;

private:
  QStringList ColumnLabels;
};

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableViewPrivate::qMRMLPlusServerLauncherTableViewPrivate(qMRMLPlusServerLauncherTableView& object)
  : q_ptr(&object)
  , IconOn(":/Icons/PlusLauncherRemoteConnect.png")
  , IconOff(":/Icons/PlusLauncherRemoteNotConnected.png")
  , IconStarting(":/Icons/PlusLauncherRemoteWait.png")
  , IconStopping(":/Icons/PlusLauncherRemoteWait.png")
{
  this->LauncherNode = nullptr;
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableViewPrivate::init()
{
  Q_Q(qMRMLPlusServerLauncherTableView);
  this->setupUi(q);

  // Set table header properties
  for (int i = 0; i < LastColumn; ++i)
  {
    this->ColumnLabels << "";
  }
  this->ColumnLabels[ID] = "ID";
  this->ColumnLabels[Name] = "Name";
  this->ColumnLabels[State] = "State";
  this->PlusServerLauncherTable->setHorizontalHeaderLabels(this->ColumnLabels);
  this->PlusServerLauncherTable->setColumnCount(this->ColumnLabels.size());
  this->PlusServerLauncherTable->horizontalHeader()->setSectionResizeMode(ID, QHeaderView::Interactive);
  this->PlusServerLauncherTable->horizontalHeader()->setSectionResizeMode(Name, QHeaderView::Stretch);
  this->PlusServerLauncherTable->horizontalHeader()->setSectionResizeMode(State, QHeaderView::ResizeToContents);

  // Select rows
  this->PlusServerLauncherTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Make connections
  QObject::connect(this->ShowRemoteCheckBox, SIGNAL(clicked()), q, SLOT(updateWidgetFromLauncherMRML()));
  QObject::connect(this->PlusServerLauncherTable, SIGNAL(itemSelectionChanged()), q, SLOT(updateSelectedServerPanel()));
  QObject::connect(this->AddServerButton, SIGNAL(clicked()), q, SLOT(onAddServer()));
  QObject::connect(this->RemoveServerButton, SIGNAL(clicked()), q, SLOT(onRemoveServer()));
  QObject::connect(this->ConfigFileComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(updateServerMRMLFromWidget()));
  QObject::connect(this->LocalControlCheckBox, SIGNAL(clicked()), q, SLOT(updateServerMRMLFromWidget()));

  this->PlusServerLauncherTable->installEventFilter(q);
}

//-----------------------------------------------------------------------------
// qMRMLPlusServerLauncherTableView methods

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableView::qMRMLPlusServerLauncherTableView(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLPlusServerLauncherTableViewPrivate(*this))
{
  Q_D(qMRMLPlusServerLauncherTableView);
  d->init();
  this->updateWidgetFromLauncherMRML();
}

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableView::~qMRMLPlusServerLauncherTableView() = default;

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::setLauncherNode(vtkMRMLNode* node)
{
  Q_D(qMRMLPlusServerLauncherTableView);

  vtkMRMLPlusServerLauncherNode* launcherNode = vtkMRMLPlusServerLauncherNode::SafeDownCast(node);

  // Connect modified events to population of the table
  qvtkReconnect(d->LauncherNode, launcherNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromLauncherMRML()));
  qvtkReconnect(d->LauncherNode, launcherNode, vtkMRMLPlusServerLauncherNode::ServerAddedEvent, this, SLOT(onServerAdded(vtkObject*, void*)));
  qvtkReconnect(d->LauncherNode, launcherNode, vtkMRMLPlusServerLauncherNode::ServerRemovedEvent, this, SLOT(onServerRemoved(vtkObject*, void*)));

  if (d->LauncherNode)
  {
    std::vector<vtkMRMLPlusServerNode*> servers = d->LauncherNode->GetServerNodes();
    for (std::vector<vtkMRMLPlusServerNode*>::iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt)
    {
      vtkMRMLPlusServerNode* serverNode = (*serverIt);
      qvtkDisconnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
    }
  }

  if (launcherNode)
  {
    // Connect modified events of contained beam nodes to update table
    std::vector<vtkMRMLPlusServerNode*> servers = launcherNode->GetServerNodes();
    for (std::vector<vtkMRMLPlusServerNode*>::iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt)
    {
      vtkMRMLPlusServerNode* serverNode = (*serverIt);
      qvtkConnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
    }
  }

  d->LauncherNode = launcherNode;
  this->updateWidgetFromLauncherMRML();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLPlusServerLauncherTableView::launcherNode()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  return d->LauncherNode;
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateWidgetFromLauncherMRML()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  // Clear table
  bool tableWasBlocking = d->PlusServerLauncherTable->blockSignals(true);
  d->PlusServerLauncherTable->clearContents();
  d->PlusServerLauncherTable->setRowCount(0);
  d->PlusServerLauncherTable->blockSignals(tableWasBlocking);

  // Disable widgets if no launcher is selected
  d->PlusServerLauncherTable->setEnabled(d->LauncherNode);
  d->AddServerButton->setEnabled(d->LauncherNode);
  d->RemoveServerButton->setEnabled(d->LauncherNode);
  d->ConfigFileComboBox->setEnabled(d->LauncherNode);
  if (!d->LauncherNode)
  {
    this->updateSelectedServerPanel();
    return;
  }

  // Keep track of the selected node so that it can be reselected after the table is rebuilt
  vtkMRMLPlusServerNode* selectedServerNode = this->getSelectedServer();

  // Block signals so that onBeamTableItemChanged function is not called when populating
  tableWasBlocking = d->PlusServerLauncherTable->blockSignals(true);

  // Populate the table with the server nodes connected to the current launcher
  std::vector<vtkMRMLPlusServerNode*> serverNodes = d->LauncherNode->GetServerNodes();
  int numberOfNodes = serverNodes.size();

  for (int i = 0; i < numberOfNodes; ++i)
  {
    vtkMRMLPlusServerNode* serverNode = serverNodes[i];
    if (!d->ShowRemoteCheckBox->isChecked() && !serverNode->GetControlledLocally())
    {
      continue;
    }

    int row = d->PlusServerLauncherTable->rowCount();
    d->PlusServerLauncherTable->insertRow(d->PlusServerLauncherTable->rowCount());

    for (int j = 0; j < LastColumn; ++j)
    {
      QTableWidgetItem* item = new QTableWidgetItem();
      item->setData(ItemTypeServerNodeId, serverNode->GetID());
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      d->PlusServerLauncherTable->setItem(row, j, item);
    }

    QPushButton* startStopButton = new QPushButton(this);
    startStopButton->setCheckable(true);
    QObject::connect(startStopButton, SIGNAL(clicked()), this, SLOT(onStartStopServer()));
    d->PlusServerLauncherTable->setCellWidget(row, State, startStopButton);

    this->updateWidgetFromServerMRML(serverNode);
  }

  // Re-select the selected node
  if (selectedServerNode)
  {
    int selectedServerRow = this->getRowForServer(selectedServerNode);
    d->PlusServerLauncherTable->selectRow(selectedServerRow);
  }

  // Unblock signals
  d->PlusServerLauncherTable->blockSignals(tableWasBlocking);

  this->updateSelectedServerPanel();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateWidgetFromServerMRML(vtkObject* caller, void* callData)
{
  Q_D(qMRMLPlusServerLauncherTableView);

  // Triggerd by vtkMRMLPlusServerNode::ModifiedEvent
  vtkMRMLPlusServerNode* serverNode = vtkMRMLPlusServerNode::SafeDownCast(caller);
  if (serverNode)
  {
    int row = this->getRowForServer(serverNode);
    if (row == -1)
    {
      return;
    }

    QTableWidgetItem* serverTableItem;

    std::string id = serverNode->GetServerID(); //TODO: unique server identifier
    d->PlusServerLauncherTable->item(row, ID)->setText(id.c_str());

    std::string name = serverNode->GetDeviceSetName();
    d->PlusServerLauncherTable->item(row, Name)->setText(name.c_str());

    // Update start/stop button status
    QPushButton* startStopButton = qobject_cast<QPushButton*>(d->PlusServerLauncherTable->cellWidget(row, State));
    startStopButton->setChecked(serverNode->GetDesiredState() == vtkMRMLPlusServerNode::On);
    if (!serverNode->GetControlledLocally())
    {
      startStopButton->setChecked(serverNode->GetState() == vtkMRMLPlusServerNode::On || serverNode->GetState() == vtkMRMLPlusServerNode::Starting);
    }
    switch (serverNode->GetState())
    {
    case vtkMRMLPlusServerNode::On:
      startStopButton->setIcon(d->IconOn);
      startStopButton->setToolTip("Server running");
      break;
    case vtkMRMLPlusServerNode::Off:
      startStopButton->setIcon(d->IconOff);
      startStopButton->setToolTip("Server not running");
      break;
    case vtkMRMLPlusServerNode::Starting:
      startStopButton->setIcon(d->IconStarting);
      startStopButton->setToolTip("Server starting");
      break;
    case vtkMRMLPlusServerNode::Stopping:
      startStopButton->setIcon(d->IconStopping);
      startStopButton->setToolTip("Server stopping");
      break;
    }
  }

  this->updateSelectedServerPanel();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateServerMRMLFromWidget()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  if (!d->LauncherNode)
  {
    return;
  }

  vtkMRMLPlusServerNode* selectedServerNode = this->getSelectedServer();
  if (!selectedServerNode)
  {
    return;
  }

  int wasModifying = selectedServerNode->StartModify();
  selectedServerNode->SetAndObserveConfigNode(vtkMRMLTextNode::SafeDownCast(d->ConfigFileComboBox->currentNode()));
  selectedServerNode->SetLogLevel(d->LogLevelComboBox->currentData().toInt());
  selectedServerNode->SetControlledLocally(d->LocalControlCheckBox->isChecked());
  selectedServerNode->EndModify(wasModifying);
}

//-----------------------------------------------------------------------------
int qMRMLPlusServerLauncherTableView::getRowForServer(vtkMRMLPlusServerNode* serverNode) const
{
  Q_D(const qMRMLPlusServerLauncherTableView);
  if (!serverNode)
  {
    return -1;
  }

  for (int i = 0; i < d->PlusServerLauncherTable->rowCount(); ++i)
  {
    QTableWidgetItem* item = d->PlusServerLauncherTable->item(i, 0);
    if (item)
    {
      QVariant rowData = item->data(ItemTypeServerNodeId);
      QString rowString = rowData.toString();
      if (rowData == serverNode->GetID())
      {
        return i;
      }
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
vtkMRMLPlusServerNode* qMRMLPlusServerLauncherTableView::getServerForRow(int row) const
{
  Q_D(const qMRMLPlusServerLauncherTableView);

  QTableWidgetItem* item = d->PlusServerLauncherTable->item(row, 0);
  std::string id = item->data(ItemTypeServerNodeId).toString().toStdString();
  auto servers = d->LauncherNode->GetServerNodes();
  for (auto server : servers)
  {
    std::string serverID = server->GetID();
    if (id == serverID)
    {
      return server;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkMRMLPlusServerNode* qMRMLPlusServerLauncherTableView::getSelectedServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);
  int row = -1;

  QList<QTableWidgetSelectionRange> ranges = d->PlusServerLauncherTable->selectedRanges();
  for (QTableWidgetSelectionRange range : ranges)
  {
    row = std::max(range.topRow(), row);
  }

  if (row < 0)
  {
    return nullptr;
  }

  QTableWidgetItem* item = d->PlusServerLauncherTable->currentItem();
  if (!item)
  {
    return nullptr;
  }

  QVariant itemData = item->data(ItemTypeServerNodeId);
  std::string serverNodeId = itemData.toString().toStdString();
  return vtkMRMLPlusServerNode::SafeDownCast(this->mrmlScene()->GetNodeByID(serverNodeId));
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateSelectedServerPanel()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();
  d->ConfigFileComboBox->setEnabled(serverNode);
  d->LogLevelComboBox->setEnabled(serverNode);
  d->LocalControlCheckBox->setEnabled(serverNode);
  d->DescriptionTextEdit->setEnabled(serverNode);

  vtkMRMLTextNode* configFileNode = nullptr;
  QString configFileNodeID;
  if (serverNode)
  {
    configFileNode = serverNode->GetConfigNode();
    if (configFileNode)
    {
      configFileNodeID = configFileNode->GetID();
    }

    std::string description = serverNode->GetDeviceSetDescription();
    d->DescriptionTextEdit->setText(description.c_str());
    d->LocalControlCheckBox->setChecked(serverNode->GetControlledLocally());

    d->ConfigFileComboBox->setEnabled(!serverNode->GetControlledLocally() || (serverNode->GetDesiredState() == vtkMRMLPlusServerNode::Off &&
      serverNode->GetState() == vtkMRMLPlusServerNode::Off));
    d->LogLevelComboBox->setEnabled(!serverNode->GetControlledLocally() || (serverNode->GetDesiredState() == vtkMRMLPlusServerNode::Off &&
      serverNode->GetState() == vtkMRMLPlusServerNode::Off));
  }
  else
  {
    d->LogLevelComboBox->setCurrentIndex(-1);
    d->DescriptionTextEdit->setText("");
    d->LocalControlCheckBox->setChecked(false);
  }

  bool configComboBoxWasBlocking = d->ConfigFileComboBox->blockSignals(true);
  d->ConfigFileComboBox->setCurrentNodeID(configFileNodeID);
  d->ConfigFileComboBox->blockSignals(configComboBoxWasBlocking);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onAddServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  if (!d->LauncherNode)
  {
    return;
  }

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLPlusServerNode* serverNode = vtkMRMLPlusServerNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLPlusServerNode"));
  d->LauncherNode->AddAndObserveServerNode(serverNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onRemoveServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  if (!d->LauncherNode)
  {
    return;
  }

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();
  this->mrmlScene()->RemoveNode(serverNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onStartStopServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  int row = -1;
  QObject* button = QObject::sender();
  for (int i = 0; i < d->PlusServerLauncherTable->rowCount(); i++)
  {
    if (d->PlusServerLauncherTable->cellWidget(i, State) == button)
    {
      row = i;
      break;
    }
  }

  if (row == -1)
  {
    return;
  }

  vtkMRMLPlusServerNode* serverNode = this->getServerForRow(row);
  if (!serverNode)
  {
    return;
  }

  serverNode->SetControlledLocally(true);
  if (serverNode->GetDesiredState() == vtkMRMLPlusServerNode::On)
  {
    serverNode->SetDesiredState(vtkMRMLPlusServerNode::Off);
  }
  else
  {
    serverNode->SetDesiredState(vtkMRMLPlusServerNode::On);
  }
}

//------------------------------------------------------------------------------
bool qMRMLPlusServerLauncherTableView::eventFilter(QObject* target, QEvent* event)
{
  Q_D(qMRMLPlusServerLauncherTableView);
  if (target == d->PlusServerLauncherTable)
  {
    // Prevent giving the focus to the previous/next widget if arrow keys are used
    // at the edge of the table (without this: if the current cell is in the top
    // row and user press the Up key, the focus goes from the table to the previous
    // widget in the tab order)
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
      QAbstractItemModel* model = d->PlusServerLauncherTable->model();
      QModelIndex currentIndex = d->PlusServerLauncherTable->currentIndex();

      if (model && (
        (keyEvent->key() == Qt::Key_Left && currentIndex.column() == 0)
        || (keyEvent->key() == Qt::Key_Up && currentIndex.row() == 0)
        || (keyEvent->key() == Qt::Key_Right && currentIndex.column() == model->columnCount() - 1)
        || (keyEvent->key() == Qt::Key_Down && currentIndex.row() == model->rowCount() - 1)))
      {
        return true;
      }
    }
  }
  return this->QWidget::eventFilter(target, event);
}

//------------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onServerAdded(vtkObject* caller, void* callData)
{
  Q_D(qMRMLPlusServerLauncherTableView);
  Q_UNUSED(caller);

  vtkMRMLPlusServerNode* serverNode = reinterpret_cast<vtkMRMLPlusServerNode*>(callData);
  if (!serverNode)
  {
    return;
  }
  qvtkConnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
}

//------------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onServerRemoved(vtkObject* caller, void* callData)
{
  Q_D(qMRMLPlusServerLauncherTableView);
  Q_UNUSED(caller);

  vtkMRMLPlusServerNode* serverNode = reinterpret_cast<vtkMRMLPlusServerNode*>(callData);
  if (!serverNode)
  {
    return;
  }
  qvtkDisconnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
}
