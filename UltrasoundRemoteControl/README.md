Ultrasound Remote Control
==========

Implements a connection to Plus ultrasound devices using OpenIGTLink commands to control the ultrasound imaging parameters though network connection.

Users
--
The "Ultrasound Remote Control" module allows users to modify the following imaging parameters:
- Depth (mm)
- Gain (%)
- Frequency (MHz)
- Dynamic Range (dB)

Developers
--
The widgets can be easily customized and added to any python or C++ module as needed.

qSlicerAbstractUltrasoundParameterWidget provides a GUI interface base class for setting/getting ultrasound imaging parameters (see the list of available parameters here: http://perk-software.cs.queensu.ca/plus/doc/nightly/user/UsImagingParameters.html), and can be subclassed to create a custom parameter control UI.

qSlicerUltrasoundDoubleParameterSlider shows one potential implementation of an ultrasound widget capable of controlling double type imaging parameters.
Example usage is shown in [UltrasoundRemoteControl.py](UltrasoundRemoteControl.py)
