cd \
mkdir dev
cd dev
mkdir Arduino
cd Arduino
git clone https://github.com/queezythegreat/arduino-cmake.git

cd RadioHeadSpike

cd client/build
cmake -G "Unix Makefiles" -DCMAKE_ARDUINO_PATH=C:/dev/Arduino/arduino-cmake ..
cd ../../server/build
cmake ../ -G "Unix Makefiles" -DCMAKE_ARDUINO_PATH=C:/dev/Arduino/arduino-cmake ..