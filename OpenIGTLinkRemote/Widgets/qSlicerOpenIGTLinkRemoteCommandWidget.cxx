
// Qt includes
#include <QDebug>
#include <QList>
#include <QRadioButton>
#include <QSet>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractCoreModule.h"
#include "qSlicerOpenIGTLinkRemoteCommandWidget.h"
#include "ui_qSlicerOpenIGTLinkRemoteCommandWidget.h"

// Other includes
#include "vtkSlicerOpenIGTLinkCommand.h"
#include "vtkSlicerOpenIGTLinkRemoteLogic.h"

// Slicer includes
#include "vtkMRMLNode.h"

// STL includes
#include <algorithm>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerOpenIGTLinkRemoteCommandWidgetPrivate: public Ui_qSlicerOpenIGTLinkRemoteCommandWidget
{
Q_DECLARE_PUBLIC( qSlicerOpenIGTLinkRemoteCommandWidget );

protected:
  qSlicerOpenIGTLinkRemoteCommandWidget* const q_ptr;

public:
  qSlicerOpenIGTLinkRemoteCommandWidgetPrivate( qSlicerOpenIGTLinkRemoteCommandWidget& object );
  
  vtkSlicerOpenIGTLinkRemoteLogic * logic();

  vtkSmartPointer<vtkSlicerOpenIGTLinkCommand> command;
};



//-----------------------------------------------------------------------------
// Private class

//------------------------------------------------------------------------------
qSlicerOpenIGTLinkRemoteCommandWidgetPrivate::qSlicerOpenIGTLinkRemoteCommandWidgetPrivate( qSlicerOpenIGTLinkRemoteCommandWidget& object )
  : q_ptr( &object )
{
}

//------------------------------------------------------------------------------
vtkSlicerOpenIGTLinkRemoteLogic * qSlicerOpenIGTLinkRemoteCommandWidgetPrivate::logic()
{
  Q_Q( qSlicerOpenIGTLinkRemoteCommandWidget );
  return vtkSlicerOpenIGTLinkRemoteLogic::SafeDownCast( q->CommandLogic );
}

//------------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::setup()
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);
  d->setupUi(this);

  d->ShowFullResponseCheckBox->setChecked(false);

  QStringList horizontalHeaders;
  horizontalHeaders << "Key" << "Value";
  d->tableWidget_metaData->setHorizontalHeaderLabels(horizontalHeaders);
  d->tableWidget_responseMetaData->setHorizontalHeaderLabels(horizontalHeaders);

  for (int c = 0; c < d->tableWidget_metaData->horizontalHeader()->count(); ++c)
  {
    d->tableWidget_metaData->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
    d->tableWidget_responseMetaData->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
  }

  connect( d->pushButton_addMetaData, &QPushButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onAddMetaDataClicked );
  connect( d->pushButton_removeMetaData, &QPushButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onRemoveMetaDataClicked );

  connect( d->radioButton_versionString, &QRadioButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onVersionButtonClicked );
  connect( d->radioButton_versionCommand, &QRadioButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onVersionButtonClicked );

  connect( d->SendCommandButton, SIGNAL( clicked() ), this, SLOT( onSendCommandClicked() ) );
  qvtkConnect(d->command, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(onQueryResponseReceived()));
}


// ==================================================================================
// qSlicerOpenIGTLinkRemoteCommandWidget methods

// Constructor
qSlicerOpenIGTLinkRemoteCommandWidget::qSlicerOpenIGTLinkRemoteCommandWidget( QWidget* _parent )
  : Superclass( _parent )
  , d_ptr( new qSlicerOpenIGTLinkRemoteCommandWidgetPrivate( *this ) )
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);

  d->command = vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New();
  this->setup();
}

//------------------------------------------------------------------------------
qSlicerOpenIGTLinkRemoteCommandWidget::~qSlicerOpenIGTLinkRemoteCommandWidget()
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);
  qvtkDisconnect(d->command, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(onQueryResponseReceived()));
  this->setMRMLScene(NULL);
  if ( this->CommandLogic != NULL )
  {
    this->CommandLogic->UnRegister(NULL);
    this->CommandLogic = NULL;
  }

  disconnect( d->pushButton_addMetaData, &QPushButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onAddMetaDataClicked );
  disconnect( d->pushButton_removeMetaData, &QPushButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onRemoveMetaDataClicked );

  disconnect( d->radioButton_versionString, &QRadioButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onVersionButtonClicked );
  disconnect( d->radioButton_versionCommand, &QRadioButton::clicked, this, &qSlicerOpenIGTLinkRemoteCommandWidget::onVersionButtonClicked );

  disconnect( d->SendCommandButton, SIGNAL(clicked()), this, SLOT(onSendCommandClicked()) );
}

//------------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::onSendCommandClicked()
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);

  // Cancel previous command if it was already in progress
  if (d->command->GetStatus() == vtkSlicerOpenIGTLinkCommand::CommandWaiting)
  {
    qDebug("qSlicerOpenIGTLinkRemoteCommandWidget::sendCommand: previous command was already in progress, cancelling it now.");
    d->logic()->CancelCommand( d->command );
  }

  vtkMRMLNode* connectorNode = d->ConnectorComboBox->currentNode();
  if ( connectorNode == NULL )
  {
    d->ResponseTextEdit->setPlainText( "Connector node not selected!" );
    return;
  }
  
  std::string commandString = d->CommandTextEdit->toPlainText().toStdString();
  if ( commandString.size() < 1 )
  {
    d->ResponseTextEdit->setPlainText( "Please type command XML in the Command field!" );
    return;
  }

  if (d->radioButton_versionCommand->isChecked())
  {
    if (d->lineEdit_commandName->text().isEmpty())
    {
      d->ResponseTextEdit->setPlainText("Please specify a command name.");
      return;
    }

    d->command->SetCommandName(d->lineEdit_commandName->text().toStdString());
    for (int i = 0; i < d->tableWidget_metaData->rowCount(); i++)
    {
      QTableWidgetItem *keyItem = d->tableWidget_metaData->item(i, 0);
      QTableWidgetItem *valueItem = d->tableWidget_metaData->item(i, 0);

      if (keyItem->text().isEmpty())
      {
        continue;
      }

      d->command->SetMetaDataElement(keyItem->text().toStdString(), valueItem->text().toStdString());
    }
  }

  // Removed until command version support is re-added
  d->command->SetCommandVersion(d->radioButton_versionCommand->isChecked() ? IGTL_HEADER_VERSION_2 : IGTL_HEADER_VERSION_1);
  
  // Logic sends command message.
  if (d->command->SetCommandText(d->CommandTextEdit->toPlainText().toStdString().c_str()))
  {
    d->logic()->SendCommand( d->command, connectorNode->GetID());
    onQueryResponseReceived();
  }
  else
  {
    d->ResponseTextEdit->setPlainText( "Command cannot be sent: XML parsing failed" );
  }
  
}

//------------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::onQueryResponseReceived()
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);
  
  vtkMRMLNode* node = d->ConnectorComboBox->currentNode();
  if ( node == NULL )
  {
    d->ResponseTextEdit->setPlainText( "Connector node not selected!" );
    return;
  }
  
  std::string message;
  std::string parameters;
  int status = d->command->GetStatus();

  QString responseGroupBoxTitle="Response details for command ID: "+QString::fromStdString(d->command->GetID())+"";
  d->responseGroupBox->setTitle(responseGroupBoxTitle);
  std::string displayedText = !d->command->GetResponseMessage().empty() ? d->command->GetResponseMessage() : "";
  if (d->command->GetResponseXML() == NULL)
  {
    displayedText = !d->command->GetResponseText().empty() ? d->command->GetResponseText() : "";
  }
  if (status == vtkSlicerOpenIGTLinkCommand::CommandSuccess)
  {
    d->ResponseTextEdit->setPlainText(QString(displayedText.c_str()));
  }
  else if (status == vtkSlicerOpenIGTLinkCommand::CommandWaiting)
  {
    d->ResponseTextEdit->setPlainText("Waiting for response...");
  }
  else if (status == vtkSlicerOpenIGTLinkCommand::CommandExpired)
  {
    d->ResponseTextEdit->setPlainText("Command timed out");
  }
  else 
  {
    d->ResponseTextEdit->setPlainText("Command failed.\n"+QString(displayedText.c_str()));
  }

  d->tableWidget_responseMetaData->clear();
  igtl::MessageBase::MetaDataMap metaData = d->command->GetResponseMetaData();
  for (igtl::MessageBase::MetaDataMap::iterator it = begin(metaData); it != end(metaData); ++it)
  {
    d->tableWidget_responseMetaData->insertRow(d->tableWidget_responseMetaData->rowCount());
    d->tableWidget_metaData->setItem(0, d->tableWidget_metaData->rowCount() - 1, new QTableWidgetItem(QString::fromStdString(it->first)));
    d->tableWidget_metaData->setItem(1, d->tableWidget_metaData->rowCount() - 1, new QTableWidgetItem(QString::fromStdString(it->second.second)));
  }
  std::string fullResponseText = !d->command->GetResponseText().empty() ? d->command->GetResponseText() : "";
  d->FullResponseTextEdit->setPlainText(QString(fullResponseText.c_str()));
}

//----------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::onAddMetaDataClicked()
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);

  d->tableWidget_metaData->insertRow(d->tableWidget_metaData->rowCount());
  d->tableWidget_metaData->setItem(0, d->tableWidget_metaData->rowCount() - 1, new QTableWidgetItem());
  d->tableWidget_metaData->setItem(1, d->tableWidget_metaData->rowCount() - 1, new QTableWidgetItem());
}

//----------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::onRemoveMetaDataClicked()
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);

  QList<QTableWidgetItem*> items = d->tableWidget_metaData->selectedItems();

  QSet<int> selectedRows;
  QTableWidgetItem * item;
  foreach(item, items)
  {
    selectedRows.insert(item->row());
  }
  QList<int> rows = selectedRows.toList();
  std::sort(rows.begin(), rows.end());

  foreach(int row, rows)
  {
    d->tableWidget_metaData->removeRow(row);
  }
}

//----------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::onVersionButtonClicked()
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);

  d->tableWidget_metaData->setEnabled(d->radioButton_versionCommand->isChecked());
  d->pushButton_addMetaData->setEnabled(d->radioButton_versionCommand->isChecked());
  d->pushButton_removeMetaData->setEnabled(d->radioButton_versionCommand->isChecked());
  d->lineEdit_commandName->setEnabled(d->radioButton_versionCommand->isChecked());
}

//------------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::setMRMLScene(vtkMRMLScene *newScene)
{
  Q_D(qSlicerOpenIGTLinkRemoteCommandWidget);

  // The scene set in the widget should match the scene set in the logic,
  // log a warning if it is not so.
  // During scene opening or closing it is normal to have one of the scene pointers
  // still/already NULL while the other is non-NULL.
  if ( this->CommandLogic->GetMRMLScene() != newScene
    && newScene != NULL && this->CommandLogic->GetMRMLScene() != NULL)
    {
    qWarning( "Inconsistent MRML scene in OpenIGTLinkRemote logic" );
    }
  
  this->Superclass::setMRMLScene(newScene);
}

//------------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::setCommandLogic(vtkMRMLAbstractLogic* newCommandLogic)
{
  if ( newCommandLogic == NULL )
  {
    qWarning( "Trying to set NULL as logic" );
    return;
  }
  
  this->CommandLogic = vtkSlicerOpenIGTLinkRemoteLogic::SafeDownCast( newCommandLogic );
  if ( this->CommandLogic != NULL )
  {
    this->CommandLogic->Register(NULL);
  }
  else
  {
    qWarning( "Logic is not an OpenIGTLinkRemoteLogic type!" );
  }
}

//------------------------------------------------------------------------------
void qSlicerOpenIGTLinkRemoteCommandWidget::setIFLogic(vtkSlicerOpenIGTLinkIFLogic* ifLogic)
{
  this->CommandLogic->SetIFLogic(ifLogic);
}
