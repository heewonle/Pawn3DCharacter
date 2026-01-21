// Fill out your copyright notice in the Description page of Project Settings.


#include "P3D_GameMode.h"
#include "BasePawn.h"
#include "P3DPlayerController.h"

AP3D_GameMode::AP3D_GameMode()
{
	DefaultPawnClass = ABasePawn::StaticClass();
	PlayerControllerClass = AP3DPlayerController::StaticClass();
}

