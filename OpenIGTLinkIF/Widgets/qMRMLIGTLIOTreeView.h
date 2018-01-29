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

#ifndef __qMRMLIGTLIOTreeView_h
#define __qMRMLIGTLIOTreeView_h

// qMRML includes
#include "qMRMLTreeView.h"

// OpenIGTLinkIF GUI includes
#include "qSlicerOpenIGTLinkIFModuleWidgetsExport.h"

class qMRMLIGTLIOTreeViewPrivate;
class vtkMRMLNode;
class vtkMRMLScene;
class vtkSlicerOpenIGTLinkIFLogic;
class vtkMRMLIGTLConnectorNode;

/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class Q_SLICER_MODULE_OPENIGTLINKIF_WIDGETS_EXPORT qMRMLIGTLIOTreeView : public qMRMLTreeView
{
  Q_OBJECT

public:
  enum {
    TYPE_UNKNOWN = 0,
    TYPE_ROOT,
    TYPE_CONNECTOR,
    TYPE_STREAM,
    TYPE_DATANODE
  };

public:
  typedef qMRMLTreeView Superclass;
  qMRMLIGTLIOTreeView(QWidget *parent=0);
  virtual ~qMRMLIGTLIOTreeView();

  // Register the logic
  void setLogic(vtkSlicerOpenIGTLinkIFLogic* logic);

//  void toggleLockForSelected();
//  void toggleVisibilityForSelected();
//  void deleteSelected();
//  void selectedAsCollection(vtkCollection* collection);
  void setSelectedNode(const char* id);

public slots:
  void setMRMLScene(vtkMRMLScene* scene);

signals:  
  //void connectorNodeUpdated(vtkMRMLIGTLConnectorNode*, int);
  void ioTreeViewUpdated(int, vtkMRMLIGTLConnectorNode*, int, vtkMRMLNode*);

protected slots:
  void onClicked(const QModelIndex& index);
  virtual void onCurrentRowChanged(const QModelIndex& index);

protected:
  QScopedPointer<qMRMLIGTLIOTreeViewPrivate> d_ptr;
  #ifndef QT_NO_CURSOR
    void mouseMoveEvent(QMouseEvent* e);
    bool viewportEvent(QEvent* e);
  #endif
  virtual void mousePressEvent(QMouseEvent* event);

  // Description:
  // rowProperty() returns row type and properties (connector node and direction).
  // The type can be either TYPE_UNKNOWN (error - cannot determined),
  // TYPE_ROOT (root), TYPE_CONNECTOR (connector node), TYPE_STREAM (stream -- IN or OUT)
  // or TYPE_DATANODE (data node).
  // If the type is TYPE_CONNECTOR, the pointer to the highlighted connector node is set
  // to 'cnode'. If the type is TYPE_STREAM or TYPE_DATANODE, the pointer to the parent
  // connector node, the direction of stream (either vtkMRMLIGTLConnectorNode::IO_INCOMING
  // or vtkMRMLIGTLConnectorNode::IO_OUTGOING), and the pointer to the data node
  // are set to 'cnode', 'dir', and 'dnode' respectively.
  int rowProperty(const QModelIndex& index, vtkMRMLIGTLConnectorNode* &cnode, int & dir, vtkMRMLNode* &dnode);

private:
  Q_DECLARE_PRIVATE(qMRMLIGTLIOTreeView);
  Q_DISABLE_COPY(qMRMLIGTLIOTreeView);

  QModelIndex CurrentIndex;

  // toggle the visibility of an OpenIGTLinkIF
//  void onVisibilityColumnClicked(vtkMRMLNode* node);

  // toggle un-/lock of an annotation
//  void onLockColumnClicked(vtkMRMLNode* node);

};

#endif
