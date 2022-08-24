# Serial Device Remote Control
Remote control interface for [GenericSerialDevices](http://perk-software.cs.queensu.ca/plus/doc/nightly/user/DeviceGenericSerial.html).

Set and get values from the TX, CTS, and RTS lines on a serial device connected through OpenIGTLink using IGTL commands. Based off of UltrasoundRemoteControl.

### Setup

Requires running a [PlusServer](http://perk-software.cs.queensu.ca/plus/doc/nightly/user/ApplicationPlusServer.html) built with [this commit](https://github.com/PlusToolkit/PlusLib/commit/a41b411bd3dfd7aadf7b6bfb17280e28e063c061) which introduces IGTL commands for certain serial device setters and getters.

A simple device config with a serial device can be found here: [Plus device config](https://github.com/PlusToolkit/PlusLibData/blob/master/ConfigFiles/PlusDeviceSet_Server_GenericSerial.xml).

Once the PlusServer is running with your config, connect to the server with the OpenIGTLinkIF Slicer module by creating a new IGTLConnector with the address as specified in the device config file. After activating the connector, in the Serial Device Remote Control module, select the newly created connector node and device ID which matches the config device ID. You should now be able to send and receive data to the device through the module interface.
