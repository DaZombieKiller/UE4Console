#pragma once
#include "FExec.h"
#include "UScriptViewportClient.h"

class FString;
class UConsole;
class ULocalPlayer;

class UGameViewportClient : public UScriptViewportClient, public FExec
{
public:
    union
    {
        struct
        {
            UConsole* ViewportConsole;
        };

        class RawData
        {
            char RawData[0x360 - sizeof(UScriptViewportClient) - sizeof(FExec)];
        };
    };

    static ULocalPlayer*(*SetupInitialLocalPlayer)(UGameViewportClient*, FString*);
};
