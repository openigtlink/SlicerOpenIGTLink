#ifndef __qSlicerOpenIGTLinkIFModule_h
#define __qSlicerOpenIGTLinkIFModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"
#include "qSlicerOpenIGTLinkIFModuleExport.h"
#include "qSlicerApplication.h"

class qSlicerOpenIGTLinkIFModulePrivate;
class vtkObject;

/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class Q_SLICER_QTMODULES_OPENIGTLINKIF_EXPORT qSlicerOpenIGTLinkIFModule :
  public qSlicerLoadableModule
{
  Q_OBJECT;
  QVTK_OBJECT;
#ifdef Slicer_HAVE_QT5
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");
#endif
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerOpenIGTLinkIFModule(QObject *parent=0);
  virtual ~qSlicerOpenIGTLinkIFModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  /// Help to use the module
  virtual QString helpText()const;

  /// Return acknowledgements
  virtual QString acknowledgementText()const;

  /// Return a custom icon for the module
  virtual QIcon icon()const;

  virtual QStringList categories()const;

protected:

  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

public slots:
  virtual void setMRMLScene(vtkMRMLScene*);
  void onNodeAddedEvent(vtkObject*, vtkObject*);
  void onNodeRemovedEvent(vtkObject*, vtkObject*);
  void importDataAndEvents();

protected:
  QScopedPointer<qSlicerOpenIGTLinkIFModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerOpenIGTLinkIFModule);
  Q_DISABLE_COPY(qSlicerOpenIGTLinkIFModule);

};

#endif
