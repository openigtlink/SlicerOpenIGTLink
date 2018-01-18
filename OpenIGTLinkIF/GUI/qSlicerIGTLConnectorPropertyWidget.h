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

#ifndef __qSlicerIGTLConnectorPropertyWidget_h
#define __qSlicerIGTLConnectorPropertyWidget_h

// Qt includes
#include <QWidget>

// CTK includes
#include <ctkVTKObject.h>

// OpenIGTLinkIF GUI includes
#include "qSlicerOpenIGTLinkIFModuleExport.h"

class qSlicerIGTLConnectorPropertyWidgetPrivate;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLNode;
class vtkObject;

/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class Q_SLICER_QTMODULES_OPENIGTLINKIF_EXPORT qSlicerIGTLConnectorPropertyWidget : public QWidget
{
  Q_OBJECT
  QVTK_OBJECT
public:
  typedef QWidget Superclass;
  qSlicerIGTLConnectorPropertyWidget(QWidget *parent = 0);
  virtual ~qSlicerIGTLConnectorPropertyWidget();

public slots:
  /// Set the MRML node of interest
  void setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode * connectorNode);

  /// Utility function that calls setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode*)
  /// It's useful to connect to vtkMRMLNode* signals when you are sure of
  /// the type
  void setMRMLIGTLConnectorNode(vtkMRMLNode* node);

protected slots:
  /// Internal function to update the widgets based on the IGTLConnector node
  void onMRMLNodeModified();

  void startCurrentIGTLConnector(bool enabled);

  /// Internal function to update the IGTLConnector node based on the property widget
  void updateIGTLConnectorNode();

protected:
  QScopedPointer<qSlicerIGTLConnectorPropertyWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerIGTLConnectorPropertyWidget);
  Q_DISABLE_COPY(qSlicerIGTLConnectorPropertyWidget);
};

#endif

