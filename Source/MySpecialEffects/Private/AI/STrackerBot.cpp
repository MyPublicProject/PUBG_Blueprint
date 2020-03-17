// Fill out your copyright notice in the Description page of Project Settings.

#include "STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "SHealthComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SphereComponent.h"
#include "MySpecialEffectsCharacter.h"
#include "Math/UnrealMathUtility.h"


// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	StaticComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticComp"));
	StaticComp->SetCanEverAffectNavigation(false);
	StaticComp->SetSimulatePhysics(true);
	RootComponent = StaticComp;

	Health = CreateDefaultSubobject<USHealthComponent>(TEXT("Health"));
	Health->OnHealthChanged.AddDynamic(this,&ASTrackerBot::OnHealthChanged);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetupAttachment(StaticComp);
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);

	MovementForce = 1000;
	bUseVelocityChange = true;
	RequiredDistanceToTarget = 100;

	ExplodedDamage = 50;
	ExplodefRadius = 200;

	SelfDamageInterval = 0.5;
	
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority)
	{
		NextPathPoint = GetNextPathPoint();

		FTimerHandle TimerHandle_CheckPowerLevel;

		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearbyBote, 1.0f, true);
	}
}


void ASTrackerBot::OnHealthChanged(USHealthComponent* OwningHealthComp, float OwningHealth, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (MaInst == nullptr)
	{
		MaInst = StaticComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, StaticComp->GetMaterial(0));
	}

	if (MaInst)
	{
		MaInst->SetScalarParameterValue("LastTimeDamageTake", GetWorld()->TimeSeconds);
	}
		
	if(OwningHealth <= 0 && !bDeid)
	{
		bDeid = true;

		Death();

		GetWorldTimerManager().ClearTimer(Handlde_SelfDamage);
	}

	UE_LOG(LogTemp, Log, TEXT("Health %s of %s"), *FString::SanitizeFloat(OwningHealth), *GetName());
}


FVector ASTrackerBot::GetNextPathPoint()
{
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);

	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		return NavPath->PathPoints[1];
	}

	return GetActorLocation();
}


void ASTrackerBot::Death()
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplodeEffects, GetActorLocation());

	if (Role == ROLE_Authority)
	{
		TArray<AActor* > IgnoreActor;
		IgnoreActor.Add(this);

		// Apply Damage
		UGameplayStatics::ApplyRadialDamage(this, ExplodedDamage, GetActorLocation(), ExplodefRadius, nullptr, IgnoreActor, this, GetInstigatorController(), false);

		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplodefRadius, 12, FColor::Red, false, 5.0f, 0.0f);

		// SetLifeSpan(0.2f);
		Destroy();
	}
}


// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_Authority && !bDeid)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget < RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();

			DrawDebugString(GetWorld(), GetActorLocation(), TEXT("End!"));
		}
		else
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation();

			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			StaticComp->AddForce(ForceDirection, NAME_None, true);

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), NextPathPoint, 32, FColor::Red, false, 0.0f, 0, 1.0f);
		}

		DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 0.0f, 1.0f);
	}
}


void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (!bStartedSelfDestruction && !bDeid)
	{
		ACharacter* PlayerCharacter = Cast<AMySpecialEffectsCharacter>(OtherActor);

		if (PlayerCharacter)
		{
			if (Role == ROLE_Authority)
			{
				GetWorldTimerManager().SetTimer(Handlde_SelfDamage, this, &ASTrackerBot::SelfDamage, SelfDamageInterval, true, 0.0f);

			}
			bStartedSelfDestruction = true;
		}
	}
}


void ASTrackerBot::SelfDamage()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}


void ASTrackerBot::OnCheckNearbyBote()
{
	const float Radius = 600;

	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.0f);

	int32 NrOfBots = 0;
	for (FOverlapResult Result : Overlaps)
	{
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());

		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;

	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);

	if (MaInst == nullptr)
	{
		MaInst = StaticComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, StaticComp->GetMaterial(0));
	}
	if (MaInst)
	{
		float Alpha = PowerLevel / (float)MaxPowerLevel;

		MaInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}
}