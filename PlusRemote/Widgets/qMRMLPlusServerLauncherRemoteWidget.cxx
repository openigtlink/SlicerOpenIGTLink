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
#include "vtkMRMLPlusServerLauncherRemoteNode.h"

// QT includes
#include <QtGui>
#include <QDebug>
#include <qfiledialog.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkMRMLAbstractLogic.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>
#include <vtkCollection.h>

// MRML includes
#include <vtkMRMLScene.h>

// OpenIGTLinkIF includes
#include <vtkMRMLTextNode.h>
#include <vtkMRMLIGTLConnectorNode.h>

//-----------------------------------------------------------------------------
const std::string CONFIG_FILE_NODE_ATTRIBUTE = "ConfigFile";
const std::string CONFIG_FILE_NAME_ATTRIBUTE = "ConfigFilename";
const std::string TEMP_CONFIG_FILE_NAME_ATTRIBUTE = "TempConfigFilename";

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

  static void onStartServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void onStopServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void onCommandReceived(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void onServerInfoResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  void onLogMessageCommand(vtkSmartPointer<vtkXMLDataElement> messageCommandElement);

  virtual void getServerInfo();
  void connectToStartedServers(std::string filename);

  vtkSmartPointer<vtkMRMLIGTLConnectorNode> createConnectorNode(std::string id, std::string hostname, int port);

public:

  vtkSmartPointer<vtkCallbackCommand> StartServerCallback;
  vtkSmartPointer<vtkCallbackCommand> StopServerCallback;
  vtkSmartPointer<vtkCallbackCommand> CommandReceivedCallback;
  vtkSmartPointer<vtkCallbackCommand> ServerInfoReceivedCallback;

  vtkWeakPointer<vtkMRMLPlusServerLauncherRemoteNode> ParameterSetNode;
  vtkWeakPointer<vtkMRMLIGTLConnectorNode> LauncherConnectorNode;

  vtkSmartPointer<vtkMRMLTextNode> LogTextNode;

  QPixmap IconDisconnected;
  QPixmap IconNotConnected;
  QPixmap IconConnected;
  QPixmap IconConnectedWarning;
  QPixmap IconConnectedError;
  QPixmap IconWaiting;
  QPixmap IconWaitingWarning;
  QPixmap IconWaitingError;
  QPixmap IconRunning;
  QPixmap IconRunningWarning;
  QPixmap IconRunningError;

  bool WasPreviouslyConnected;
};

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherRemoteWidgetPrivate::qMRMLPlusServerLauncherRemoteWidgetPrivate(qMRMLPlusServerLauncherRemoteWidget& object)
  : q_ptr(&object)
  , LauncherConnectorNode(NULL)
  , IconDisconnected(QPixmap(":/Icons/PlusLauncherRemoteDisconnected.png"))
  , IconNotConnected(QPixmap(":/Icons/PlusLauncherRemoteNotConnected.png"))
  , IconConnected(QPixmap(":/Icons/PlusLauncherRemoteConnect.png"))
  , IconConnectedWarning(QPixmap(":/Icons/PlusLauncherRemoteConnectWarning.png"))
  , IconConnectedError(QPixmap(":/Icons/PlusLauncherRemoteConnectError.png"))
  , IconWaiting(QPixmap(":/Icons/PlusLauncherRemoteWait.png"))
  , IconWaitingWarning(QPixmap(":/Icons/PlusLauncherRemoteWaitWarning.png"))
  , IconWaitingError(QPixmap(":/Icons/PlusLauncherRemoteWaitError.png"))
  , IconRunning(QPixmap(":/Icons/PlusLauncherRemoteRunning.png"))
  , IconRunningWarning(QPixmap(":/Icons/PlusLauncherRemoteRunningWarning.png"))
  , IconRunningError(QPixmap(":/Icons/PlusLauncherRemoteRunningError.png"))
  , WasPreviouslyConnected(false)
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

  this->ConfigFileSelectorComboBox->addAttribute("vtkMRMLTextNode", CONFIG_FILE_NODE_ATTRIBUTE.c_str());

  this->StartServerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->StartServerCallback->SetCallback(qMRMLPlusServerLauncherRemoteWidgetPrivate::onStartServerResponse);
  this->StartServerCallback->SetClientData(this);

  this->StopServerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->StopServerCallback->SetCallback(qMRMLPlusServerLauncherRemoteWidgetPrivate::onStopServerResponse);
  this->StopServerCallback->SetClientData(this);

  this->CommandReceivedCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->CommandReceivedCallback->SetCallback(qMRMLPlusServerLauncherRemoteWidgetPrivate::onCommandReceived);
  this->CommandReceivedCallback->SetClientData(this);

  this->ServerInfoReceivedCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->ServerInfoReceivedCallback->SetCallback(qMRMLPlusServerLauncherRemoteWidgetPrivate::onServerInfoResponse);
  this->ServerInfoReceivedCallback->SetClientData(this);

  this->LogLevelComboBox->addItem("Error", vtkMRMLPlusServerLauncherRemoteNode::LogLevelError);
  this->LogLevelComboBox->addItem("Warning", vtkMRMLPlusServerLauncherRemoteNode::LogLevelWarning);
  this->LogLevelComboBox->addItem("Info", vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo);
  this->LogLevelComboBox->addItem("Debug", vtkMRMLPlusServerLauncherRemoteNode::LogLevelDebug);
  //this->LogLevelComboBox->addItem("Trace", vtkMRMLPlusServerLauncherRemoteNode::LogLevelTrace); // Callback in vtkPlusLogger does not trigger on trace

  QObject::connect(this->LauncherConnectCheckBox, SIGNAL(toggled(bool)), q, SLOT(onConnectCheckBoxChanged(bool)));
  QObject::connect(this->LoadConfigFileButton, SIGNAL(clicked()), q, SLOT(onLoadConfigFile()));
  QObject::connect(this->ConfigFileSelectorComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(onConfigFileChanged(vtkMRMLNode*)));
  QObject::connect(this->LogLevelComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(onLogLevelChanged(int)));
  QObject::connect(this->StartStopServerButton, SIGNAL(clicked()), q, SLOT(onStartStopButton()));
  QObject::connect(this->ClearLogButton, SIGNAL(clicked()), q, SLOT(onClearLogButton()));
  QObject::connect(this->HostnameLineEdit, SIGNAL(textEdited(const QString &)), q, SLOT(onHostChanged(const QString &)));
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkMRMLIGTLConnectorNode> qMRMLPlusServerLauncherRemoteWidgetPrivate::createConnectorNode(std::string name, std::string hostname, int port)
{
  Q_Q(qMRMLPlusServerLauncherRemoteWidget);

  vtkMRMLIGTLConnectorNode* launcherConnectorNode = NULL;

  std::vector<vtkMRMLNode*> connectorNodes = std::vector<vtkMRMLNode*>();
  q->mrmlScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", connectorNodes);

  for (std::vector<vtkMRMLNode*>::iterator connectorNodeIt = connectorNodes.begin(); connectorNodeIt != connectorNodes.end(); ++connectorNodeIt)
  {
    vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(*connectorNodeIt);
    if (!connectorNode)
    {
      continue;
    }

    std::string connectorNodeHostname = connectorNode->GetServerHostname();
    if (strcmp(connectorNodeHostname.c_str(), hostname.c_str()) == 0 && connectorNode->GetServerPort() == port)
    {
      launcherConnectorNode = connectorNode;
      break;
    }
  }

  if (!launcherConnectorNode)
  {
    vtkMRMLNode* node = q->mrmlScene()->AddNewNodeByClass("vtkMRMLIGTLConnectorNode", "PlusServerLauncherConnector");
    launcherConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  }

  if (launcherConnectorNode && launcherConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateOff)
  {
    launcherConnectorNode->Stop();
  }

  launcherConnectorNode->SetName(name.c_str());
  launcherConnectorNode->SetServerHostname(hostname);
  launcherConnectorNode->SetServerPort(port);
  launcherConnectorNode->Start();

  return launcherConnectorNode;
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidgetPrivate::onStartServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  igtlioCommand* startServerCommand = igtlioCommand::SafeDownCast(caller);
  if (!startServerCommand)
  {
    return;
  }

  qMRMLPlusServerLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusServerLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }
  startServerCommand->RemoveObserver(d->StartServerCallback);

  if (startServerCommand->GetSuccessful())
  {
    std::string responseContent = startServerCommand->GetResponseContent();
    vtkSmartPointer<vtkXMLDataElement> responseXML = vtkXMLUtilities::ReadElementFromString(responseContent.c_str());
    if (responseXML)
    {
      vtkSmartPointer<vtkXMLDataElement> resultElement = responseXML->FindNestedElementWithName("Result");
      if (resultElement)
      {
        const char* servers = resultElement->GetAttribute("Servers");
        if (servers)
        {
          std::string serverString = servers;
          d->connectToStartedServers(serverString);
        }
      }
    }
    d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerRunning);
    // TODO: check for success
  }
  else
  {
    // Expired or returned with failure.
    d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerOff);
  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidgetPrivate::onStopServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  igtlioCommand* stopServerCommand = igtlioCommand::SafeDownCast(caller);
  if (!stopServerCommand)
  {
    return;
  }

  qMRMLPlusServerLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusServerLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }
  stopServerCommand->RemoveObserver(d->StopServerCallback);

  if (stopServerCommand->GetSuccessful())
  {
    d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerOff);
  }
  else
  {
    // Expired or returned with failure
    d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerRunning);
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidgetPrivate::onCommandReceived(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  qMRMLPlusServerLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusServerLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }

  igtlioCommand* command = static_cast<igtlioCommand*>(calldata);
  if (!command)
  {
    return;
  }

  std::string name = command->GetName();
  vtkSmartPointer<vtkXMLDataElement> rootElement = vtkXMLUtilities::ReadElementFromString(command->GetCommandContent().c_str());

  if (name == "ServerStarted")
  {
    // FOR NOW, ONLY CONTROL ONE SERVER THAT IS MANAGED BY THIS PARAMETER NODE
    // IN THE FUTURE, MAY WANT TO SEE LIST OF CURRENTLY RUNNING SERVERS AND PROVIDE OPTION TO MANAGE ALL OF THEM

    //d->CurrentErrorLevel = vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo;
    //d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerRunning);
    //vtkSmartPointer<vtkXMLDataElement> plusConfigurationResponseElement = rootElement->FindNestedElementWithName("ServerStarted");
    //if (plusConfigurationResponseElement)
    //{
    //  std::string ports = plusConfigurationResponseElement->GetAttribute("Servers");
    //  if (!ports.empty())
    //  {
    //    d->connectToStartedServers(ports);
    //  }
    //}
  }
  else if (name == "ServerStopped")
  {
    vtkSmartPointer<vtkXMLDataElement> serverStoppedElement = rootElement->FindNestedElementWithName("ServerStopped");
    if (serverStoppedElement)
    {
      if (serverStoppedElement->GetAttribute("ConfigFileName"))
      {
        std::string configFileName = serverStoppedElement->GetAttribute("ConfigFileName");
        vtkMRMLTextNode* configNode = d->ParameterSetNode->GetCurrentConfigNode();
        if (!configFileName.empty() && configNode && strcmp(configFileName.c_str(), configNode->GetAttribute(CONFIG_FILE_NAME_ATTRIBUTE.c_str())) == 0)
        {
          d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerOff);
        }
      }
    }
  }
  else if (name == "LogMessage")
  {
    d->onLogMessageCommand(rootElement);
  }
  else if (name == "Echo")
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (connectorNode)
  {
    connectorNode->SendCommandResponse(command);
  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidgetPrivate::connectToStartedServers(std::string serverString)
{
  /* String is in format:
    OutputChannelId:ListeningPort;OutputChannelId:ListeningPort;OutputChannelId:ListeningPort;

    Created from <PlusOpenIGTLinkServer/> elements in the config file
  */

  std::istringstream ss(serverString);
  std::string serverToken;

  while (std::getline(ss, serverToken, ';'))
  {
    std::string token;
    std::istringstream innerSS(serverToken);
    std::vector<std::string> nameAndPortTokens = std::vector<std::string>();
    while (std::getline(innerSS, token, ':'))
    {
      nameAndPortTokens.push_back(token);
    }

    if (nameAndPortTokens.size() != 2)
    {
      continue;
    }

    std::string nameString = nameAndPortTokens[0];
    std::string portString = nameAndPortTokens[1];

    bool success;
    int port = QString::fromStdString(portString).toInt(&success);
    if (success)
    {
      std::string connectorName = this->ParameterSetNode->GetServerLauncherHostname() + std::string(" || ") + serverToken;
      this->createConnectorNode(connectorName.c_str(), this->ParameterSetNode->GetServerLauncherHostname(), port);
    }
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidgetPrivate::onLogMessageCommand(vtkSmartPointer<vtkXMLDataElement> messageCommand)
{
  Q_Q(qMRMLPlusServerLauncherRemoteWidget);

  if (!this->ParameterSetNode)
  {
    return;
  }

  for (int i = 0; i < messageCommand->GetNumberOfNestedElements(); ++i)
  {
    vtkSmartPointer<vtkXMLDataElement> nestedElement = messageCommand->GetNestedElement(i);
    if (strcmp(nestedElement->GetName(), "LogMessage") != 0)
    {
      continue;
    }

    std::string messageContents = nestedElement->GetAttribute("Message");
    if (messageContents.empty())
    {
      return;
    }

    int logLevel = 0;
    if (!nestedElement->GetScalarAttribute("LogLevel", logLevel))
    {
      logLevel = vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo;
    }

    std::stringstream message;
    if (logLevel == vtkMRMLPlusServerLauncherRemoteNode::LogLevelError)
    {
      message << "<font color = \"" << COLOR_ERROR << "\">";
    }
    else if (logLevel == vtkMRMLPlusServerLauncherRemoteNode::LogLevelWarning)
    {
      message << "<font color = \"" << COLOR_WARNING << "\">";
    }
    else
    {
      message << "<font color = \"" << COLOR_NORMAL << "\">";
    }
    message << messageContents << "<br / >";
    message << "</font>";

    bool logLevelChanged = false;
    if (logLevel < this->ParameterSetNode->GetCurrentErrorLevel())
    {
      this->ParameterSetNode->SetCurrentErrorLevel(logLevel);
    }

    // TODO: tracking position behavior:: see QPlusStatusIcon::ParseMessage
    this->ServerLogTextEdit->moveCursor(QTextCursor::End);
    this->ServerLogTextEdit->insertHtml(QString::fromStdString(message.str()));
    this->ServerLogTextEdit->moveCursor(QTextCursor::End);

  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidgetPrivate::getServerInfo()
{
  vtkMRMLIGTLConnectorNode* connectorNode = this->ParameterSetNode->GetLauncherConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // TODO: Update with proper command syntax when finalized
  std::stringstream commandText;
  commandText << "<Command>" << std::endl;
  commandText << "  <PlusOpenIGTLinkServer/>" << std::endl;
  commandText << "</Command>" << std::endl;

  igtlioCommandPointer getServerInfoCommand = igtlioCommandPointer::New();
  getServerInfoCommand->BlockingOff();
  getServerInfoCommand->SetName("GetServerInfo");
  getServerInfoCommand->SetCommandContent(commandText.str().c_str());
  getServerInfoCommand->SetTimeoutSec(1.0);
  getServerInfoCommand->AddObserver(igtlioCommand::CommandCompletedEvent, this->ServerInfoReceivedCallback);
  connectorNode->SendCommand(getServerInfoCommand);

}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidgetPrivate::onServerInfoResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{

  igtlioCommand* getServerInfoCommand = igtlioCommand::SafeDownCast(caller);
  if (!getServerInfoCommand)
  {
    return;
  }

  qMRMLPlusServerLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusServerLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }
  getServerInfoCommand->RemoveObserver(d->StopServerCallback);

  if (getServerInfoCommand->GetSuccessful())
  {
    std::string responseContent = getServerInfoCommand->GetResponseContent();
    vtkSmartPointer<vtkXMLDataElement> getServerInfoElement = vtkXMLUtilities::ReadElementFromString(responseContent.c_str());
    for (int i = 0; i < getServerInfoElement->GetNumberOfNestedElements(); ++i)
    {
      vtkSmartPointer<vtkXMLDataElement> nestedElement = getServerInfoElement->GetNestedElement(i);
      if (strcmp(nestedElement->GetName(), "PlusOpenIGTLinkServer") == 0)
      {
        std::string id = nestedElement->GetAttribute("OutputChannelId");
        int port = 0;

        if (!id.empty() && nestedElement->GetScalarAttribute("ListeningPort", port))
        {
          std::string hostname = d->ParameterSetNode->GetServerLauncherHostname();
          d->createConnectorNode(id, hostname, d->ParameterSetNode->GetServerLauncherPort());
        }
      }
    }

  }
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

  d->ConfigFileSelectorComboBox->setMRMLScene(this->mrmlScene());

  this->initializeParameterSetNode();

  // Update UI
  this->updateWidgetFromMRML();

  // observe close event so we can re-add a parameters node if necessary
  this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent, this, SLOT(onMRMLSceneEndCloseEvent()));
}

//------------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::initializeParameterSetNode()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  vtkSmartPointer<vtkCollection> collection =
    vtkSmartPointer<vtkCollection>::Take(this->mrmlScene()->GetNodesByClassByName("vtkMRMLPlusServerLauncherRemoteNode", "PlusRemote"));

  vtkMRMLPlusServerLauncherRemoteNode* plusServerLauncherRemoteNode = NULL;
  if (collection->GetNumberOfItems() > 0)
  {
    plusServerLauncherRemoteNode = vtkMRMLPlusServerLauncherRemoteNode::SafeDownCast(collection->GetItemAsObject(0));
  }

  if (!plusServerLauncherRemoteNode)
  {
    plusServerLauncherRemoteNode = vtkMRMLPlusServerLauncherRemoteNode::SafeDownCast(
      this->mrmlScene()->AddNewNodeByClass("vtkMRMLPlusServerLauncherRemoteNode"));
  }

  this->setParameterSetNode(plusServerLauncherRemoteNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onMRMLSceneEndCloseEvent()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  this->setParameterSetNode(NULL);
  this->initializeParameterSetNode();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  std::string tooltipSuffix = "\nClick to view log";
  if (!this->mrmlScene() || this->mrmlScene()->IsClosing() || !d->ParameterSetNode)
  {
    bool checkBoxSignals = d->LauncherConnectCheckBox->blockSignals(true);
    d->LauncherConnectCheckBox->setDisabled(true);
    d->LauncherConnectCheckBox->setChecked(false);
    d->LauncherConnectCheckBox->blockSignals(checkBoxSignals);
    d->LauncherStatusButton->setIcon(d->IconDisconnected);
    d->LauncherStatusButton->setToolTip(QString::fromStdString("Launcher disconnected." + tooltipSuffix));
    d->ConfigFileTextEdit->setText("");
    d->StartStopServerButton->setEnabled(false);
    return;
  }

  int disabledModify = d->ParameterSetNode->StartModify();

  int state = vtkMRMLIGTLConnectorNode::StateOff;
  vtkMRMLIGTLConnectorNode* launcherConnectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (launcherConnectorNode)
  {
    state = launcherConnectorNode->GetState();
  }

  bool connectionEnabled = state != vtkMRMLIGTLConnectorNode::StateOff;

  bool checkBoxSignals = d->LauncherConnectCheckBox->blockSignals(true);
  d->LauncherConnectCheckBox->setEnabled(true);
  d->LauncherConnectCheckBox->setChecked(connectionEnabled);
  d->LauncherConnectCheckBox->blockSignals(checkBoxSignals);
  d->StartStopServerButton->setEnabled(true);

  d->HostnameLineEdit->setEnabled(!connectionEnabled);

  vtkMRMLTextNode* configFileNode = d->ParameterSetNode->GetCurrentConfigNode();
  if (configFileNode)
  {
    d->ConfigFileTextEdit->setText(configFileNode->GetText());
  }
  else
  {
    d->ConfigFileTextEdit->setText("");
  }

  bool configFileSelected = configFileNode != NULL;

  std::string hostname = d->ParameterSetNode->GetServerLauncherHostname();
  int port = d->ParameterSetNode->GetServerLauncherPort();
  int cursorPosition = d->HostnameLineEdit->cursorPosition();
  std::stringstream hostnameAndPort;
  hostnameAndPort << hostname << ":" << port;
  d->HostnameLineEdit->setText(QString::fromStdString(hostnameAndPort.str()));
  d->HostnameLineEdit->setCursorPosition(cursorPosition);

  std::string logIconMessage = "";

  // Attempt to subscribe to log messages if connected
  int serverState = d->ParameterSetNode->GetServerState();
  bool connected = state == vtkMRMLIGTLConnectorNode::StateConnected;
  if (connected)
  {
    if (!d->WasPreviouslyConnected)
    {
      d->WasPreviouslyConnected = true;
      this->subscribeToLogMessages();
    }
  }
  else
  {
    serverState = vtkMRMLPlusServerLauncherRemoteNode::ServerOff;
    d->ParameterSetNode->SetServerState(serverState);
    d->WasPreviouslyConnected = false;
  }

  // Adjust buttons based on server state, and construct log button message
  switch (serverState)
  {
  case vtkMRMLPlusServerLauncherRemoteNode::ServerRunning:
    d->StartStopServerButton->setEnabled(true);
    d->StartStopServerButton->setText("Stop server");
    d->ConfigFileSelectorComboBox->setDisabled(true);
    d->LogLevelComboBox->setDisabled(true);
    logIconMessage = "Server running.";
    break;
  case vtkMRMLPlusServerLauncherRemoteNode::ServerStarting:
    d->StartStopServerButton->setDisabled(true);
    d->StartStopServerButton->setText("Launching...");
    d->ConfigFileSelectorComboBox->setDisabled(true);
    d->LogLevelComboBox->setDisabled(true);
    logIconMessage = "Server starting.";
    break;
  case vtkMRMLPlusServerLauncherRemoteNode::ServerStopping:
    d->StartStopServerButton->setDisabled(true);
    d->StartStopServerButton->setText("Stopping...");
    d->ConfigFileSelectorComboBox->setDisabled(true);
    d->LogLevelComboBox->setDisabled(true);
    logIconMessage = "Server stopping.";
    break;
  case vtkMRMLPlusServerLauncherRemoteNode::ServerOff:
  default:
    d->StartStopServerButton->setEnabled(connected && configFileSelected);
    d->StartStopServerButton->setText("Launch server");
    d->ConfigFileSelectorComboBox->setEnabled(true);
    d->LogLevelComboBox->setEnabled(true);
    if (state == vtkMRMLIGTLConnectorNode::StateConnected)
    {
      logIconMessage = "Launcher connected.";
    }
    else if (state == vtkMRMLIGTLConnectorNode::StateWaitConnection)
    {
      logIconMessage = "Launcher not connected.";
    }
    else
    {
      logIconMessage = "Launcher disconnected.";
    }
    break;
  }

  // Finish status message
  int errorLevel = d->ParameterSetNode->GetCurrentErrorLevel();
  switch (errorLevel)
  {
    case vtkMRMLPlusServerLauncherRemoteNode::LogLevelError:
      logIconMessage += " Error detected.";
      break;
    case vtkMRMLPlusServerLauncherRemoteNode::LogLevelWarning:
      logIconMessage += " Warning detected.";
      break;
    default:
      break;
  }
  logIconMessage += tooltipSuffix;
  d->LauncherStatusButton->setToolTip(QString::fromStdString(logIconMessage));

  // Set correct icon
  this->updateStatusIcon();

  d->ConfigFileSelectorComboBox->setCurrentNode(d->ParameterSetNode->GetCurrentConfigNode());
  d->LogLevelComboBox->setCurrentIndex(d->LogLevelComboBox->findData(d->ParameterSetNode->GetLogLevel()));

  d->ParameterSetNode->EndModify(disabledModify);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::updateStatusIcon()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  int connectionEnabled = vtkMRMLIGTLConnectorNode::StateOff;
  int serverState = vtkMRMLPlusServerLauncherRemoteNode::ServerOff;
  int errorLevel = vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo;

  int state = vtkMRMLIGTLConnectorNode::StateOff;
  if (d->ParameterSetNode)
  {
    vtkMRMLIGTLConnectorNode* launcherConnectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
    if (launcherConnectorNode)
    {
      state = launcherConnectorNode->GetState();
    }
    serverState = d->ParameterSetNode->GetServerState();
    errorLevel = d->ParameterSetNode->GetCurrentErrorLevel();
  }

  if (state == vtkMRMLIGTLConnectorNode::StateOff)
  {
    d->LauncherStatusButton->setIcon(d->IconDisconnected);
  }
  else if (state == vtkMRMLIGTLConnectorNode::StateWaitConnection)
  {
    d->LauncherStatusButton->setIcon(d->IconNotConnected);
  }
  else if (errorLevel >= vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo)
  {
    switch (serverState)
    {
      case vtkMRMLPlusServerLauncherRemoteNode::ServerOff:
        d->LauncherStatusButton->setIcon(d->IconConnected);
        break;
      case vtkMRMLPlusServerLauncherRemoteNode::ServerRunning:
        d->LauncherStatusButton->setIcon(d->IconRunning);
        break;
      case vtkMRMLPlusServerLauncherRemoteNode::ServerStarting:
      case vtkMRMLPlusServerLauncherRemoteNode::ServerStopping:
        d->LauncherStatusButton->setIcon(d->IconWaiting);
        break;
      default:
        break;
    }
  }
  else if (errorLevel == vtkMRMLPlusServerLauncherRemoteNode::LogLevelWarning)
  {
    switch (serverState)
    {
    case vtkMRMLPlusServerLauncherRemoteNode::ServerOff:
      d->LauncherStatusButton->setIcon(d->IconConnectedWarning);
      break;
    case vtkMRMLPlusServerLauncherRemoteNode::ServerRunning:
      d->LauncherStatusButton->setIcon(d->IconRunningWarning);
      break;
    case vtkMRMLPlusServerLauncherRemoteNode::ServerStarting:
    case vtkMRMLPlusServerLauncherRemoteNode::ServerStopping:
      d->LauncherStatusButton->setIcon(d->IconWaitingWarning);
      break;
    default:
      break;
    }
  }
  else if (errorLevel == vtkMRMLPlusServerLauncherRemoteNode::LogLevelError)
  {
    switch (serverState)
    {
    case vtkMRMLPlusServerLauncherRemoteNode::ServerOff:
      d->LauncherStatusButton->setIcon(d->IconConnectedError);
      break;
    case vtkMRMLPlusServerLauncherRemoteNode::ServerRunning:
      d->LauncherStatusButton->setIcon(d->IconRunningError);
      break;
    case vtkMRMLPlusServerLauncherRemoteNode::ServerStarting:
    case vtkMRMLPlusServerLauncherRemoteNode::ServerStopping:
      d->LauncherStatusButton->setIcon(d->IconWaitingError);
      break;
    default:
      break;
    }
  }
    
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onConnectCheckBoxChanged(bool connect)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  std::string hostname = d->ParameterSetNode->GetServerLauncherHostname();
  int port = d->ParameterSetNode->GetServerLauncherPort();

  vtkMRMLIGTLConnectorNode* launcherConnectorNode = NULL;
  launcherConnectorNode = d->ParameterSetNode->GetLauncherConnectorNode();

  // If there is a connector node selected, but the hostname or port doesn't match, stop observing the connector
  if (launcherConnectorNode)
  {
    if (strcmp(launcherConnectorNode->GetServerHostname(), hostname.c_str()) != 0 || launcherConnectorNode->GetServerPort() != port)
    {
      launcherConnectorNode = NULL;
    }
  }

  if (!launcherConnectorNode)
  {
    std::vector<vtkMRMLNode*> connectorNodes = std::vector<vtkMRMLNode*>();
    //TODO find static method to get node type name
    this->mrmlScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", connectorNodes);

    for (std::vector<vtkMRMLNode*>::iterator connectorNodeIt = connectorNodes.begin(); connectorNodeIt != connectorNodes.end(); ++connectorNodeIt)
    {
      vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(*connectorNodeIt);
      if (!connectorNode)
      {
        continue;
      }

      if (strcmp(connectorNode->GetServerHostname(), hostname.c_str()) == 0 && connectorNode->GetServerPort() == port)
      {
        launcherConnectorNode = connectorNode;
        break;
      }
    }
  }

  //TODO find static method to get node type name
  if (connect)
  {
    if (!launcherConnectorNode)
    {
      vtkMRMLNode* node = this->mrmlScene()->AddNewNodeByClass("vtkMRMLIGTLConnectorNode", "PlusServerLauncherConnector");
      launcherConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
    }

    if (launcherConnectorNode && launcherConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateOff)
    {
      launcherConnectorNode->Stop();
    }

    launcherConnectorNode->SetServerHostname(hostname);
    launcherConnectorNode->SetServerPort(port);
    launcherConnectorNode->Start();

  }
  else
  {
    if (launcherConnectorNode)
    {
      launcherConnectorNode->Stop();
    }
  }

  this->setAndObserveLauncherConnectorNode(launcherConnectorNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::setAndObserveLauncherConnectorNode(vtkMRMLIGTLConnectorNode* connectorNode)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  d->ParameterSetNode->SetAndObserveLauncherConnectorNode(connectorNode);

  if (!connectorNode)
  {
    qvtkDisconnect(d->LauncherConnectorNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkDisconnect(d->LauncherConnectorNode, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkDisconnect(d->LauncherConnectorNode, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(updateWidgetFromMRML()));
    d->LauncherConnectorNode->RemoveObserver(d->CommandReceivedCallback);
    d->LauncherConnectorNode = NULL;
    return;
  }

  if (d->LauncherConnectorNode != connectorNode)
  {
    qvtkReconnect(d->LauncherConnectorNode, connectorNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkReconnect(d->LauncherConnectorNode, connectorNode, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkReconnect(d->LauncherConnectorNode, connectorNode, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(updateWidgetFromMRML()));

    if (d->LauncherConnectorNode)
    {
      d->LauncherConnectorNode->RemoveObserver(d->CommandReceivedCallback);
    }

    d->LauncherConnectorNode = connectorNode;

    if (d->LauncherConnectorNode)
    {
      d->LauncherConnectorNode->AddObserver(vtkMRMLIGTLConnectorNode::CommandReceivedEvent, d->CommandReceivedCallback);
    }

  }
}

//------------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onLoadConfigFile()
{
  QString filename = QFileDialog::getOpenFileName(NULL, "Select config file", "", "Config Files (*.xml);;AllFiles (*)");
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly))
  {
    return;
  }

  QFileInfo fileInfo(file);
  QString configFileContents = file.readAll();
  std::string configFileContentString = configFileContents.toStdString();

  vtkSmartPointer<vtkMRMLTextNode> configFileNode = vtkSmartPointer<vtkMRMLTextNode>::New();
  configFileNode->SaveWithSceneOn();
  configFileNode->SetAttribute(CONFIG_FILE_NODE_ATTRIBUTE.c_str(), "true");
  configFileNode->SetAttribute(CONFIG_FILE_NAME_ATTRIBUTE.c_str(), fileInfo.fileName().toStdString().c_str());

  configFileNode->SetText(configFileContentString.c_str());

  this->mrmlScene()->AddNode(configFileNode);

  std::string name = fileInfo.fileName().toStdString();

  std::string configFileText = configFileNode->GetText();
  if (configFileText.empty())
  {
    this->mrmlScene()->RemoveNode(configFileNode);
    return;
  }

  vtkSmartPointer<vtkXMLDataElement> configFileXML = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(configFileText.c_str()));
  if (!configFileXML)
  {
    this->mrmlScene()->RemoveNode(configFileNode);
    return;
  }

  vtkSmartPointer<vtkXMLDataElement> dataCollectionElement = configFileXML->FindNestedElementWithName("DataCollection");
  if (dataCollectionElement)
  {
    vtkSmartPointer<vtkXMLDataElement> deviceSetElement = dataCollectionElement->FindNestedElementWithName("DeviceSet");
    if (deviceSetElement)
    {
      std::string nameAttribute = deviceSetElement->GetAttribute("Name");
      if (!nameAttribute.empty())
      {
        name = nameAttribute;
      }
    }

  }
  configFileNode->SetName(name.c_str());
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onConfigFileChanged(vtkMRMLNode* currentNode)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLTextNode* configFileNode = vtkMRMLTextNode::SafeDownCast(d->ConfigFileSelectorComboBox->currentNode());
  d->ParameterSetNode->SetAndObserveCurrentConfigNode(configFileNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onLogLevelChanged(int index)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  int logLevel = d->LogLevelComboBox->currentData().toInt();
#else
  int logLevel = d->LogLevelComboBox->itemData(d->LogLevelComboBox->currentIndex()).toInt();
#endif
  d->ParameterSetNode->SetLogLevel(logLevel);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onHostChanged(const QString &text)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  std::string hostnameAndPort = text.toStdString();
  std::vector<std::string> tokens = std::vector<std::string>();
  std::istringstream ss(hostnameAndPort);
  std::string item;
  while (std::getline(ss, item, ':'))
  {
    tokens.push_back(item);
  }

  std::string hostname = d->ParameterSetNode->GetServerLauncherHostname();
  if (tokens.size() >= 1)
  {
    hostname = tokens[0];
  }

  int port = d->ParameterSetNode->GetServerLauncherPort();
  if (tokens.size() > 1)
  {
    bool success = false;
    int parsedPort = QVariant(tokens[1].c_str()).toInt(&success);
    if (success)
    {
      port = parsedPort;
    }
  }

  d->ParameterSetNode->SetServerLauncherHostname(hostname.c_str());
  d->ParameterSetNode->SetServerLauncherPort(port);

  hostname = d->ParameterSetNode->GetServerLauncherHostname();
  port = d->ParameterSetNode->GetServerLauncherPort();

  // If the input string contained some invalid characters, update widget from parameter node to remove them
  std::stringstream reformattedSS;
  reformattedSS << hostname << ":" << port;
  std::string reformattedString = reformattedSS.str();
  if (strcmp(reformattedString.c_str(), hostnameAndPort.c_str()) != 0)
  {
    this->updateWidgetFromMRML();
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onStartStopButton()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  if (d->ParameterSetNode->GetServerState() == vtkMRMLPlusServerLauncherRemoteNode::ServerOff)
  {
    this->launchServer();
  }
  else if (d->ParameterSetNode->GetServerState() == vtkMRMLPlusServerLauncherRemoteNode::ServerRunning)
  {
    this->stopServer();
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::onClearLogButton()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);
  d->ServerLogTextEdit->setPlainText("");
  if (d->ParameterSetNode)
  {
    d->ParameterSetNode->SetCurrentErrorLevel(vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo);
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::launchServer()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  vtkMRMLTextNode* configFileNode = d->ParameterSetNode->GetCurrentConfigNode();

  if (!connectorNode || !configFileNode)
  {
    return;
  }

  std::string fileName = configFileNode->GetAttribute(CONFIG_FILE_NAME_ATTRIBUTE.c_str());

  std::string configFile = configFileNode->GetText();
  std::stringstream addConfigFileCommandText;
  addConfigFileCommandText << "<Command>" << std::endl;
  addConfigFileCommandText << "  <ConfigFileName Value= \"" << fileName << "\"/>" << std::endl;
  if (!configFile.empty())
  {
    addConfigFileCommandText << "  <ConfigFileContent>" << std::endl;
    addConfigFileCommandText << configFile << std::endl;
    addConfigFileCommandText << "  </ConfigFileContent>" << std::endl;
  }
  addConfigFileCommandText << "</Command>" << std::endl;
  std::string addConfigFileCommandString = addConfigFileCommandText.str();

  vtkSmartPointer<igtlioCommand> addOrUpdateConfigFileCommand = vtkSmartPointer<igtlioCommand>::New();
  addOrUpdateConfigFileCommand->BlockingOn();
  addOrUpdateConfigFileCommand->SetName("AddConfigFile");
  addOrUpdateConfigFileCommand->SetCommandContent(addConfigFileCommandString);
  addOrUpdateConfigFileCommand->SetTimeoutSec(2.0);
  addOrUpdateConfigFileCommand->SetCommandMetaDataElement("ConfigFileName", fileName);
  addOrUpdateConfigFileCommand->SetCommandMetaDataElement("ConfigFileContent", configFile);
  connectorNode->SendCommand(addOrUpdateConfigFileCommand);

  if (!addOrUpdateConfigFileCommand->GetSuccessful())
  {
    return;
  }

  
  IANA_ENCODING_TYPE encodingType = IANA_TYPE_US_ASCII;
  addOrUpdateConfigFileCommand->GetResponseMetaDataElement("ConfigFileName", fileName, encodingType);
  configFileNode->SetAttribute(TEMP_CONFIG_FILE_NAME_ATTRIBUTE.c_str(), fileName.c_str());

  int logLevel = d->ParameterSetNode->GetLogLevel();
  std::stringstream startServerCommandText;
  startServerCommandText << "<Command>" << std::endl;
  startServerCommandText << "  <LogLevel Value= \"" << logLevel << "\"/>" << std::endl;
  startServerCommandText << "  <ConfigFileName Value= \"" << fileName << "\"/>" << std::endl;
  startServerCommandText << "</Command>" << std::endl;

  std::stringstream logLevelSS;
  logLevelSS << d->ParameterSetNode->GetLogLevel();
  std::string logLevelString = logLevelSS.str();

  vtkSmartPointer<igtlioCommand> startServerCommand = vtkSmartPointer<igtlioCommand>::New();
  startServerCommand->BlockingOff();
  startServerCommand->SetName("StartServer");
  startServerCommand->SetCommandContent(startServerCommandText.str());
  startServerCommand->SetTimeoutSec(2.0);
  startServerCommand->SetCommandMetaDataElement("LogLevel", logLevelString);
  startServerCommand->SetCommandMetaDataElement("ConfigFileName", fileName);
  startServerCommand->AddObserver(igtlioCommand::CommandCompletedEvent, d->StartServerCallback);
  connectorNode->SendCommand(startServerCommand);
  d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerStarting);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::stopServer()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  vtkMRMLTextNode* configFileNode = d->ParameterSetNode->GetCurrentConfigNode();
  std::string fileName = configFileNode->GetAttribute(TEMP_CONFIG_FILE_NAME_ATTRIBUTE.c_str());

  //TODO: device name based on parameter node name?
  vtkSmartPointer<igtlioCommand> stopServerCommand = vtkSmartPointer<igtlioCommand>::New();
  stopServerCommand->BlockingOff();
  stopServerCommand->SetName("StopServer");
  stopServerCommand->SetTimeoutSec(10.0);
  stopServerCommand->SetCommandMetaDataElement("ConfigFileName", fileName);
  stopServerCommand->AddObserver(igtlioCommand::CommandCompletedEvent, d->StopServerCallback);
  connectorNode->SendCommand(stopServerCommand);
  d->ParameterSetNode->SetServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerStopping);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::subscribeToLogMessages()
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (!connectorNode)
  {
    return;
  }
  vtkSmartPointer<igtlioCommand> subscribeLogCommand = vtkSmartPointer<igtlioCommand>::New();
  subscribeLogCommand->BlockingOff();
  subscribeLogCommand->SetName("LogSubscribe");
  subscribeLogCommand->SetTimeoutSec(0.1);
  connectorNode->SendCommand(subscribeLogCommand);
}

//-----------------------------------------------------------------------------
bool qMRMLPlusServerLauncherRemoteWidget::logVisible() const
{
  Q_D(const qMRMLPlusServerLauncherRemoteWidget);
  return d->LogGroupBox->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::setLogVisible(bool visible)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);
  d->LogGroupBox->setVisible(visible);
}

//------------------------------------------------------------------------------
vtkMRMLPlusServerLauncherRemoteNode* qMRMLPlusServerLauncherRemoteWidget::plusRemoteLauncherNode()const
{
  Q_D(const qMRMLPlusServerLauncherRemoteWidget);
  return d->ParameterSetNode;
}


//------------------------------------------------------------------------------
void qMRMLPlusServerLauncherRemoteWidget::setParameterSetNode(vtkMRMLPlusServerLauncherRemoteNode* parameterNode)
{
  Q_D(qMRMLPlusServerLauncherRemoteWidget);
  if (d->ParameterSetNode == parameterNode)
  {
    return;
  }

  // Connect modified event on ParameterSetNode to updating the widget
  qvtkReconnect(d->ParameterSetNode, parameterNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set parameter set node
  d->ParameterSetNode = parameterNode;

  if (!d->ParameterSetNode)
  {
    return;
  }

  // Update UI
  this->updateWidgetFromMRML();
}