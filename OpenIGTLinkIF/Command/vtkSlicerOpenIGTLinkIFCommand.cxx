#include "vtkSlicerOpenIGTLinkIFCommand.h"

#include <sstream>

#include <vtkObjectFactory.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>
#include <vtkXMLUtilities.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerOpenIGTLinkIFCommand);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFCommand::vtkSlicerOpenIGTLinkIFCommand()
: ID(NULL)
, DeviceID(NULL)
, CommandXML(NULL)
, CommandTimeoutSec(10)
, ExportedCommandText(NULL)
, ResponseXML(NULL)
, ResponseTextInternal(NULL)
, Status(CommandUnknown)
, Direction(CommandOut)
, Blocking(true)
{
  this->SetID("");
  this->SetDeviceID("");

  this->CommandXML = vtkXMLDataElement::New();
  this->CommandXML->SetName("Command");
}

//----------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFCommand::~vtkSlicerOpenIGTLinkIFCommand()
{
  this->SetID(NULL);
  if (this->CommandXML)
    {
    this->CommandXML->Delete();
    this->CommandXML=NULL;
    }
  this->SetExportedCommandText(NULL);
  if (this->ResponseXML)
    {
    this->ResponseXML->Delete();
    this->ResponseXML=NULL;
    }
  this->SetResponseTextInternal(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "ID: " << ( (this->GetID()) ? this->GetID() : "None" ) << "\n";
  os << "Status: " << vtkSlicerOpenIGTLinkIFCommand::StatusToString(this->GetStatus()) << "\n";
  os << "CommandText: " << ( (this->GetCommandText()) ? this->GetCommandText() : "None" ) << "\n";
  os << "CommandXML: ";
  if (this->CommandXML)
    {
    this->CommandXML->PrintXML(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << "\n";
    }
  os << "ResponseText: " << ( (this->GetResponseText()) ? this->GetResponseText() : "None" ) << "\n";
  os << "ResponseXML: ";
  if (this->ResponseXML)
    {
    this->ResponseXML->PrintXML(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << "\n";
    }
  os << "DeviceId: " << this->GetDeviceID() << "\n";
  os << "CommandId: " << this->GetQueryID() << "\n";
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkIFCommand::GetCommandName()
{
  return this->GetCommandAttribute("Name");
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFCommand::SetCommandName(const char* name)
{
  this->SetCommandAttribute("Name", name);
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkIFCommand::GetCommandAttribute(const char* attName)
{
  return this->CommandXML->GetAttribute(attName);
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFCommand::SetCommandAttribute(const char* attName, const char* attValue)
{
  this->CommandXML->SetAttribute(attName, attValue);
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkIFCommand::GetCommandText()
{
  std::ostringstream os;
  this->CommandXML->PrintXML(os, vtkIndent(0));
  this->SetExportedCommandText(os.str().c_str());
  return this->ExportedCommandText;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkIFCommand::SetCommandText(const char* text)
{
  if (text==NULL)
    {
    vtkErrorMacro("Failed to set OpenIGTLink command from text: empty input");
    return false;
    }

  vtkSmartPointer<vtkXMLDataElement> parsedElem = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(text));
  if (parsedElem == NULL)
  {
    vtkErrorMacro("Failed to set OpenIGTLink command from text: not an XML string: "<<text);
    return false;
  }

  this->CommandXML->DeepCopy(parsedElem);
  return true;
}

//----------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkIFCommand::GetNumberOfResponses()
{
  int numberOfNestedElements = this->ResponseXML->GetNumberOfNestedElements();
  return numberOfNestedElements;
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkIFCommand::GetResponseMessage(int responseID/*=0*/)
{
  if (this->ResponseXML == nullptr)
    {
    return "";
    }

  if (responseID == 0)
    {
    // We might expect the response message to be stored in XML as:
    // <Command Message="MESSAGE_TEXT"></Command>
    const char* responseAttribute = this->GetResponseAttribute("Message");
    if (responseAttribute)
      {
      return responseAttribute;
      }
    }

  // If responseID is > 0 or if above is not true, look instead for:
  // <Command><Response Success="true/false" Message="MESSAGE_TEXT"></Response></Command>
  int numberOfNestedElements = this->ResponseXML->GetNumberOfNestedElements();
  if (responseID >= numberOfNestedElements)
    {
    vtkErrorMacro("Could not find requested command response: responseID is out of range (Number of responses is" << numberOfNestedElements << ")");
    return "";
    }

  vtkXMLDataElement* nestedElement = this->ResponseXML->GetNestedElement(responseID);
  if (!nestedElement)
    {
    vtkErrorMacro("Could not find requested command response");
    return "";
    }

  const char* message = nestedElement->GetAttribute("Message");
  if (message != nullptr)
    {
    return nestedElement->GetCharacterData();
    }

  vtkErrorMacro("Could not find response message");
  return "";
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkIFCommand::GetResponseAttribute(const char* attName)
{
  if (this->ResponseXML==NULL)
    {
    return NULL;
    }
  return this->ResponseXML->GetAttribute(attName);
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkIFCommand::GetResponseText()
{
  return this->ResponseTextInternal;
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFCommand::SetResponseText(const char* text)
{
  this->SetResponseTextInternal(text);

  if (this->ResponseXML)
    {
    this->ResponseXML->Delete();
    this->ResponseXML=NULL;
    }

  if (text==NULL)
  {
    this->SetStatus(CommandFail);
    return;
  }

  this->ResponseXML = vtkXMLUtilities::ReadElementFromString(text);
  if (this->ResponseXML == NULL)
  {
    // The response is not XML
    vtkWarningMacro("OpenIGTLink command response is not XML: "<<text);
    this->SetStatus(CommandFail);
    return;
  }

  // Retrieve status from XML string
  const char* status = this->ResponseXML->GetAttribute("Status");
  if (status == nullptr)
  {
    vtkWarningMacro("OpenIGTLink command response: missing Status attribute: " << text);
  }
  else
  {
    if (strcmp(status, "SUCCESS") == 0)
    {
      this->SetStatus(CommandSuccess);
    }
    else if (strcmp(status, "FAIL") == 0)
    {
      this->SetStatus(CommandFail);
    }
    else
    {
      vtkErrorMacro("OpenIGTLink command response: invalid Status attribute value: " << this->ResponseXML->GetAttribute("Success"));
      this->SetStatus(CommandFail);
    }
  }
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkIFCommand::StatusToString(int status)
{
  switch (status)
  {
  case CommandUnknown: return "Unknown";
  case CommandWaiting: return "Waiting";
  case CommandSuccess: return "Success";
  case CommandFail: return "Fail";
  case CommandExpired: return "Expired";
  case CommandCancelled: return "Cancelled";
  default:
    return "Invalid";
  }
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkIFCommand::IsInProgress()
{
  return this->Status == CommandWaiting;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkIFCommand::IsCompleted()
{
  return
    this->Status == CommandSuccess ||
    this->Status == CommandFail ||
    this->Status == CommandExpired ||
    this->Status == CommandCancelled;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkIFCommand::IsSucceeded()
{
  return this->Status == CommandSuccess;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkIFCommand::IsFailed()
{
  return
    this->Status == CommandFail ||
    this->Status == CommandExpired ||
    this->Status == CommandCancelled;
}
