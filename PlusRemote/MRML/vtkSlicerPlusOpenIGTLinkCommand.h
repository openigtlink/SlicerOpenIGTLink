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

#ifndef __vtkSlicerPlusOpenIGTLinkCommand_h
#define __vtkSlicerPlusOpenIGTLinkCommand_h

// OpenIGTLinkIF includes
#include "vtkSlicerOpenIGTLinkCommand.h"

// PlusRemote includes
#include "vtkSlicerPlusRemoteModuleMRMLExport.h"

// VTK includes
#include "vtkCommand.h"

class vtkXMLDataElement;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLUSREMOTE_MODULE_MRML_EXPORT vtkSlicerPlusOpenIGTLinkCommand : public vtkSlicerOpenIGTLinkCommand
{
public:
  
  enum StatusValues
  {
    // Command initialization:
    CommandUnknown = vtkSlicerOpenIGTLinkCommand::CommandUnknown,
    // Command in progress:
    CommandWaiting = vtkSlicerOpenIGTLinkCommand::CommandWaiting,
    // Command completed:
    CommandSuccess = vtkSlicerOpenIGTLinkCommand::CommandResponseReceived,
    CommandFail = vtkSlicerOpenIGTLinkCommand::CommandFailed,
    CommandExpired = vtkSlicerOpenIGTLinkCommand::CommandExpired, // timeout elapsed before command response was received
    CommandCancelled = vtkSlicerOpenIGTLinkCommand::CommandCancelled // cancel command was requested before command response was received
  };

  static vtkSlicerPlusOpenIGTLinkCommand *New();
  vtkTypeMacro(vtkSlicerPlusOpenIGTLinkCommand, vtkSlicerOpenIGTLinkCommand);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Common command information (both command and response)

  /// Unique identifier of the command. It is used for generating the OpenIGTLink STRING command ID (CMD_[ID])
  vtkGetStringMacro(ID);
  vtkSetStringMacro(ID);

  /// Status of the command request (Unknown->Waiting->Success/Fail/Expired/Cancelled->Unknown)
  int GetStatus() { return this->GetCommandStatus(); };
  void SetStatus(int status) { this->SetCommandStatus(status); };

  /// Convert status enum to human-readable string
  static const char* StatusToString(int status);
  
  // Command information
  virtual void SetName(std::string) override;

  /// Set command name (required)
  const char* GetCommandName();
  void SetCommandName(const char* name);

  /// Set optional command attributes
  const char* GetCommandAttribute(const char* attName);
  void SetCommandAttribute(const char* attName, const char* attValue);

  /// Generate command XML string from ID, name, and attributes
  virtual std::string GetCommandText();

  /// Set the command content
  virtual void SetCommandContent(std::string) override;

  /// Set command name and attributes from an XML string. Returns with true on success.
  virtual bool SetCommandText(const char* text);

  /// If >0 then commands expires after the specified timeout (state changes from Waiting to Expired).
  // Default timeout is 10 seconds, as most commands should return with a result immediately.
  double GetCommandTimeoutSec() { return this->GetTimeoutSec(); };
  void SetCommandTimeoutSec(double timeoutSec) { this->SetTimeoutSec(timeoutSec); };

  // Response information

  /// Get the message string from the response (stored in Message attribute)
  const char* GetResponseMessage();

  /// Get custom response attributes
  const char* GetResponseAttribute(const char* attName);
  
  /// Set the response content
  virtual void SetResponseContent(std::string) override;

  /// Get the raw command response text that was set using SetResponseText.
  /// It contains the response text as it received it, so it is valid even if XML parsing of the text failed.
  const std::string GetResponseText();
  /// Set the command response from XML. Updates response message, status, and attributes.
  /// In case of XML parsing error, the state is set to CommandFail.
  virtual void SetResponseText(const char* text);
  
  /// Get the response as an XML element. Returns NULL if the response text was not set or was invalid.
  vtkGetMacro(ResponseXML, vtkXMLDataElement*);

  /// Returns true if command execution is in progress
  bool IsInProgress();

  /// Returns true if command execution is completed (with either success or failure)
  bool IsCompleted();

  /// Returns true if command execution is completed with success
  bool IsSucceeded();

  /// Returns true if command execution is completed but not successfully
  bool IsFailed();
    
protected:
  vtkSlicerPlusOpenIGTLinkCommand();
  virtual ~vtkSlicerPlusOpenIGTLinkCommand();

  void UpdateCommandContent();
  void UpdateResponseContent();

private:
  vtkSlicerPlusOpenIGTLinkCommand(const vtkSlicerPlusOpenIGTLinkCommand&); // Not implemented
  void operator=(const vtkSlicerPlusOpenIGTLinkCommand&);               // Not implemented

  char* ID;
  vtkXMLDataElement* CommandXML;
  vtkXMLDataElement* ResponseXML;
};

#endif