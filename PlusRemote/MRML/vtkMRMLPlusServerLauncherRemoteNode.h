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

#ifndef __vtkMRMLPlusServerLauncherRemoteNode_h
#define __vtkMRMLPlusServerLauncherRemoteNode_h

// MRML includes
#include <vtkMRMLNode.h>

// PlusRemote includes
#include "vtkSlicerPlusRemoteModuleMRMLExport.h"

class vtkMRMLScene;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLTextNode;

/// \ingroup Segmentations
/// \brief Parameter set node for the plus remote launcher widget
///
class VTK_SLICER_PLUSREMOTE_MODULE_MRML_EXPORT vtkMRMLPlusServerLauncherRemoteNode : public vtkMRMLNode
{

public:

  enum ServerStatus
  {
    ServerOff,
    ServerRunning,
    ServerStarting,
    ServerStopping,
  };

  static vtkMRMLPlusServerLauncherRemoteNode* New();
  vtkTypeMacro(vtkMRMLPlusServerLauncherRemoteNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Standard MRML node methods
  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual void ReadXMLAttributes(const char** atts) override;
  virtual void WriteXML(ostream& of, int indent) override;
  virtual void Copy(vtkMRMLNode* node) override;
  virtual const char* GetNodeTagName() override { return "PlusRemoteLauncher"; }

protected:
  vtkMRMLPlusServerLauncherRemoteNode();
  virtual ~vtkMRMLPlusServerLauncherRemoteNode();
  vtkMRMLPlusServerLauncherRemoteNode(const vtkMRMLPlusServerLauncherRemoteNode&);
  void operator=(const vtkMRMLPlusServerLauncherRemoteNode&);

public:

  enum
  {
    LogLevelError = 1,
    LogLevelWarning = 2,
    LogLevelInfo = 3,
    LogLevelDebug = 4,
    LogLevelTrace = 5,
  };

  vtkGetMacro(ServerState, int);
  vtkSetMacro(ServerState, int);

  vtkGetStringMacro(ServerLauncherHostname);
  vtkSetStringMacro(ServerLauncherHostname);

  void SetServerLauncherPort(int port);
  vtkGetMacro(ServerLauncherPort, int);

  vtkGetMacro(LogLevel, int);
  vtkSetMacro(LogLevel, int);

  vtkGetMacro(CurrentErrorLevel, int);
  vtkSetMacro(CurrentErrorLevel, int);

  /// Get launcher connector node
  vtkMRMLIGTLConnectorNode* GetLauncherConnectorNode();
  /// Set and observe launcher connector node
  void SetAndObserveLauncherConnectorNode(vtkMRMLIGTLConnectorNode* node);

  /// Get current config file node
  vtkMRMLTextNode* GetCurrentConfigNode();
  /// Set and observe current config file node
  void SetAndObserveCurrentConfigNode(vtkMRMLTextNode* node);

private:
  int ServerLauncherState;
  int ServerState;
  char* ServerLauncherHostname;
  int ServerLauncherPort;
  int LogLevel;
  int CurrentErrorLevel;

  static const int DefaultPort = 18904;
};

#endif // __vtkMRMLPlusServerLauncherRemoteNode_h
