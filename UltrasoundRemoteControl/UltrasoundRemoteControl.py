import os.path, datetime
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
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

    self.layout.addStretch(1)

    self.parameterWidgets = [
    self.depthSlider,
    self.gainSlider,
    self.frequencySlider,
    self.dynamicRangeSlider,
    self.powerSlider,
    self.focusDepthSlider
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
