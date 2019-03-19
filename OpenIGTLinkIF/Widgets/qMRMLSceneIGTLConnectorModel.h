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

#ifndef __qMRMLSceneIGTLConnectorModel_h
#define __qMRMLSceneIGTLConnectorModel_h

// qMRMLWidgets includes
#include "qMRMLSceneDisplayableModel.h"

// OpenIGTLinkIF GUI includes
#include "qSlicerOpenIGTLinkIFModuleWidgetsExport.h"

// Logic includes
#include "../Logic/vtkSlicerOpenIGTLinkIFLogic.h"

//class qMRMLSceneIGTLConnectorModelPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_OpenIGTLink
class Q_SLICER_MODULE_OPENIGTLINKIF_WIDGETS_EXPORT qMRMLSceneIGTLConnectorModel : public qMRMLSceneDisplayableModel
{
  Q_OBJECT

public:
  qMRMLSceneIGTLConnectorModel(QObject* parent = 0);
  virtual ~qMRMLSceneIGTLConnectorModel();

  void setLogic(vtkSlicerOpenIGTLinkIFLogic* logic);

  enum Columns
  {
    NameColumn = 0,
    TypeColumn,
    StatusColumn,
    HostnameColumn,
    PortColumn
  };

  virtual void updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column);

  /// As we reimplement insertNode, we need don't want to hide the other functions.
  using qMRMLSceneModel::insertNode;
  /// Reimplemented to listen to the vtkMRMLIGTLConnectorNode events for
  /// connector state changes.
  virtual QStandardItem* insertNode(vtkMRMLNode* node, QStandardItem* parent, int row);

protected:

  virtual vtkMRMLNode* parentNode(vtkMRMLNode* node)const;

  virtual void updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item);

  virtual QFlags<Qt::ItemFlag> nodeFlags(vtkMRMLNode* node, int column)const;

private:
  Q_DISABLE_COPY(qMRMLSceneIGTLConnectorModel);

  vtkSlicerOpenIGTLinkIFLogic* Logic;

};

#endif
