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

// PlusRemote includes
#include "qMRMLPlusServerLauncherRemoteWidget.h"
#include "ui_qMRMLPlusServerLauncherRemoteWidget.h"
#include "vtkMRMLPlusServerLauncherNode.h"

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkMRMLAbstractLogic.h>

// MRML includes
#include <vtkMRMLScene.h>

// OpenIGTLinkIF includes
#include <vtkMRMLTextNode.h>

const std::string COLOR_NORMAL = "#000000";
const std::string COLOR_WARNING = "#FF8000";
const std::string COLOR_ERROR = "#D70000";

//-----------------------------------------------------------------------------
class qMRMLPlusServerLauncherRemoteWidgetPrivate : public Ui_qMRMLPlusServerLauncherRemoteWidget
{
  Q_DECLARE_PUBLIC(qMRMLPlusServerLauncherRemoteWidget);

protected:
  qMRMLPlusServerLauncherRemoteWidget* const q_ptr;
public:
  qMRMLPlusServerLauncherRemoteWidgetPrivate(qMRMLPlusServerLauncherRemoteWidget& object);
  ~qMRMLPlusServerLauncherRemoteWidgetPrivate();
  void init();

public:

};

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherRemoteWidgetPrivate::qMRMLPlusServerLauncherRemoteWidgetPrivate(qMRMLPlusServerLauncherRemoteWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherRemoteWidgetPrivate::~qMRMLPlusServerLauncherRemoteWidgetPrivate()
{
  Q_Q(qMRMLPlusServerLauncherRemoteWidget);
}

void qMRMLPlusServerLauncherRemoteWidgetPrivate::init()
{
  Q_Q(qMRMLPlusServerLauncherRemoteWidget);
  this->setupUi(q);

  //this->ConfigFileSelectorComboBox->addAttribute("vtkMRMLTextNode", CONFIG_FILE_NODE_ATTRIBUTE.c_str());
  QObject::connect(this->PlusServerLauncherComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(updateWidgetFromMRML()));
  QObject::connect(this->HostnameLineEdit, SIGNAL(textEdited(const QString&)), q, SLOT(updateMRMLFromWidget()));
  QObject::connect(this->PortLineEdit, SIGNAL(textEdited(const QString&)), q, SLOT(updateMRMLFromWidget()));
}

//-----------------------------------------------------------------------------
// qMRMLPlusServerLauncherRemoteWidget methods

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherRemoteWidget::qMRMLPlusServerLauncherRemoteWidget(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLPlusServerLauncherRemoteWidgetPrivate(*this))
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);
  d->init();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherRemoteWidget::~qMRMLPlusServerLauncherRemoteWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);
  if (newScene == this->mrmlScene())
  {
    return;
  }

  Superclass::setMRMLScene(newScene);

  // Update UI
  this->updateWidgetFromMRML();

  // observe close event so we can re-add a parameters node if necessary
  this->qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent, this, SLOT(onMRMLSceneEndCloseEvent()));
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onMRMLSceneEndCloseEvent()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  vtkMRMLPlusServerLauncherNode* launcherNode = vtkMRMLPlusServerLauncherNode::SafeDownCast(d->PlusServerLauncherComboBox->currentNode());
  this->qvtkReconnect(launcherNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  d->HostnameLineEdit->setEnabled(launcherNode);
  d->PortLineEdit->setEnabled(launcherNode);

  if (!launcherNode)
  {
    return;
  }

  bool wasBlocking = true;

  wasBlocking = d->HostnameLineEdit->blockSignals(true);
  std::string hostname = launcherNode->GetHostname();
  d->HostnameLineEdit->setText(hostname.c_str());
  d->HostnameLineEdit->blockSignals(wasBlocking);

  wasBlocking = d->PortLineEdit->blockSignals(true);
  int port = launcherNode->GetPort();
  d->PortLineEdit->setText(QVariant(port).toString());
  d->PortLineEdit->blockSignals(wasBlocking);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::updateMRMLFromWidget()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  vtkMRMLPlusServerLauncherNode* launcherNode = vtkMRMLPlusServerLauncherNode::SafeDownCast(d->PlusServerLauncherComboBox->currentNode());
  if (!launcherNode)
  {
    return;
  }
  this->qvtkReconnect(nullptr, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
  int wasModifying = launcherNode->StartModify();

  // Hostname
  const QString hostnameString = d->HostnameLineEdit->text();
  launcherNode->SetHostname(hostnameString.toStdString());

  // Port
  const QString portString = d->PortLineEdit->text();
  int port = QVariant(portString).toInt();
  launcherNode->SetPort(port);

  launcherNode->EndModify(wasModifying);
  this->qvtkReconnect(launcherNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
}
