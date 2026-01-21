// Fill out your copyright notice in the Description page of Project Settings.

#include "P3DPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "DronePawn.h"

AP3DPlayerController::AP3DPlayerController()
    :
    PawnInputMappingContext(nullptr),
    DroneInputMappingContext(nullptr),
    MoveAction(nullptr),
    LookAction(nullptr),
    InteractAction(nullptr),
    Move2DAction(nullptr),
    RollAction(nullptr),
    UpDownAction(nullptr),
    ReturnToPlayerAction(nullptr)
{
}

void AP3DPlayerController::BeginPlay()
{
    Super::BeginPlay();
    // 시작 IMC는 Ground로 (Pawn 캐시는 OnPossess에서 확정)
    ApplyIMC(PawnInputMappingContext);
}

void AP3DPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!InPawn) return;

    // 드론을 잡았으면 Drone IMC
    if (InPawn->IsA(ADronePawn::StaticClass()))
    {
        ApplyIMC(DroneInputMappingContext);
        return;
    }

    // 지상 Pawn이면 캐시 갱신
    CachedPlayerPawn = InPawn;
    ApplyIMC(PawnInputMappingContext);
}

void AP3DPlayerController::ApplyIMC(UInputMappingContext* IMC)
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer) return;

    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (!Subsystem) return;

    //  관리하는 2개 컨텍스트만 정리
    if (PawnInputMappingContext)  Subsystem->RemoveMappingContext(PawnInputMappingContext);
    if (DroneInputMappingContext) Subsystem->RemoveMappingContext(DroneInputMappingContext);

    if (IMC)
    {
        // Priority 0 = 가장 높은 우선순위로 쓰는 편
        Subsystem->AddMappingContext(IMC, 0);
    }
}

ADronePawn* AP3DPlayerController::SpawnDroneNear(APawn* PlayerPawn)
{
    if (!IsValid(PlayerPawn) || !DronePawnClass) return nullptr;

    UWorld* World = GetWorld();
    if (!World) return nullptr;

    const FVector BaseLoc = PlayerPawn->GetActorLocation();
    const FVector Fwd = PlayerPawn->GetActorForwardVector();

    const FVector SpawnLoc = BaseLoc + Fwd * 200.f + FVector(0, 0, 120.f);
    const FRotator SpawnRot = PlayerPawn->GetActorRotation();

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    return World->SpawnActor<ADronePawn>(DronePawnClass, SpawnLoc, SpawnRot, Params);
}

void AP3DPlayerController::ToggleDrone()
{
    APawn* CurrentPawn = GetPawn();
    if (!IsValid(CurrentPawn)) return;

    // 지상 -> 드론
    if (!CurrentPawn->IsA(ADronePawn::StaticClass()))
    {
        if (!IsValid(CachedPlayerPawn))
            CachedPlayerPawn = CurrentPawn;

        if (!DronePawnClass)
        {     
            return;
        }
        if (!IsValid(CachedDronePawn))
            CachedDronePawn = SpawnDroneNear(CachedPlayerPawn);
        if (IsValid(CachedDronePawn))
        {
            Possess(CachedDronePawn);
        }
        return;
    }

    // 드론 -> 지상
    ReturnToPlayer();
}

void AP3DPlayerController::ReturnToPlayer()
{
    if (!IsValid(CachedPlayerPawn)) return;

    Possess(CachedPlayerPawn);

 
    if (IsValid(CachedDronePawn))
    {
        CachedDronePawn->Destroy();
        CachedDronePawn = nullptr;
    }
}
