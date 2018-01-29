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

#ifndef __qMRMLIGTLIOModel_h
#define __qMRMLIGTLIOModel_h

#include "qMRMLSceneModel.h"

// OpenIGTLinkIF GUI includes
#include "qSlicerOpenIGTLinkIFModuleWidgetsExport.h"

// Logic includes
#include "../Logic/vtkSlicerOpenIGTLinkIFLogic.h"

class vtkMRMLNode;
class qMRMLIGTLIOModelPrivate;

/// \ingroup Slicer_QtModules_OpenIGTLink
class Q_SLICER_MODULE_OPENIGTLINKIF_WIDGETS_EXPORT qMRMLIGTLIOModel : public qMRMLSceneModel
{
  Q_OBJECT

 public:
  
  enum Columns{
    NameColumn = 0,
    TypeColumn,
    StatusColumn,
    VisualizationColumn,
    PushOnConnectColumn,
    NumColumns
  };
  enum Direction{
    UNDEFINED = 0,
    INCOMING,
    OUTGOING,
  };

 public:
  typedef qMRMLSceneModel Superclass;
  qMRMLIGTLIOModel(QObject *parent=0);
  virtual ~qMRMLIGTLIOModel();

  void setLogic(vtkSlicerOpenIGTLinkIFLogic* logic);

  virtual QStandardItem* insertNode(vtkMRMLNode* node, QStandardItem* parent, int row);
  virtual void updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column);
  virtual void updateNodeFromItem(vtkMRMLNode* node, QStandardItem* item);

 protected:
  qMRMLIGTLIOModel(qMRMLIGTLIOModelPrivate* pimpl, QObject *parent=0);

  virtual void updateIOTreeBranch(vtkMRMLIGTLConnectorNode* node, QStandardItem* item, qMRMLIGTLIOModel::Direction dir);
  virtual QStandardItem* insertIOTree(vtkMRMLNode* node);
  virtual QStandardItem* insertNode(vtkMRMLNode* node);

 protected slots:
  virtual void onDeviceVisibilityModified(vtkObject*);
  virtual void onMRMLSceneNodeAdded(vtkMRMLScene* scene, vtkMRMLNode* node);
  virtual void onMRMLSceneNodeAboutToBeRemoved(vtkMRMLScene* scene, vtkMRMLNode* node);
  virtual void onMRMLSceneNodeRemoved(vtkMRMLScene* scene, vtkMRMLNode* node);


 private:
  Q_DECLARE_PRIVATE(qMRMLIGTLIOModel);
  Q_DISABLE_COPY(qMRMLIGTLIOModel);

  vtkSlicerOpenIGTLinkIFLogic* Logic;



};



#endif //__qMRMLIGTLIOModel_h


