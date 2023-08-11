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

#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

// OpenIGTLinkIO includes
#include <igtlioCommand.h>

// VTK includes
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidgetPrivate::qSlicerAbstractUltrasoundParameterWidgetPrivate(qSlicerAbstractUltrasoundParameterWidget* q)
  : q_ptr(q)
  , CmdSetParameter(igtlioCommandPointer::New())
  , CmdGetParameter(igtlioCommandPointer::New())
{
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidgetPrivate::init()
{
  Q_Q(qSlicerAbstractUltrasoundParameterWidget);
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::qSlicerAbstractUltrasoundParameterWidget(qSlicerAbstractUltrasoundParameterWidgetPrivate* d)
  : d_ptr(d)
{
  connect(&d->PeriodicParameterTimer, SIGNAL(timeout()), this, SLOT(getUltrasoundParameter()));
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::~qSlicerAbstractUltrasoundParameterWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setInteractionInProgress(bool interactionStatus)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  d->InteractionInProgress = interactionStatus;
}

//-----------------------------------------------------------------------------
bool qSlicerAbstractUltrasoundParameterWidget::getInteractionInProgress()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->InteractionInProgress;
}

//-----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* qSlicerAbstractUltrasoundParameterWidget::getConnectorNode()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->ConnectorNode.GetPointer();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  if (d->ConnectorNode == node)
    {
    return;
    }

  this->qvtkReconnect(d->ConnectorNode, node, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(onConnectionChanged()));
  this->qvtkReconnect(d->ConnectorNode, node, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(onConnectionChanged()));

  d->ConnectorNode = node;
  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::onConnectionChanged()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  if (d->ConnectorNode && d->ConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::StateConnected)
  {
    this->onConnected();
    if (d->PushOnConnect)
    {
      this->setUltrasoundParameter();
    }
    else
    {
      this->getUltrasoundParameter();
    }
    d->PeriodicParameterTimer.start(2000);
  }
  else
  {
    d->PeriodicParameterTimer.stop();
    this->onDisconnected();
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setDeviceID(const char* deviceID)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  d->DeviceID = deviceID ? deviceID : "";
}

//-----------------------------------------------------------------------------
const char* qSlicerAbstractUltrasoundParameterWidget::deviceID()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->DeviceID.c_str();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setUltrasoundParameter()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);

  if (!d->ConnectorNode || d->ConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateConnected)
  {
    return;
  }
  d->InteractionInProgress = true;

  std::string value = this->getParameterValue();
  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("Command");
  rootElement->SetAttribute("Name", "SetUsParameter");
  rootElement->SetAttribute("UsDeviceId", d->DeviceID.c_str());
  vtkNew<vtkXMLDataElement> nestedElement;
  nestedElement->SetName("Parameter");
  nestedElement->SetAttribute("Name", d->ParameterName.c_str());
  nestedElement->SetAttribute("Value", value.c_str());
  rootElement->AddNestedElement(nestedElement);

  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(rootElement, ss);

  igtlioCommandPointer oldSetParameterCommand = d->CmdSetParameter;
  d->CmdSetParameter = igtlioCommandPointer::New();
  d->CmdSetParameter->SetName("SetUsParameter");
  d->CmdSetParameter->SetTimeoutSec(5.0);
  d->CmdSetParameter->BlockingOff();
  d->CmdSetParameter->SetCommandContent(ss.str());
  d->CmdSetParameter->SetResponseContent("");
  d->CmdSetParameter->ClearResponseMetaData();

  this->qvtkReconnect(oldSetParameterCommand, d->CmdSetParameter, igtlioCommand::CommandCompletedEvent, this, SLOT(onSetUltrasoundParameterCompleted()));
  d->ConnectorNode->SendCommand(d->CmdSetParameter);

}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::getUltrasoundParameter()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);

  if (!d->ConnectorNode || d->ConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateConnected)
  {
    return;
  }

  if (d->CmdGetParameter->IsInProgress())
  {
    // Get command is already in progress
    return;
  }

  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("Command");
  rootElement->SetAttribute("Name", "GetUsParameter");
  rootElement->SetAttribute("UsDeviceId", d->DeviceID.c_str());
  vtkNew<vtkXMLDataElement> nestedElement;
  nestedElement->SetName("Parameter");
  nestedElement->SetAttribute("Name", d->ParameterName.c_str());
  rootElement->AddNestedElement(nestedElement);

  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(rootElement, ss);

  igtlioCommandPointer oldGetParameterCommand = d->CmdGetParameter;
  d->CmdGetParameter = igtlioCommandPointer::New();
  d->CmdGetParameter->SetName("GetUsParameter");
  d->CmdSetParameter->SetTimeoutSec(5.0);
  d->CmdGetParameter->BlockingOff();
  d->CmdGetParameter->SetCommandContent(ss.str());
  d->CmdGetParameter->SetResponseContent("");
  d->CmdGetParameter->ClearResponseMetaData();

  this->qvtkReconnect(oldGetParameterCommand, d->CmdGetParameter, igtlioCommand::CommandCompletedEvent, this, SLOT(onGetUltrasoundParameterCompleted()));
  d->ConnectorNode->SendCommand(d->CmdGetParameter);

}

//-----------------------------------------------------------------------------
const char* qSlicerAbstractUltrasoundParameterWidget::parameterName()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->ParameterName.c_str();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setParameterName(const char* parameterName)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  d->ParameterName = parameterName ? parameterName : "";
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::onSetUltrasoundParameterCompleted()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  d->InteractionInProgress = false;
  this->setParameterCompleted();
  this->getUltrasoundParameter();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::onGetUltrasoundParameterCompleted()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);

  if (!d->CmdGetParameter->GetSuccessful())
  {
    return;
  }

  if (d->InteractionInProgress || d->CmdSetParameter->IsInProgress())
  {
    // User is currently interacting with the widget or
    // SetUSParameterCommand is currently in progress
    // Don't update the current value
    return;
  }

  std::string value = "";
  IANA_ENCODING_TYPE encoding;
  if (d->CmdGetParameter->GetResponseMetaDataElement(d->ParameterName, value, encoding))
  {
    if (!value.empty())
    {
      this->setParameterValue(value);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setPushOnConnect(bool pushOnConnect)
{
Q_D(qSlicerAbstractUltrasoundParameterWidget);
  d->PushOnConnect = pushOnConnect;
}

//-----------------------------------------------------------------------------
bool qSlicerAbstractUltrasoundParameterWidget::pushOnConnect()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->PushOnConnect;
}
