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

#ifndef __vtkMRMLPlusServerNode_h
#define __vtkMRMLPlusServerNode_h

// MRML includes
#include <vtkMRMLNode.h>

// vtkAddon includes  
#include <vtkAddonSetGet.h>

// PlusRemote includes
#include "vtkSlicerPlusRemoteModuleMRMLExport.h"

class vtkMRMLScene;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLTextNode;
class vtkMRMLPlusServerLauncherNode;

/// \ingroup Segmentations
/// \brief Parameter set node for the plus remote launcher widget
///
class VTK_SLICER_PLUSREMOTE_MODULE_MRML_EXPORT vtkMRMLPlusServerNode : public vtkMRMLNode
{

public:

  enum State
  {
    Off = 0,
    On,
    Starting,
    Stopping,
    NumberOfStates,
  };

  static const std::string CONFIG_REFERENCE_ROLE;
  static const std::string LAUNCHER_REFERENCE_ROLE;
  static const std::string PLUS_SERVER_CONNECTOR_REFERENCE_ROLE;

  static const std::string PLUS_SERVER_ATTRIBUTE_PREFIX;
  static const std::string SERVER_ID_ATTRIBUTE_NAME;

  static vtkMRMLPlusServerNode* New();
  vtkTypeMacro(vtkMRMLPlusServerNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Standard MRML node methods
  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual void ReadXMLAttributes(const char** atts) override;
  virtual void WriteXML(ostream& of, int indent) override;
  virtual void Copy(vtkMRMLNode* node) override;
  virtual const char* GetNodeTagName() override { return "PlusServer"; }

  void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData) override;

protected:
  vtkMRMLPlusServerNode();
  virtual ~vtkMRMLPlusServerNode();
  vtkMRMLPlusServerNode(const vtkMRMLPlusServerNode&);
  void operator=(const vtkMRMLPlusServerNode&);

public:

  enum
  {
    LOG_ERROR = 1,
    LOG_WARNING = 2,
    LOG_INFO = 3,
    LOG_DEBUG = 4,
    LOG_TRACE = 5,
    LogLast,
  };

  /// Get the ID of the server. Used for identifying the server when communicating with Plus
  vtkGetMacro(ServerID, std::string);
  /// Set the ID of the server. Used for identifying the server when communicating with Plus
  vtkSetMacro(ServerID, std::string);

  /// Get the flag indicating if the server is controlled locally. If true, then the logic will attempt to synchronize the
  /// current server state and the desired server state
  vtkGetMacro(ControlledLocally, bool);
  /// Set the flag indicating if the server is controlled locally. If true, then the logic will attempt to synchronize the
  /// current server state and the desired server state
  vtkSetMacro(ControlledLocally, bool);

  /// Set the desired server state. Lets the logic know what state it should attempt to acheive
  vtkGetMacro(DesiredState, int);
  // Get the desired server state. Lets the logic know what state it should attempt to acheive
  vtkSetMacro(DesiredState, int);

  /// Get the current state of the server node
  vtkGetMacro(State, int);
 /// Set the current state of the server node
  void SetState(int state);

  /// Get the log level of the server node
  vtkGetMacro(LogLevel, int);
  /// Set the log level of the server node
  vtkSetMacro(LogLevel, int);

  /// Get the DeviceSetName from the config file
  vtkGetMacro(DeviceSetName, std::string);
  /// Set the DeviceSetName from the config file
  vtkSetMacro(DeviceSetName, std::string);
  /// Get the DeviceSetDescription from the config file
  vtkGetMacro(DeviceSetDescription, std::string);
  /// Set the DeviceSetDescription from the config file
  vtkSetMacro(DeviceSetDescription, std::string);

  /// Get timeout length before a server state should be consided stale
  vtkGetMacro(TimeoutSeconds, double);
  /// Set timeout length before a server state should be consided stale
  vtkSetMacro(TimeoutSeconds, double);
  /// Get the length of time since the server state was last updated
  double GetSecondsSinceStateChange();

  /// Sets the desired state to ON
  /// \sa SetDesiredState()
  void StartServer();
  /// Sets the desired state to OFF
  /// \sa SetDesiredState()
  void StopServer();

  /// Get the config file node
  vtkMRMLTextNode* GetConfigNode();
  /// Set and observe config file node
  void SetAndObserveConfigNode(vtkMRMLTextNode* node);

  /// Get the launcher node
  vtkMRMLPlusServerLauncherNode* GetLauncherNode();
  /// Set and observe launcher node
  void SetAndObserveLauncherNode(vtkMRMLPlusServerLauncherNode* node);

  void AddAndObservePlusOpenIGTLinkServerConnector(vtkMRMLIGTLConnectorNode* connectorNode);
  std::vector<vtkMRMLIGTLConnectorNode*> GetPlusOpenIGTLinkConnectorNodes();

  struct PlusOpenIGTLinkServerInfo
  {
    std::string OutputChannelId;
    int ListeningPort;
  };

  struct PlusConfigFileInfo
  {
    std::string Name;
    std::string Description;
    std::vector<vtkMRMLPlusServerNode::PlusOpenIGTLinkServerInfo> Servers;
  };

  /// Parses the config file content and returns the config file information
  static PlusConfigFileInfo GetPlusConfigFileInfo(std::string content);

  /// Returns a list of PlusOpenIGTLinkServerInfo that identify the PlusOpenIGTLinkServers identified by the config file
  std::vector<PlusOpenIGTLinkServerInfo> GetPlusOpenIGTLinkServers() const { return this->PlusOpenIGTLinkServers; };
  /// Sets the list of PlusOpenIGTLinkServerInfo that identify the PlusOpenIGTLinkServers identified by the config file
  void SetPlusOpenIGTLinkServers(std::vector<PlusOpenIGTLinkServerInfo> plusOpenIGTLinkServers) { this->PlusOpenIGTLinkServers = plusOpenIGTLinkServers; };

  void SetDesiredStateFromString(const char* stateString);
  void SetStateFromString(const char* stateString);
  static const char* GetLogLevelAsString(int logLevel);
  static const char* GetDesiredStateAsString(int state);
  static int GetDesiredStateFromString(const char*);
  static const char* GetStateAsString(int state);
  static int GetStateFromString(const char*);

  /// Calls GetPlusConfigFileInfo on the currently attached config file to update the DeviceSetName, DeviceSetDescription and PlusOpenIGTLinkServers
  /// \sa GetPlusConfigFileInfo()
  /// \sa SetPlusOpenIGTLinkServers()
  /// \sa GetDeviceSetName()
  /// \sa GetDeviceSetDescription()
  void UpdateConfigFileInfo();

protected:

  std::string ServerID;
  bool ControlledLocally;
  int DesiredState;
  int State;
  int LogLevel;
  std::string DeviceSetName;
  std::string DeviceSetDescription;
  std::vector<PlusOpenIGTLinkServerInfo> PlusOpenIGTLinkServers;
  double TimeoutSeconds;
  double StateChangedTimestamp;
};

#endif // __vtkMRMLPlusServerNode_h
