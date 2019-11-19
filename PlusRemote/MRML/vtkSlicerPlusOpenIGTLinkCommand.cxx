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
#include "vtkSlicerPlusOpenIGTLinkCommand.h"

#include <sstream>

#include <vtkObjectFactory.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>
#include <vtkXMLUtilities.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlusOpenIGTLinkCommand);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkSlicerPlusOpenIGTLinkCommand::vtkSlicerPlusOpenIGTLinkCommand()
  : ID(NULL)
  , CommandXML(NULL)
  , ResponseXML(NULL)
{
  this->SetCommandTimeoutSec(10);
  this->CommandXML = vtkXMLDataElement::New();
  this->CommandXML->SetName("Command");
}

//----------------------------------------------------------------------------
vtkSlicerPlusOpenIGTLinkCommand::~vtkSlicerPlusOpenIGTLinkCommand()
{
  this->SetID(NULL);
  if (this->CommandXML)
  {
    this->CommandXML->Delete();
    this->CommandXML = NULL;
  }
  if (this->ResponseXML)
  {
    this->ResponseXML->Delete();
    this->ResponseXML = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "ID: " << ((this->GetID()) ? this->GetID() : "None") << "\n";
  os << "Status: " << vtkSlicerPlusOpenIGTLinkCommand::StatusToString(this->GetStatus()) << "\n";
  os << "CommandText: " << (!this->GetCommandText().empty() ? this->GetCommandText() : "None") << "\n";
  os << "CommandXML: ";
  if (this->CommandXML)
  {
    this->CommandXML->PrintXML(os, indent.GetNextIndent());
  }
  else
  {
    os << "None" << "\n";
  }
  os << "ResponseText: " << (!this->GetResponseText().empty() ? this->GetResponseText() : "None") << "\n";
  os << "ResponseXML: ";
  if (this->ResponseXML)
  {
    this->ResponseXML->PrintXML(os, indent.GetNextIndent());
  }
  else
  {
    os << "None" << "\n";
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::SetName(std::string name)
{
  this->SetCommandName(name.c_str());
}

//----------------------------------------------------------------------------
const char* vtkSlicerPlusOpenIGTLinkCommand::GetCommandName()
{
  return this->GetCommandAttribute("Name");
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::SetCommandName(const char* name)
{
  this->SetCommandAttribute("Name", name);
  this->Command->SetName(name);
  this->UpdateCommandContent();
}

//----------------------------------------------------------------------------
const char* vtkSlicerPlusOpenIGTLinkCommand::GetCommandAttribute(const char* attName)
{
  return this->CommandXML->GetAttribute(attName);
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::SetCommandAttribute(const char* attName, const char* attValue)
{
  this->CommandXML->SetAttribute(attName, attValue);
  this->UpdateCommandContent();
}

//----------------------------------------------------------------------------
std::string vtkSlicerPlusOpenIGTLinkCommand::GetCommandText()
{
  if (!this->CommandXML)
  {
    return nullptr;
  }
  return this->GetCommandContent();
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::SetCommandContent(std::string content)
{
  this->SetCommandText(content.c_str());
}

//----------------------------------------------------------------------------
bool vtkSlicerPlusOpenIGTLinkCommand::SetCommandText(const char* text)
{
  if (text == NULL)
  {
    vtkErrorMacro("Failed to set OpenIGTLink command from text: empty input");
    return false;
  }

  vtkSmartPointer<vtkXMLDataElement> parsedElem = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(text));
  if (parsedElem == NULL)
  {
    vtkErrorMacro("Failed to set OpenIGTLink command from text: not an XML string: " << text);
    return false;
  }

  this->CommandXML->DeepCopy(parsedElem);
  this->UpdateCommandContent();
  return true;
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::UpdateCommandContent()
{
  if (!this->CommandXML)
  {
    return;
  }
  std::ostringstream os;
  this->CommandXML->PrintXML(os, vtkIndent(0));
  std::string commandContent = os.str();
  this->Command->SetCommandContent(commandContent);
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::UpdateResponseContent()
{
  if (!this->ResponseXML)
  {
    return;
  }
  std::ostringstream os;
  this->ResponseXML->PrintXML(os, vtkIndent(0));
  std::string responseContent = os.str();
  this->Command->SetResponseContent(responseContent);
}

//----------------------------------------------------------------------------
const char* vtkSlicerPlusOpenIGTLinkCommand::GetResponseMessage()
{
  return this->GetResponseAttribute("Message");
}

//----------------------------------------------------------------------------
const char* vtkSlicerPlusOpenIGTLinkCommand::GetResponseAttribute(const char* attName)
{
  if (this->ResponseXML == NULL)
  {
    return NULL;
  }
  return this->ResponseXML->GetAttribute(attName);
}

//----------------------------------------------------------------------------
const std::string vtkSlicerPlusOpenIGTLinkCommand::GetResponseText()
{
  if (!this->ResponseXML)
  {
    return nullptr;
  }
  return this->GetResponseContent();
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::SetResponseContent(std::string content)
{
  this->SetResponseText(content.c_str());
}

//----------------------------------------------------------------------------
void vtkSlicerPlusOpenIGTLinkCommand::SetResponseText(const char* text)
{
  if (this->ResponseXML)
  {
    this->ResponseXML->Delete();
    this->ResponseXML = NULL;
  }

  if (text == NULL)
  {
    SetStatus(CommandFail);
    return;
  }

  this->ResponseXML = vtkXMLUtilities::ReadElementFromString(text);
  if (this->ResponseXML == NULL)
  {
    // The response is not XML
    vtkWarningMacro("OpenIGTLink command response is not XML: " << text);
    this->SetStatus(CommandFail);
    return;
  }

  // Retrieve status from XML string
  if (this->ResponseXML->GetAttribute("Status") == NULL)
  {
    vtkWarningMacro("OpenIGTLink command response: missing Status attribute: " << text);
  }
  else
  {
    if (strcmp(this->ResponseXML->GetAttribute("Status"), "SUCCESS") == 0)
    {
      SetStatus(CommandSuccess);
    }
    else if (strcmp(this->ResponseXML->GetAttribute("Status"), "FAIL") == 0)
    {
      SetStatus(CommandFail);
    }
    else
    {
      vtkErrorMacro("OpenIGTLink command response: invalid Status attribute value: " << this->ResponseXML->GetAttribute("Status"));
      SetStatus(CommandFail);
    }
  }
  this->UpdateResponseContent();
}

//----------------------------------------------------------------------------
const char* vtkSlicerPlusOpenIGTLinkCommand::StatusToString(int status)
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
bool vtkSlicerPlusOpenIGTLinkCommand::IsInProgress()
{
  return this->GetStatus() == CommandWaiting;
}

//----------------------------------------------------------------------------
bool vtkSlicerPlusOpenIGTLinkCommand::IsCompleted()
{
  return
    this->GetStatus() == CommandSuccess ||
    this->GetStatus() == CommandFail ||
    this->GetStatus() == CommandExpired ||
    this->GetStatus() == CommandCancelled;
}

//----------------------------------------------------------------------------
bool vtkSlicerPlusOpenIGTLinkCommand::IsSucceeded()
{
  return this->GetStatus() == CommandSuccess;
}

//----------------------------------------------------------------------------
bool vtkSlicerPlusOpenIGTLinkCommand::IsFailed()
{
  return
    this->GetStatus() == CommandFail ||
    this->GetStatus() == CommandExpired ||
    this->GetStatus() == CommandCancelled;
}