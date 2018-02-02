/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLIGTLConnectorNode_h
#define __vtkMRMLIGTLConnectorNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkMRMLIGTLQueryNode.h"

// OpenIGTLinkIO includes
#include "igtlioConnector.h"
#include "igtlioDeviceFactory.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include <vtkMRMLScene.h>

#include <vtkCallbackCommand.h>

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLConnectorNode : public vtkMRMLNode
{
 public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------
  
  enum{
    ConnectedEvent = igtlio::Connector::ConnectedEvent,
    DisconnectedEvent = igtlio::Connector::DisconnectedEvent,
    ActivatedEvent = igtlio::Connector::ActivatedEvent,
    DeactivatedEvent = igtlio::Connector::DeactivatedEvent,
    ReceiveEvent = 118948, // deprecated. it was for query response, OpenIGTLinkIO doesn't support query event. it is replaced with command message.
    NewDeviceEvent = igtlio::Connector::NewDeviceEvent,
    DeviceModifiedEvent = igtlio::Connector::DeviceContentModifiedEvent,
    CommandReceivedEvent    = igtlio::Device::CommandReceivedEvent, // COMMAND device got a query, COMMAND received
    CommandResponseReceivedEvent = igtlio::Device::CommandResponseReceivedEvent  // COMMAND device got a response, RTS_COMMAND received
  };

  static vtkMRMLIGTLConnectorNode *New();
  vtkTypeMacro(vtkMRMLIGTLConnectorNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE
    {return "IGTLConnector";};

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData ) VTK_OVERRIDE;
  
  
  void ConnectEvents();
  // Description:
  // Set and start observing MRML node for outgoing data.
  // If devType == NULL, a converter is chosen based only on MRML Tag.
  int RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType=NULL);
  
  // Description:
  // Stop observing and remove MRML node for outgoing data.
  void UnregisterOutgoingMRMLNode(vtkMRMLNode* node);


  // Process IO connector incoming events
  void ProcessIOConnectorEvents( vtkObject *caller, unsigned long event, void *callData );
  
  // Description:
  // Register MRML node for incoming data.
  // Returns a pointer to the node information in IncomingMRMLNodeInfoList
  igtlio::Connector::NodeInfoType* RegisterIncomingMRMLNode(vtkMRMLNode* node);
  
  // Description:
  // Unregister MRML node for incoming data.
  void UnregisterIncomingMRMLNode(vtkMRMLNode* node);
  
  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfOutgoingMRMLNodes();
  
  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetOutgoingMRMLNode(unsigned int i);
  
  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfIncomingMRMLNodes();
  
  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetIncomingMRMLNode(unsigned int i);
  
  // Description:
  // A function to explicitly push node to OpenIGTLink. The function is called either by
  // external nodes or MRML event hander in the connector node.
  int PushNode(vtkMRMLNode* node);
  
  // Query queueing mechanism is needed to send all queries from the connector thread.
  // Queries can be pushed to the end of the QueryQueue by calling RequestInvoke from any thread,
  // and they will be Invoked in the main thread.
  // Use a weak pointer to make sure we don't try to access the query node after it is deleted from the scene.
  std::list< vtkWeakPointer<vtkMRMLIGTLQueryNode> > QueryWaitingQueue;
  vtkMutexLock* QueryQueueMutex;

  //----------------------------------------------------------------
  // For controling remote devices
  //----------------------------------------------------------------
  // Description:
  // Push query int the query list.
  void PushQuery(vtkMRMLIGTLQueryNode* query);
  
  // Description:
  // Removes query from the query list.
  void CancelQuery(vtkMRMLIGTLQueryNode* node);

  ///  Send the given command from the given device.
  /// - If using BLOCKING, the call blocks until a response appears or timeout. Return response.
  /// - If using ASYNCHRONOUS, wait for the CommandResponseReceivedEvent event. Return device.
  ///
  igtlio::CommandDevicePointer SendCommand(std::string device_id, std::string command, std::string content, igtlio::SYNCHRONIZATION_TYPE synchronized = igtlio::BLOCKING, double timeout_s = 5);

  /// Send a command response from the given device. Asynchronous.
  /// Precondition: The given device has received a query that is not yet responded to.
  /// Return device.
  igtlio::CommandDevicePointer SendCommandResponse(std::string device_id, std::string command, std::string content);
  
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
  
  igtlio::Connector* IOConnector;
  
  std::vector<std::string> GetDeviceTypeFromMRMLNodeType(const char* NodeTag);
  
  std::vector<std::string> GetNodeTagFromDeviceType(const char * deviceType);
  
  typedef std::map<std::string, igtlio::Connector::NodeInfoType>   NodeInfoMapType;
  
  typedef std::map<std::string, vtkSmartPointer <igtlio::Device> > MessageDeviceMapType;

  typedef std::map<std::string,  std::vector<std::string> > DeviceTypeToNodeTagMapType;
  
  MessageDeviceMapType  MRMLIDToDeviceMap;

  DeviceTypeToNodeTagMapType DeviceTypeToNodeTagMap;

#ifndef __VTK_WRAP__
  //BTX
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference) VTK_OVERRIDE;
  
  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference) VTK_OVERRIDE;
  
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference) VTK_OVERRIDE;
  //ETX
#endif // __VTK_WRAP__
  
 protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLIGTLConnectorNode();
  ~vtkMRMLIGTLConnectorNode();
  vtkMRMLIGTLConnectorNode(const vtkMRMLIGTLConnectorNode&);
  void operator=(const vtkMRMLIGTLConnectorNode&);

  unsigned int AssignNodeToDevice(vtkMRMLNode* node, igtlio::DevicePointer device);

  void ProcessNewDeviceEvent(vtkObject *caller, unsigned long event, void *callData );

  void ProcessIncomingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlio::Device * modifiedDevice);

  void ProcessOutgoingDeviceModifiedEvent(vtkObject *caller, unsigned long event, igtlio::Device * modifiedDevice);

  vtkMRMLNode* GetOrAddMRMLNodeforDevice(igtlio::Device* device);
  
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
  
  NodeInfoMapType IncomingMRMLNodeInfoMap;
  
};

#endif

