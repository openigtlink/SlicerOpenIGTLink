/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME vtkSlicerOpenIGTLinkCommand - class for storing an OpenIGTLink command message and response
// .SECTION Description
// This class is used for storing OpenIGTLink command and response data.


#ifndef __vtkSlicerOpenIGTLinkCommand_h
#define __vtkSlicerOpenIGTLinkCommand_h

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// VTK includes
#include <vtkCommand.h>
#include <vtkSmartPointer.h>
#include <vtkXMLDataElement.h>

// OpenIGTLink includes
#include <igtlMessageBase.h>


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkSlicerOpenIGTLinkCommand :
  public vtkObject
{
public:

  enum CommandStatus
  {
    // Command initialization:
    CommandUnknown,
    // Command in progress:
    CommandWaiting,
    // Command completed:
    CommandSuccess,
    CommandFail,
    CommandExpired, // timeout elapsed before command response was received
    CommandCancelled // cancel command was requested before command response was received
  };

  enum CommandDirection
  {
    CommandIn,
    CommandOut
  };

  enum Events
  {
    // vtkCommand::UserEvent + 123 is just a random value that is very unlikely to be used for anything else in this class
    CommandCompletedEvent = vtkCommand::UserEvent + 123
  };

  static vtkSlicerOpenIGTLinkCommand *New();
  vtkTypeMacro(vtkSlicerOpenIGTLinkCommand, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Common command information (both command and response)

  /// Unique identifier of the command.
  vtkSetMacro(ID, std::string);
  vtkGetMacro(ID, std::string);

  /// Unique identifier of the device used by the command.
  vtkGetMacro(DeviceID, std::string);
  vtkSetMacro(DeviceID, std::string);

  /// Unique identifier of the command that is being sent/received.
  vtkSetMacro(QueryID, int);
  vtkGetMacro(QueryID, int);

  /// Status of the command request (Unknown->Waiting->Success/Fail/Expired/Cancelled->Unknown)
  vtkGetMacro(Status, int);
  vtkSetMacro(Status, int);

  vtkGetMacro(CommandVersion, int);
  vtkSetMacro(CommandVersion, int);

  /// Convert status enum to human-readable string
  static std::string StatusToString(int status);

  // Command information

  /// Set command name (required)
  vtkSetMacro(CommandName, std::string);
  vtkGetMacro(CommandName, std::string);

  /// Set optional command attributes
  std::string GetCommandAttribute(const std::string attName);
  void SetCommandAttribute(const std::string attName, const std::string attValue);

  /// Generate command XML string from ID, name, and attributes
  virtual std::string GetCommandText();

  /// Set command name and attributes from an XML string. Returns with true on success.
  virtual bool SetCommandText(const std::string text);

  /// If >0 then commands expires after the specified timeout (state changes from Waiting to Expired).
  /// Default timeout is 10 seconds, as most commands should return with a result immediately.
  vtkGetMacro(CommandTimeoutSec, double);
  vtkSetMacro(CommandTimeoutSec, double);

  /// 
  vtkGetMacro(Blocking, bool);
  vtkSetMacro(Blocking, bool);
  vtkBooleanMacro(Blocking, bool);

  // Direction of the command.
  vtkGetMacro(Direction, int);
  vtkSetMacro(Direction, int);
  void SetDirectionIn() { this->Direction = CommandIn; };
  void SetDirectionOut() { this->Direction = CommandOut; };
  bool IsDirectionIn() { return this->Direction == CommandIn; };
  bool IsDirectionOut() { return this->Direction == CommandOut; };

  // Response information

  /// Get the number of nested elements in the response XML
  int GetNumberOfResponses();

  /// Get the message string from the response (stored in Message attribute)
  std::string GetResponseMessage();

  /// Get custom response attributes
  std::string GetResponseAttribute(const std::string attName);

  /// Get the raw command response text that was set using SetResponseText.
  /// It contains the response text as it received it, so it is valid even if XML parsing of the text failed.
  std::string GetResponseText();
  /// Set the command response from XML. Updates response message, status, and attributes.
  /// In case of XML parsing error, the state is set to CommandFail.
  virtual void SetResponseText(const std::string text);

  /// Get the response as an XML element. Returns NULL if the response text was not set or was invalid.
  vtkXMLDataElement* GetResponseXML() { return this->ResponseXML; };

  /// Returns true if command execution is in progress
  bool IsInProgress();

  /// Returns true if command execution is completed (with either success or failure)
  bool IsCompleted();

  /// Returns true if command execution is completed with success
  bool IsSucceeded();

  /// Returns true if command execution is completed but not successfully
  bool IsFailed();

  void SetMetaDataElement(const std::string key, IANA_ENCODING_TYPE encoding, const std::string value);
  void SetMetaDataElement(const std::string key, const std::string value);
  bool GetMetaDataElement(const std::string key, IANA_ENCODING_TYPE& outEncoding, std::string& outValue);
  igtl::MessageBase::MetaDataMap GetMetaData();
  void ClearMetaData();

  void SetReponseMetaDataElement(const std::string key, IANA_ENCODING_TYPE encoding, const std::string value);
  void SetReponseMetaDataElement(const std::string key, const std::string value);
  bool GetReponseMetaDataElement(const std::string key, IANA_ENCODING_TYPE& outEncoding, std::string& outValue);
  igtl::MessageBase::MetaDataMap GetResponseMetaData() const;
  void ClearResponseMetaData();

protected:
  vtkSlicerOpenIGTLinkCommand();
  virtual ~vtkSlicerOpenIGTLinkCommand();

  /// Helper method for storing the returned command text.
  /// The exported command text is stored as a member variable to allow returning it
  /// as a simple const char pointer
  vtkSetMacro(ExportedCommandText, std::string);

  /// Helper functions for storing the raw response text.
  vtkSetMacro(ResponseTextInternal, std::string);
  vtkGetMacro(ResponseTextInternal, std::string);


private:
  vtkSlicerOpenIGTLinkCommand(const vtkSlicerOpenIGTLinkCommand&); // Not implemented
  void operator=(const vtkSlicerOpenIGTLinkCommand&);               // Not implemented

  // Name of the Command to be sent
  std::string                         CommandName;
  // ID of the vtkSlicerOpenIGTLinkCommand object
  std::string                         ID;
  // ID of the device sending the command
  std::string                         DeviceID;
  // ID of the Query contained in the command device
  int                                 QueryID;

  int                                 CommandVersion;

  int                                 Status;
  double                              CommandTimeoutSec;
  bool                                Blocking;

  vtkSmartPointer<vtkXMLDataElement>  CommandXML;
  vtkSmartPointer<vtkXMLDataElement>  ResponseXML;
  std::string                         ExportedCommandText;
  std::string                         ResponseTextInternal;

  int                                 Direction;

  igtl::MessageBase::MetaDataMap      MetaData;
  igtl::MessageBase::MetaDataMap      ResponseMetaData;
};

#endif
