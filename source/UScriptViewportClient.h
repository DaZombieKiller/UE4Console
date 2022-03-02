#pragma once
#include "UObject.h"

class UScriptViewportClient : public UObject
{
    char RawData[0x38 - sizeof(UObject)];
};
