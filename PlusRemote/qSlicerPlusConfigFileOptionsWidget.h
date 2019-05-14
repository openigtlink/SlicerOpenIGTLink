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

#ifndef __qSlicerPlusConfigFileIOOptionsWidget_h
#define __qSlicerPlusConfigFileIOOptionsWidget_h

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerIOOptionsWidget.h"

#include "qSlicerPlusRemoteModuleExport.h"

class qSlicerPlusConfigFileIOOptionsWidgetPrivate;

/// \ingroup Slicer_QtModules_PlusConfigFile
class Q_SLICER_QTMODULES_PlusConfigFile_EXPORT qSlicerPlusConfigFileIOOptionsWidget :
  public qSlicerIOOptionsWidget
{
  Q_OBJECT
public:
  qSlicerPlusConfigFileIOOptionsWidget(QWidget *parent=nullptr);
  ~qSlicerPlusConfigFileIOOptionsWidget() override;

protected slots:
  /// Update IO plugin properties
  void updateProperties();

private:
  Q_DECLARE_PRIVATE_D(qGetPtrHelper(qSlicerIOOptions::d_ptr), qSlicerPlusConfigFileIOOptionsWidget);
  Q_DISABLE_COPY(qSlicerPlusConfigFileIOOptionsWidget);
};

#endif
