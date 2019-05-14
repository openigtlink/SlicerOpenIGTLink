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

// PlusRemote includes
#include "vtkMRMLPlusServerLauncherNode.h"
#include "vtkMRMLPlusServerNode.h"

// OpenIGTLinkIF includes
#include <vtkMRMLIGTLConnectorNode.h>

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTextNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
const std::string vtkMRMLPlusServerLauncherNode::CONNECTOR_REFERENCE_ROLE = "connectorRef";
const std::string vtkMRMLPlusServerLauncherNode::PLUS_SERVER_REFERENCE_ROLE = "plusServerRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusServerLauncherNode);

//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherNode::vtkMRMLPlusServerLauncherNode()
  : Hostname("localhost")
  , Port(18904)
  , LogLevel(LOG_INFO)
{
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherNode::~vtkMRMLPlusServerLauncherNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLStdStringMacro(Hostname, Hostname);
  vtkMRMLWriteXMLIntMacro(Port, Port);
  vtkMRMLWriteXMLIntMacro(LogLevel, LogLevel);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStdStringMacro(Hostname, Hostname);
  vtkMRMLReadXMLIntMacro(Port, Port);
  vtkMRMLReadXMLIntMacro(LogLevel, LogLevel);
  vtkMRMLReadXMLEndMacro();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::Copy(vtkMRMLNode* anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();
  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyStdStringMacro(Hostname);
  vtkMRMLCopyIntMacro(Port);
  vtkMRMLCopyIntMacro(LogLevel);
  vtkMRMLCopyEndMacro();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintStdStringMacro(Hostname);
  vtkMRMLPrintIntMacro(Port);
  vtkMRMLPrintIntMacro(LogLevel);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkMRMLPlusServerLauncherNode::GetConnectorNode()
{
  return vtkMRMLIGTLConnectorNode::SafeDownCast(this->GetNodeReference(CONNECTOR_REFERENCE_ROLE.c_str()));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::SetAndObserveConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  this->SetNodeReferenceID(CONNECTOR_REFERENCE_ROLE.c_str(), (node ? node->GetID() : NULL));
  this->Modified();
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerNode* vtkMRMLPlusServerLauncherNode::StartServer(vtkMRMLTextNode* configFileNode, int logLevel/*=LOG_INFO*/)
{
  if (!this->Scene)
  {
    vtkErrorMacro("vtkMRMLPlusServerLauncherNode::StartServer: Invalid scene");
    return nullptr;
  }

  if (!configFileNode)
  {
    vtkErrorMacro("vtkMRMLPlusServerLauncherNode::StartServer: Invalid config file");
    return nullptr;
  }

  std::stringstream ss;
  ss << "PlusServer_" << configFileNode->GetName();
  std::string baseName = ss.str();

  vtkSmartPointer<vtkMRMLPlusServerNode> serverNode = vtkMRMLPlusServerNode::SafeDownCast(
    this->Scene->AddNewNodeByClass("vtkMRMLPlusServerNode", baseName));
  if (!serverNode)
  {
    vtkErrorMacro("vtkMRMLPlusServerLauncherNode::StartServer: Could not create server node");
    return nullptr;
  }

  this->AddAndObserveServerNode(serverNode);
  serverNode->SetAndObserveConfigNode(configFileNode);
  serverNode->SetLogLevel(logLevel);
  serverNode->StartServer();
  return serverNode;
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::AddAndObserveServerNode(vtkMRMLPlusServerNode * serverNode)
{
  int wasModifying = this->StartModify();
  this->AddNodeReferenceID(PLUS_SERVER_REFERENCE_ROLE.c_str(), (serverNode ? serverNode->GetID() : NULL));
  serverNode->SetNodeReferenceID(vtkMRMLPlusServerNode::LAUNCHER_REFERENCE_ROLE.c_str(), this->GetID());
  this->InvokeEvent(ServerAddedEvent, serverNode);
  this->Modified();
  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::RemoveServerNode(vtkMRMLPlusServerNode* node)
{
  for (int i = 0; i < this->GetNumberOfNodeReferences(PLUS_SERVER_REFERENCE_ROLE.c_str()); ++i)
  {
    const char* id = this->GetNthNodeReferenceID(PLUS_SERVER_REFERENCE_ROLE.c_str(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      int wasModifying = this->StartModify();
      this->RemoveNthNodeReferenceID(PLUS_SERVER_REFERENCE_ROLE.c_str(), i);
      this->InvokeEvent(ServerRemovedEvent, node);
      this->Modified();
      this->EndModify(wasModifying);
      return;
    }
  }
}

//----------------------------------------------------------------------------
std::vector<vtkMRMLPlusServerNode*> vtkMRMLPlusServerLauncherNode::GetServerNodes()
{
  std::vector<vtkMRMLPlusServerNode*> serverNodes;
  std::vector<vtkMRMLNode*> nodes;
  this->GetNodeReferences(vtkMRMLPlusServerLauncherNode::PLUS_SERVER_REFERENCE_ROLE.c_str(), nodes);

  for (std::vector<vtkMRMLNode*>::iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
  {
    vtkMRMLPlusServerNode* serverNode = vtkMRMLPlusServerNode::SafeDownCast(*nodeIt);
    if (!serverNode)
    {
      continue;
    }
    serverNodes.push_back(serverNode);
  }

  return serverNodes;
}