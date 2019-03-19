/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QMouseEvent>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>

// CTK includes
#include "ctkModelTester.h"

// qMRML includes
#include "qMRMLSceneModel.h"
//#include "qMRMLIGTLIOSortFilterProxyModel.h"
#include "qMRMLSortFilterProxyModel.h"
#include "qMRMLSceneTransformModel.h"
#include "qMRMLIGTLIOModel.h"
#include "qMRMLTreeView.h"
#include "qMRMLItemDelegate.h"

// OpenIGTLinkIF GUI includes
#include "qMRMLIGTLIOTreeView.h"

// OpenIGTLinkIF Logic includes
#include "vtkSlicerOpenIGTLinkIFLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include "igtlioConnector.h"

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class qMRMLIGTLIOTreeViewPrivate
{
  Q_DECLARE_PUBLIC(qMRMLIGTLIOTreeView);
protected:
  qMRMLIGTLIOTreeView* const q_ptr;
public:
  qMRMLIGTLIOTreeViewPrivate(qMRMLIGTLIOTreeView& object);
  void init();

  //void setSortFilterProxyModel(qMRMLIGTLIOSortFilterProxyModel* newSortModel);
  void setSortFilterProxyModel(qMRMLSortFilterProxyModel* newSortModel);

  qMRMLIGTLIOModel*                 SceneModel;
  //qMRMLIGTLIOSortFilterProxyModel*        SortFilterModel;
  qMRMLSortFilterProxyModel*        SortFilterModel;
  vtkSlicerOpenIGTLinkIFLogic*      Logic;
};

//------------------------------------------------------------------------------
qMRMLIGTLIOTreeViewPrivate::qMRMLIGTLIOTreeViewPrivate(qMRMLIGTLIOTreeView& object)
  : q_ptr(&object)
{
  this->SceneModel = 0;
  this->SortFilterModel = 0;
}

//------------------------------------------------------------------------------
void qMRMLIGTLIOTreeViewPrivate::init()
{
  Q_Q(qMRMLIGTLIOTreeView);

  q->setItemDelegate(new qMRMLItemDelegate(q));

  QObject::connect(q, SIGNAL(clicked(QModelIndex)),
                   q, SLOT(onClicked(QModelIndex)));

  this->SceneModel = new qMRMLIGTLIOModel(q);
  q->setSceneModel(this->SceneModel, "IGTLConnector");
  QStringList nodeTypes = QStringList();
  nodeTypes.append("vtkMRMLIGTLConnectorNode");

  q->setNodeTypes(nodeTypes);
  q->setUniformRowHeights(true);
  this->SortFilterModel = q->sortFilterProxyModel();

  //q->expandToDepth(4);
  q->expandAll();

}


//------------------------------------------------------------------------------
//void qMRMLIGTLIOTreeViewPrivate::setSortFilterProxyModel(qMRMLIGTLIOSortFilterProxyModel* newSortModel)
void qMRMLIGTLIOTreeViewPrivate::setSortFilterProxyModel(qMRMLSortFilterProxyModel* newSortModel)
{
  Q_Q(qMRMLIGTLIOTreeView);

  if (newSortModel == this->SortFilterModel)
  {
    return;
  }

  // delete the previous filter
  delete this->SortFilterModel;
  this->SortFilterModel = newSortModel;
  // Set the input of the view
  // if no filter is given then let's show the scene model directly
  q->QTreeView::setModel(this->SortFilterModel
                         ? static_cast<QAbstractItemModel*>(this->SortFilterModel)
                         : static_cast<QAbstractItemModel*>(this->SceneModel));
  // Setting a new model to the view resets the selection model

  // The following call has been replaced by onClick();
  QObject::connect(q->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
                   q, SLOT(onCurrentRowChanged(QModelIndex)));

  if (!this->SortFilterModel)
  {
    return;
  }
  this->SortFilterModel->setParent(q);
  // Set the input of the filter
  this->SortFilterModel->setSourceModel(this->SceneModel);

  // resize the view if new rows are added/removed
  QObject::connect(this->SortFilterModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)),
                   q, SLOT(onNumberOfVisibleIndexChanged()));
  QObject::connect(this->SortFilterModel, SIGNAL(rowsInserted(QModelIndex, int, int)),
                   q, SLOT(onNumberOfVisibleIndexChanged()));

  //q->expandToDepth(4);
  q->expandAll();
}



//------------------------------------------------------------------------------
qMRMLIGTLIOTreeView::qMRMLIGTLIOTreeView(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qMRMLIGTLIOTreeViewPrivate(*this))
    //  : d_ptr(new qMRMLIGTLIOTreeViewPrivate(*this))
{
  Q_D(qMRMLIGTLIOTreeView);
  d->init();

  // we need to enable mouse tracking to set the appropriate cursor while mouseMove occurs
  this->setMouseTracking(true);
}

//------------------------------------------------------------------------------
qMRMLIGTLIOTreeView::~qMRMLIGTLIOTreeView()
{
}

//------------------------------------------------------------------------------
void qMRMLIGTLIOTreeView::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLIGTLIOTreeView);
  Q_ASSERT(d->SortFilterModel);
  // only qMRMLSceneModel needs the scene, the other proxies don't care.
  d->SceneModel->setMRMLScene(scene);

  //this->expandToDepth(4);
  this->expandAll();
}

//------------------------------------------------------------------------------
// Click and selected event handling
//------------------------------------------------------------------------------

int qMRMLIGTLIOTreeView::rowProperty(const QModelIndex& index, vtkMRMLIGTLConnectorNode*& cnode, int& dir, vtkMRMLNode*& dnode)
{
  Q_D(qMRMLIGTLIOTreeView);
  Q_ASSERT(d->SortFilterModel);

  //vtkMRMLIGTLConnectorNode * cnode;
  QStandardItem* parent1;
  QStandardItem* grandParent;

  cnode = NULL;
  dnode = NULL;
  dir = igtlioConnector::IO_UNSPECIFIED;

  qMRMLSceneModel* sceneModel = qobject_cast<qMRMLSceneModel*>(d->SortFilterModel->sourceModel());
  QStandardItem* item = sceneModel->itemFromIndex(d->SortFilterModel->mapToSource(index));
  if (!item)
  {
    return TYPE_UNKNOWN;
  }

  parent1 = item->parent();
  if (!parent1)
  {
    return TYPE_ROOT; // Root
  }

  // Check if the item is "IN" or "OUT", parent must be connector node
  QString text = parent1->child(item->row(), 0)->text();
  if (text.compare(QString("IN")) == 0)
  {
    vtkMRMLNode* node = sceneModel->mrmlNodeFromIndex(parent1->index());
    cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
    dir = igtlioConnector::IO_INCOMING;
    return TYPE_STREAM;
  }
  else if (text.compare(QString("OUT")) == 0)
  {
    vtkMRMLNode* node = sceneModel->mrmlNodeFromIndex(parent1->index());
    cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
    dir = igtlioConnector::IO_OUTGOING;
    return TYPE_STREAM;
  }

  // If not, the selected item must be a data node
  grandParent = parent1->parent();
  if (!grandParent)
  {
    return TYPE_UNKNOWN;
  }

  vtkMRMLNode* node = sceneModel->mrmlNodeFromIndex(grandParent->index());
  cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);

  // toLatin1() creates a temporary array, so we need to save it to an object to keeep the buffer valid
  QByteArray idStrArray = parent1->child(item->row(), 0)->data().toString().toLatin1();
  const char* datastr = idStrArray.data();
  if (strncmp(datastr, "io", 2) == 0)
  {
    // remove the prefix "io". See NOTE in qMRMLIGTLIOModel.cxx
    dnode = this->mrmlScene()->GetNodeByID(&datastr[2]);
  }
  else
  {
    dnode = this->mrmlScene()->GetNodeByID(datastr);
  }
  //vtkMRMLNode* dnode = sceneModel->mrmlNodeFromIndex(index); // Does this work?

  if (cnode && parent1 && grandParent)
  {
    QString text = grandParent->child(parent1->row(), 0)->text();
    if (text.compare(QString("IN")) == 0)
    {
      dir = igtlioConnector::IO_INCOMING;
      return TYPE_DATANODE;
    }
    else
    {
      dir = igtlioConnector::IO_OUTGOING;
      return TYPE_DATANODE;
    }
  }
  else
  {
    cnode = NULL;
    dir = igtlioConnector::IO_UNSPECIFIED;
    return TYPE_UNKNOWN;
  }

  return TYPE_UNKNOWN;

}


void qMRMLIGTLIOTreeView::onClicked(const QModelIndex& index)
{
  //Q_D(qMRMLIGTLIOTreeView);

  vtkMRMLIGTLConnectorNode* cnode;
  vtkMRMLNode* dnode;
  int dir;
  int type = rowProperty(index, cnode, dir, dnode);
  if (!cnode)
  {
    return;
  }
  igtlioDevice* device = NULL;
  if (dnode)
  {
    if (dir == igtlioConnector::IO_OUTGOING)
    {
      device = static_cast<igtlioDevice*>(cnode->CreateDeviceForOutgoingMRMLNode(dnode));
    }
    else if (dir == igtlioConnector::IO_INCOMING)
    {
      device = static_cast<igtlioDevice*>(cnode->GetDeviceFromIncomingMRMLNode(dnode->GetID()));
    }
  }
  if (index.column() == qMRMLIGTLIOModel::VisualizationColumn)
  {
    //qMRMLSceneModel* sceneModel = qobject_cast<qMRMLSceneModel*>(d->SortFilterModel->sourceModel());
    //QStandardItem* item = sceneModel->itemFromIndex(d->SortFilterModel->mapToSource(index));

    if (dnode && device && type == TYPE_DATANODE)
    {
      // Toggle the visibility
      const char* attr = dnode->GetAttribute("IGTLVisible");
      if (attr && strcmp(attr, "true") == 0)
      {
        dnode->SetAttribute("IGTLVisible", "false");
        device->SetVisibility(false);
      }
      else
      {
        dnode->SetAttribute("IGTLVisible", "true");
        device->SetVisibility(true);
      }
      cnode->InvokeEvent(cnode->DeviceModifiedEvent);
    }
    emit ioTreeViewUpdated(type, cnode, dir, dnode);
  }
  else if (index.column() == qMRMLIGTLIOModel::PushOnConnectColumn)
  {
    if (dnode && device && type == TYPE_DATANODE && dir == igtlioConnector::IO_OUTGOING)
    {
      // Toggle the checkbox for "push on connect" feature
      const char* attr = dnode->GetAttribute("OpenIGTLinkIF.pushOnConnect");
      if (attr && strcmp(attr, "true") == 0)
      {
        dnode->SetAttribute("OpenIGTLinkIF.pushOnConnect", "false");
        device->SetPushOnConnect(false);
      }
      else
      {
        dnode->SetAttribute("OpenIGTLinkIF.pushOnConnect", "true");
        device->SetPushOnConnect(true);
      }
      cnode->InvokeEvent(cnode->DeviceModifiedEvent);
    }
    emit ioTreeViewUpdated(type, cnode, dir, dnode);
  }
  else if (index != this->CurrentIndex)
  {
    this->CurrentIndex = index;
    //emit connectorNodeUpdated(cnode, dir);
    emit ioTreeViewUpdated(type, cnode, dir, dnode);
  }
}


void qMRMLIGTLIOTreeView::onCurrentRowChanged(const QModelIndex& index)
{
  //Q_D(qMRMLIGTLIOTreeView);
  //Q_ASSERT(d->SortFilterModel);
  //Q_ASSERT(this->currentNode() == d->SortFilterModel->mrmlNodeFromIndex(index));

  vtkMRMLIGTLConnectorNode* cnode;
  vtkMRMLNode* dnode;
  int dir;
  int type = rowProperty(index, cnode, dir, dnode);
  emit ioTreeViewUpdated(type, cnode, dir, dnode);

  //emit connectorNodeUpdated(cnode, dir);
  //emit ioTreeViewUpdated(type, cnode, dir);
}


//------------------------------------------------------------------------------
//
// MouseMove event handling
//
//------------------------------------------------------------------------------

#ifndef QT_NO_CURSOR
//------------------------------------------------------------------------------
bool qMRMLIGTLIOTreeView::viewportEvent(QEvent* e)
{

  // reset the cursor if we leave the viewport
  if (e->type() == QEvent::Leave)
  {
    setCursor(QCursor());
  }

  return QTreeView::viewportEvent(e);
}

//------------------------------------------------------------------------------
void qMRMLIGTLIOTreeView::mouseMoveEvent(QMouseEvent* e)
{
  this->QTreeView::mouseMoveEvent(e);

  //if (index.column() == qMRMLSceneIGTLConnectorModel::VisibilityColumn || index.column() == qMRMLSceneIGTLConnectorModel::LockColumn || index.column() == qMRMLSceneIGTLConnectorModel::EditColumn)
  //  {
  //  // we are over a column with a clickable icon
  //  // let's change the cursor
  //  QCursor handCursor(Qt::PointingHandCursor);
  //  this->setCursor(handCursor);
  //  // .. and bail out
  //  return;
  //  }
  //else if(this->cursor().shape() == Qt::PointingHandCursor)
  //  {
  //  // if we are NOT over such a column and we already have a changed cursor,
  //  // reset it!
  //  this->setCursor(QCursor());
  //  }

}
#endif

//------------------------------------------------------------------------------
//
// Layout and behavior customization
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void qMRMLIGTLIOTreeView::setSelectedNode(const char* id)
{
  Q_D(qMRMLIGTLIOTreeView);

  vtkMRMLNode* node = this->mrmlScene()->GetNodeByID(id);
  if (node)
  {
    this->setCurrentIndex(d->SortFilterModel->indexFromMRMLNode(node));
    //this->setCurrentIndex(d->SceneModel->indexFromNode(node));
  }
}


//------------------------------------------------------------------------------
void qMRMLIGTLIOTreeView::mousePressEvent(QMouseEvent* event)
{
  // skip qMRMLTreeView
  this->QTreeView::mousePressEvent(event);
}

//------------------------------------------------------------------------------
//
// Connections to other classes
//
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Set and observe the logic
//-----------------------------------------------------------------------------
void qMRMLIGTLIOTreeView::setLogic(vtkSlicerOpenIGTLinkIFLogic* logic)
{
  Q_D(qMRMLIGTLIOTreeView);
  if (!logic)
  {
    return;
  }

  d->Logic = logic;

  // propagate down to model
  d->SceneModel->setLogic(d->Logic);
}
