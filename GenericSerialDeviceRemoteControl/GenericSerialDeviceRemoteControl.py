from __main__ import vtk, slicer
from slicer.ScriptedLoadableModule import ScriptedLoadableModule, ScriptedLoadableModuleWidget, ScriptedLoadableModuleLogic
from slicer.util import VTKObservationMixin
import logging


class GenericSerialDeviceRemoteControl(ScriptedLoadableModule):
    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "Serial Device Remote Control"
        self.parent.categories = ["IGT"]
        self.parent.dependencies = ["OpenIGTLinkRemote"]
        self.parent.contributors = ["Adam Aji (SonoVol, PerkinElmer)"]
        self.parent.helpText = """
            Send commands to and read output from serial devices over their CTS/RTS/TX lines.
            <p>
            This module provides an interface for communicating with a Serial Device over a PlusServer (<a href="https://plustoolkit.github.io/">PlusToolkit</a>).
            See <a href="http://perk-software.cs.queensu.ca/plus/doc/nightly/user/DeviceGenericSerial.html"> the PlusLib documentation for GenericSerialDevices.</a>
            </p>
        """
        self.parent.acknowledgementText = ""


class GenericSerialDeviceRemoteControlWidget(ScriptedLoadableModuleWidget):

    def __init__(self, parent=None):
        ScriptedLoadableModuleWidget.__init__(self, parent)
        self.ui = None
        self.logic = GenericSerialDeviceRemoteControlLogic()
        self.connectorNodeObserverTagList = []
        self.plusRemoteNode = None

    def setup(self):
        """Instantiate and connect widgets to events."""
        ScriptedLoadableModuleWidget.setup(self)

        self.connectorNodeObserverTagList = []

        loaded_ui = slicer.util.loadUI(self.resourcePath(f"UI/{self.moduleName}.ui"))
        self.layout.addWidget(loaded_ui)
        self.ui = slicer.util.childWidgetVariables(loaded_ui)

        self.ui.connectorNodeSelector.nodeTypes = ["vtkMRMLIGTLConnectorNode"]
        self.ui.connectorNodeSelector.setMRMLScene(slicer.mrmlScene)

        self.ui.connectorNodeSelector.connect("nodeActivated(vtkMRMLNode*)", self.onConnectorNodeSelected)
        self.ui.connectorNodeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onConnectorNodeSelected)
        self.ui.deviceIdComboBox.connect("currentIndexChanged(int)", self.onDeviceIdChanged)

        self.plusRemoteNode = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLPlusRemoteNode')
        if self.plusRemoteNode is None:
            self.plusRemoteNode = slicer.vtkMRMLPlusRemoteNode()
            self.plusRemoteNode.SetName("PlusRemoteNode")
            slicer.mrmlScene.AddNode(self.plusRemoteNode)
        self.plusRemoteNode.AddObserver(slicer.vtkMRMLPlusRemoteNode.DeviceIdsReceivedEvent, self.requestDeviceIdsCompleted)

        self.onConnectorNodeSelected(self.ui.connectorNodeSelector.currentNode())

        self.ui.sendCommandButton.clicked.connect(
            lambda state, lineEdit=self.ui.sendCommandEdit, responseEdit=self.ui.commandResponseOutput:
            self.onSendCommand(state, commandEdit=lineEdit, responseLabel=responseEdit)
        )
        self.ui.sendGetCTSButton.clicked.connect(
            lambda state, responseEdit=self.ui.commandResponseOutput:
            self.onGetCTS(state, responseLabel=responseEdit)
        )
        self.ui.sendSetRTSButton.clicked.connect(
            lambda state, setEdit=self.ui.sendSetRTSEdit, responseEdit=self.ui.commandResponseOutput:
            self.onSetRTS(state, setEdit=setEdit, responseLabel=responseEdit)
        )
        self.ui.sendGetRTSButton.clicked.connect(
            lambda state, responseEdit=self.ui.commandResponseOutput:
            self.onGetRTS(state, responseLabel=responseEdit)
        )

        self.layout.addStretch(1)

    def onSendCommand(self, _, commandEdit, responseLabel):
        """
        Send a SendText command on button press.

        Input: button state (bool), command text line edit (qLineEdit), output response label (qLabel)
        """
        command = commandEdit.text
        self.logic.sendSendTextCommand(
            command,
            lambda caller, event: self.onUpdateResponseLabel(caller, event, responseLabel=responseLabel),
            lambda caller, event: self.onUpdateResponseLabelError(caller, event, responseLabel=responseLabel)
        )

    def onGetCTS(self, _, responseLabel):
        """
        Send a GetCTS command on button press.

        Input: button state (bool), output response label (qLabel)
        """
        self.logic.sendGetCTSCommand(
            lambda caller, event: self.onUpdateResponseLabel(caller, event, responseLabel=responseLabel),
            lambda caller, event: self.onUpdateResponseLabelError(caller, event, responseLabel=responseLabel)
        )

    def onSetRTS(self, _, setEdit, responseLabel):
        """
        Send a SetRTS command on button press.

        Input: button state (bool), input line edit (qLineEdit), output response label (qLabel)
        """
        set_value = setEdit.text
        if set_value not in ["True", "False"]:
            logging.error("Invalid RTS value sent. Accepted values are 'True' or 'False'. Not sending.")
            return
        self.logic.sendSetRTSCommand(
            set_value,
            lambda caller, event: self.onUpdateResponseLabel(caller, event, responseLabel=responseLabel),
            lambda caller, event: self.onUpdateResponseLabelError(caller, event, responseLabel=responseLabel)
        )

    def onGetRTS(self, _, responseLabel):
        """
        Send a GetRTS command on button press.

        Input: button state (bool), output response label (qLabel)
        """
        self.logic.sendGetRTSCommand(
            lambda caller, event: self.onUpdateResponseLabel(caller, event, responseLabel=responseLabel),
            lambda caller, event: self.onUpdateResponseLabelError(caller, event, responseLabel=responseLabel)
        )

    def onUpdateResponseLabel(self, caller, event, responseLabel):
        """
        Call back function on command completion to update the qLabel with response text.

        Input: caller (vtkSlicerOpenIGTLinkCommand), event (int), response label (qLabel)
        """
        response = caller.GetResponseMetaDataElement("Message")
        responseLabel.setText(response)

    def onUpdateResponseLabelError(self, caller, event, responseLabel):
        """
        Call back function on command error (expiry or cancellation) to update the qLabel with response text.

        Input: caller (vtkSlicerOpenIGTLinkCommand), event (int), response label (qLabel)
        """
        responseLabel.setText("There was an error processing the command.")

    def requestDeviceIdsCompleted(self, obj=None, event=None, caller=None):  # pylint: disable=unused-argument
        """Update the Device ID combobox when the server connection status has changed."""
        wasBlocked = self.ui.deviceIdComboBox.blockSignals(True)
        self.ui.deviceIdComboBox.clear()
        deviceIds = vtk.vtkStringArray()
        self.plusRemoteNode.GetDeviceIDs(deviceIds)
        currentDeviceId = self.plusRemoteNode.GetCurrentDeviceID()

        for value_index in range(deviceIds.GetNumberOfValues()):
            deviceId = deviceIds.GetValue(value_index)
            self.ui.deviceIdComboBox.addItem(deviceId)

        current_index = self.ui.deviceIdComboBox.findText(currentDeviceId)
        self.ui.deviceIdComboBox.setCurrentIndex(current_index)
        self.ui.deviceIdComboBox.blockSignals(wasBlocked)
        self.onDeviceIdChanged()

    def onConnectorNodeSelected(self, connectorNode):
        """
        Observe connector node events when a new connector node is selected.

        Input: connector node (vtkMRMLIGTLConnectorNode)
        """
        for obj, tag in self.connectorNodeObserverTagList:
            obj.RemoveObserver(tag)
        self.connectorNodeObserverTagList = []

        # Add observers for connect/disconnect events
        if connectorNode is not None:
            events = [
                [slicer.vtkMRMLIGTLConnectorNode.ConnectedEvent, self.onConnectorNodeConnectionChanged],
                [slicer.vtkMRMLIGTLConnectorNode.DisconnectedEvent, self.onConnectorNodeConnectionChanged]
            ]
            for tagEventHandler in events:
                connectorNodeObserverTag = connectorNode.AddObserver(tagEventHandler[0], tagEventHandler[1])
                self.connectorNodeObserverTagList.append((connectorNode, connectorNodeObserverTag))

        self.plusRemoteNode.SetAndObserveOpenIGTLinkConnectorNode(connectorNode)
        self.onConnectorNodeConnectionChanged()

    def onConnectorNodeConnectionChanged(self, obj=None, event=None, caller=None):  # pylint: disable=unused-argument
        """When the selected connector node connection has changed, request for a current list of device ids."""
        connectorNode = self.ui.connectorNodeSelector.currentNode()
        if (connectorNode is not None and connectorNode.GetState() == slicer.vtkMRMLIGTLConnectorNode.StateConnected):
            self.logic.setConnectorNode(connectorNode)
            self.plusRemoteNode.SetDeviceIDType("")
            slicer.modules.plusremote.logic().RequestDeviceIDs(self.plusRemoteNode)

    def onDeviceIdChanged(self):
        """Propagate device id changes to downstream nodes and logic."""
        currentDeviceId = self.ui.deviceIdComboBox.currentText
        self.plusRemoteNode.SetCurrentDeviceID(currentDeviceId)
        self.logic.setDeviceID(currentDeviceId)

    def cleanup(self):
        self.logic.cleanup()


class GenericSerialDeviceRemoteControlLogic(ScriptedLoadableModuleLogic, VTKObservationMixin):
    """Abstract class behavior for vtkPlusGenericSerialDevice control over OpenIGTLink/Plus."""

    def __init__(self):
        """Initialize this class."""
        ScriptedLoadableModuleLogic.__init__(self)
        VTKObservationMixin.__init__(self)
        self._runningCommandObservers = {}  # command: [event, method] dict for tracking active observers
        self._finishedCommands = []
        self._deviceId = ""
        self._connectorNode = None

    def setConnectorNode(self, node):
        """
        Set the connector node that will be connected to the ultrasound device.

        Input: node to set to (vtkMRMLIGTLConnectorNode)
        """
        self.removeObservers(self.onConnectionChanged)
        if node:
            self.addObserver(node, slicer.vtkMRMLIGTLConnectorNode.ConnectedEvent, self.onConnectionChanged)
            self.addObserver(node, slicer.vtkMRMLIGTLConnectorNode.DisconnectedEvent, self.onConnectionChanged)
        self._connectorNode = node
        self.onConnectionChanged()

    def onConnectionChanged(self, caller=None, event=None, callData=None):
        """Call when the connector node is changed, or if the device is (dis)connected."""
        if self._connectorNode and self._connectorNode.GetState() == slicer.vtkMRMLIGTLConnectorNode.StateConnected:
            self.onConnected()
        else:
            self.onDisconnected()

    def setDeviceID(self, deviceId):
        """
        Set the device id from the Plus config file of the associated vtkPlusGenericSerialDevice.

        Input: device id (str, ex. "MySerialDevice")
        """
        self._deviceId = deviceId

    def deviceId(self):
        """
        Get the currently set Plus config file device id.

        Output: device id (str)
        """
        return self._deviceId

    def _sendCommand(self, commandName, commandXML, callbackOnComplete, callbackOnExpire, callbackOnCancel, blocking, timeoutSeconds):
        """
        Set up a command and send it over the current connector node.

        Inputs: command name (str), command xml content (str), custom callback function for when command returns (function), command timeout in sec (int), command blocking (bool)
        """
        if self._connectorNode is None or self._connectorNode.GetState() != slicer.vtkMRMLIGTLConnectorNode.StateConnected:
            logging.error("Connector node unavailable!")
            if isinstance(self._connectorNode, slicer.vtkMRMLIGTLConnectorNode):
                logging.error("State: %s", self._connectorNode.GetState())
            return
        command = slicer.vtkSlicerOpenIGTLinkCommand()
        command.SetName(commandName)
        command.SetCommandContent(commandXML)
        command.SetTimeoutSec(timeoutSeconds)
        command.SetBlocking(blocking)
        command.ClearResponseMetaData()
        observers = []
        if callbackOnComplete:
            self.addObserver(command, slicer.vtkSlicerOpenIGTLinkCommand.CommandCompletedEvent, callbackOnComplete)
            observers.append([command.CommandCompletedEvent, callbackOnComplete])
        if callbackOnExpire:
            self.addObserver(command, slicer.vtkSlicerOpenIGTLinkCommand.CommandExpiredEvent, callbackOnExpire)
            observers.append([command.CommandExpiredEvent, callbackOnExpire])
        if callbackOnCancel:
            self.addObserver(command, slicer.vtkSlicerOpenIGTLinkCommand.CommandCancelledEvent, callbackOnCancel)
            observers.append([command.CommandCancelledEvent, callbackOnCancel])
        self.addObserver(command, slicer.vtkSlicerOpenIGTLinkCommand.CommandCompletedEvent, self._cleanupCommandOnCompletion)
        observers.append([command.CommandCompletedEvent, self._cleanupCommandOnCompletion])
        self._runningCommandObservers[command] = observers
        self._connectorNode.SendCommand(command)
        self.cleanupFinishedCommands()
        return command

    def _cleanupCommandOnCompletion(self, caller, event):
        """Stage a command to have its observers removed once completed."""
        self._finishedCommands.append(caller)

    def cleanupFinishedCommands(self):
        """Clear observers from staged finished commands."""
        for caller in self._finishedCommands:
            observers = self._runningCommandObservers[caller]
            for observer in observers:
                if self.hasObserver(caller, *observer):
                    self.removeObserver(caller, *observer)
            del self._runningCommandObservers[caller]
        self._finishedCommands = []

    def sendSendTextCommand(self, sendText, callbackOnComplete, callbackOnExpire=None, callbackOnCancel=None, blocking=False, timeoutSeconds=5.0):
        """
        Send a SendText command over IGTL, which sends data over the TX line on the serial device.

        Input: text to send (str), callback function to run when command returns (function)
        """
        logging.info("Sending text: %s", sendText)
        commandXML = f"""<Command Name="SendText" DeviceId="{self._deviceId}" Text="{sendText}" />"""
        return self._sendCommand("SendText", commandXML, callbackOnComplete, callbackOnExpire, callbackOnCancel, blocking, timeoutSeconds)

    def sendGetCTSCommand(self, callbackOnComplete, callbackOnExpire=None, callbackOnCancel=None, blocking=False, timeoutSeconds=5.0):
        """
        Send a GetCTS command over IGTL, which grabs the value from the CTS line on the serial device.

        Input: callback function to run when command returns (function)
        Output: success (bool)
        """
        logging.info("Sending GetCTS")
        commandXML = f"""<Command Name="SerialCommand" DeviceId="{self._deviceId}" CommandName="GetCTS" />"""
        return self._sendCommand("SerialCommand", commandXML, callbackOnComplete, callbackOnExpire, callbackOnCancel, blocking, timeoutSeconds)

    def sendSetRTSCommand(self, set_value, callbackOnComplete=None, callbackOnExpire=None, callbackOnCancel=None, blocking=False, timeoutSeconds=5.0):
        """
        Send a SendRTS command over IGTL, which sets the value on the RTS line on the serial device.

        Input: value to set (str, valid values "True"/"False"), callback function to run when command returns (function)
        """
        logging.info("Sending SetRTS")
        commandXML = f"""<Command Name="SerialCommand" DeviceId="{self._deviceId}" CommandName="SetRTS" CommandValue="{set_value}"/>"""
        return self._sendCommand("SerialCommand", commandXML, callbackOnComplete, callbackOnExpire, callbackOnCancel, blocking, timeoutSeconds)

    def sendGetRTSCommand(self, callbackOnComplete, callbackOnExpire=None, callbackOnCancel=None, blocking=False, timeoutSeconds=5.0):
        """
        Send a GetRTS command over IGTL, which grabs the value from the RTS line on the serial device.

        Input: callback function to run when command returns (function)
        """
        logging.info("Sending GetRTS")
        commandXML = f"""<Command Name="SerialCommand" DeviceId="{self._deviceId}" CommandName="GetRTS" />"""
        return self._sendCommand("SerialCommand", commandXML, callbackOnComplete, callbackOnExpire, callbackOnCancel, blocking, timeoutSeconds)

    def onConnected(self):
        """
        Call when a connection has been established.

        Not required to override for widget to function.
        """
        return

    def onDisconnected(self):
        """
        Call when a device has been disconnected, or when the connection has not been set.

        Not required to override for widget to function.
        """
        return

    def onReload(self):
        """When the module is reloaded for development purposes."""
        return

    def cleanup(self):
        self.removeObservers()
