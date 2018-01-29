
// Qt includes
#include <QButtonGroup>

// OpenIGTLinkIF GUI includes
#include "qSlicerIGTLConnectorPropertyWidget.h"
#include "ui_qSlicerIGTLConnectorPropertyWidget.h"

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class qSlicerIGTLConnectorPropertyWidgetPrivate : public Ui_qSlicerIGTLConnectorPropertyWidget
{
  Q_DECLARE_PUBLIC(qSlicerIGTLConnectorPropertyWidget);
protected:
  qSlicerIGTLConnectorPropertyWidget* const q_ptr;
public:
  qSlicerIGTLConnectorPropertyWidgetPrivate(qSlicerIGTLConnectorPropertyWidget& object);
  void init();

  vtkMRMLIGTLConnectorNode * IGTLConnectorNode;

  QButtonGroup ConnectorTypeButtonGroup;
};

//------------------------------------------------------------------------------
qSlicerIGTLConnectorPropertyWidgetPrivate::qSlicerIGTLConnectorPropertyWidgetPrivate(qSlicerIGTLConnectorPropertyWidget& object)
  : q_ptr(&object)
{
  this->IGTLConnectorNode = 0;
}

//------------------------------------------------------------------------------
void qSlicerIGTLConnectorPropertyWidgetPrivate::init()
{
  Q_Q(qSlicerIGTLConnectorPropertyWidget);
  this->setupUi(q);
  QObject::connect(this->ConnectorNameEdit, SIGNAL(editingFinished()),
                   q, SLOT(updateIGTLConnectorNode()));
  QObject::connect(this->ConnectorStateCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(startCurrentIGTLConnector(bool)));
  QObject::connect(this->PersistentStateCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(updateIGTLConnectorNode()));
  QObject::connect(this->LogConnectionErrorCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(updateIGTLConnectorNode()));
  QObject::connect(this->ConnectorHostNameEdit, SIGNAL(editingFinished()),
                   q, SLOT(updateIGTLConnectorNode()));
  QObject::connect(this->ConnectorPortEdit, SIGNAL(editingFinished()),
                   q, SLOT(updateIGTLConnectorNode()));
  QObject::connect(&this->ConnectorTypeButtonGroup, SIGNAL(buttonClicked(int)),
                   q, SLOT(updateIGTLConnectorNode()));

  this->ConnectorNotDefinedRadioButton->setVisible(false);
  this->ConnectorTypeButtonGroup.addButton(this->ConnectorNotDefinedRadioButton, igtlio::Connector::TYPE_NOT_DEFINED);
  this->ConnectorTypeButtonGroup.addButton(this->ConnectorServerRadioButton, igtlio::Connector::TYPE_SERVER);
  this->ConnectorTypeButtonGroup.addButton(this->ConnectorClientRadioButton, igtlio::Connector::TYPE_CLIENT);

}

//------------------------------------------------------------------------------
qSlicerIGTLConnectorPropertyWidget::qSlicerIGTLConnectorPropertyWidget(QWidget *_parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerIGTLConnectorPropertyWidgetPrivate(*this))
{
  Q_D(qSlicerIGTLConnectorPropertyWidget);
  d->init();
}

//------------------------------------------------------------------------------
qSlicerIGTLConnectorPropertyWidget::~qSlicerIGTLConnectorPropertyWidget()
{
}

//------------------------------------------------------------------------------
void qSlicerIGTLConnectorPropertyWidget::setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode * connectorNode)
{
  Q_D(qSlicerIGTLConnectorPropertyWidget);
  qvtkReconnect(d->IGTLConnectorNode, connectorNode, vtkCommand::ModifiedEvent,
                this, SLOT(onMRMLNodeModified()));

  foreach(int evendId, QList<int>()
          << igtlio::Connector::ActivatedEvent
          << igtlio::Connector::ConnectedEvent
          << igtlio::Connector::DisconnectedEvent
          << igtlio::Connector::DeactivatedEvent)
    {
    qvtkReconnect(d->IGTLConnectorNode, connectorNode, evendId,
                  this, SLOT(onMRMLNodeModified()));
    }

  d->IGTLConnectorNode = connectorNode;

  this->onMRMLNodeModified();
  this->setEnabled(connectorNode != 0);
}

//------------------------------------------------------------------------------
void qSlicerIGTLConnectorPropertyWidget::setMRMLIGTLConnectorNode(vtkMRMLNode* node)
{
  this->setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode::SafeDownCast(node));
}

namespace
{
//------------------------------------------------------------------------------
void setNameEnabled(qSlicerIGTLConnectorPropertyWidgetPrivate * d, bool enabled)
{
  d->ConnectorNameEdit->setEnabled(enabled);
  d->ConnectorNameLabel->setEnabled(enabled);
}

//------------------------------------------------------------------------------
void setTypeEnabled(qSlicerIGTLConnectorPropertyWidgetPrivate * d, bool enabled)
{
  d->ConnectorTypeLabel->setEnabled(enabled);
  d->ConnectorServerRadioButton->setEnabled(enabled);
  d->ConnectorClientRadioButton->setEnabled(enabled);
}

//------------------------------------------------------------------------------
void setStateEnabled(qSlicerIGTLConnectorPropertyWidgetPrivate * d, bool enabled)
{
  d->ConnectorStateLabel->setEnabled(enabled);
  d->ConnectorStateCheckBox->setEnabled(enabled);
}

//------------------------------------------------------------------------------
void setHostnameEnabled(qSlicerIGTLConnectorPropertyWidgetPrivate * d, bool enabled)
{
  d->ConnectorHostNameEdit->setEnabled(enabled);
  d->ConnectorHostnameLabel->setEnabled(enabled);
}

//------------------------------------------------------------------------------
void setPortEnabled(qSlicerIGTLConnectorPropertyWidgetPrivate * d, bool enabled)
{
  d->ConnectorPortEdit->setEnabled(enabled);
  d->ConnectorPortLabel->setEnabled(enabled);
}
}

//------------------------------------------------------------------------------
void qSlicerIGTLConnectorPropertyWidget::onMRMLNodeModified()
{
  Q_D(qSlicerIGTLConnectorPropertyWidget);
  if (!d->IGTLConnectorNode)
    {
    return;
    }
  d->ConnectorNameEdit->setText(d->IGTLConnectorNode->GetName());
  d->ConnectorHostNameEdit->setText(d->IGTLConnectorNode->IOConnector->GetServerHostname());
  d->ConnectorPortEdit->setText(QString("%1").arg(d->IGTLConnectorNode->IOConnector->GetServerPort()));
  int type = d->IGTLConnectorNode->IOConnector->GetType();
  d->ConnectorNotDefinedRadioButton->setChecked(type == igtlio::Connector::TYPE_NOT_DEFINED);
  d->ConnectorServerRadioButton->setChecked(type == igtlio::Connector::TYPE_SERVER);
  d->ConnectorClientRadioButton->setChecked(type == igtlio::Connector::TYPE_CLIENT);

  setStateEnabled(d, type != igtlio::Connector::TYPE_NOT_DEFINED);

  bool deactivated = d->IGTLConnectorNode->IOConnector->GetState() == igtlio::Connector::STATE_OFF;
  if (deactivated)
    {
    setNameEnabled(d, true);
    setTypeEnabled(d, true);
    setHostnameEnabled(d, type == igtlio::Connector::TYPE_CLIENT);
    setPortEnabled(d, type != igtlio::Connector::TYPE_NOT_DEFINED);
    }
  else
    {
    setNameEnabled(d, false);
    setTypeEnabled(d, false);
    setHostnameEnabled(d, false);
    setPortEnabled(d, false);
    }
  d->ConnectorStateCheckBox->setChecked(!deactivated);
  d->PersistentStateCheckBox->setChecked(d->IGTLConnectorNode->IOConnector->GetPersistent() == igtlio::Connector::PERSISTENT_ON);
  //d->LogConnectionErrorCheckBox->setChecked(d->IGTLConnectorNode->IOConnector->GetLogErrorIfServerConnectionFailed());
}

//------------------------------------------------------------------------------
void qSlicerIGTLConnectorPropertyWidget::startCurrentIGTLConnector(bool value)
{
  Q_D(qSlicerIGTLConnectorPropertyWidget);
  Q_ASSERT(d->IGTLConnectorNode);
  if (value)
    {
    d->IGTLConnectorNode->IOConnector->Start();
    }
  else
    {
    d->IGTLConnectorNode->IOConnector->Stop();
    }
}

//------------------------------------------------------------------------------
void qSlicerIGTLConnectorPropertyWidget::updateIGTLConnectorNode()
{
  Q_D(qSlicerIGTLConnectorPropertyWidget);

  d->IGTLConnectorNode->DisableModifiedEventOn();

  d->IGTLConnectorNode->SetName(d->ConnectorNameEdit->text().toLatin1());
  d->IGTLConnectorNode->IOConnector->SetType(d->ConnectorTypeButtonGroup.checkedId());
  d->IGTLConnectorNode->IOConnector->SetServerHostname(d->ConnectorHostNameEdit->text().toStdString());
  d->IGTLConnectorNode->IOConnector->SetServerPort(d->ConnectorPortEdit->text().toInt());
  d->IGTLConnectorNode->IOConnector->SetPersistent(d->PersistentStateCheckBox->isChecked() ?
                                      igtlio::Connector::PERSISTENT_ON :
                                      igtlio::Connector::PERSISTENT_OFF);
  //d->IGTLConnectorNode->SetLogErrorIfServerConnectionFailed(d->LogConnectionErrorCheckBox->isChecked());

  d->IGTLConnectorNode->DisableModifiedEventOff();
  d->IGTLConnectorNode->InvokePendingModifiedEvent();
}
