#include "DronePawn.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

#include "P3DPlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


// 생성자 / 기본

ADronePawn::ADronePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// ===== 1) Root Collision =====
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SetRootComponent(SphereComp);
	SphereComp->InitSphereRadius(30.f);
	SphereComp->SetCollisionProfileName(TEXT("Pawn"));
	SphereComp->SetSimulatePhysics(false);

	// ===== 2) Mesh =====
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(SphereComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetSimulatePhysics(false);

	// ===== 3) SpringArm =====
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(SphereComp);
	SpringArmComp->TargetArmLength = 320.f;
	SpringArmComp->bUsePawnControlRotation = false;

	// ===== 4) Camera =====
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);
	CameraComp->bUsePawnControlRotation = false;

	// ===== (기본값들: 헤더에 UPROPERTY로 두는 걸 권장) =====
	// Gravity / Ground
	bEnableGravity = true;
	GravityAccel = -980.f;

	GroundProbeDistance = 10.f;   
	GroundSnapMax = 20.f;
	GroundStickForce = 2500.f;
	CoyoteTime = 0.08f;

	bUseSphereSweep = true;

	//접지 확정 파라미터(반지름 오판정 제거)
	GroundedTolerance = 2.0f;     // Gap 허용(1~3 추천)
	WalkableFloorZ = 0.6f;        // 바닥 노멀Z(0.55~0.75 튜닝)

	// Thrust
	ThrustAccel = 2200.f;
	ThrustDrag = 6.0f;
	MaxRiseSpeed = 900.f;
	MaxFallSpeed = 2200.f;

	// Move
	NormalSpeed = 900.f;
	AirControlMultiplier = 0.4f;

	// Look/Roll
	MouseSensitivity = 0.15f;
	MouseSensitivityPitch = 0.15f;
	PitchMin = -80.f;
	PitchMax = 80.f;

	RollSpeedDegPerSec = 140.f;
	RollMaxAbs = 65.f;

	// State
	VerticalVelocity = 0.f;
	bGrounded = false;
	TimeSinceGrounded = 999.f;

	// Debug
	bDrawGroundDebug = false;
}

void ADronePawn::BeginPlay()
{
	Super::BeginPlay();
}

// 입력 바인딩 (기존 유지)

void ADronePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AP3DPlayerController* PlayerController = Cast<AP3DPlayerController>(GetController()))
		{
			// Move2D (WASD)
			if (PlayerController->Move2DAction)
			{
				EnhancedInput->BindAction(PlayerController->Move2DAction, ETriggerEvent::Triggered, this, &ADronePawn::Move2D);
				EnhancedInput->BindAction(PlayerController->Move2DAction, ETriggerEvent::Completed, this, &ADronePawn::Move2D);
			}

			// UpDown (Space/Shift)
			if (PlayerController->UpDownAction)
			{
				EnhancedInput->BindAction(PlayerController->UpDownAction, ETriggerEvent::Triggered, this, &ADronePawn::UpDown);
				EnhancedInput->BindAction(PlayerController->UpDownAction, ETriggerEvent::Completed, this, &ADronePawn::UpDown);
			}

			// Look (Mouse XY)
			if (PlayerController->LookAction)
			{
				EnhancedInput->BindAction(PlayerController->LookAction, ETriggerEvent::Triggered, this, &ADronePawn::Look);
			}

			// Roll (Q/E or Wheel)
			if (PlayerController->RollAction)
			{
				EnhancedInput->BindAction(PlayerController->RollAction, ETriggerEvent::Triggered, this, &ADronePawn::Roll);
				EnhancedInput->BindAction(PlayerController->RollAction, ETriggerEvent::Completed, this, &ADronePawn::Roll);
			}

			// Return
			if (PlayerController->ReturnToPlayerAction)
			{
				EnhancedInput->BindAction(PlayerController->ReturnToPlayerAction, ETriggerEvent::Started, this, &ADronePawn::ReturnToPlayer);
			}
		}
	}
}

void ADronePawn::Move2D(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (MoveInput.IsNearlyZero())
	{
		CachedMoveInput = FVector2D::ZeroVector;
		return;
	}

	CachedMoveInput = MoveInput;
}

void ADronePawn::UpDown(const FInputActionValue& Value)
{
	CachedUpDownInput = Value.Get<float>();
}

void ADronePawn::Look(const FInputActionValue& Value)
{
	CachedLookInput = Value.Get<FVector2D>(); // X=Yaw, Y=Pitch
}

void ADronePawn::Roll(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	CachedRollInput = FMath::IsNearlyZero(Axis) ? 0.f : Axis;
}

void ADronePawn::ReturnToPlayer(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[Drone] ReturnToPlayer fired. Type=%d"), (int32)Value.GetValueType());

	if (AP3DPlayerController* PC = Cast<AP3DPlayerController>(GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drone] Calling PC->ReturnToPlayer()"));
		PC->ReturnToPlayer();
	}
}

// Tick (완성형)

void ADronePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 0) 회전
	TickRotation(DeltaTime);

	// 1) 바닥 탐색(후보 찾기)
	FHitResult GroundHit;
	const bool bHitGround = ProbeGround(GroundHit);

	// 1-1) 접지 "확정"은 Hit가 아니라 Gap + 바닥 노멀로 판단
	float Gap = 999999.f;           // 스피어 표면 ~ 바닥 틈(근사)
	bool bIsFloor = false;

	const float SphereR = SphereComp ? SphereComp->GetScaledSphereRadius() : 45.f;

	if (bHitGround)
	{
		Gap = GroundHit.Distance - SphereR;                     // 표면-바닥 틈(대략)
		bIsFloor = (GroundHit.ImpactNormal.Z >= WalkableFloorZ);
	}

	const bool bActuallyGrounded = bHitGround && bIsFloor && (Gap <= GroundedTolerance);

	if (bActuallyGrounded)
	{
		TimeSinceGrounded = 0.f;
	}
	else
	{
		TimeSinceGrounded += DeltaTime;
	}

	bGrounded = bActuallyGrounded || (TimeSinceGrounded <= CoyoteTime);

	// 2) 수평 이동 (Yaw-only 평면 이동: 기울기와 무관, Z=0)
	float ControlMul = 1.f;
	if (bEnableGravity && !bGrounded)
	{
		ControlMul = FMath::Clamp(AirControlMultiplier, 0.f, 1.f);
	}

	const float CurSpeed = NormalSpeed * ControlMul;

	const float RightAxis = CachedMoveInput.X;   // A/D
	const float ForwardAxis = CachedMoveInput.Y; // W/S

	if (!FMath::IsNearlyZero(RightAxis) || !FMath::IsNearlyZero(ForwardAxis))
	{
		const FRotator YawOnly(0.f, GetActorRotation().Yaw, 0.f);
		const FVector Fwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
		const FVector Rgt = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

		FVector WorldHorizontal = (Fwd * ForwardAxis + Rgt * RightAxis) * CurSpeed * DeltaTime;
		WorldHorizontal.Z = 0.f;

		AddActorWorldOffset(WorldHorizontal, true);
	}

	// 3) 수직 이동 (월드) = 중력 + 추진(가속/감속)
	if (bEnableGravity)
	{
		TickVertical_World(DeltaTime, GroundHit, bHitGround, Gap, bIsFloor, bActuallyGrounded);
	}
	else
	{
		// 중력 OFF 모드 (즉시형)
		const float DirectZ = CachedUpDownInput * (NormalSpeed * 0.8f);
		if (!FMath::IsNearlyZero(DirectZ))
		{
			AddActorWorldOffset(FVector(0, 0, DirectZ * DeltaTime), true);
		}

		VerticalVelocity = 0.f;
		bGrounded = false;
		TimeSinceGrounded = 999.f;
	}

	// Debug
	if (bDrawGroundDebug && GetWorld())
	{
		const FVector P = GetActorLocation();
		DrawDebugString(GetWorld(), P + FVector(0, 0, 90.f),
			FString::Printf(TEXT("Hit:%d Floor:%d  Gap:%.2f  Grounded:%d  VelZ:%.1f"),
				bHitGround ? 1 : 0, bIsFloor ? 1 : 0, Gap, bGrounded ? 1 : 0, VerticalVelocity),
			nullptr, FColor::White, 0.f, true);
	}
}

// 회전
void ADronePawn::TickRotation(float DeltaTime)
{
	if (!CachedLookInput.IsNearlyZero())
	{
		const float YawDelta = CachedLookInput.X * MouseSensitivity;
		const float PitchDelta = CachedLookInput.Y * MouseSensitivityPitch;

		FRotator Cur = GetActorRotation();

		const float CurPitch = FRotator::NormalizeAxis(Cur.Pitch);
		const float NewPitch = FMath::Clamp(CurPitch + PitchDelta, PitchMin, PitchMax);

		Cur.Yaw = FRotator::NormalizeAxis(Cur.Yaw + YawDelta);
		Cur.Pitch = NewPitch;

		SetActorRotation(Cur);
		CachedLookInput = FVector2D::ZeroVector;
	}

	if (!FMath::IsNearlyZero(CachedRollInput))
	{
		FRotator Cur = GetActorRotation();

		const float RollDelta = CachedRollInput * RollSpeedDegPerSec * DeltaTime;
		const float CurRoll = FRotator::NormalizeAxis(Cur.Roll);
		const float NewRoll = FMath::Clamp(CurRoll + RollDelta, -RollMaxAbs, RollMaxAbs);

		Cur.Roll = NewRoll;
		SetActorRotation(Cur);
	}
}


// 수직 처리: 월드 수직낙하 + 스냅/떨림 방지 + 추진 가속/감속
void ADronePawn::TickVertical_World(
	float DeltaTime,
	const FHitResult& GroundHit,
	bool bHitGround,
	float Gap,
	bool bIsFloor,
	bool bActuallyGrounded
)
{
	// 1) 추진 가속(입력 기반)
	const float Input = FMath::Clamp(CachedUpDownInput, -1.f, 1.f);

	// 입력이 있으면 가속 누적
	VerticalVelocity += (Input * ThrustAccel) * DeltaTime;

	// 입력이 거의 없으면 드래그로 서서히 0으로
	if (FMath::IsNearlyZero(Input, 0.02f))
	{
		VerticalVelocity = FMath::FInterpTo(VerticalVelocity, 0.f, DeltaTime, ThrustDrag);
	}

	// 2) 중력/접지 스틱(떨림 방지)
	if (!bGrounded)
	{
		VerticalVelocity += GravityAccel * DeltaTime; // 월드 -Z
	}
	else
	{
		// 상승 입력(또는 이미 상승 속도)이면 "붙이기" 금지 + 이륙
		const bool bTryingToRise = (Input > 0.05f) || (VerticalVelocity > 0.f);

		if (bTryingToRise)
		{
			bGrounded = false;
			TimeSinceGrounded = CoyoteTime + 1.f; // 코요테 끊기(이륙 즉시 공중으로)
		}
		else
		{
			// 내려가려는 상황에서만 살짝 붙임(떨림 제거)
			VerticalVelocity = FMath::Min(VerticalVelocity, -GroundStickForce * DeltaTime);
		}
	}

	// 3) 속도 제한
	VerticalVelocity = FMath::Clamp(VerticalVelocity, -MaxFallSpeed, MaxRiseSpeed);

	// 4) 이번 프레임 이동량(월드 Z)
	float DeltaZ = VerticalVelocity * DeltaTime;

	// 5) 접지 스냅(진짜 바닥인데 내려가려는 상황이면 딱 붙이기)
	if (bHitGround && bIsFloor)
	{
		if (Gap >= 0.f && Gap <= GroundSnapMax && DeltaZ < 0.f)
		{
			// Gap 중에서 ProbeDistance만큼은 남기고 내려가서 안정화(경사에서 끼임 방지)
			const float SnapDown = FMath::Max(0.f, Gap - GroundProbeDistance);
			DeltaZ = -SnapDown;
			VerticalVelocity = 0.f;
		}
	}

	// 6) 실제 이동(월드 Z) + 충돌 처리
	FHitResult MoveHit;
	AddActorWorldOffset(FVector(0, 0, DeltaZ), true, &MoveHit);

	if (MoveHit.bBlockingHit)
	{
		VerticalVelocity = 0.f;

		// 바닥으로 충돌한 경우 grounded 확정
		if (MoveHit.ImpactNormal.Z >= WalkableFloorZ)
		{
			bGrounded = true;
			TimeSinceGrounded = 0.f;
		}
	}
}
// Ground Probe: Sphere Sweep 권장 (채널 설정 중요)

bool ADronePawn::ProbeGround(FHitResult& OutHit) const
{
	if (!GetWorld()) return false;

	const FVector Start = GetActorLocation();
	const float SphereR = SphereComp ? SphereComp->GetScaledSphereRadius() : 45.f;

	// 중심에서 (반지름 + ProbeDistance) 만큼 아래를 확인
	const float TraceLen = SphereR + GroundProbeDistance;
	const FVector End = Start + FVector(0, 0, -TraceLen);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DroneGroundProbe), false);
	Params.AddIgnoredActor(this);

	const ECollisionChannel Channel = ECC_Visibility;

	bool bHit = false;

	if (!bUseSphereSweep)
	{
		bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);
	}
	else
	{
		// 살짝 작은 반지름이 모서리/벽 긁힘에 안정적
		const float SweepR = FMath::Max(1.f, SphereR - 2.f);
		const FCollisionShape Shape = FCollisionShape::MakeSphere(SweepR);

		bHit = GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, Channel, Shape, Params);
	}

	// Debug draw
	if (bDrawGroundDebug && GetWorld())
	{
		const FColor C = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), Start, End, C, false, 0.f, 0, 1.2f);

		if (bHit)
		{
			DrawDebugPoint(GetWorld(), OutHit.ImpactPoint, 10.f, FColor::Yellow, false, 0.f);
		}
	}

	return bHit;
}
