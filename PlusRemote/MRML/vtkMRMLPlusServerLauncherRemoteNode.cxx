/*==============================================================================

  Program: 3D Slicer

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
#include "vtkMRMLPlusServerLauncherRemoteNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

// OpenIGTLinkIF includes
#include <vtkMRMLIGTLConnectorNode.h>
#include <vtkMRMLTextNode.h>
#include <vtkMRMLNode.h>

//------------------------------------------------------------------------------
static const char* LAUNCHER_CONNECTOR_REFERENCE_ROLE = "launcherConnectorRef";
static const char* CURRENT_CONFIG_REFERENCE_ROLE = "currentConfigRef";

const int PORT_MIN = 0;
const int PORT_MAX = 65535;

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusServerLauncherRemoteNode);

//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherRemoteNode::vtkMRMLPlusServerLauncherRemoteNode()
  : ServerLauncherHostname(NULL)
  , ServerLauncherPort(vtkMRMLPlusServerLauncherRemoteNode::DefaultPort)
  , ServerState(vtkMRMLPlusServerLauncherRemoteNode::ServerOff)
  , LogLevel(vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo)
  , CurrentErrorLevel(vtkMRMLPlusServerLauncherRemoteNode::LogLevelInfo)
{
  this->SetServerLauncherHostname("localhost");
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherRemoteNode::~vtkMRMLPlusServerLauncherRemoteNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherRemoteNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  of << " hostname=\"" << this->ServerLauncherHostname << "\"";
  of << " serverLauncherPort =\"" << this->ServerLauncherPort << "\"";
  of << " serverState =\"" << this->ServerState << "\"";

}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherRemoteNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "hostname"))
      {
      this->SetServerLauncherHostname(attValue);
      }
    else if (!strcmp(attName, "serverLauncherPort"))
      {
      vtkVariant portVariant = attValue;
      portVariant.ToInt();
      bool valid = true;
      int portValue = portVariant.ToInt(&valid);
      if (!valid)
        {
        portValue = vtkMRMLPlusServerLauncherRemoteNode::DefaultPort;
        }
      this->SetServerLauncherPort(portValue);
      }
    else if (!strcmp(attName, "serverState"))
      {
      vtkVariant stateVariant = attValue;
      bool valid = true;
      stateVariant.ToInt();
      int stateValue = stateVariant.ToInt(&valid);
      if (!valid)
        {
          stateValue = vtkMRMLPlusServerLauncherRemoteNode::ServerOff;
        }
      this->SetServerState(stateValue);
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherRemoteNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLPlusServerLauncherRemoteNode* otherNode = vtkMRMLPlusServerLauncherRemoteNode::SafeDownCast(anode);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherRemoteNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkMRMLPlusServerLauncherRemoteNode::GetLauncherConnectorNode()
{
  return vtkMRMLIGTLConnectorNode::SafeDownCast(this->GetNodeReference(LAUNCHER_CONNECTOR_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherRemoteNode::SetAndObserveLauncherConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  this->SetNodeReferenceID(LAUNCHER_CONNECTOR_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLTextNode* vtkMRMLPlusServerLauncherRemoteNode::GetCurrentConfigNode()
{
  return vtkMRMLTextNode::SafeDownCast(this->GetNodeReference(CURRENT_CONFIG_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherRemoteNode::SetAndObserveCurrentConfigNode(vtkMRMLTextNode* node)
{
  this->SetNodeReferenceID(CURRENT_CONFIG_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherRemoteNode::SetServerLauncherPort(int port)
{
  int clampedPort = std::max(PORT_MIN, std::min(PORT_MAX, port));
  this->ServerLauncherPort = clampedPort;
}