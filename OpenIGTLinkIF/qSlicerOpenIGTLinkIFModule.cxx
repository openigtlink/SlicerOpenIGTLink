/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL:  $
  Date:      $Date:  $
  Version:   $Revision: $

==========================================================================*/

// Qt includes
#include <QTimer>

// Slicer base includes
#include "vtkSlicerVersionConfigure.h" // For Slicer_VERSION_MAJOR,Slicer_VERSION_MINOR
#include <qSlicerCoreApplication.h>
#include <qSlicerCoreIOManager.h>
#include <qSlicerNodeWriter.h>

// OpenIGTLink includes
#include <igtlObjectFactoryBase.h>

// OpenIGTLinkIF includes
#include "qSlicerOpenIGTLinkIFModule.h"
#include "qSlicerOpenIGTLinkIFModuleWidget.h"
#include "qSlicerTextFileReader.h"

// OpenIGTLinkIF Logic includes
#include "vtkSlicerOpenIGTLinkIFLogic.h"

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"


//-----------------------------------------------------------------------------
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  #include <QtPlugin>
  Q_EXPORT_PLUGIN2(qSlicerOpenIGTLinkIFModule, qSlicerOpenIGTLinkIFModule);
#endif
//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class qSlicerOpenIGTLinkIFModulePrivate
{
public:
  qSlicerOpenIGTLinkIFModulePrivate();

  QTimer ImportDataAndEventsTimer;
};

//-----------------------------------------------------------------------------
// qSlicerOpenIGTLinkIFModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerOpenIGTLinkIFModulePrivate::qSlicerOpenIGTLinkIFModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerOpenIGTLinkIFModule methods

//-----------------------------------------------------------------------------
qSlicerOpenIGTLinkIFModule::qSlicerOpenIGTLinkIFModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerOpenIGTLinkIFModulePrivate)
{
  Q_D(qSlicerOpenIGTLinkIFModule);

  connect(&d->ImportDataAndEventsTimer, SIGNAL(timeout()),
          this, SLOT(importDataAndEvents()));

  vtkMRMLScene* scene = qSlicerCoreApplication::application()->mrmlScene();
  if (scene)
  {
    // Need to listen for any new connector nodes being added to start/stop timer
    this->qvtkConnect(scene, vtkMRMLScene::NodeAddedEvent,
                      this, SLOT(onNodeAddedEvent(vtkObject*, vtkObject*)));
    this->qvtkConnect(scene, vtkMRMLScene::NodeRemovedEvent,
                      this, SLOT(onNodeRemovedEvent(vtkObject*, vtkObject*)));
  }
  //d->ImportDataAndEventsTimer.start(5);
}

//-----------------------------------------------------------------------------
qSlicerOpenIGTLinkIFModule::~qSlicerOpenIGTLinkIFModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerOpenIGTLinkIFModule::helpText()const
{
  return "The OpenIGTLink IF module manages communications between 3D Slicer"
         " and other OpenIGTLink-compliant software through the network.";
}

//-----------------------------------------------------------------------------
QString qSlicerOpenIGTLinkIFModule::acknowledgementText()const
{
  return "This module was developed by Junichi Tokuda (Brigham and Women's Hospital), "
         "Jean-Christophe Fillion-Robin (Kitware, Inc.) and OpenIGTLink community."
         "The research was funded by NIH (R01CA111288, P41RR019703, P41EB015898, "
         "P01CA067165, U54EB005149) and NEDO, Japan.";
}

//-----------------------------------------------------------------------------
QIcon qSlicerOpenIGTLinkIFModule::icon()const
{
  return QIcon(":/Icons/OpenIGTLinkIF.png");
}


//-----------------------------------------------------------------------------
QStringList qSlicerOpenIGTLinkIFModule::categories() const
{
  return QStringList() << "IGT";
}


//-----------------------------------------------------------------------------
void qSlicerOpenIGTLinkIFModule::setup()
{
  this->Superclass::setup();

  qSlicerCoreApplication* app = qSlicerCoreApplication::application();

  // If the object factory is initialized simultaneously on multiple threads,
  // there can be a conflict between factory initializations.
  // This calls the ensures that the initialization is called on a single thread
  igtl::ObjectFactoryBase::CreateInstance("");
}

//-----------------------------------------------------------------------------
void qSlicerOpenIGTLinkIFModule::setMRMLScene(vtkMRMLScene* scene)
{
  vtkMRMLScene* oldScene = this->mrmlScene();
  this->Superclass::setMRMLScene(scene);

  if (scene == NULL)
  {
    return;
  }

  // Need to listen for any new connector nodes being added to start/stop timer
  this->qvtkReconnect(oldScene, scene, vtkMRMLScene::NodeAddedEvent,
                      this, SLOT(onNodeAddedEvent(vtkObject*, vtkObject*)));
  this->qvtkReconnect(oldScene, scene, vtkMRMLScene::NodeRemovedEvent,
                      this, SLOT(onNodeRemovedEvent(vtkObject*, vtkObject*)));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerOpenIGTLinkIFModule::createWidgetRepresentation()
{
  return new qSlicerOpenIGTLinkIFModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerOpenIGTLinkIFModule::createLogic()
{
  return vtkSlicerOpenIGTLinkIFLogic::New();
}


// --------------------------------------------------------------------------
void qSlicerOpenIGTLinkIFModule::onNodeAddedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerOpenIGTLinkIFModule);

  vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
  {
    // If the timer is not active
    if (!d->ImportDataAndEventsTimer.isActive())
    {
      d->ImportDataAndEventsTimer.start(5);
    }
  }
}


// --------------------------------------------------------------------------
void qSlicerOpenIGTLinkIFModule::onNodeRemovedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerOpenIGTLinkIFModule);

  vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
  {
    // If the timer is active
    if (d->ImportDataAndEventsTimer.isActive())
    {
      // Check if there is any other connector node left in the Scene
      vtkMRMLScene* scene = qSlicerCoreApplication::application()->mrmlScene();
      if (scene)
      {
        std::vector<vtkMRMLNode*> nodes;
        this->mrmlScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
        if (nodes.size() == 0)
        {
          // The last connector was removed
          d->ImportDataAndEventsTimer.stop();
        }
      }
    }
  }
}


//-----------------------------------------------------------------------------
void qSlicerOpenIGTLinkIFModule::importDataAndEvents()
{
  vtkMRMLAbstractLogic* l = this->logic();
  vtkSlicerOpenIGTLinkIFLogic* igtlLogic = vtkSlicerOpenIGTLinkIFLogic::SafeDownCast(l);
  if (igtlLogic)
  {
    igtlLogic->CallConnectorTimerHander();
  }
}
