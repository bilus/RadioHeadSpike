// The server keeps a list of paired clients. See onPairing() for details.

#ifndef DLY_PAIRED_DEVICES_H
#define DLY_PAIRED_DEVICES_H

#include "message.h"

// A paired device.

struct Device 
{
  Message::Address address;
  struct Stats
  {
    unsigned long numTotal;
  };
  Stats stats;
};

////////////////////////////////////////////////////////////////////////////////

#define MAX_PAIRED_DEVICES 64
Device thePairedDevices[MAX_PAIRED_DEVICES];
unsigned short thePairedDeviceCount = 0;

////////////////////////////////////////////////////////////////////////////////

// Find a paired device by address. Returns its index in the thePairedDevices array 
// or -1 if not found.
const unsigned long findPairedDevice(const Message::Address& address)
{
  for (int i = 0; i < thePairedDeviceCount; ++i)
  {
    if (thePairedDevices[i].address == address)
      return i;
  }
  return -1;
}

// Adds a new client to the list of paired devices. 
// Returns true if the client has been added, false it's been already paired.
bool addPairedDevice(uint8_t& address)
{
  // If the device isn't already there in the array, add it and reset its stats.
  
  if (-1 != findPairedDevice(address))
    return false;
  
  Device device;
  device.address = address;
  memset(&device.stats, 0, sizeof(device.stats));
  thePairedDevices[thePairedDeviceCount++] = device;
  return true;
}

// "Remove" paired devices from the array.
void emptyPairedDevices()
{
  thePairedDeviceCount = 0;
}

// Return the number of paired devices.
const unsigned short getPairedDeviceCount()
{
  return thePairedDeviceCount;
}

// Give access to stats of a device given its address. Returns NULL if device not found.
Device::Stats* findPairedDeviceStats(const Message::Address& address)
{
  const unsigned long i = findPairedDevice(address);
  if (i != -1)
  {
     return &thePairedDevices[i].stats;
  }      
  else   
  {      
    return NULL;
  }      
}

#endif