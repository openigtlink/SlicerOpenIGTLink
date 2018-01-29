/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2011 Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerIGTLIONodeSelectorWidget_h
#define __qSlicerIGTLIONodeSelectorWidget_h

// Qt includes
#include <QWidget>

// CTK includes
#include <ctkVTKObject.h>


// OpenIGTLinkIF GUI includes
#include "qSlicerOpenIGTLinkIFModuleWidgetsExport.h"

class qSlicerIGTLIONodeSelectorWidgetPrivate;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLNode;
class vtkMRMLScene;
class vtkObject;

/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class Q_SLICER_MODULE_OPENIGTLINKIF_WIDGETS_EXPORT qSlicerIGTLIONodeSelectorWidget : public QWidget
{
  Q_OBJECT
  QVTK_OBJECT
public:
  typedef QWidget Superclass;
  qSlicerIGTLIONodeSelectorWidget(QWidget *parent = 0);
  virtual ~qSlicerIGTLIONodeSelectorWidget();

  enum {
    UNDEFINED,
    INCOMING,
    OUTGOING
  };

public slots:

  /// Set the MRML scene
  void setMRMLScene(vtkMRMLScene* scene);

  /// Set the MRML node of interest
  //void setCurrentNode(vtkMRMLNode* node);

  /// Set the MRML node of interest
  void updateEnabledStatus(int type, vtkMRMLIGTLConnectorNode* cnode, int dir, vtkMRMLNode* dnode);
  


protected slots:
  /// Add node to the I/O tree
  void onAddNodeButtonClicked();
  
  /// Remove node from the I/O tree
  void onRemoveNodeButtonClicked();

  /// Send node from the I/O tree
  void onSendButtonClicked();

signals:
  //void addNode(vtkMRMLNode*);
  //void removeNode(vtkMRMLNode*);

protected:
  QScopedPointer<qSlicerIGTLIONodeSelectorWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerIGTLIONodeSelectorWidget);
  Q_DISABLE_COPY(qSlicerIGTLIONodeSelectorWidget);
};

#endif

