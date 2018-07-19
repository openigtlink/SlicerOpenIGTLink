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

#ifndef __qSlicerUltrasoundDoubleParameterSlider_h
#define __qSlicerUltrasoundDoubleParameterSlider_h

// UltrasoundRemoteControl includes
#include "qSlicerUltrasoundRemoteControlModuleWidgetsExport.h"

#include "qSlicerAbstractUltrasoundParameterWidget.h"

class qSlicerUltrasoundDoubleParameterSliderPrivate;

/// \ingroup Slicer_QtModules_UltrasoundRemoteControl
class Q_SLICER_MODULE_ULTRASOUNDREMOTECONTROL_WIDGETS_EXPORT qSlicerUltrasoundDoubleParameterSlider : public qSlicerAbstractUltrasoundParameterWidget
{
  Q_OBJECT
  QVTK_OBJECT

  Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
  Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
  Q_PROPERTY(double pageStep READ pageStep WRITE setPageStep)
  Q_PROPERTY(QString suffix READ suffix WRITE setSuffix)

public:
  typedef qSlicerAbstractUltrasoundParameterWidget Superclass;
  explicit qSlicerUltrasoundDoubleParameterSlider(QWidget *parent = 0);
  virtual ~qSlicerUltrasoundDoubleParameterSlider();

public slots:

  virtual double minimum() const;
  virtual void setMinimum(double minimum);

  virtual double maximum() const;
  virtual void setMaximum(double maximum);

  virtual double singleStep() const;
  virtual void setSingleStep(double singleStep);

  virtual double pageStep() const;
  virtual void setPageStep(double pageStep);

  virtual QString suffix() const;
  virtual void setSuffix(const QString &suffix);

  virtual void onConnected();
  virtual void onDisconnected();

protected:

  virtual std::string getParameterValue();
  virtual void setParameterValue(std::string);

private:
  Q_DECLARE_PRIVATE(qSlicerUltrasoundDoubleParameterSlider);
  Q_DISABLE_COPY(qSlicerUltrasoundDoubleParameterSlider);
};

#endif
