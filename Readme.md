## Installation

Install CMake by downloading it from http://www.cmake.org/download/ and run the following:

> ./install

or on Windows:

> install.bat

**Note:** This will install arduino-cmake to ~/dev/Arduino/arduino-cmake on a Mac and in X:\dev\Arduino\arduino-cmake on Windows (X is the current hard drive).


## Building

To build the client and upload it:

> cd client
> ./deploy <serial port> <address>

This will open screen with output from the serial port. To close it press C-a, C-\ and press y.

Example:

> ./deploy /dev/cu.usbserial-A703L3MY 1

To build the server:

> cd server
> ./deploy <serial port>

Example:

> ./deploy /dev/cu.usbserial-A703KYPS

**Note:** No address is specified for the server. It always has an address of 1.

On Windows use deploy.bat.


## Serial monitor

The `deploy` script will automatically open the monitor after building but you can use the `monitor` script at any time:

> ./monitor <serial port>
 
Example:

> ./monitor /dev/cu.usbserial-A703L3MY

or on Windows

> monitor COM1

**Important:** This will reset the device.

**Note:** On Windows use monitor.bat. Remember to have putty.exe downloaded to your project directory as monitor.bat uses putty. You can download putty.exe from http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html

## Changing CMakeLists.txt. Adding libraries.

After you modify any of the CMakeLists.txt, e.g. to add a library (at the top of each file), regenerate the Makefile:

> cd build
> cmake ..

or on Windows:

> cd build
> cmake .. -G "Unix Makefiles" -D<path to arduino-cmake>

by default that's:

> cmake .. -G "Unix Makefiles" -DC:/dev/Arduino/arduino-cmake



## Surprises/issues

1. Code with server sending a reply and client consuming it, has higher throughput than one-way communication (up to ~11 successes per second vs 8.5).
2. If you comment out Serial.print lines from server after a datagram is received, it considerably slows down (to 3 per s).


## Reference

1. http://duinoworks.bakketti.com/?p=11
2. https://github.com/queezythegreat/arduino-cmake#mac-serial-terminals


## Useful stuff

1. http://www.furrysoft.de/?page=goserial