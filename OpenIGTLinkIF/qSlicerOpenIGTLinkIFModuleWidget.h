#ifndef __qSlicerOpenIGTLinkIFModuleWidget_h
#define __qSlicerOpenIGTLinkIFModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerOpenIGTLinkIFModuleExport.h"

class qSlicerOpenIGTLinkIFModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class Q_SLICER_QTMODULES_OPENIGTLINKIF_EXPORT qSlicerOpenIGTLinkIFModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerOpenIGTLinkIFModuleWidget(QWidget *parent=0);
  virtual ~qSlicerOpenIGTLinkIFModuleWidget();

public slots:

protected:
  QScopedPointer<qSlicerOpenIGTLinkIFModuleWidgetPrivate> d_ptr;

  virtual void setup();
  virtual void setMRMLScene(vtkMRMLScene*);

protected slots:
  void onAddConnectorButtonClicked();
  void onRemoveConnectorButtonClicked();

private:
  Q_DECLARE_PRIVATE(qSlicerOpenIGTLinkIFModuleWidget);
  Q_DISABLE_COPY(qSlicerOpenIGTLinkIFModuleWidget);

};

#endif
