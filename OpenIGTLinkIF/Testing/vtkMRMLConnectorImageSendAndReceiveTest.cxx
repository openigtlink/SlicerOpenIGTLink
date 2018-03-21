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
#include <vtkMRMLVectorVolumeNode.h>
#include "vtkImageData.h"
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkTestingOutputWindow.h"


class ImageObserver: public vtkObject
{
public:
  static ImageObserver *New(){
  VTK_STANDARD_NEW_BODY(ImageObserver);
  };
  vtkTypeMacro(ImageObserver, vtkObject);
  ~ImageObserver(){};
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE
  {
    vtkObject::PrintSelf(os, indent);
  };
  void onImagedReceivedEventFunc(vtkObject* caller, unsigned long eid,  void *calldata)
  {
    std::cout << "*** Image received from server" << std::endl;
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
  ImageObserver()
  {
  testSuccessful = 0;
  };
};

int vtkMRMLConnectorImageSendAndReceiveTest(int argc, char * argv [] )
{
  vtkSmartPointer<vtkMRMLScene> scene = vtkMRMLScene::New();
  // Setup the Server and client, as well as the event observers.
  vtkMRMLIGTLConnectorNode * serverConnectorNode = vtkMRMLIGTLConnectorNode::New();
  // The connector type, server port, and etc,  are set by the qSlicerIGTLConnectorPropertyWidget
  // To make the test simple, just set the port directly.
  serverConnectorNode->SetTypeServer(18945);
  serverConnectorNode->Start();
  serverConnectorNode->SetScene(scene);
  igtl::Sleep(20);
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> clientConnectorNode = vtkMRMLIGTLConnectorNode::New();
  vtkSmartPointer<ImageObserver> imageClientObsever = ImageObserver::New();
  clientConnectorNode->AddObserver(clientConnectorNode->NewDeviceEvent, imageClientObsever, &ImageObserver::onImagedReceivedEventFunc);
  clientConnectorNode->SetTypeClient("localhost", 18945);
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
  vtkSmartPointer<vtkImageData> testImage = imageClientObsever->CreateTestImage();
  vtkSmartPointer<vtkMRMLVectorVolumeNode> volumeNode = vtkSmartPointer<vtkMRMLVectorVolumeNode>::New();
  volumeNode->SetAndObserveImageData(testImage);
  scene->AddNode(volumeNode);
  igtlio::DevicePointer imageDevice = reinterpret_cast<igtlio::Device*>(serverConnectorNode->CreateDeviceForOutgoingMRMLNode(volumeNode));
  if (strcmp(imageDevice->GetDeviceType().c_str(), "IMAGE")!=0)
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
  if (imageClientObsever->testSuccessful==1)
    {
    imageClientObsever->Delete();
    return EXIT_SUCCESS;
    }
  imageClientObsever->Delete();
  return EXIT_FAILURE;
}
