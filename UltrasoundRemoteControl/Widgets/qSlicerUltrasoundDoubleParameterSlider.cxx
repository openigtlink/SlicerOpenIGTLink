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

// UltrasoundRemoteControl includes
#include "qSlicerUltrasoundDoubleParameterSlider.h"
#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

#include "vtkMRMLIGTLConnectorNode.h"

// VTK includes
#include <vtkVariant.h>
#include <vtkXMLUtilities.h>

// Qt includes
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

// CTK includes
#include <ctkDoubleSlider.h>
#include <ctkDoubleSpinBox.h>
#include <ctkSliderWidget.h>

//-----------------------------------------------------------------------------
class qSlicerUltrasoundDoubleParameterSliderPrivate : qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerUltrasoundDoubleParameterSlider);

  qSlicerUltrasoundDoubleParameterSliderPrivate(qSlicerUltrasoundDoubleParameterSlider& p);
  ~qSlicerUltrasoundDoubleParameterSliderPrivate();

public:
  virtual void init();
  virtual void setupUi(QWidget* qSlicerUltrasoundDoubleParameterSlider);

protected:
  qSlicerUltrasoundDoubleParameterSlider* const q_ptr;

public:
  QHBoxLayout* horizontalLayout;
  ctkSliderWidget* valueSlider;
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSliderPrivate::qSlicerUltrasoundDoubleParameterSliderPrivate(qSlicerUltrasoundDoubleParameterSlider& object)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&object)
  , q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSliderPrivate::~qSlicerUltrasoundDoubleParameterSliderPrivate()
{
  Q_Q(qSlicerUltrasoundDoubleParameterSlider);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSliderPrivate::init()
{
  Q_Q(qSlicerUltrasoundDoubleParameterSlider);
  qSlicerAbstractUltrasoundParameterWidgetPrivate::init();

  this->setupUi(q);
  QObject::connect(this->valueSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));

  QObject::connect(this->valueSlider->slider(), SIGNAL(sliderPressed()), q, SLOT(interactionInProgressOn()));
  QObject::connect(this->valueSlider->slider(), SIGNAL(sliderReleased()), q, SLOT(interactionInProgressOff()));
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSliderPrivate::setupUi(QWidget* qSlicerUltrasoundDoubleParameterSlider)
{
  if (qSlicerUltrasoundDoubleParameterSlider->objectName().isEmpty())
  {
    qSlicerUltrasoundDoubleParameterSlider->setObjectName(QStringLiteral("qSlicerUltrasoundDoubleParameterSlider"));
  }

  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(qSlicerUltrasoundDoubleParameterSlider->sizePolicy().hasHeightForWidth());
  qSlicerUltrasoundDoubleParameterSlider->setSizePolicy(sizePolicy);
  qSlicerUltrasoundDoubleParameterSlider->setMinimumSize(QSize(10, 10));
  horizontalLayout = new QHBoxLayout(qSlicerUltrasoundDoubleParameterSlider);
  horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
  horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
  qSlicerUltrasoundDoubleParameterSlider->setLayout(horizontalLayout);

  this->valueSlider = new ctkSliderWidget(qSlicerUltrasoundDoubleParameterSlider);
  this->valueSlider->setObjectName(QStringLiteral("doubleParameterSlider"));
  this->valueSlider->setTracking(false);
  horizontalLayout->addWidget(this->valueSlider);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundDoubleParameterSlider methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSlider::qSlicerUltrasoundDoubleParameterSlider(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(new qSlicerUltrasoundDoubleParameterSliderPrivate(*this))
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->init();

  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSlider::~qSlicerUltrasoundDoubleParameterSlider()
{
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::onConnected()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->valueSlider->setEnabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::onDisconnected()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->valueSlider->setDisabled(true);
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundDoubleParameterSlider::getParameterValue()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  return vtkVariant(d->valueSlider->value()).ToString();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setParameterValue(std::string expectedParameterString)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);

  bool wasBlocking = d->valueSlider->blockSignals(true);
  bool valid = false;
  double value = vtkVariant(expectedParameterString).ToDouble(&valid);
  if (valid)
  {
    d->valueSlider->setValue(value);
  }
  d->valueSlider->blockSignals(wasBlocking);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDoubleParameterSlider::minimum() const
{
  Q_D(const qSlicerUltrasoundDoubleParameterSlider);
  return d->valueSlider->minimum();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setMinimum(double minimum)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->valueSlider->setMinimum(minimum);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDoubleParameterSlider::maximum() const
{
  Q_D(const qSlicerUltrasoundDoubleParameterSlider);
  return d->valueSlider->maximum();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setMaximum(double maximum)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->valueSlider->setMaximum(maximum);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDoubleParameterSlider::singleStep() const
{
  Q_D(const qSlicerUltrasoundDoubleParameterSlider);
  return d->valueSlider->singleStep();
}


//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setSingleStep(double singleStep)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->valueSlider->setSingleStep(singleStep);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDoubleParameterSlider::pageStep() const
{
  Q_D(const qSlicerUltrasoundDoubleParameterSlider);
  return d->valueSlider->pageStep();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setPageStep(double pageStep)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->valueSlider->setPageStep(pageStep);
}

//-----------------------------------------------------------------------------
QString qSlicerUltrasoundDoubleParameterSlider::suffix() const
{
  Q_D(const qSlicerUltrasoundDoubleParameterSlider);
  return d->valueSlider->suffix();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setSuffix(const QString& suffix)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->valueSlider->setSuffix(suffix);
}
