/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLConnectorNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLIGTLConnectorNode_h
#define __vtkMRMLIGTLConnectorNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkMRMLIGTLQueryNode.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkWeakPointer.h>

// OpenIGTLinkIO includes
#include <igtlioCommand.h>

// std includes
#include <list>

class vtkMRMLIGTLQueryNode;
class vtkMutexLock;
class vtkSlicerOpenIGTLinkCommand;

typedef void* IGTLDevicePointer;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLConnectorNode : public vtkMRMLNode
{
public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  enum
  {
    ConnectedEvent = 118944,
    DisconnectedEvent = 118945,
    ActivatedEvent = 118946,
    DeactivatedEvent = 118947,
    NewDeviceEvent = 118949,
    DeviceModifiedEvent = 118950,
    RemovedDeviceEvent = 118951,
    CommandReceivedEvent = 119001, // Command received
    CommandResponseReceivedEvent = 119002, // Command response
    CommandCompletedEvent = 119003, // Command completed (could be response received, or expired, or cancelled)
  };

  enum
  {
    StateOff,
    StateWaitConnection,
    StateConnected,
    State_Last // this line must be last
  };

  enum
  {
    TypeNotDefined,
    TypeServer,
    TypeClient,
    Type_Last // this line must be last
  };

  static vtkMRMLIGTLConnectorNode* New();
  vtkTypeMacro(vtkMRMLIGTLConnectorNode, vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual vtkMRMLNode* CreateNodeInstance() override;

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes(const char** atts) override;

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) override;

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode* node) override;

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() override
  {
    return "IGTLConnector";
  };

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents(vtkObject* caller, unsigned long event, void* callData) override;

  int SetTypeServer(int port);

  int SetTypeClient(std::string hostname, int port);

  void SetType(int type);
  int GetType();

  const char* GetServerHostname();
  int GetServerPort();

  void SetServerHostname(std::string hostname);
  void SetServerPort(int port);

  void SetUseStreamingVolume(bool useStreamingVolume);
  bool GetUseStreamingVolume();

  int Start();

  int Stop();

  /// Call periodically to perform processing in the main thread.
  /// Suggested timeout 5ms.
  void PeriodicProcess();

  void ConnectEvents();
  // Description:
  // Set and start observing MRML node for outgoing data.
  // If devType == NULL, a converter is chosen based only on MRML Tag.
  int RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType = NULL);

  // Description:
  // Stop observing and remove MRML node for outgoing data.
  void UnregisterOutgoingMRMLNode(vtkMRMLNode* node);

  // Process IO connector incoming events
  // event ID is specified in OpenIGTLinkIO
  void ProcessIOConnectorEvents(vtkObject* caller, unsigned long event, void* callData);

  // Process IO incoming device events
  // event ID is specified in OpenIGTLinkIO
  void ProcessIODeviceEvents(vtkObject* caller, unsigned long event, void* callData);

  // Process IO connector incoming command events
  // event ID is specified in OpenIGTLinkIO
  void ProcessIOCommandEvents(vtkObject* caller, unsigned long event, void* callData);

  // Description:
  // Add a new Device.
  // If a Device with an identical device_id already exist, the method will fail.
  int AddDevice(IGTLDevicePointer device);

  // Description:
  // Add a new Device.
  // If a Device with an identical device_id already exist, the method will fail.
  int RemoveDevice(IGTLDevicePointer device);

  // Description:
  // Register MRML node for incoming data.
  // Returns true on success.
  bool RegisterIncomingMRMLNode(vtkMRMLNode* node, IGTLDevicePointer device);

  // Description:
  // Unregister MRML node for incoming data.
  void UnregisterIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfOutgoingMRMLNodes();

  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetOutgoingMRMLNode(unsigned int i);

  IGTLDevicePointer GetDeviceFromOutgoingMRMLNode(const char* outgoingNodeID);
  IGTLDevicePointer GetDeviceFromIncomingMRMLNode(const char* incomingNodeID);

  //IGTLDevicePointer GetDevice(const std::string& deviceType, const std::string& deviceName);

  // Get device for outgoing MRML node. If a device has not been created then it is created.
  IGTLDevicePointer CreateDeviceForOutgoingMRMLNode(vtkMRMLNode* dnode);

  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfIncomingMRMLNodes();

  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetIncomingMRMLNode(unsigned int i);

  // Description:
  // Get the state of the connector:
  int GetState();

  // Controls if active connection will be resumed when
  // scene is loaded.
  bool GetPersistent();
  void SetPersistent(bool persistent);

  // Controls if CRC checksum for received messages should be checked.
  // If CRC check is enabled then messages with invalid CRC are ignored.
  bool GetCheckCRC();
  void SetCheckCRC(bool check);

  void SetRestrictDeviceName(int restrictDeviceName);

  // Description:
  // A function to explicitly push node to OpenIGTLink. The function is called either by
  // external nodes or MRML event hander in the connector node.
  int PushNode(vtkMRMLNode* node);

  // Description:
  // Calls PushNode() for all nodes with the "OpenIGTLinkIF.pushOnConnect" attribute set to "true"
  void PushOnConnect();

  typedef std::list< vtkWeakPointer<vtkMRMLIGTLQueryNode> > QueryListType;
  // Query queueing mechanism is needed to send all queries from the connector thread.
  // Queries can be pushed to the end of the QueryQueue by calling RequestInvoke from any thread,
  // and they will be Invoked in the main thread.
  // Use a weak pointer to make sure we don't try to access the query node after it is deleted from the scene.
  QueryListType QueryWaitingQueue;
  vtkMutexLock* QueryQueueMutex;

  //----------------------------------------------------------------
  // For controling remote devices
  //----------------------------------------------------------------
  // Description:
  // Push query into the query list.
  int PushQuery(vtkMRMLIGTLQueryNode* query);

  // Description:
  // Removes query from the query list.
  void CancelQuery(vtkMRMLIGTLQueryNode* node);

  //----------------------------------------------------------------
  // Sending commands
  //----------------------------------------------------------------

  ///  Send the given command from the given device.
  /// - If using BLOCKING, the call blocks until a response appears or timeout.
  /// - If using ASYNCHRONOUS, wait for the CommandResponseReceivedEvent event.
  /// - Returns command that can be observed, and will contain response
  igtlioCommandPointer SendCommand(std::string name, std::string content, bool blocking = true, double timeout_s = 5.0, igtl::MessageBase::MetaDataMap* metaData = NULL, int clientId = -1);

  /// Sends the specified command and attaches an observer
  /// Blocking behavior and device_id are set in the command object
  /// Records the id of the command in the command object
  /// Invokes a CommandResponse event on the command object when a response is received.
  void SendCommand(igtlioCommandPointer command);
  void SendCommand(vtkSlicerOpenIGTLinkCommand* command);

  /// Send a command response from the given device. Asynchronous.
  int SendCommandResponse(igtlioCommandPointer command);
  void SendCommandResponse(vtkSlicerOpenIGTLinkCommand* command);

  /// Cancels the command in the specified device
  void CancelCommand(igtlioCommandPointer command);
  void CancelCommand(int commandId, int clientId);

  //----------------------------------------------------------------
  // For OpenIGTLink time stamp access
  //----------------------------------------------------------------

  // Description:
  // Turn lock flag on to stop updating MRML node. Call this function before
  // reading the content of the MRML node and the corresponding time stamp.
  void LockIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Turn lock flag off to start updating MRML node. Make sure to call this function
  // after reading the content / time stamp.
  void UnlockIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Get OpenIGTLink's time stamp information. Returns 0, if it fails to obtain time stamp.
  int GetIGTLTimeStamp(vtkMRMLNode* node, int& second, int& nanosecond);

  std::vector<std::string> GetDeviceTypeFromMRMLNodeType(const char* NodeTag);

  std::vector<std::string> GetNodeTagFromDeviceType(const char* deviceType);

#ifndef __VTK_WRAP__
  //BTX
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference* reference) override;

  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference* reference) override;

  virtual void OnNodeReferenceModified(vtkMRMLNodeReference* reference) override;
  //ETX
#endif // __VTK_WRAP__

  // Outgoing header version
  // Should be set before messages are sent
  vtkGetMacro(OutgoingMessageHeaderVersionMaximum, int);
  vtkSetMacro(OutgoingMessageHeaderVersionMaximum, int);

protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLIGTLConnectorNode();
  ~vtkMRMLIGTLConnectorNode();
  vtkMRMLIGTLConnectorNode(const vtkMRMLIGTLConnectorNode&);
  void operator=(const vtkMRMLIGTLConnectorNode&);

  //----------------------------------------------------------------
  // Outgoing header version
  //----------------------------------------------------------------
  int OutgoingMessageHeaderVersionMaximum;

  //----------------------------------------------------------------
  // Reference role strings
  //----------------------------------------------------------------
  char* IncomingNodeReferenceRole;
  char* IncomingNodeReferenceMRMLAttributeName;

  char* OutgoingNodeReferenceRole;
  char* OutgoingNodeReferenceMRMLAttributeName;

  vtkSetStringMacro(IncomingNodeReferenceRole);
  vtkGetStringMacro(IncomingNodeReferenceRole);
  vtkSetStringMacro(IncomingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(IncomingNodeReferenceMRMLAttributeName);

  vtkSetStringMacro(OutgoingNodeReferenceRole);
  vtkGetStringMacro(OutgoingNodeReferenceRole);
  vtkSetStringMacro(OutgoingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(OutgoingNodeReferenceMRMLAttributeName);

private:
  class vtkInternal;
  vtkInternal* Internal;
  bool UseStreamingVolume;
};

#endif

