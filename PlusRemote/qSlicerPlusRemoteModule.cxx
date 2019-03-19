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

// Qt includes
#include <QDir>
#include <QPointer>
#include <QSound>
#include <QTimer>
#include <QtPlugin>

// Slicer includes
#include "qSlicerCoreApplication.h"
#include "qSlicerApplication.h"
#include <vtkSlicerVersionConfigure.h> // For Slicer_VERSION_MAJOR, Slicer_VERSION_MINOR

#include <qSlicerPlusRemoteModuleWidget.h>
#include <qSlicerPlusRemoteModule.h>
#include <vtkSlicerPlusRemoteLogic.h>

#include <vtkMRMLScene.h>

// Plus remote includes
#include <vtkMRMLPlusRemoteNode.h>

static const double UPDATE_PLUS_REMOTE_NODES_PERIOD_SEC = 0.2;

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  #include <QtPlugin>
  Q_EXPORT_PLUGIN2(qSlicerPlusRemoteModule, qSlicerPlusRemoteModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ToolPlusRemote
class qSlicerPlusRemoteModulePrivate
{
public:
  qSlicerPlusRemoteModulePrivate();
  ~qSlicerPlusRemoteModulePrivate();

  QTimer UpdateAllPlusRemoteNodesTimer;
};

//-----------------------------------------------------------------------------
// qSlicerPlusRemoteModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPlusRemoteModulePrivate::qSlicerPlusRemoteModulePrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerPlusRemoteModulePrivate::~qSlicerPlusRemoteModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlusRemoteModule methods

//-----------------------------------------------------------------------------
qSlicerPlusRemoteModule::qSlicerPlusRemoteModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlusRemoteModulePrivate)
{
  Q_D(qSlicerPlusRemoteModule);

  vtkMRMLScene* scene = qSlicerCoreApplication::application()->mrmlScene();
  if (scene)
  {
    connect(&d->UpdateAllPlusRemoteNodesTimer, SIGNAL(timeout()), this, SLOT(updateAllPlusRemoteNodes()));

    // Need to listen for any new plus remote nodes being added to start/stop timer
    this->qvtkConnect(scene, vtkMRMLScene::NodeAddedEvent, this, SLOT(onNodeAddedEvent(vtkObject*, vtkObject*)));
    this->qvtkConnect(scene, vtkMRMLScene::NodeRemovedEvent, this, SLOT(onNodeRemovedEvent(vtkObject*, vtkObject*)));
  }
}

//-----------------------------------------------------------------------------
qSlicerPlusRemoteModule::~qSlicerPlusRemoteModule()
{
  Q_D(qSlicerPlusRemoteModule);
}

//-----------------------------------------------------------------------------
QString qSlicerPlusRemoteModule::title() const
{
  return "Plus Remote";
}

//-----------------------------------------------------------------------------
QString qSlicerPlusRemoteModule::helpText() const
{
  return "This is a convenience module for sending commands a <a href=\"www.plustoolkit.org\">PLUS server</a> for recording data and reconstruction of 3D volumes from tracked 2D image slices.";
}

//-----------------------------------------------------------------------------
QString qSlicerPlusRemoteModule::acknowledgementText() const
{
  return "This work was funded by Cancer Care Ontario Applied Cancer Research Unit (ACRU) and the Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO) grants.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlusRemoteModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Amelie Meyer (PerkLab, Queen's University)");
  moduleContributors << QString("Franklin King (PerkLab, Queen's University)");
  moduleContributors << QString("Kyle Sunderland (PerkLab, Queen's University)");
  moduleContributors << QString("Tamas Ungi (PerkLab, Queen's University)");
  moduleContributors << QString("Andras Lasso (PerkLab, Queen's University)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerPlusRemoteModule::icon() const
{
  return QIcon(":/Icons/PlusRemote.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlusRemoteModule::categories() const
{
  return QStringList() << "IGT";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlusRemoteModule::dependencies() const
{
  return QStringList() << "OpenIGTLinkRemote" << "VolumeRendering";
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModule::setup()
{
  Q_D(qSlicerPlusRemoteModule);
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
void qSlicerPlusRemoteModule::setMRMLScene(vtkMRMLScene* _mrmlScene)
{
  this->Superclass::setMRMLScene(_mrmlScene);
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPlusRemoteModule::createWidgetRepresentation()
{
  Q_D(qSlicerPlusRemoteModule);
  qSlicerPlusRemoteModuleWidget* plusRemoteWidget = new qSlicerPlusRemoteModuleWidget;
  return plusRemoteWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlusRemoteModule::createLogic()
{
  return vtkSlicerPlusRemoteLogic::New();
}

// --------------------------------------------------------------------------
void qSlicerPlusRemoteModule::onNodeAddedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerPlusRemoteModule);

  vtkMRMLPlusRemoteNode* plusRemoteNode = vtkMRMLPlusRemoteNode::SafeDownCast(node);
  if (plusRemoteNode)
  {
    // If the timer is not active
    if (!d->UpdateAllPlusRemoteNodesTimer.isActive())
    {
      d->UpdateAllPlusRemoteNodesTimer.start(UPDATE_PLUS_REMOTE_NODES_PERIOD_SEC * 1000.0);
    }
  }
}

// --------------------------------------------------------------------------
void qSlicerPlusRemoteModule::onNodeRemovedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerPlusRemoteModule);

  vtkMRMLPlusRemoteNode* plusRemoteNode = vtkMRMLPlusRemoteNode::SafeDownCast(node);
  if (plusRemoteNode)
  {
    // If the timer is active
    if (d->UpdateAllPlusRemoteNodesTimer.isActive())
    {
      // Check if there is any other plus remote node left in the Scene
      vtkMRMLScene* scene = qSlicerCoreApplication::application()->mrmlScene();
      if (scene)
      {
        std::vector<vtkMRMLNode*> nodes;
        this->mrmlScene()->GetNodesByClass("vtkMRMLPlusRemoteNode", nodes);
        if (nodes.size() == 0)
        {
          // The last plus remote node was removed
          d->UpdateAllPlusRemoteNodesTimer.stop();
        }
      }
    }
  }
}

// --------------------------------------------------------------------------
void qSlicerPlusRemoteModule::updateAllPlusRemoteNodes()
{
  Q_D(qSlicerPlusRemoteModule);
  vtkSlicerPlusRemoteLogic* plusRemoteLogic = vtkSlicerPlusRemoteLogic::SafeDownCast(this->Superclass::logic());
  if (!plusRemoteLogic)
  {
    return;
  }
  plusRemoteLogic->UpdateAllPlusRemoteNodes();
}