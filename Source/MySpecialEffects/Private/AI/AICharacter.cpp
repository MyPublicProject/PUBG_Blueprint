// Fill out your copyright notice in the Description page of Project Settings.

#include "AICharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon.h"

// Sets default values
AAICharacter::AAICharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	// GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	SpringComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringComp"));
	SpringComp->bUsePawnControlRotation = true;
	SpringComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringComp);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &AAICharacter::OnHealthChanged);

	ZoomeFOV = 65.0;
	ZoomInterSpeed = 20.0f;

	SetReplicates(true);
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void AAICharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = CameraComp->FieldOfView;

	if (Role = ROLE_Authority)// 在服务器上运行
	{
		//  生成武器
		FActorSpawnParameters SpawnParams;

		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<AWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "AI Weapon");
		}
	}
}


void AAICharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}


void AAICharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}


void AAICharacter::BeginCrouch()
{
	Crouch();
}


void AAICharacter::EndCrouch()
{
	UnCrouch();
}


void AAICharacter::BeginZoom()
{
	bWantsToZoom = true;
}


void AAICharacter::EndZoom()
{
	bWantsToZoom = false;
}


void AAICharacter::StareFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->BeginFire();
	}
}


void AAICharacter::EndFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->EndFire();
	}
}


void AAICharacter::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bDied)
	{
		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		DetachFromControllerPendingDestroy();
		SetLifeSpan(10.0f);
	}
}


// Called every frame
void AAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomeFOV : DefaultFOV;

	// 插值函数
	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterSpeed);

	CameraComp->SetFieldOfView(NewFOV);

}


// Called to bind functionality to input
void AAICharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAICharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAICharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &AAICharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &AAICharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AAICharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AAICharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &AAICharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &AAICharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AAICharacter::StareFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AAICharacter::EndFire);
}


FVector AAICharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}

void AAICharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAICharacter, CurrentWeapon);
	DOREPLIFETIME(AAICharacter, bDied);
}

