#include "vtkSlicerConfigure.h" 

//OpenIGTLink includes
#include "igtlOSUtil.h"
#include "igtlioDevice.h"

// IF module includes
#include "vtkMRMLIGTLConnectorNode.h"

// VTK includes
#include <vtkTimerLog.h>
#include <vtksys/SystemTools.hxx>
#include <vtkSmartPointer.h>
#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkWeakPointer.h>
#include <vtkCallbackCommand.h>
#include "vtkMRMLBitStreamNode.h"
#include "vtkImageData.h"
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkTestingOutputWindow.h"

class VideoObserver: public vtkObject
{
public:
  static VideoObserver *New(){
  VTK_STANDARD_NEW_BODY(VideoObserver);
  };
  vtkTypeMacro(VideoObserver, vtkObject);
  ~VideoObserver(){};
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE
  {
    vtkObject::PrintSelf(os, indent);
  };
  void onVideoReceivedEventFunc(vtkObject* caller, unsigned long eid,  void *calldata)
  {
    std::cout << "*** Video received from server" << std::endl;
    testSuccessful +=1;
  };
  int testSuccessful;
  vtkSmartPointer<vtkImageData> CreateTestImage()
  {
    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    image->SetSpacing(1.5, 1.2, 1);
    image->SetExtent(0, 19, 0, 49, 0, 1);
    image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

    int scalarSize = image->GetScalarSize();
    unsigned char* ptr = reinterpret_cast<unsigned char*>(image->GetScalarPointer());
    unsigned char color = 0;
    std::fill(ptr, ptr+scalarSize, color++);

    return image;
  };
protected:
  VideoObserver()
  {
  testSuccessful = 0;
  };
};

int vtkMRMLConnectorVideoSendAndReceiveTest(int argc, char * argv [] )
{
  vtkSmartPointer<vtkMRMLScene> scene = vtkMRMLScene::New();
  // Setup the Server and client, as well as the event observers.
  vtkMRMLIGTLConnectorNode * serverConnectorNode = vtkMRMLIGTLConnectorNode::New();
  // The connector type, server port, and etc,  are set by the qSlicerIGTLConnectorPropertyWidget
  // To make the test simple, just set the port directly.
  serverConnectorNode->SetTypeServer(18946);
  serverConnectorNode->Start();
  serverConnectorNode->SetScene(scene);
  igtl::Sleep(20);
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> clientConnectorNode = vtkMRMLIGTLConnectorNode::New();
  vtkSmartPointer<VideoObserver> videoClientObsever = VideoObserver::New();
  clientConnectorNode->AddObserver(clientConnectorNode->NewDeviceEvent, videoClientObsever, &VideoObserver::onVideoReceivedEventFunc);
  clientConnectorNode->SetTypeClient("localhost", 18946);
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
      clientConnectorNode->Stop();
      serverConnectorNode->Stop();
      clientConnectorNode->Delete();
      serverConnectorNode->Delete();
      scene->Delete();
      return EXIT_FAILURE;
    }
  }
  std::string device_name = "TestDevice";
  vtkSmartPointer<vtkImageData> testImage = videoClientObsever->CreateTestImage();
  vtkSmartPointer<vtkMRMLBitStreamNode> volumeNode = vtkSmartPointer<vtkMRMLBitStreamNode>::New();
  volumeNode->SetAndObserveImageData(testImage);
  scene->AddNode(volumeNode);
  igtlio::DevicePointer videoDevice = reinterpret_cast<igtlio::Device*>(serverConnectorNode->CreateDeviceForOutgoingMRMLNode(volumeNode));
  if (strcmp(videoDevice->GetDeviceType().c_str(), "VIDEO")!=0)
    {
    clientConnectorNode->Stop();
    serverConnectorNode->Stop();
    clientConnectorNode->Delete();
    serverConnectorNode->Delete();
    scene->Delete();
    return EXIT_FAILURE;
    }
  serverConnectorNode->PushNode(volumeNode);
  
  // Make sure the Client receive the response message.
  starttime = vtkTimerLog::GetUniversalTime();
  while (vtkTimerLog::GetUniversalTime() - starttime < timeout)
    {
    clientConnectorNode->PeriodicProcess();
    vtksys::SystemTools::Delay(5);
    }
  vtksys::SystemTools::Delay(5000);
  clientConnectorNode->Stop();
  serverConnectorNode->Stop();
  clientConnectorNode->Delete();
  serverConnectorNode->Delete();
  scene->Delete();
  //Condition only holds when both onCommandReceivedEventFunc and onCommanResponseReceivedEventFunc are called.
  if (videoClientObsever->testSuccessful==1)
    {
    videoClientObsever->Delete();
    return EXIT_SUCCESS;
    }
  videoClientObsever->Delete();
  return EXIT_FAILURE;
}
