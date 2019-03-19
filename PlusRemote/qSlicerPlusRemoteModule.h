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

#ifndef __qSlicerPlusRemoteModule_h
#define __qSlicerPlusRemoteModule_h

// CTK includes
#include <ctkVTKObject.h>

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerPlusRemoteModuleExport.h"

class qSlicerPlusRemoteModulePrivate;

/// \ingroup Slicer_QtModules_PlusRemote
class Q_SLICER_QTMODULES_PLUSREMOTE_EXPORT qSlicerPlusRemoteModule
  : public qSlicerLoadableModule
{
  Q_OBJECT;
  QVTK_OBJECT;
#ifdef Slicer_HAVE_QT5
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");
#endif
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerPlusRemoteModule(QObject* parent = 0);
  virtual ~qSlicerPlusRemoteModule();

  virtual QString title()const;
  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;

  virtual QIcon icon()const;

  virtual QStringList categories()const;
  virtual QStringList dependencies() const;

protected:

  /// Initialize the module.
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation* createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

public slots:
  virtual void setMRMLScene(vtkMRMLScene*);
  void onNodeAddedEvent(vtkObject*, vtkObject*);
  void onNodeRemovedEvent(vtkObject*, vtkObject*);

  void updateAllPlusRemoteNodes();

protected:
  QScopedPointer<qSlicerPlusRemoteModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlusRemoteModule);
  Q_DISABLE_COPY(qSlicerPlusRemoteModule);

};

#endif
