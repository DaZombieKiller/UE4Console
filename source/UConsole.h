#pragma once
#include "UObject.h"
#include "FOutputDevice.h"

class UClass;

class UConsole : public UObject, public FOutputDevice
{
    char RawData[0x130 - sizeof(UObject) - sizeof(FOutputDevice)];
public:
    static UClass*(*GetPrivateStaticClass)(void);
};
