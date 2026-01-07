// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Project_TOKIGameMode.generated.h"

/**
 *  Simple Game Mode for a top-down perspective game
 *  Sets the default gameplay framework classes
 *  Check the Blueprint derived class for the set values
 */
UCLASS(abstract)
class AProject_TOKIGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	AProject_TOKIGameMode();
};



