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

#ifndef __vtkSlicerOpenIGTLinkCommand_h
#define __vtkSlicerOpenIGTLinkCommand_h

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkSlicerOpenIGTLinkCommand : public vtkObject
{
public:
  static vtkSlicerOpenIGTLinkCommand* New();
  vtkTypeMacro(vtkSlicerOpenIGTLinkCommand, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerOpenIGTLinkCommand();
  virtual ~vtkSlicerOpenIGTLinkCommand();

private:
  vtkSlicerOpenIGTLinkCommand(const vtkSlicerOpenIGTLinkCommand&); // Not implemented
  void operator=(const vtkSlicerOpenIGTLinkCommand&);               // Not implemented

public:
  enum
  {
    CommandCancelled = igtlioCommandStatus::CommandCancelled,
    CommandExpired = igtlioCommandStatus::CommandExpired,
    CommandFailed = igtlioCommandStatus::CommandFailed,
    CommandResponseReceived = igtlioCommandStatus::CommandResponseReceived,
    CommandResponseSent = igtlioCommandStatus::CommandResponseSent,
    CommandUnknown = igtlioCommandStatus::CommandUnknown,
    CommandWaiting = igtlioCommandStatus::CommandWaiting,
  };

  enum
  {
    CommandCompletedEvent = igtlioCommand::CommandCompletedEvent,
    CommandCancelledEvent = igtlioCommand::CommandCancelledEvent,
    CommandExpiredEvent = igtlioCommand::CommandExpiredEvent,
    CommandReceivedEvent = igtlioCommand::CommandReceivedEvent,
    CommandResponseEvent = igtlioCommand::CommandResponseEvent,
  };

public:
  void SendCommand(vtkMRMLIGTLConnectorNode* connectorNode);

  void SetCommand(igtlioCommand* command);

  virtual void SetName(std::string);
  std::string GetName();

  virtual void SetCommandContent(std::string);
  virtual std::string GetCommandContent();

  void ClearCommandMetaData();
  void SetCommandMetaDataElement(std::string key, std::string value);
  std::string GetCommandMetaDataElement(std::string key);

  virtual void SetResponseContent(std::string);
  virtual std::string GetResponseContent();

  void ClearResponseMetaData();
  void SetResponseMetaDataElement(std::string key, std::string value);
  std::string GetResponseMetaDataElement(std::string key);

  std::map<std::string, std::pair<IANA_ENCODING_TYPE, std::string> > GetCommandMetaData();
  std::map<std::string, std::pair<IANA_ENCODING_TYPE, std::string> > GetResponseMetaData();

  bool GetSuccessful();

  void SetTimeoutSec(double);
  double GetTimeoutSec();

  void SetSentTimestamp(double);
  double GetSentTimestamp();

  void SetBlocking(bool);
  bool GetBlocking();
  vtkBooleanMacro(Blocking, bool);

  int GetCommandStatus();
  void SetCommandStatus(int status);

  bool IsInProgress() { return this->Command->IsInProgress(); };
  bool IsCompleted() { return this->Command->IsCompleted(); };

  std::string GetErrorMessage();
  void SetErrorMessage(std::string);

  igtlioCommand* GetCommand() { return this->Command; };

  void ClearCommand();

protected:
  static void CommandCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

protected:

  vtkSmartPointer<vtkCallbackCommand> Callback;
  igtlioCommandPointer Command;
};

#endif //__vtkSlicerOpenIGTLinkCommand_h
