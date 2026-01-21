// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BasePawn.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class USpringArmComponent; // 스프링 암 관련 클래스 헤더
class UCameraComponent; // 카메라 관련 클래스 전방 선언
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;

//Enhanced Input에서 액션 값을 받을 때 사용하는 구조체
struct FInputActionValue;

UCLASS()
class PAWN3DCHARACTER_API ABasePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABasePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:	
	virtual void Tick(float DeltaTime) override;
	// ===== 충돌 캡슐 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	// ===== 스켈레탈 메시 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;
	// 스프링 암 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent * SpringArmComp;
	// 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComp;
	
	UPROPERTY(EditAnywhere, Category = "Move")
	float NormalSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Look")
	float MouseSensitivity = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Look")
	float MouseSensitivityPitch = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Look")
	float PitchMin = -60.f;

	UPROPERTY(EditAnywhere, Category = "Look")
	float PitchMax = 20.f;

	// ===== 애니용 =====
	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	float CurrentSpeed2D = 0.f;

	// Interact SM용 변수
	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	bool bWantsInteract = false;     // “요청”

	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	bool bIsInteracting = false;     // “진행중(락)”

	// AnimNotify에서 호출할 함수(Interact State 시작/끝을 확정)
	UFUNCTION(BlueprintCallable)
	void Notify_InteractStart();

	UFUNCTION(BlueprintCallable)
	void Notify_InteractEnd();

private:

	// ===== Input Callbacks (FInputActionValue 사용) =====
	void Move(const FInputActionValue& Value);
	void MoveCompleted(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);

	// ===== Cached Input =====
	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	FVector2D CachedLookInput = FVector2D::ZeroVector;
	FVector PrevLocation = FVector::ZeroVector;
};