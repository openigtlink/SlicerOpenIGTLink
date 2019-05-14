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

/// Qt includes
#include <QFileInfo>
#include <QSettings>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerIOOptions_p.h"

// CTK includes
#include <ctkFlowLayout.h>
#include <ctkUtils.h>

// VTK includes
#include <vtkNew.h>

/// PlusRemote includes
#include "qSlicerPlusConfigFileIOOptionsWidget.h"
#include "ui_qSlicerPlusConfigFileIOOptionsWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Volumes
class qSlicerPlusConfigFileIOOptionsWidgetPrivate
  : public qSlicerIOOptionsPrivate
  , public Ui_qSlicerPlusConfigFileIOOptionsWidget
{
public:
};

//-----------------------------------------------------------------------------
qSlicerPlusConfigFileIOOptionsWidget::qSlicerPlusConfigFileIOOptionsWidget(QWidget* parentWidget)
  : qSlicerIOOptionsWidget(new qSlicerPlusConfigFileIOOptionsWidgetPrivate, parentWidget)
{
  Q_D(qSlicerPlusConfigFileIOOptionsWidget);
  d->setupUi(this);

  ctkFlowLayout::replaceLayout(this);

  //connect(d->AutoOpacitiesCheckBox, SIGNAL(toggled(bool)),
  //        this, SLOT(updateProperties()));
}

//-----------------------------------------------------------------------------
qSlicerPlusConfigFileIOOptionsWidget::~qSlicerPlusConfigFileIOOptionsWidget()
= default;

//-----------------------------------------------------------------------------
void qSlicerPlusConfigFileIOOptionsWidget::updateProperties()
{
  Q_D(qSlicerPlusConfigFileIOOptionsWidget);

  /*d->Properties["autoOpacities"] = d->AutoOpacitiesCheckBox->isChecked();*/
}
