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
#include "qSlicerUltrasoundTGCParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

#include "vtkMRMLIGTLConnectorNode.h"

// VTK includes
#include <vtkVariant.h>
#include <vtkXMLUtilities.h>

// Qt includes
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

// CTK includes
#include <ctkDoubleSlider.h>
#include <ctkDoubleSpinBox.h>
#include <ctkSliderWidget.h>

// STL includes
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <sstream>

//-----------------------------------------------------------------------------
class qSlicerUltrasoundTGCParameterWidgetPrivate : qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerUltrasoundTGCParameterWidget);

  qSlicerUltrasoundTGCParameterWidgetPrivate(qSlicerUltrasoundTGCParameterWidget& p);
  ~qSlicerUltrasoundTGCParameterWidgetPrivate();

public:
  virtual void init();
  virtual void setupUi(QWidget* qSlicerUltrasoundTGCParameterWidget);

protected:
  qSlicerUltrasoundTGCParameterWidget* const q_ptr;

public:
  QGridLayout* gridLayout;
  QLabel* topLabel;
  QLabel* midLabel;
  QLabel* bottomLabel;
  ctkSliderWidget* topGainSlider;
  ctkSliderWidget* midGainSlider;
  ctkSliderWidget* bottomGainSlider;
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundTGCParameterWidgetPrivate::qSlicerUltrasoundTGCParameterWidgetPrivate(qSlicerUltrasoundTGCParameterWidget& object)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&object)
  , q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundTGCParameterWidgetPrivate::~qSlicerUltrasoundTGCParameterWidgetPrivate()
{
  Q_Q(qSlicerUltrasoundTGCParameterWidget);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidgetPrivate::init()
{
  Q_Q(qSlicerUltrasoundTGCParameterWidget);
  qSlicerAbstractUltrasoundParameterWidgetPrivate::init();

  this->setupUi(q);

  // connections
  QObject::connect(this->topGainSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
  QObject::connect(this->topGainSlider->slider(), SIGNAL(sliderPressed()), q, SLOT(interactionInProgressOn()));
  QObject::connect(this->topGainSlider->slider(), SIGNAL(sliderReleased()), q, SLOT(interactionInProgressOff()));

  QObject::connect(this->midGainSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
  QObject::connect(this->midGainSlider->slider(), SIGNAL(sliderPressed()), q, SLOT(interactionInProgressOn()));
  QObject::connect(this->midGainSlider->slider(), SIGNAL(sliderReleased()), q, SLOT(interactionInProgressOff()));

  QObject::connect(this->bottomGainSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
  QObject::connect(this->bottomGainSlider->slider(), SIGNAL(sliderPressed()), q, SLOT(interactionInProgressOn()));
  QObject::connect(this->bottomGainSlider->slider(), SIGNAL(sliderReleased()), q, SLOT(interactionInProgressOff()));
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidgetPrivate::setupUi(QWidget* qSlicerUltrasoundTGCParameterWidget)
{
  if (qSlicerUltrasoundTGCParameterWidget->objectName().isEmpty())
  {
    qSlicerUltrasoundTGCParameterWidget->setObjectName(QStringLiteral("qSlicerUltrasoundTGCParameterWidget"));
  }

  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(qSlicerUltrasoundTGCParameterWidget->sizePolicy().hasHeightForWidth());
  qSlicerUltrasoundTGCParameterWidget->setSizePolicy(sizePolicy);
  qSlicerUltrasoundTGCParameterWidget->setMinimumSize(QSize(10, 10));

  // create layout
  gridLayout = new QGridLayout(qSlicerUltrasoundTGCParameterWidget);
  gridLayout->setObjectName(QStringLiteral("tgcGridLayout"));
  gridLayout->setSizeConstraint(QLayout::SetMinimumSize);
  qSlicerUltrasoundTGCParameterWidget->setLayout(gridLayout);

  // add top, mid and bottom gain sliders
  this->topLabel = new QLabel("Top Gain:");
  this->topLabel->setObjectName(QStringLiteral("tgcTopLabel"));
  gridLayout->addWidget(this->topLabel, 0, 0);
  this->topGainSlider = new ctkSliderWidget(qSlicerUltrasoundTGCParameterWidget);
  this->topGainSlider->setObjectName(QStringLiteral("tgcTopSlider"));
  this->topGainSlider->setTracking(false);
  gridLayout->addWidget(this->topGainSlider, 0, 1);

  this->midLabel = new QLabel("Mid Gain:");
  this->midLabel->setObjectName(QStringLiteral("tgcMidLabel"));
  gridLayout->addWidget(this->midLabel, 1, 0);
  this->midGainSlider = new ctkSliderWidget(qSlicerUltrasoundTGCParameterWidget);
  this->midGainSlider->setObjectName(QStringLiteral("tgcMidSlider"));
  this->midGainSlider->setTracking(false);
  gridLayout->addWidget(this->midGainSlider, 1, 1);

  this->bottomLabel = new QLabel("Bottom Gain:");
  this->bottomLabel->setObjectName(QStringLiteral("tgcBottomLabel"));
  gridLayout->addWidget(this->bottomLabel, 2, 0);
  this->bottomGainSlider = new ctkSliderWidget(qSlicerUltrasoundTGCParameterWidget);
  this->bottomGainSlider->setObjectName(QStringLiteral("tgcBottomSlider"));
  this->bottomGainSlider->setTracking(false);
  gridLayout->addWidget(this->bottomGainSlider, 2, 1);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundTGCParameterWidget methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundTGCParameterWidget::qSlicerUltrasoundTGCParameterWidget(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(new qSlicerUltrasoundTGCParameterWidgetPrivate(*this))
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->init();

  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundTGCParameterWidget::~qSlicerUltrasoundTGCParameterWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::onConnected()
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->topGainSlider->setEnabled(true);
  d->midGainSlider->setEnabled(true);
  d->bottomGainSlider->setEnabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::onDisconnected()
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->topGainSlider->setDisabled(true);
  d->midGainSlider->setDisabled(true);
  d->bottomGainSlider->setDisabled(true);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundTGCParameterWidget::getTopGain()
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  return d->topGainSlider->value();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setTopGain(double gain)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  bool topBlocking = d->topGainSlider->blockSignals(true);
  d->topGainSlider->setValue(gain);
  d->topGainSlider->blockSignals(topBlocking);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundTGCParameterWidget::getMidGain()
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  return d->midGainSlider->value();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setMidGain(double gain)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  bool midBlocking = d->midGainSlider->blockSignals(true);
  d->midGainSlider->setValue(gain);
  d->midGainSlider->blockSignals(midBlocking);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundTGCParameterWidget::getBottomGain()
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  return d->bottomGainSlider->value();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setBottomGain(double gain)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  bool bottomBlocking = d->bottomGainSlider->blockSignals(true);
  d->bottomGainSlider->setValue(gain);
  d->bottomGainSlider->blockSignals(bottomBlocking);
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundTGCParameterWidget::getParameterValue()
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  std::string topVal = vtkVariant(d->topGainSlider->value()).ToString();
  std::string midVal = vtkVariant(d->midGainSlider->value()).ToString();
  std::string bottomVal = vtkVariant(d->bottomGainSlider->value()).ToString();
  std::string val = topVal + " " + midVal + " " + bottomVal;
  return val;
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setParameterValue(std::string expectedParameterString)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  std::stringstream ss;
  ss.str(expectedParameterString);
  std::vector<double> gains((std::istream_iterator<double>(ss)), std::istream_iterator<double>());
  if (gains.size() != 3)
  {
    return;
  }

  this->setTopGain(gains[0]);
  this->setMidGain(gains[1]);
  this->setBottomGain(gains[2]);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundTGCParameterWidget::minimum() const
{
  Q_D(const qSlicerUltrasoundTGCParameterWidget);

  double topMin = d->topGainSlider->minimum();
  double midMin = d->midGainSlider->minimum();
  double bottomMin = d->bottomGainSlider->minimum();

  return std::min({ topMin, midMin, bottomMin });
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setMinimum(double minimum)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->topGainSlider->setMinimum(minimum);
  d->midGainSlider->setMinimum(minimum);
  d->bottomGainSlider->setMinimum(minimum);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundTGCParameterWidget::maximum() const
{
  Q_D(const qSlicerUltrasoundTGCParameterWidget);

  double topMax = d->topGainSlider->maximum();
  double midMax = d->midGainSlider->maximum();
  double bottomMax = d->bottomGainSlider->maximum();

  return std::max({ topMax, midMax, bottomMax });
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setMaximum(double maximum)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->topGainSlider->setMaximum(maximum);
  d->midGainSlider->setMaximum(maximum);
  d->bottomGainSlider->setMaximum(maximum);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundTGCParameterWidget::singleStep() const
{
  Q_D(const qSlicerUltrasoundTGCParameterWidget);
  // must always be the same val for all three sliders
  return d->topGainSlider->singleStep();
}


//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setSingleStep(double singleStep)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->topGainSlider->setSingleStep(singleStep);
  d->midGainSlider->setSingleStep(singleStep);
  d->bottomGainSlider->setSingleStep(singleStep);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundTGCParameterWidget::pageStep() const
{
  Q_D(const qSlicerUltrasoundTGCParameterWidget);
  // must always be the same val for all three sliders
  return d->topGainSlider->pageStep();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setPageStep(double pageStep)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->topGainSlider->setPageStep(pageStep);
  d->midGainSlider->setPageStep(pageStep);
  d->bottomGainSlider->setPageStep(pageStep);
}

//-----------------------------------------------------------------------------
QString qSlicerUltrasoundTGCParameterWidget::suffix() const
{
  Q_D(const qSlicerUltrasoundTGCParameterWidget);
  // must always be the same val for all three sliders
  return d->topGainSlider->suffix();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundTGCParameterWidget::setSuffix(const QString& suffix)
{
  Q_D(qSlicerUltrasoundTGCParameterWidget);
  d->topGainSlider->setSuffix(suffix);
  d->midGainSlider->setSuffix(suffix);
  d->bottomGainSlider->setSuffix(suffix);
}
