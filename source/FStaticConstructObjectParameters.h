#pragma once
#include "FName.h"

class UClass;
class UObject;
class UPackage;
class FObjectInstancingGraph;

enum EObjectFlags
{
};

enum EInternalObjectFlags
{
};

struct FStaticConstructObjectParameters
{
	UClass* Class;
	UObject* Outer;
	FName Name;
	EObjectFlags SetFlags;
	EInternalObjectFlags InternalSetFlags;
	bool bCopyTransientsFromClassDefaults;
	bool bAssumeTemplateIsArchetype;
	UObject* Template;
	FObjectInstancingGraph* InstanceGraph;
	UPackage* ExternalPackage;
};
