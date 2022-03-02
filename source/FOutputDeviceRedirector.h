#pragma once
#include "FOutputDevice.h"

class FOutputDeviceRedirector : public FOutputDevice
{
    char Buffer[0x360 - sizeof(FOutputDevice)];
public:
    static void(*AddOutputDevice)(FOutputDeviceRedirector*, FOutputDevice*);
};
