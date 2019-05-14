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
  and was supported through CANARIE's Research Software Program, and Cancer
  Care Ontario.

==============================================================================*/

#include "vtkMRMLPlusServerNode.h"

// PlusRemote includes
#include "vtkMRMLPlusServerLauncherNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>
#include <vtksys/SystemTools.hxx>

// STD includes
#include <sstream>

// OpenIGTLinkIF includes
#include <vtkMRMLIGTLConnectorNode.h>
#include <vtkMRMLTextNode.h>
#include <vtkMRMLNode.h>

//------------------------------------------------------------------------------
const std::string vtkMRMLPlusServerNode::CONFIG_REFERENCE_ROLE = "configNodeRef";
const std::string vtkMRMLPlusServerNode::LAUNCHER_REFERENCE_ROLE = "launcherNodeRef";
const std::string vtkMRMLPlusServerNode::PLUS_SERVER_CONNECTOR_REFERENCE_ROLE = "plusServerConnectorNodeRef";

//------------------------------------------------------------------------------
const std::string vtkMRMLPlusServerNode::PLUS_SERVER_ATTRIBUTE_PREFIX = "PLUS_SERVER";
const std::string vtkMRMLPlusServerNode::SERVER_ID_ATTRIBUTE_NAME = PLUS_SERVER_ATTRIBUTE_PREFIX + "serverID";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusServerNode);

//----------------------------------------------------------------------------
vtkMRMLPlusServerNode::vtkMRMLPlusServerNode()
  : ControlledLocally(true)
  , DesiredState(State::Off)
  , State(State::Off)
  , LogLevel(LOG_INFO)
  , TimeoutSeconds(15.0)
{
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerNode::~vtkMRMLPlusServerNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLStdStringMacro(serverID, ServerID);
  vtkMRMLWriteXMLEnumMacro(desiredState, DesiredState);
  vtkMRMLWriteXMLIntMacro(logLevel, LogLevel);
  vtkMRMLWriteXMLStdStringMacro(deviceSetDescription, DeviceSetDescription);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStdStringMacro(serverID, ServerID);
  vtkMRMLReadXMLEnumMacro(desiredState, DesiredState);
  vtkMRMLReadXMLIntMacro(logLevel, LogLevel);
  vtkMRMLReadXMLStdStringMacro(deviceSetDescription, DeviceSetDescription);
  vtkMRMLReadXMLEndMacro();
  this->UpdateConfigFileInfo();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::Copy(vtkMRMLNode* anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();
  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyStdStringMacro(ServerID);
  vtkMRMLCopyEnumMacro(DesiredState);
  vtkMRMLCopyEnumMacro(State);
  vtkMRMLCopyIntMacro(LogLevel);
  vtkMRMLCopyStdStringMacro(DeviceSetDescription);
  vtkMRMLCopyEndMacro();
  this->UpdateConfigFileInfo();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintStdStringMacro(ServerID);
  vtkMRMLPrintEnumMacro(DesiredState)
  vtkMRMLPrintEnumMacro(State);
  vtkMRMLPrintIntMacro(LogLevel);
  vtkMRMLPrintStdStringMacro(DeviceSetDescription);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
  {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene");
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    return;
  }

  if (eventID == vtkCommand::ModifiedEvent
    && caller == this->GetConfigNode())
  {
    // Update the config file info
    this->UpdateConfigFileInfo();
  }
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetState(int state)
{
  if (this->State != state)
  {
    this->State = state;
    this->StateChangedTimestamp = vtkTimerLog::GetUniversalTime();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double vtkMRMLPlusServerNode::GetSecondsSinceStateChange()
{
  return vtkTimerLog::GetUniversalTime() - this->StateChangedTimestamp;
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherNode* vtkMRMLPlusServerNode::GetLauncherNode()
{
  return vtkMRMLPlusServerLauncherNode::SafeDownCast(this->GetNodeReference(LAUNCHER_REFERENCE_ROLE.c_str()));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetAndObserveLauncherNode(vtkMRMLPlusServerLauncherNode* node)
{
  this->SetNodeReferenceID(LAUNCHER_REFERENCE_ROLE.c_str(), (node ? node->GetID() : NULL));
  node->AddNodeReferenceID(vtkMRMLPlusServerLauncherNode::PLUS_SERVER_REFERENCE_ROLE.c_str(), this->GetID());
  this->Modified();
}

//----------------------------------------------------------------------------
vtkMRMLTextNode* vtkMRMLPlusServerNode::GetConfigNode()
{
  return vtkMRMLTextNode::SafeDownCast(this->GetNodeReference(CONFIG_REFERENCE_ROLE.c_str()));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetAndObserveConfigNode(vtkMRMLTextNode* configFileNode)
{
  this->SetNodeReferenceID(CONFIG_REFERENCE_ROLE.c_str(), (configFileNode ? configFileNode->GetID() : NULL));

  int wasModifying = this->StartModify();
  this->UpdateConfigFileInfo();
  this->Modified();
  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::AddAndObservePlusOpenIGTLinkServerConnector(vtkMRMLIGTLConnectorNode* connectorNode)
{
  this->AddNodeReferenceID(PLUS_SERVER_CONNECTOR_REFERENCE_ROLE.c_str(), (connectorNode ? connectorNode->GetID() : NULL));
}

//----------------------------------------------------------------------------
std::vector<vtkMRMLIGTLConnectorNode*> vtkMRMLPlusServerNode::GetPlusOpenIGTLinkConnectorNodes()
{
  std::vector<vtkMRMLIGTLConnectorNode*> connectorNodes;
  std::vector<vtkMRMLNode*> nodes;
  this->GetNodeReferences(vtkMRMLPlusServerNode::PLUS_SERVER_CONNECTOR_REFERENCE_ROLE.c_str(), nodes);
  for (std::vector<vtkMRMLNode*>::iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
  {
    vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(*nodeIt);
    if (!connectorNode)
    {
      continue;
    }
    connectorNodes.push_back(connectorNode);
  }

  return connectorNodes;
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::StartServer()
{
  this->SetDesiredState(State::On);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::StopServer()
{
  this->SetDesiredState(State::Off);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::UpdateConfigFileInfo()
{
  int wasModifying = this->StartModify();

  vtkMRMLTextNode* configFileNode = this->GetConfigNode();
  if (!configFileNode)
  {
    return;
  }

  // Set the config file node attribute that identifies the Server ID
  // Server ID is used to start/stop servers, and to detect if a server is currently running
  if (configFileNode->GetAttribute(SERVER_ID_ATTRIBUTE_NAME.c_str()))
  {
    this->SetServerID(configFileNode->GetAttribute(SERVER_ID_ATTRIBUTE_NAME.c_str()));
  }
  else
  {
    if (this->ServerID.empty())
    {
      this->SetServerID(configFileNode->GetName());
    }
    configFileNode->SetAttribute(SERVER_ID_ATTRIBUTE_NAME.c_str(), this->ServerID.c_str());
  }

  std::string content;
  if (configFileNode && configFileNode->GetText())
  {
    content = configFileNode->GetText();
  }

  vtkMRMLPlusServerNode::PlusConfigFileInfo configInfo = vtkMRMLPlusServerNode::GetPlusConfigFileInfo(content);
  this->SetDeviceSetName(configInfo.Name);
  this->SetDeviceSetDescription(configInfo.Description);
  this->PlusOpenIGTLinkServers = configInfo.Servers;

  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerNode::PlusConfigFileInfo vtkMRMLPlusServerNode::GetPlusConfigFileInfo(std::string content)
{
  vtkMRMLPlusServerNode::PlusConfigFileInfo configFileInfo;

  vtkSmartPointer<vtkXMLDataElement> rootElement;
  if (!content.empty())
  {
    rootElement = vtkSmartPointer<vtkXMLDataElement>::Take(
      vtkXMLUtilities::ReadElementFromString(content.c_str()));
  }
  if (!rootElement)
  {
    return configFileInfo;
  }

  for (int i = 0; i < rootElement->GetNumberOfNestedElements(); ++i)
  {
    vtkSmartPointer<vtkXMLDataElement> nestedElement = rootElement->GetNestedElement(i);
    std::string nestedElementName = nestedElement->GetName();

    if (nestedElementName == "DataCollection")
    {
      vtkSmartPointer<vtkXMLDataElement> dataCollectionElement = nestedElement;
      for (int j = 0; j < dataCollectionElement->GetNumberOfNestedElements(); ++j)
      {
        vtkSmartPointer<vtkXMLDataElement> nestedDataCollectionElement = dataCollectionElement->GetNestedElement(j);
        std::string nestedDataCollectionElementName = nestedDataCollectionElement->GetName();
        if (nestedDataCollectionElementName == "DeviceSet")
        {
          if (nestedDataCollectionElement->GetAttribute("Name"))
          {
            configFileInfo.Name = nestedDataCollectionElement->GetAttribute("Name");
          }
          if (nestedDataCollectionElement->GetAttribute("Description"))
          {
            configFileInfo.Description = nestedDataCollectionElement->GetAttribute("Description");
          }
        }
      }
    }

    if (nestedElementName == "PlusOpenIGTLinkServer")
    {
      vtkSmartPointer<vtkXMLDataElement> plusOpenIGTLinkServerElement = nestedElement;

      vtkMRMLPlusServerNode::PlusOpenIGTLinkServerInfo serverInfo;
      serverInfo.ListeningPort = -1;
      if (plusOpenIGTLinkServerElement->GetAttribute("ListeningPort"))
      {
        serverInfo.ListeningPort = vtkVariant(plusOpenIGTLinkServerElement->GetAttribute("ListeningPort")).ToInt();
      }
      if (plusOpenIGTLinkServerElement->GetAttribute("OutputChannelId"))
      {
        serverInfo.OutputChannelId = plusOpenIGTLinkServerElement->GetAttribute("OutputChannelId");
      }
      configFileInfo.Servers.push_back(serverInfo);
    }
  }
  return configFileInfo;
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetDesiredStateFromString(const char* stateString)
{
  int state = this->GetStateFromString(stateString);
  if (state < 0)
  {
    return;
  }
  this->SetDesiredState(state);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetStateFromString(const char* stateString)
{
  int state = this->GetStateFromString(stateString);
  if (state < 0)
  {
    return;
  }
  this->SetState(state);
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlusServerNode::GetDesiredStateAsString(int state)
{
  return GetStateAsString(state);
}

//----------------------------------------------------------------------------
const char* vtkMRMLPlusServerNode::GetStateAsString(int state)
{
  std::string stateString = "Unknown";
  switch (state)
  {
  case State::Off:
    return "Off";
  case State::On:
    return "On";
  case State::Starting:
    return "Starting";
  case State::Stopping:
    return "Stopping";
  }
  return "";
}

//----------------------------------------------------------------------------
int vtkMRMLPlusServerNode::GetDesiredStateFromString(const char* stateString)
{
  return GetStateFromString(stateString);
}

//----------------------------------------------------------------------------
int vtkMRMLPlusServerNode::GetStateFromString(const char* stateString)
{
  if (!stateString)
  {
    return -1;
  }

  if (strcmp(stateString, GetStateAsString(State::On)) == 0)
  {
    return State::On;
  }

  if (strcmp(stateString, GetStateAsString(State::Off)) == 0)
  {
    return State::Off;
  }
  
  if (strcmp(stateString, GetStateAsString(State::Starting)) == 0)
  {
    return State::Starting;
  }

  if (strcmp(stateString, GetStateAsString(State::Stopping)) == 0)
  {
    return State::Stopping;
  }

  return -1;
}

//------------------------------------------------------------------------------
const char* vtkMRMLPlusServerNode::GetLogLevelAsString(int logLevel)
{
  switch (logLevel)
  {
  case LOG_ERROR:
    return "Error";
  case LOG_WARNING:
    return "Warning";
  case LOG_INFO:
    return "Info";
  case LOG_DEBUG:
    return "Debug";
  case LOG_TRACE:
    return "Trace";
  }
  return "Unknown";
}
