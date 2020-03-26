// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"



static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(TEXT("COOP.DebugWeapons"), DebugWeaponDrawing, TEXT("wu qi tiao shi xian @"), ECVF_Cheat);


// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SkeletalComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalComp"));
	RootComponent = SkeletalComp;

	MuzzleSocketNmae = "MuzzleSocket";
	TracerCompNmae = "Target";

	BaseDamage = 20.0f;
	RateOfFire = 600.0;
	BulletSpread = 1.0f;

	SetReplicates(true);

	// 网络更新
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}


void AWeapon::PlayFireEffects(FVector TraceEnd)
{

	// 开火特效
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, SkeletalComp, MuzzleSocketNmae);
	}

	if (TracerEffect)
	{
		//  获得骨骼某一块 的位置
		FVector MuzzleLocation = SkeletalComp->GetSocketLocation(MuzzleSocketNmae);

		// 生成的烟雾特效
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);

		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerCompNmae, TraceEnd);
		}
	}

	// 抖屏
	APawn* MyPawn = Cast<APawn>(GetOwner());
	if (MyPawn)
	{
		APlayerController* PC = Cast<APlayerController>(MyPawn->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(CameraShake);
		}
	}
}


void AWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	// 击中不同部位的伤害
	UParticleSystem* SelectedEffect = nullptr;

	SelectedEffect = FleshImpactEffect;

	// 击中特效
	if (SelectedEffect)
	{
		FVector MuzzleLocation = SkeletalComp->GetSocketLocation(MuzzleSocketNmae);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}


void AWeapon::Fire()
{
	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	AActor* MyOwer = GetOwner();
	if (MyOwer)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwer->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		// 子弹扫射
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwer);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;		// 是否返回碰撞 PhysicalMaterial

		// 枪的烟雾特效的末尾
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;

		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TracerEndPoint, ECC_GameTraceChannel1, QueryParams))
		{
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;

// 			if (SurfaceType == SURFACE_FLESHVULNERABLE)
// 			{
// 				ActualDamage *= 4.0f;
// 			}

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwer->GetInstigatorController(), MyOwer, DamageType);

			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

			// Hit.ImpactNormal
			TracerEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TracerEndPoint, FColor::Red, false, 1.0f, 0, 1.0f);
		}

		PlayFireEffects(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		// 获得代码运行到这里的瞬时时间
		LastFireTime = GetWorld()->TimeSeconds;
	}
}


void AWeapon::OnRep_HitScanTrace()
{
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}


void AWeapon::ServerFire_Implementation()
{
	Fire();
}


bool AWeapon::ServerFire_Validate()
{
	return true;
}


void AWeapon::BeginFire()
{
	float FireDelay = LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds;

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AWeapon::Fire, TimeBetweenShots, true, FireDelay);
}


void AWeapon::EndFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}


// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AWeapon, HitScanTrace, COND_SkipOwner);
}