#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DronePawn.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;

// Enhanced Input에서 액션 값을 받을 때 사용하는 구조체
struct FInputActionValue;

UCLASS()
class PAWN3DCHARACTER_API ADronePawn : public APawn
{
	GENERATED_BODY()

public:
	ADronePawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	virtual void Tick(float DeltaTime) override;

	// ===== Root Collision =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereComp = nullptr;

	// ===== Mesh =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp = nullptr;

	// ===== Camera =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComp = nullptr;

	// =========================================================
	// Gravity / Ground / Thrust / Move  튜닝 파라미터
	// =========================================================

	// ===== Gravity =====
	UPROPERTY(EditAnywhere, Category = "Drone|Gravity")
	bool bEnableGravity = true;

	UPROPERTY(EditAnywhere, Category = "Drone|Gravity")
	float GravityAccel = -980.f; // cm/s^2

	// ===== Ground Probe =====
	// 중심에서 (반지름 + GroundProbeDistance) 만큼 아래를 체크
	UPROPERTY(EditAnywhere, Category = "Drone|Ground")
	float GroundProbeDistance = 12.f;

	// 바닥이 가까우면 스냅(떨림 제거)
	UPROPERTY(EditAnywhere, Category = "Drone|Ground")
	float GroundSnapMax = 20.f;

	// 접지 시 살짝 아래로 붙이는 힘(떠오름/미세 떨림 방지)
	UPROPERTY(EditAnywhere, Category = "Drone|Ground")
	float GroundStickForce = 2500.f;

	// 바닥에서 살짝 떨어져도 접지로 취급하는 시간
	UPROPERTY(EditAnywhere, Category = "Drone|Ground")
	float CoyoteTime = 0.08f;

	// 라인트레이스 대신 스윕 권장
	UPROPERTY(EditAnywhere, Category = "Drone|Ground")
	bool bUseSphereSweep = true;

	//  "진짜 접지" 확정용: 반지름 때문에 떠 있는데 접지로 오판정되는 문제 제거
	// Gap = (Hit.Distance - SphereRadius) 가 이 값 이하일 때만 접지 확정
	UPROPERTY(EditAnywhere, Category = "Drone|Ground")
	float GroundedTolerance = 2.0f; // 1~3 튜닝

	// 바닥 노멀 조건: 벽/모서리 등에 스윕이 걸려 접지로 오판정되는 문제 제거
	UPROPERTY(EditAnywhere, Category = "Drone|Ground")
	float WalkableFloorZ = 0.6f; // 0.55~0.75 튜닝 (클수록 더 "평평한" 바닥만 인정)

	// ===== Vertical Thrust (Space/Shift) =====
	UPROPERTY(EditAnywhere, Category = "Drone|Thrust")
	float ThrustAccel = 2200.f; // cm/s^2

	// 입력이 없을 때 속도를 0으로 끌어오는 감속 계수(클수록 빨리 멈춤)
	UPROPERTY(EditAnywhere, Category = "Drone|Thrust")
	float ThrustDrag = 6.0f; // 1/s

	UPROPERTY(EditAnywhere, Category = "Drone|Thrust")
	float MaxRiseSpeed = 900.f; // cm/s

	UPROPERTY(EditAnywhere, Category = "Drone|Thrust")
	float MaxFallSpeed = 2200.f; // cm/s (절대값)

	// ===== Move / Air control =====
	UPROPERTY(EditAnywhere, Category = "Drone|Move")
	float NormalSpeed = 900.f;

	UPROPERTY(EditAnywhere, Category = "Drone|Move")
	float AirControlMultiplier = 0.4f; // 0.3~0.5 권장

	// ===== Debug =====
	UPROPERTY(EditAnywhere, Category = "Drone|Debug")
	bool bDrawGroundDebug = false;

	// 6DOF 회전 튜닝 파라미터 (cpp에서 사용)
	UPROPERTY(EditAnywhere, Category = "Drone|Look")
	float MouseSensitivity = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Drone|Look")
	float MouseSensitivityPitch = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Drone|Look")
	float PitchMin = -80.f;

	UPROPERTY(EditAnywhere, Category = "Drone|Look")
	float PitchMax = 80.f;

	UPROPERTY(EditAnywhere, Category = "Drone|Roll")
	float RollSpeedDegPerSec = 140.f;

	UPROPERTY(EditAnywhere, Category = "Drone|Roll")
	float RollMaxAbs = 65.f;

private:
	// ===== Input Callbacks =====
	void Move2D(const FInputActionValue& Value);        // Axis2D: WASD
	void UpDown(const FInputActionValue& Value);        // Axis1D: Space/Shift
	void Look(const FInputActionValue& Value);          // Axis2D: Mouse
	void Roll(const FInputActionValue& Value);          // Axis1D: Q/E or Wheel
	void ReturnToPlayer(const FInputActionValue& Value);// Digital

private:
	// ===== Cached Input =====
	FVector2D CachedMoveInput = FVector2D::ZeroVector; // X=Right, Y=Forward
	float     CachedUpDownInput = 0.f;                 // +Up / -Down
	FVector2D CachedLookInput = FVector2D::ZeroVector; // X=Yaw, Y=Pitch
	float     CachedRollInput = 0.f;                   // Roll 입력(축)

private:
	// ===== State =====
	float VerticalVelocity = 0.f;     // cm/s
	bool  bGrounded = false;
	float TimeSinceGrounded = 999.f; // 코요테 누적

private:
	// ===== Internals =====
	void TickRotation(float DeltaTime);

	// 수직(월드 Z): 중력 + 추진(가속/감속) + 스냅/떨림 방지
	// 접지 오판정 제거/이륙 허용/스냅 정밀화를 위해 Gap/바닥노멀 정보도 받음
	void TickVertical_World(
		float DeltaTime,
		const struct FHitResult& GroundHit,
		bool bHitGround,
		float Gap,
		bool bIsFloor,
		bool bActuallyGrounded
	);

	// Ground probe
	bool ProbeGround(struct FHitResult& OutHit) const;
};
