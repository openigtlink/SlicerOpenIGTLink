#include "vtkSlicerConfigure.h" 

//OpenIGTLink includes
#include "igtlOSUtil.h"
#include "igtlioCommandDevice.h"

// IF module includes
#include "vtkMRMLIGTLConnectorNode.h"

// VTK includes
#include <vtkTimerLog.h>
#include <vtksys/SystemTools.hxx>
#include <vtkSmartPointer.h>
#include <vtkObject.h>
#include <vtkWeakPointer.h>
#include <vtkCallbackCommand.h>

static int testSuccessful = 0;
static std::string ResponseString = "<Command>\n <Result success=true> <Parameter Name=”Depth” /> </Result>\n </Command>";

void onCommandReceivedEventFunc(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);
  igtlio::CommandDevice* clientDevice = reinterpret_cast<igtlio::CommandDevice*>(calldata);
  connectorNode->SendCommandResponse(clientDevice->GetDeviceName(), "Get", ResponseString);
  std::cout << "*** COMMAND received from client" << std::endl;
  testSuccessful +=1;
}

void onCommanResponseReceivedEventFunc(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  std::cout << "*** COMMAND response received from server" << std::endl;
  igtlio::CommandDevice* serverDevice = reinterpret_cast<igtlio::CommandDevice*>(calldata);
  std::vector<igtlio::CommandDevice::QueryType> queries = serverDevice->GetQueries();
  if (queries.size()==1)
    {
    igtlio::CommandDevicePointer commandDevice = reinterpret_cast<igtlio::CommandDevice*>(queries[0].Response.GetPointer());
    std::cout<<commandDevice->GetContent().content;
    if (ResponseString.compare(commandDevice->GetContent().content) == 0)
      {
      testSuccessful +=1;
      }
    }
}

class CommandObserver:vtkObject
{
public:
  static CommandObserver *New(){
  VTK_STANDARD_NEW_BODY(CommandObserver);
  };
  vtkTypeMacro(CommandObserver, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE
  {
    vtkObject::PrintSelf(os, indent);
  };
  
  vtkSmartPointer<class vtkCallbackCommand> CommandReceivedEventCallback;
  vtkSmartPointer<class vtkCallbackCommand> CommandResponseReceivedEventCallback;
protected:
  CommandObserver()
  {
    CommandReceivedEventCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    CommandReceivedEventCallback->SetCallback(onCommandReceivedEventFunc);
    CommandReceivedEventCallback->SetClientData(this);
    CommandResponseReceivedEventCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    CommandResponseReceivedEventCallback->SetCallback(onCommanResponseReceivedEventFunc);
    CommandResponseReceivedEventCallback->SetClientData(this);
  };
  ~CommandObserver(){};
};

int main(int argc, char * argv [] )
{
  // Setup the Server and client, as well as the event observers.
  vtkMRMLIGTLConnectorNode * serverConnectorNode = vtkMRMLIGTLConnectorNode::New();
  CommandObserver* commandServerObsever = CommandObserver::New();
  serverConnectorNode->AddObserver(serverConnectorNode->CommandReceivedEvent, commandServerObsever->CommandReceivedEventCallback);
  // The connector type, server port, and etc,  are set by the qSlicerIGTLConnectorPropertyWidget
  // To make the test simple, just set the port directly.
  serverConnectorNode->SetTypeServer(18944);
  serverConnectorNode->Start();
  igtl::Sleep(20);
  vtkMRMLIGTLConnectorNode * clientConnectorNode = vtkMRMLIGTLConnectorNode::New();
  CommandObserver* commandClientObsever = CommandObserver::New();
  clientConnectorNode->AddObserver(clientConnectorNode->CommandResponseReceivedEvent, commandClientObsever->CommandResponseReceivedEventCallback);
  clientConnectorNode->SetTypeClient("localhost", 18944);
  clientConnectorNode->Start();
  
  // Make sure the server and client are connected.
  double timeout = 5;
  double starttime = vtkTimerLog::GetUniversalTime();

  // Client connects to server.
  while (vtkTimerLog::GetUniversalTime() - starttime < timeout)
  {
    serverConnectorNode->PeriodicProcess();
    clientConnectorNode->PeriodicProcess();
    vtksys::SystemTools::Delay(5);

    if (clientConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::StateConnected)
    {
      std::cout << "SUCCESS: connected to server" << std::endl;
      break;
    }
    if (clientConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::StateOff)
    {
      std::cout << "FAILURE to connect to server" << std::endl;
      return false;
    }
  }
  
  std::string device_name = "TestDevice";
  clientConnectorNode->SendCommand(device_name,"Get", "<Command>\n <Parameter Name=\"Depth\" />\n </Command>", false);
  
  // Make sure the Server receive the command message.
  starttime = vtkTimerLog::GetUniversalTime();
  while (vtkTimerLog::GetUniversalTime() - starttime < timeout)
    {
    serverConnectorNode->PeriodicProcess();
    vtksys::SystemTools::Delay(5);
    }
  
  // Make sure the Client receive the response message.
  starttime = vtkTimerLog::GetUniversalTime();
  while (vtkTimerLog::GetUniversalTime() - starttime < timeout)
    {
    clientConnectorNode->PeriodicProcess();
    vtksys::SystemTools::Delay(5);
    }
  clientConnectorNode->Stop();
  serverConnectorNode->Stop();
  
  //Condition only holds when both onCommandReceivedEventFunc and onCommanResponseReceivedEventFunc are called.
  if (testSuccessful==2)
    {
    return true;
    }
  return false;
}