@ECHO OFF
set TARGET_SERIAL_PORT=%1
REM Force rebuild.
touch server.cpp 
cd build
make && make server-upload && cd .. && monitor.bat %TARGET_SERIAL_PORT%
echo Deployed to %TARGET_SERIAL_PORT%