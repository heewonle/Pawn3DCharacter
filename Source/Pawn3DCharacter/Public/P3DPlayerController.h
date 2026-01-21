// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "P3DPlayerController.generated.h"


class UInputMappingContext; // IMC 관련 전방 선언
class UInputAction; // IA 관련 전방 선언
class ADronePawn;

UCLASS()
class PAWN3DCHARACTER_API AP3DPlayerController : public APlayerController
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;

	// Possess가 바뀔 때마다 IMC를 맞춰 끼우기 위해 오버라이드
	virtual void OnPossess(APawn* InPawn) override;
public:
	AP3DPlayerController();

public:
    //PawnCharacter Input 세팅
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|PawnCharacter")
    UInputMappingContext* PawnInputMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|PawnCharacter")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|PawnCharacter")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|PawnCharacter")
    UInputAction* InteractAction;


    //Drone Input 세팅
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Drone")
    UInputMappingContext* DroneInputMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Drone")
    UInputAction* Move2DAction;  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Drone")
    UInputAction* UpDownAction;  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Drone")
    UInputAction* RollAction;    

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Drone")
    UInputAction* ReturnToPlayerAction;

    // 드론 Pawn BP/클래스 지정(에디터에서)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    TSubclassOf<ADronePawn> DronePawnClass;

    // 호출용: E를 눌렀을 때
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void ToggleDrone();

    UFUNCTION(BlueprintCallable, Category = "Drone")
    void ReturnToPlayer();

private:
    // 현재 “원래 플레이어 Pawn”을 기억해뒀다가 복귀에 사용
    UPROPERTY()
    APawn* CachedPlayerPawn = nullptr;

    UPROPERTY()
    ADronePawn* CachedDronePawn = nullptr;

private:
    void ApplyIMC(UInputMappingContext* IMC);

    ADronePawn* SpawnDroneNear(APawn* PlayerPawn);
};
