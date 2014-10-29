@ECHO OFF
set TARGET_SERIAL_PORT=%1
set CLIENT_ADDRESS=%2
REM Force rebuild because CLIENT_ADDRESS may have changed.
touch client.cpp 
cd build
make && make client-upload && cd .. && monitor.bat %TARGET_SERIAL_PORT%
echo Deployed to %TARGET_SERIAL_PORT%