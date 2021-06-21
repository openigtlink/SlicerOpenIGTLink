import os.path, datetime
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin
import logging

#
# UltrasoundRemoteControl
#

class UltrasoundRemoteControl(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Ultrasound Remote Control"
    self.parent.categories = ["IGT"]
    self.parent.dependencies = ["OpenIGTLinkRemote"]
    self.parent.contributors = ["Kyle Sunderland (PerkLab, Queen's University), Tamas Ungi (PerkLab, Queen's University), Andras Lasso (PerkLab, Queen's University)"]
    self.parent.helpText = """TODO"""
    self.parent.acknowledgementText = """This work was funded by Cancer Care Ontario Applied Cancer Research Unit (ACRU) and the Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO) grants."""

#
# qUltrasoundRemoteControlWidget
#
class UltrasoundRemoteControlWidget(ScriptedLoadableModuleWidget):

  def __init__(self, parent = None):
    ScriptedLoadableModuleWidget.__init__(self, parent)

  def setup(self):
    # Instantiate and connect widgets
    ScriptedLoadableModuleWidget.setup(self)

    self.connectorNodeObserverTagList = []

    # Plus parameters
    plusParametersCollapsibleButton = ctk.ctkCollapsibleButton()
    plusParametersCollapsibleButton.text = "Plus parameters"
    plusParametersCollapsibleButton.collapsed = False
    self.layout.addWidget(plusParametersCollapsibleButton)
    plusParametersLayout = qt.QFormLayout(plusParametersCollapsibleButton)

    self.connectorNodeSelector = slicer.qMRMLNodeComboBox()
    self.connectorNodeSelector.nodeTypes = ["vtkMRMLIGTLConnectorNode"]
    self.connectorNodeSelector.setMRMLScene(slicer.mrmlScene)
    plusParametersLayout.addRow("Connector node:", self.connectorNodeSelector)

    self.deviceIDComboBox = qt.QComboBox()
    plusParametersLayout.addRow("Device ID:", self.deviceIDComboBox)

    # Ultrasound parameters
    ultrasoundParametersCollapsibleButton = ctk.ctkCollapsibleButton()
    ultrasoundParametersCollapsibleButton.text = "Ultrasound parameters"
    ultrasoundParametersCollapsibleButton.collapsed = False
    self.layout.addWidget(ultrasoundParametersCollapsibleButton)
    ultrasoundParametersLayout = qt.QFormLayout(ultrasoundParametersCollapsibleButton)

    self.depthSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.depthSlider.setParameterName("DepthMm")
    self.depthSlider.setSuffix(" mm")
    self.depthSlider.setMinimum(10.0)
    self.depthSlider.setMaximum(150.0)
    self.depthSlider.setSingleStep(1.0)
    self.depthSlider.setPageStep(10.0)
    ultrasoundParametersLayout.addRow("Depth:",  self.depthSlider)

    self.gainSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.gainSlider.setParameterName("GainPercent")
    self.gainSlider.setSuffix("%")
    self.gainSlider.setMinimum(0.0)
    self.gainSlider.setMaximum(100.0)
    self.gainSlider.setSingleStep(1.0)
    self.gainSlider.setPageStep(10.0)
    ultrasoundParametersLayout.addRow("Gain:", self.gainSlider)

    self.frequencySlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.frequencySlider.setParameterName("FrequencyMhz")
    self.frequencySlider.setSuffix(" MHz")
    self.frequencySlider.setMinimum(2.0)
    self.frequencySlider.setMaximum(5.0)
    self.frequencySlider.setSingleStep(0.5)
    self.frequencySlider.setPageStep(1.0)
    ultrasoundParametersLayout.addRow("Frequency:", self.frequencySlider)

    self.dynamicRangeSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.dynamicRangeSlider.setParameterName("DynRangeDb")
    self.dynamicRangeSlider.setSuffix(" dB")
    self.dynamicRangeSlider.setMinimum(10.0)
    self.dynamicRangeSlider.setMaximum(100.0)
    self.dynamicRangeSlider.setSingleStep(1.0)
    self.dynamicRangeSlider.setPageStep(10.0)
    ultrasoundParametersLayout.addRow("Dynamic Range:", self.dynamicRangeSlider)

    self.powerSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.powerSlider.setParameterName("PowerDb")
    self.powerSlider.setSuffix("Db")
    self.powerSlider.setMinimum(-20.0)
    self.powerSlider.setMaximum(0.0)
    self.powerSlider.setSingleStep(1.0)
    self.powerSlider.setPageStep(5.0)
    ultrasoundParametersLayout.addRow("Power:", self.powerSlider)

    self.focusDepthSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.focusDepthSlider.setParameterName("FocusDepthPercent")
    self.focusDepthSlider.setSuffix("%")
    self.focusDepthSlider.setMinimum(0)
    self.focusDepthSlider.setMaximum(100)
    self.focusDepthSlider.setSingleStep(3)
    self.focusDepthSlider.setPageStep(10)
    ultrasoundParametersLayout.addRow("Focus Zone:", self.focusDepthSlider)

    self.tgcSlider = slicer.qSlicerUltrasoundTGCParameterWidget()
    self.tgcSlider.setParameterName("TimeGainCompensation")
    self.tgcSlider.setSuffix("Db")
    self.tgcSlider.setMinimum(0)
    self.tgcSlider.setMaximum(30)
    self.tgcSlider.setSingleStep(3)
    self.tgcSlider.setPageStep(10)
    ultrasoundParametersLayout.addRow("TGC:", self.tgcSlider)

    self.layout.addStretch(1)

    self.parameterWidgets = [
    self.depthSlider,
    self.gainSlider,
    self.frequencySlider,
    self.dynamicRangeSlider,
    self.powerSlider,
    self.focusDepthSlider,
    self.tgcSlider
    ]

    self.connectorNodeSelector.connect("nodeActivated(vtkMRMLNode*)", self.onConnectorNodeSelected)
    self.connectorNodeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onConnectorNodeSelected)
    self.deviceIDComboBox.connect("currentIndexChanged(int)", self.onDeviceIdChanged)

    self.plusRemoteNode = slicer.mrmlScene.GetFirstNodeByClass('vtkMRMLPlusRemoteNode')
    if self.plusRemoteNode is None:
      self.plusRemoteNode = slicer.vtkMRMLPlusRemoteNode()
      self.plusRemoteNode.SetName("PlusRemoteNode")
      slicer.mrmlScene.AddNode(self.plusRemoteNode)
    self.plusRemoteNode.AddObserver(slicer.vtkMRMLPlusRemoteNode.DeviceIdsReceivedEvent, self.requestDeviceIDsCompleted)

    self.onConnectorNodeSelected(self.connectorNodeSelector.currentNode())

  def onReload(self):
    pass

  def requestDeviceIDsCompleted(self, object=None, event=None, caller=None):
    wasBlocked = self.deviceIDComboBox.blockSignals(True)
    self.deviceIDComboBox.clear()
    deviceIDs = vtk.vtkStringArray()
    self.plusRemoteNode.GetDeviceIDs(deviceIDs)
    currentDeviceID = self.plusRemoteNode.GetCurrentDeviceID()

    for valueIndex in range(deviceIDs.GetNumberOfValues()):
      deviceID = deviceIDs.GetValue(valueIndex)
      self.deviceIDComboBox.addItem(deviceID)

    currentIndex = self.deviceIDComboBox.findText(currentDeviceID)
    self.deviceIDComboBox.setCurrentIndex(currentIndex)
    self.deviceIDComboBox.blockSignals(wasBlocked)
    self.onDeviceIdChanged()

  def onConnectorNodeSelected(self, connectorNode):
    for obj, tag in self.connectorNodeObserverTagList:
      obj.RemoveObserver(tag)
    self.connectorNodeObserverTagList = []

    # Add observers for connect/disconnect events
    if connectorNode is not None:
      events = [[slicer.vtkMRMLIGTLConnectorNode.ConnectedEvent, self.onConnectorNodeConnectionChanged], [slicer.vtkMRMLIGTLConnectorNode.DisconnectedEvent, self.onConnectorNodeConnectionChanged]]
      for tagEventHandler in events:
        connectorNodeObserverTag = connectorNode.AddObserver(tagEventHandler[0], tagEventHandler[1])
        self.connectorNodeObserverTagList.append((connectorNode, connectorNodeObserverTag))

    self.plusRemoteNode.SetAndObserveOpenIGTLinkConnectorNode(connectorNode)
    for widget in self.parameterWidgets:
      widget.setConnectorNode(connectorNode)
    self.onConnectorNodeConnectionChanged()

  def onConnectorNodeConnectionChanged(self, object=None, event=None, caller=None):
    connectorNode = self.connectorNodeSelector.currentNode()
    if (connectorNode is not None and connectorNode.GetState() == slicer.vtkMRMLIGTLConnectorNode.StateConnected):
      self.plusRemoteNode.SetDeviceIDType("")
      slicer.modules.plusremote.logic().RequestDeviceIDs(self.plusRemoteNode)

  def onDeviceIdChanged(self):
    currentDeviceID = self.deviceIDComboBox.currentText
    self.plusRemoteNode.SetCurrentDeviceID(currentDeviceID)
    for widget in self.parameterWidgets:
      widget.setDeviceID(currentDeviceID)

#
# UltrasoundRemoteControlLogic
#
class UltrasoundRemoteControlLogic(ScriptedLoadableModuleLogic):
  def __init__(self, parent = None):
    ScriptedLoadableModuleLogic.__init__(self, parent)

  def __del__(self):
    # Clean up commands
    pass

#
# Abstract widget that can be used as a base class to create custom scripted ultrasound widgets.
# TODO: In the future it would be better to make it so that we can override the virtual methods of qSlicerAbstractUltrasoundParameterWidget
# in Python and use that as a base class instead.
class AbstractScriptedUltrasoundWidget(qt.QWidget, VTKObservationMixin):
  def __init__(self):
    qt.QWidget.__init__(self)
    VTKObservationMixin.__init__(self)
    self._cmdSetParameter = None
    self._cmdGetParameter = None
    self._interactionInProgress = False
    self._deviceID = ""
    self._parameterName = ""
    self._connectorNode = None
    self._periodicParameterTimer = qt.QTimer()
    self._periodicParameterTimer.connect("timeout()", self.sendGetUltrasoundParameterCommand)

  def setInteractionInProgress(self, interactionStatus):
    """
    Returns True if the user is currently interacting with the widget.
    Ex. Dragging a slider
    """
    self._interactionInProgress = interactionStatus

  def interactionInProgress(self):
    """
    Returns True if the user is currently interacting with the widget.
    Ex. Dragging a slider
    """
    return self._interactionInProgress

  def connectorNode(self):
    """
    Get the connector node that will be connected to the ultrasound device.
    """
    return self.connectorNode

  def setConnectorNode(self, node):
    """
    Set the connector node that will be connected to the ultrasound device.
    """
    self.removeObservers(self.onConnectionChanged)
    if node:
      self.addObserver(node, slicer.vtkMRMLIGTLConnectorNode.ConnectedEvent, self.onConnectionChanged)
      self.addObserver(node, slicer.vtkMRMLIGTLConnectorNode.DisconnectedEvent, self.onConnectionChanged)
    self._connectorNode = node
    self.onConnectionChanged()

  def onConnectionChanged(self, caller=None, event=None, callData=None):
    """
    Called when the connector node is changed, or if the device is (dis)connected.
    """
    if self._connectorNode and self._connectorNode.GetState() == slicer.vtkMRMLIGTLConnectorNode.StateConnected:
      self.onConnected()
      self.sendGetUltrasoundParameterCommand()
      self._periodicParameterTimer.start(2000)
    else:
      self._periodicParameterTimer.stop()
      self.onDisconnected()

  def setDeviceID(self, deviceID):
    """
    Set the device id from the Plus config file that will be used to set/get the ultrasound parameters.
    Ex. "VideoDevice"
    """
    self._deviceID = deviceID

  def deviceID(self):
    """
    Get the device id from the Plus config file that will be used to set/get the ultrasound parameters.
    Ex. "VideoDevice"
    """
    return self._deviceID

  def sendSetUltrasoundParameterCommand(self):
    """
    Send a Set command to the ultrasound to request a change in parameter.
    Calls onSetUltrasoundParameterCompleted when the command has completed.
    """
    if self._connectorNode is None or self._connectorNode.GetState() != slicer.vtkMRMLIGTLConnectorNode.StateConnected:
      return
    setUSParameterXML = """
    <Command Name=\"SetUsParameter\" UsDeviceId=\"{deviceID}\">
      <Parameter Name=\"{parameterName}\" Value=\"{parameterValue}\" />
    </Command>
    """.format(deviceID=self._deviceID, parameterName=self._parameterName, parameterValue=self.parameterValue())
    self._cmdSetParameter = slicer.vtkSlicerOpenIGTLinkCommand()
    self._cmdSetParameter.SetName("SetUsParameter")
    self._cmdSetParameter.SetTimeoutSec(5.0)
    self._cmdSetParameter.SetBlocking(False)
    self._cmdSetParameter.SetCommandContent(setUSParameterXML)
    self._cmdSetParameter.ClearResponseMetaData()
    self.setInteractionInProgress(True)
    self.removeObservers(self.onSetUltrasoundParameterCompleted)
    self.addObserver(self._cmdSetParameter, slicer.vtkSlicerOpenIGTLinkCommand.CommandCompletedEvent, self.onSetUltrasoundParameterCompleted)
    self._connectorNode.SendCommand(self._cmdSetParameter)

  def onSetUltrasoundParameterCompleted(self, caller=None, event=None, callData=None):
    """
    Called when SetUsParameter has completed.
    """
    self._interactionInProgress = False
    self.sendGetUltrasoundParameterCommand() # Confirm new value

  def sendGetUltrasoundParameterCommand(self):
    """
    Send a Get command to the ultrasound requesting a parameter value.
    Calls onGetUltrasoundParameterCompleted when the command has completed.
    """
    if self._connectorNode is None or self._connectorNode.GetState() != slicer.vtkMRMLIGTLConnectorNode.StateConnected:
      return
    if self._cmdGetParameter and self._cmdGetParameter.IsInProgress():
      # Get command is already in progress
      return
    getUSParameterXML = """
    <Command Name=\"GetUsParameter\" UsDeviceId=\"{deviceID}\">
      <Parameter Name=\"{parameterName}\" />
    </Command>
    """.format(deviceID=self._deviceID, parameterName=self._parameterName)
    self._cmdGetParameter = slicer.vtkSlicerOpenIGTLinkCommand()
    self._cmdGetParameter.SetName("GetUsParameter")
    self._cmdGetParameter.SetTimeoutSec(5.0)
    self._cmdGetParameter.SetBlocking(False)
    self._cmdGetParameter.SetCommandContent(getUSParameterXML)
    self._cmdGetParameter.ClearResponseMetaData()
    self.removeObservers(self.onGetUltrasoundParameterCompleted)
    self.addObserver(self._cmdGetParameter, slicer.vtkSlicerOpenIGTLinkCommand.CommandCompletedEvent, self.onGetUltrasoundParameterCompleted)
    self._connectorNode.SendCommand(self._cmdGetParameter)

  def onGetUltrasoundParameterCompleted(self, caller=None, event=None, callData=None):
    """
    Called when GetUsParameter has completed.
    """
    if not self._cmdGetParameter.GetSuccessful():
      return
    if self._interactionInProgress or self._cmdGetParameter.IsInProgress():
      # User is currently interacting with the widget or
      # SetUSParameterCommand is currently in progress
      # Don't update the current value
      return
    value = self._cmdGetParameter.GetResponseMetaDataElement(self._parameterName)
    self.setParameterValue(value) # Set the value in the widget

  def parameterName(self):
    """
    Get name of the ultrasound parameter being controlled.
    Ex. "DepthMm"
    """
    return self.parameterName

  def setParameterName(self, parameterName):
    """
    Set name of the ultrasound parameter being controlled.
    Ex. "DepthMm"
    """
    self._parameterName = parameterName

  def onConnected(self):
    """
    Called when a connection has been established.
    Not required to override for widget to function.
    """
    pass

  def onDisconnected(self):
    """
    Called when a device has been disconnected, or when the connection has not been set.
    Not required to override for widget to function.
    """
    pass

  ###################################################
  # These methods should be overridden by subclasses.
  def parameterValue(self):
    """
    Return the parameter value from the widget.
    Ex. Slider/spinbox value, or any value set by widget.
    Required to override for widget to function.
    """
    return ""

  def setParameterValue(self, value):
    """
    Update the state of the widget to reflect the specified parameter value.
    Ex. Slider/spinbox value, or any value set by widget.
    Required to override for widget to function.
    """
    pass
  ###################################################
