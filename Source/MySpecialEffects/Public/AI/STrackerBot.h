// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

UCLASS()
class MYSPECIALEFFECTS_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:

	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "MeshComp")
	class UStaticMeshComponent* StaticComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "MeshComp")
	class USHealthComponent* Health;

	UPROPERTY(VisibleDefaultsOnly, Category = "MeshComp")
	class USphereComponent* SphereComp;

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float OwningHealth, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	FVector GetNextPathPoint();			// —∞’““∆∂ØŒª÷√

	FVector NextPathPoint;

	void Death();						// À¿Õˆ∫Ø ˝

	bool bDeid;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	UParticleSystem* ExplodeEffects;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float RequiredDistanceToTarget;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float SelfDamageInterval;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float ExplodedDamage;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float ExplodefRadius;

	UMaterialInstanceDynamic* MaInst;

	void SelfDamage();

	bool bStartedSelfDestruction;

	FTimerHandle Handlde_SelfDamage;

public:	

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:

	void OnCheckNearbyBote();

	int32 PowerLevel;

};
