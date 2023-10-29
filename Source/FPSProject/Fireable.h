﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IInteract.h"
#include "UObject/Interface.h"
#include "Fireable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UFireable : public UInteract
{
	GENERATED_BODY()
};

/**
 * 
 */
class FPSPROJECT_API IFireable
{
	GENERATED_BODY()
	
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	bool Fire();
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	bool Reload();
};
