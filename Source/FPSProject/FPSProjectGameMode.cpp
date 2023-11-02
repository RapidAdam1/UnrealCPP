// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSProjectGameMode.h"
#include "FPSProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFPSProjectGameMode::AFPSProjectGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("Game/CPP_Project/Blueprints/FirstPerson/Blueprints/BP_FirstPersonCharacter.uasset"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
