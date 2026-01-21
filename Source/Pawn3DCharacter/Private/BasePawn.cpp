#include "BasePawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "P3DPlayerController.h"
#include "Engine/Engine.h"

ABasePawn::ABasePawn()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(true);

    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    SetRootComponent(CapsuleComp);

    CapsuleComp->InitCapsuleSize(42.f, 96.f);
    CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

    MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CapsuleComp);

    MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
    MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

    CapsuleComp->SetSimulatePhysics(false);
    MeshComp->SetSimulatePhysics(false);

    SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
    SpringArmComp->SetupAttachment(CapsuleComp);
    SpringArmComp->TargetArmLength = 300.f;
    SpringArmComp->bUsePawnControlRotation = false;

    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
    CameraComp->SetupAttachment(SpringArmComp);
    CameraComp->bUsePawnControlRotation = false;
}

void ABasePawn::BeginPlay()
{
    Super::BeginPlay();
    PrevLocation = GetActorLocation();
}

void ABasePawn::Move(const FInputActionValue& Value)
{
    // Interact 중엔 입력이 들어와도 이동 입력 자체를 무시(락)
    if (bIsInteracting)
    {
        CachedMoveInput = FVector2D::ZeroVector;
        return;
    }

    const FVector2D MoveInput = Value.Get<FVector2D>();

    // 완전 0이면 비우기
    if (MoveInput.IsNearlyZero())
    {
        CachedMoveInput = FVector2D::ZeroVector;
        return;
    }

    CachedMoveInput = MoveInput;
}

void ABasePawn::MoveCompleted(const FInputActionValue& Value)
{
    CachedMoveInput = FVector2D::ZeroVector;
}

void ABasePawn::Look(const FInputActionValue& Value)
{
    const FVector2D LookInput = Value.Get<FVector2D>();
    CachedLookInput = LookInput;
}

void ABasePawn::Interact(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[BasePawn] Interact CALLED"));

    //이미 상호작용 중이면 무시
    if (bIsInteracting)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BasePawn] Already interacting -> ignore"));
        return;
    }
    bWantsInteract = true;

    // 이동 입력은 즉시 끊어서, 상호작용 시작 시 미끄러지는 느낌 방지
    CachedMoveInput = FVector2D::ZeroVector;
}

void ABasePawn::Notify_InteractStart()
{
    // Interact State(또는 Interact 애니) 시작 프레임에서 호출
    bIsInteracting = true;
    bWantsInteract = false; // 요청 소비(중복 진입 방지)

    // 이동 완전 차단
    CachedMoveInput = FVector2D::ZeroVector;

    UE_LOG(LogTemp, Warning, TEXT("[BasePawn] Notify_InteractStart"));
}

void ABasePawn::Notify_InteractEnd()
{
    // Interact 애니 마지막 프레임에서 호출
    bIsInteracting = false;

    if (AP3DPlayerController* PC = Cast<AP3DPlayerController>(GetController())) { PC->ToggleDrone(); }
}


void ABasePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (AP3DPlayerController* PlayerController = Cast<AP3DPlayerController>(GetController()))
        {
            if (PlayerController->MoveAction)
            {
                EnhancedInput->BindAction(PlayerController->MoveAction, ETriggerEvent::Triggered, this, &ABasePawn::Move);

                EnhancedInput->BindAction(PlayerController->MoveAction, ETriggerEvent::Completed, this, &ABasePawn::MoveCompleted);
                EnhancedInput->BindAction(PlayerController->MoveAction, ETriggerEvent::Canceled, this, &ABasePawn::MoveCompleted);
            }

            if (PlayerController->LookAction)
            {
                EnhancedInput->BindAction(PlayerController->LookAction, ETriggerEvent::Triggered, this, &ABasePawn::Look);
            }

            if (PlayerController->InteractAction)
            {
                EnhancedInput->BindAction(PlayerController->InteractAction, ETriggerEvent::Started, this, &ABasePawn::Interact);
            }
        }
    }
}

void ABasePawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const float SafeDT = FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);

    // Interact 중이면 이동/회전 입력 적용 자체를 막고 싶다면 여기서도 한 번 더 방어
    const bool bCanControl = IsLocallyControlled() && !bIsInteracting;

    if (bCanControl)
    {
        // ===== 이동(평면) =====
        if (!CachedMoveInput.IsNearlyZero())
        {
            const float CurSpeed = NormalSpeed;

            const FVector LocalDelta(
                CachedMoveInput.Y,
                CachedMoveInput.X,
                0.f
            );

            AddActorLocalOffset(LocalDelta * CurSpeed * DeltaTime, false);
        }

        // ===== 회전(직접) =====
        if (!CachedLookInput.IsNearlyZero())
        {
            const float YawDelta = CachedLookInput.X * MouseSensitivity;
            const float PitchDelta = CachedLookInput.Y * MouseSensitivityPitch;

            AddActorLocalRotation(FRotator(0.f, YawDelta, 0.f));

            if (SpringArmComp)
            {
                FRotator ArmRot = SpringArmComp->GetRelativeRotation();
                const float CurPitch = FRotator::NormalizeAxis(ArmRot.Pitch);
                const float NewPitch = FMath::Clamp(CurPitch + PitchDelta, PitchMin, PitchMax);

                ArmRot.Pitch = NewPitch;
                ArmRot.Roll = 0.f;
                SpringArmComp->SetRelativeRotation(ArmRot);
            }

            CachedLookInput = FVector2D::ZeroVector;
        }
    }
    else
    {
        // Interact 중이면 입력 캐시를 정리
        if (bIsInteracting)
        {
            CachedMoveInput = FVector2D::ZeroVector;
            CachedLookInput = FVector2D::ZeroVector;
        }
    }

    // ===== 속도/이동 상태 계산 =====
    const FVector NewLoc = GetActorLocation();
    const FVector WorldDelta = NewLoc - PrevLocation;

    CurrentSpeed2D = FVector(WorldDelta.X, WorldDelta.Y, 0.f).Size() / SafeDT;

    // Interact 중에는 강제로 “이동 아님”
    if (bIsInteracting)
    {
        bIsMoving = false;
    }
    else
    {
        constexpr float StartMoveSpeed = 20.f;
        constexpr float StopMoveSpeed = 10.f;

        if (!bIsMoving) bIsMoving = (CurrentSpeed2D > StartMoveSpeed);
        else            bIsMoving = (CurrentSpeed2D > StopMoveSpeed);
    }

    PrevLocation = NewLoc;
}
