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

// CTK includes
#include "ctkModelTester.h"

// qMRML includes
#include "qMRMLSceneModel.h"
#include "qMRMLSortFilterProxyModel.h"
#include "qMRMLSceneTransformModel.h"
#include "qMRMLSceneIGTLConnectorModel.h"
#include "qMRMLTreeView.h"

// OpenIGTLinkIF GUI includes
#include "qMRMLIGTLConnectorTreeView.h"

// OpenIGTLinkIF Logic includes
#include "vtkSlicerOpenIGTLinkIFLogic.h"

// MRML includes
#include <vtkMRMLNode.h>

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class qMRMLIGTLConnectorTreeViewPrivate
{
  Q_DECLARE_PUBLIC(qMRMLIGTLConnectorTreeView);
protected:
  qMRMLIGTLConnectorTreeView* const q_ptr;
public:
  qMRMLIGTLConnectorTreeViewPrivate(qMRMLIGTLConnectorTreeView& object);
  void init();

  qMRMLSceneIGTLConnectorModel*     SceneModel;
  qMRMLSortFilterProxyModel*        SortFilterModel;
  vtkSlicerOpenIGTLinkIFLogic*      Logic;
};

//------------------------------------------------------------------------------
qMRMLIGTLConnectorTreeViewPrivate::qMRMLIGTLConnectorTreeViewPrivate(qMRMLIGTLConnectorTreeView& object)
  : q_ptr(&object)
{
  this->SceneModel = 0;
  this->SortFilterModel = 0;
}

//------------------------------------------------------------------------------
void qMRMLIGTLConnectorTreeViewPrivate::init()
{
  Q_Q(qMRMLIGTLConnectorTreeView);

  this->SceneModel = new qMRMLSceneIGTLConnectorModel(q);
  q->setSceneModel(this->SceneModel, "IGTLConnector");

  QStringList nodeTypes = QStringList();
  nodeTypes.append("vtkMRMLIGTLConnectorNode");

  q->setNodeTypes(nodeTypes);
  this->SortFilterModel = q->sortFilterProxyModel();

  q->setUniformRowHeights(true);

  //QObject::connect(q, SIGNAL(clicked(QModelIndex)),
  //q, SLOT(onClicked(QModelIndex)));
}

//------------------------------------------------------------------------------
qMRMLIGTLConnectorTreeView::qMRMLIGTLConnectorTreeView(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qMRMLIGTLConnectorTreeViewPrivate(*this))
{
  Q_D(qMRMLIGTLConnectorTreeView);
  d->init();

  // we need to enable mouse tracking to set the appropriate cursor while mouseMove occurs
  this->setMouseTracking(true);
}

//------------------------------------------------------------------------------
qMRMLIGTLConnectorTreeView::~qMRMLIGTLConnectorTreeView()
{
}

//------------------------------------------------------------------------------
void qMRMLIGTLConnectorTreeView::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLIGTLConnectorTreeView);
  Q_ASSERT(d->SortFilterModel);
  // only qMRMLSceneModel needs the scene, the other proxies don't care.
  d->SceneModel->setMRMLScene(scene);

  this->expandAll();
}

//------------------------------------------------------------------------------
//
// MouseMove event handling
//
//------------------------------------------------------------------------------

#ifndef QT_NO_CURSOR
//------------------------------------------------------------------------------
bool qMRMLIGTLConnectorTreeView::viewportEvent(QEvent* e)
{

  // reset the cursor if we leave the viewport
  if (e->type() == QEvent::Leave)
  {
    setCursor(QCursor());
  }

  return QTreeView::viewportEvent(e);
}

//------------------------------------------------------------------------------
void qMRMLIGTLConnectorTreeView::mouseMoveEvent(QMouseEvent* e)
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
void qMRMLIGTLConnectorTreeView::setSelectedNode(const char* id)
{
  Q_D(qMRMLIGTLConnectorTreeView);

  vtkMRMLNode* node = this->mrmlScene()->GetNodeByID(id);

  if (node)
  {
    this->setCurrentIndex(d->SortFilterModel->indexFromMRMLNode(node));
  }
}

//------------------------------------------------------------------------------
void qMRMLIGTLConnectorTreeView::mousePressEvent(QMouseEvent* event)
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
void qMRMLIGTLConnectorTreeView::setLogic(vtkSlicerOpenIGTLinkIFLogic* logic)
{
  Q_D(qMRMLIGTLConnectorTreeView);
  if (!logic)
  {
    return;
  }

  d->Logic = logic;

  // propagate down to model
  d->SceneModel->setLogic(d->Logic);
}
