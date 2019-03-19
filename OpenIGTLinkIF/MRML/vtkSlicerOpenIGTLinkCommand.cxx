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

// VTK includes
#include <vtkCallbackCommand.h>

// OpenIGTLinkIF includes
#include <vtkSlicerOpenIGTLinkCommand.h>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerOpenIGTLinkCommand);

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkCommand::vtkSlicerOpenIGTLinkCommand()
  : Callback(vtkSmartPointer<vtkCallbackCommand>::New())
  , Command(igtlioCommandPointer::New())
{
  this->Callback->SetCallback(vtkSlicerOpenIGTLinkCommand::CommandCallback);
  this->Callback->SetClientData(this);
  this->ClearCommand();
}

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkCommand::~vtkSlicerOpenIGTLinkCommand()
{
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::ClearCommand()
{
  if (this->Command)
  {
    this->Command->RemoveObserver(this->Callback);
  }
  this->Command = igtlioCommandPointer::New();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SendCommand(vtkMRMLIGTLConnectorNode* connectorNode)
{
  if (!connectorNode->HasObserver(igtlioCommand::CommandCancelledEvent, this->Callback))
  {
    this->Command->AddObserver(igtlioCommand::CommandCancelledEvent, this->Callback);
    this->Command->AddObserver(igtlioCommand::CommandCompletedEvent, this->Callback);
    this->Command->AddObserver(igtlioCommand::CommandExpiredEvent, this->Callback);
    this->Command->AddObserver(igtlioCommand::CommandReceivedEvent, this->Callback);
    this->Command->AddObserver(igtlioCommand::CommandResponseEvent, this->Callback);
  }

  connectorNode->SendCommand(this->Command);
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::CommandCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata)
{
  vtkSlicerOpenIGTLinkCommand* self = static_cast<vtkSlicerOpenIGTLinkCommand*>(clientdata);
  self->InvokeEvent(eid);
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetName(std::string name)
{
  this->Command->SetName(name);
}

//---------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetName()
{
  return this->Command->GetName();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetCommandContent(std::string content)
{
  this->Command->SetCommandContent(content);
}

//---------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetCommandContent()
{
  return this->Command->GetCommandContent();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::ClearCommandMetaData()
{
  this->Command->ClearCommandMetaData();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetCommandMetaDataElement(std::string key, std::string value)
{
  this->Command->SetCommandMetaDataElement(key, value);
}

//---------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetCommandMetaDataElement(std::string key)
{
  IANA_ENCODING_TYPE encodingType;
  std::string outValue;
  this->Command->GetCommandMetaDataElement(key, outValue, encodingType);
  return outValue;
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetResponseContent(std::string content)
{
  this->Command->SetResponseContent(content);
}

//---------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetResponseContent()
{
  return this->Command->GetResponseContent();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::ClearResponseMetaData()
{
  this->Command->ClearResponseMetaData();
}

//---------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetResponseMetaDataElement(std::string key)
{
  IANA_ENCODING_TYPE encodingType;
  std::string outValue;
  this->Command->GetResponseMetaDataElement(key, outValue, encodingType);
  return outValue;
}

//---------------------------------------------------------------------------
std::map<std::string, std::pair<IANA_ENCODING_TYPE, std::string> > vtkSlicerOpenIGTLinkCommand::GetCommandMetaData()
{
  return this->Command->GetCommandMetaData();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetResponseMetaDataElement(std::string key, std::string value)
{
  this->Command->SetResponseMetaDataElement(key, value);
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetTimeoutSec(double timeoutSec)
{
  this->Command->SetTimeoutSec(timeoutSec);
}

//---------------------------------------------------------------------------
double vtkSlicerOpenIGTLinkCommand::GetTimeoutSec()
{
  return this->Command->GetTimeoutSec();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetSentTimestamp(double timeoutSec)
{
  this->Command->SetSentTimestamp(timeoutSec);
}

//---------------------------------------------------------------------------
double vtkSlicerOpenIGTLinkCommand::GetSentTimestamp()
{
  return this->Command->GetSentTimestamp();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetBlocking(bool blocking)
{
  this->Command->SetBlocking(blocking);
}

//---------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::GetBlocking()
{
  return this->Command->GetBlocking();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetCommandStatus(int status)
{
  return this->Command->SetStatus((igtlioCommandStatus)status);
}

//---------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkCommand::GetCommandStatus()
{
  return this->Command->GetStatus();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetErrorMessage(std::string error)
{
  return this->Command->SetErrorMessage(error);
}

//---------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetErrorMessage()
{
  return this->Command->GetErrorMessage();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  this->Command->PrintSelf(os, indent);
}
