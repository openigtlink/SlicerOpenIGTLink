![Alt text](OpenIGTLinkIF.png)

### Overview

SlicerOpenIGTLink is a [3D Slicer](http://slicer.org) extension designed for faciliate the communication between 3D Slicer and other platform via [OpenIGTLink](http://openigtlink.org/).
![Alt text](Screenshots/Overview.png?raw=true "Overview")
### Usage:
1. After install the extension, the user need to switch to the module `OpenIGTLinkIF`.

2. Add a connector by pressing the '+' button. The connector represents the connection between the server and the client. Note that the communication is bi-direction, both server and client can receive data and send data to each other.
![Alt text](Screenshots/AddConnector.png?raw=true "Add Connector")

3. Set the properties of the connector in the 'Properties' setting box. If the connector is of client type, the IP address should be entered in the "Hostname" field. The 'Port' field should be higher than 1024, as port numbers 0 to 1024 are reserved for privileged services and designated as well-known ports.
![Alt text](Screenshots/Properties.png?raw=true "Set Properties")

4. Activate the server or client by checking the checkbox 'Active'. The status of the connector will be updated if a connection is established or disconnected.
![Alt text](Screenshots/Activate.png?raw=true "Activate")

5. In the I/O Configuration collapsible button, users can check the recieved data from or send local mrmlnode to server/client.
![Alt text](Screenshots/IOConfiguration.png?raw=true "I/O Configuration")

6. Depends on the communication design of the servers/clients, some might need to send a query or command message to trigger the data communication. 
  6.1 Here is an example of receiving images from an imager server 
* Start the 'ImagerServer' in OpenIGTLink example programs using the following command.
  ~~~~
  $ ./ImagerServer 18944 5 PathTo/OpenIGTLink/Examples/Imager/img
  ~~~~   
  In Slicer, once the client is activated, the image will be transferred to Slicer automatically.
  6.2 Here is an example of receiving images from an imager server using vtkMRMLIGTLQueryNode.
* Start the 'ImagerServer3' in OpenIGTLink example programs using the following command.
  ~~~~
  $ ./ImagerServer3 18944 5 PathTo/OpenIGTLink/Examples/Imager/img
  ~~~~ 
  In Slicer, open the python interactor and type the following commands:
  ~~~~ 
  >>> a = slicer.vtkMRMLIGTLQueryNode()
  >>> a.SetQueryType(a.TYPE_GET)
  >>> a.SetIGTLName("IMAGE")
  >>> b = slicer.mrmlScene.GetNodeByID("vtkMRMLIGTLConnectorNode1") #replace the ID with the correct connector node ID 
  >>> b.PushQuery(a)
  ~~~~ 
  The image could be shown in the Slicer viewers.
  
### Disclaimer

OpenIGTLinkIF, same as 3D Slicer, is a research software. **OpenIGTLinkIF is NOT an FDA-approved medical device**. It is not intended for clinical use. The user assumes full responsibility to comply with the appropriate regulations.  

### Support

Please feel free to contact us for questions, feedback, suggestions, bugs, or you can create issues in the issue tracker: https://github.com/openigtlink/SlicerOpenIGTLink/issues

* [Junichi Tokuda](https://github.com/tokjun) tokuda@bwh.harvard.edu

* [Longquan Chen](https://github.com/leochan2009) lchen@bwh.harvard.edu

### Acknowledgments

Development of OpenIGTLinkIF is supported in part by the following NIH grants: 
* R01 EB020667 OpenIGTLink: a network communication interface for closed-loop image-guided interventions
* P41 EB015898 National Center for Image Guided Therapy (NCIGT), http://ncigt.org


### References

1. Tokuda J, Fischer GS, Papademetris X, Yaniv Z, Ibanez L, Cheng P, Liu H, Blevins J, Arata J, Golby AJ, Kapur T, Pieper S, Burdette EC, Fichtinger G, Tempany CM, Hata N. OpenIGTLink: an open network protocol for image-guided therapy environment. Int J Med Robot. 2009 Dec;5(4):423-34.
2. Arata J, Kozuka H, Kim HW, Takesue N, Vladimirov B, Sakaguchi M, Tokuda J, Hata N, Chinzei K, Fujimoto H. Open core control software for surgical robots. Int J Comput Assist Radiol Surg. 2010 May;5(3):211-20.
3. Tokuda J, Fischer GS, DiMaio SP, Gobbi DG, Csoma C, Mewes PW, Fichtinger G, Tempany CM, Hata N. Integrated navigation and control software system for MRI-guided robotic prostate interventions. Comput Med Imaging Graph. 2010 Jan;34(1):3-8.
4. Elhawary H, Liu H, Patel P, Norton I, Rigolo L, Papademetris X, Hata N, Golby AJ. Intraoperative real-time querying of white matter tracts during frameless stereotactic neuronavigation. Neurosurgery. 2011 Feb;68(2):506-16.
5. Egger J, Tokuda J, Chauvin L, Freisleben B, Nimsky C, Kapur T, Wells W. Integration of the OpenIGTLink Network Protocol for image-guided therapy with the medical platform MeVisLab, Int J Med Robot. 2012 Sep;8(3):282-90.
6. Tauscher S, Tokuda J, Schreiber G, Neff T, Hata N, Ortmaier T. OpenIGTLink interface for state control and visualisation of a robot for image-guided therapy systems. Int J Comput Assist Radiol Surg. 2015 Mar;10(3):285-92.
7. Frank T, Krieger A, Leonard S, Patel NA, Tokuda J. ROS-IGTL-Bridge: an open network interface for image-guided therapy using the ROS environment. Int J Comput Assist Radiol Surg. 2017 May 31. doi: 10.1007/s11548-017-1618-1. 
