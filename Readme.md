## Installation

1. Install CMake by downloading it from http://www.cmake.org/download/

2. Run the following:

> ./install

**Note:** This will install arduino-cmake to ~/dev/Arduino/arduino-cmake 

## Building

1. To build the client and upload it:

> cd client
> ./deploy

This will open screen with output from the serial port. To close it press C-a, C-\ and press y.

To build the server:

> cd server
> ./deploy

**Important:** It assumes concrete serial numbers of both devices with paths to the terminal devices being /dev/cu.usbserial-A703KYPS and /dev/cu.usbserial-A703L3MY respectively for the server and the client. To change it, edit both CMakeLists.txt file and deploy script.

FIXME: Make it possible to specify a device as a parameter to the deploy script.

## Changing CMakeLists.txt. Adding libraries.

After you modify any of the CMakeLists.txt, e.g. to add a library (at the top of each file), regenerate the Makefile:

> cd build
> cmake ../

## Surprises

1. Code with server sending a reply and client consuming it, has lower throughput than one-way communication (up to ~11 successes per second vs 8.5).
2. If you comment out Serial.print lines from server after a datagram is received, it considerably slows down (to 3 per s).

## Reference

http://duinoworks.bakketti.com/?p=11
https://github.com/queezythegreat/arduino-cmake#mac-serial-terminals