# EmbeddedImageAcquisition
Author: Hasan Baig
email: hasan.baig@hotmail.com
WEb: http://www.hasanbaig.com

These codes are developed for embedded image acquisition using PIC microcontrollers and C328 serial camera module. It extracts the image from C328 camera module and store it into RAM first. The GPRS controller is included to receive commands from server to request image data.

The folder "C328_behaviour" contains the code which models the response behavior of the C328 camera module when the controller, placed in "Source_Controller" folder interacts with it. This C328 behavior is written only for testing purposes using Proteus Simulator. 

The folder "Source_Controller" contains two sub folders i.e. "ControllerWithGPS" and "ControllerWithRAM". The first one contains the complete code including the functions to send data to GPS module. The later folder contains code without GPS module. 

