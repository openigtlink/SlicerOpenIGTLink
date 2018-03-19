#include "vtkSlicerOpenIGTLinkCommand.h"

#include <sstream>

#include <vtkObjectFactory.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>
#include <vtkXMLUtilities.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerOpenIGTLinkCommand);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkSlicerOpenIGTLinkCommand::vtkSlicerOpenIGTLinkCommand()
: CommandName(NULL)
, ID(NULL)
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
vtkSlicerOpenIGTLinkCommand::~vtkSlicerOpenIGTLinkCommand()
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
void vtkSlicerOpenIGTLinkCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "ID: " << ( (this->GetID()) ? this->GetID() : "None" ) << "\n";
  os << "Status: " << vtkSlicerOpenIGTLinkCommand::StatusToString(this->GetStatus()) << "\n";
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
const char* vtkSlicerOpenIGTLinkCommand::GetCommandAttribute(const char* attName)
{
  return this->CommandXML->GetAttribute(attName);
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetCommandAttribute(const char* attName, const char* attValue)
{
  this->CommandXML->SetAttribute(attName, attValue);
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkCommand::GetCommandText()
{
  std::ostringstream os;
  this->CommandXML->PrintXML(os, vtkIndent(0));
  this->SetExportedCommandText(os.str().c_str());
  return this->ExportedCommandText;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::SetCommandText(const char* text)
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
int vtkSlicerOpenIGTLinkCommand::GetNumberOfResponses()
{
  int numberOfNestedElements = this->ResponseXML->GetNumberOfNestedElements();
  return numberOfNestedElements;
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkCommand::GetResponseMessage(int responseID/*=0*/)
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
    vtkErrorMacro("Could not find requested command response: responseID is out of range (Number of responses is " << numberOfNestedElements << ")");
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
const char* vtkSlicerOpenIGTLinkCommand::GetResponseAttribute(const char* attName)
{
  if (this->ResponseXML==NULL)
    {
    return NULL;
    }
  return this->ResponseXML->GetAttribute(attName);
}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkCommand::GetResponseText()
{
  return this->ResponseTextInternal;
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetResponseText(const char* text)
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

}

//----------------------------------------------------------------------------
const char* vtkSlicerOpenIGTLinkCommand::StatusToString(int status)
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
bool vtkSlicerOpenIGTLinkCommand::IsInProgress()
{
  return this->Status == CommandWaiting;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::IsCompleted()
{
  return
    this->Status == CommandSuccess ||
    this->Status == CommandFail ||
    this->Status == CommandExpired ||
    this->Status == CommandCancelled;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::IsSucceeded()
{
  return this->Status == CommandSuccess;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::IsFailed()
{
  return
    this->Status == CommandFail ||
    this->Status == CommandExpired ||
    this->Status == CommandCancelled;
}
