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

#ifndef __qSlicerPlusRemoteModuleWidgetsAbstractPlugin_h
#define __qSlicerPlusRemoteModuleWidgetsAbstractPlugin_h

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QDesignerCustomWidgetInterface>
#else
#include <QtUiPlugin/QDesignerCustomWidgetInterface>
#endif

// PlusRemote includes
#include "qSlicerPlusRemoteModuleWidgetsPluginsExport.h"

class Q_SLICER_MODULE_PLUSREMOTE_WIDGETS_PLUGINS_EXPORT qSlicerPlusRemoteModuleWidgetsAbstractPlugin
  : public QDesignerCustomWidgetInterface
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetInterface")
#endif
  Q_INTERFACES(QDesignerCustomWidgetInterface);
public:

  qSlicerPlusRemoteModuleWidgetsAbstractPlugin();
  // Don't re implement this method.
  QString group() const;
  // You can re implement these methods
  virtual QIcon icon() const;
  virtual QString toolTip() const;
  virtual QString whatsThis() const;

};

#endif
