// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSProjectCharacter.h"

#include "AITypes.h"
#include "AnimationCoreLibrary.h"
#include "HealthComponent.h"
#include "InteractComp.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

AFPSProjectCharacter::AFPSProjectCharacter()
{
	// Character doesnt have a rifle at start
	bHasRifle = false;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));
	
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	
	_HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HeatlhComp"));
	_InteractComp = CreateDefaultSubobject<UInteractComp>(TEXT("InteractComp"));
}

void AFPSProjectCharacter::BeginPlay()
{
	Super::BeginPlay();
	WallJumpsLeft = WallJumps;

	OnDashUpdate.Broadcast(CurrentDashes, _Dashes);
}

void AFPSProjectCharacter::Move(const FInputActionValue& Value)
{
	if(bMovementLocked)
		return;
	
	FVector2D MovementVector = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), MovementVector.Y);
	AddMovementInput(GetActorRightVector(), MovementVector.X);
}

void AFPSProjectCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AFPSProjectCharacter::SprintStart()
{
	if(!bIsCrouched)
	{
		GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed * 1.2;
		LerpCamFOV(DefaultFieldOfView*1.1,GetFirstPersonCameraComponent()->FieldOfView);
	}
}

void AFPSProjectCharacter::SprintStop()
{
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
	LerpCamFOV(DefaultFieldOfView,GetFirstPersonCameraComponent()->FieldOfView);
}

void AFPSProjectCharacter::StartCrouch()
{
	SprintStop();
	Crouch();
}

void AFPSProjectCharacter::StopCrouch()
{
	UnCrouch();
}

void AFPSProjectCharacter::Slide()
{
	UE_LOG(LogTemp,Error,TEXT("Sliding"));
}

void AFPSProjectCharacter::Dash()
{
	if(bMovementLocked || CurrentDashes <= 0)
		return;
	
	FVector Speed = GetActorForwardVector();
	Speed.Normalize(0.01f);
	Speed.Z = 0;
	LaunchCharacter(Speed * _DashForce,true,false);
	CurrentDashes-=1;
	if(_DashTimer.IsValid())
		GetWorldTimerManager().ClearTimer(_DashTimer);
	GetWorld()->GetTimerManager().SetTimer(_DashTimer,this,&AFPSProjectCharacter::DashRecharge,_DashChargeRate,true);
	
	OnDashUpdate.Broadcast(CurrentDashes,_Dashes);
}

void AFPSProjectCharacter::DashRecharge()
{
	CurrentDashes++;
	OnDashUpdate.Broadcast(CurrentDashes,_Dashes);
	if(CurrentDashes >= _Dashes)
	{
		GetWorldTimerManager().ClearTimer(_DashTimer);
		_DashTimer.Invalidate();
		CurrentDashes = _Dashes;
	}
}



void AFPSProjectCharacter::WallRun_Implementation()
{

	/*			-Wall Checks Conditions-
	 * 0-BothWalls Not Hit -> GTFO of the function & Set Current Wall False
	 * 1-RightWall Not Hit -> Must Be Left
	 * 2-LeftWall Not Hit -> Must Be Right
	 * 3-Both Walls Hit -> Get the Closest
	 * 
	 * 4-Is Desired the Current Wall
	 *		4a Is Wall Forward Vec same?
	 *		4 YES -> return
	 *		4 No -> Rotate Player
	 */
	
	if(!PlayerCanWallRide())
		return;
	
	FHitResult L_Wall = CheckWallInDirection(false);
	FHitResult R_Wall = CheckWallInDirection(true);
	FHitResult DesiredWall;

	
	
	if(!L_Wall.bBlockingHit && !R_Wall.bBlockingHit) //*-0-* 
	{
		CurrentWall = DesiredWall;
		DetatchFromWall();
		return;
	} 
	
	if(R_Wall.Distance == 0)				//*-1-*
		DesiredWall = L_Wall;
	else if(L_Wall.Distance == 0)			//*-2-*
		DesiredWall = R_Wall;
	else									//*-3-*
		L_Wall.Distance < R_Wall.Distance ? DesiredWall = L_Wall : DesiredWall = R_Wall;

	if(!DesiredWall.GetActor()->ActorHasTag("WallRun"))
		return;
	
	// Dot Product to See if the players velocity is going same direction
	/*
	FVector ForwardDir = GetVelocity();
	ForwardDir.Normalize();
	if(FVector::DotProduct(ForwardDir,GetWallForwardVector(DesiredWall)) < 0.7f)
	{
		UE_LOG(LogTemp,Warning,TEXT("NOT LOOKING SAME DIR"));
		return;
	}
	 */
	
	
	if(DesiredWall.GetActor()==R_Wall.GetActor())
		bRightWall = true;
	else
		bRightWall = false;

	
	if(DesiredWall.GetActor() == CurrentWall.GetActor())
	{
		PlayerGrabWall(DesiredWall);
		RotateTowardsForward(GetWallForwardVector(DesiredWall));
	}
	else
		PlayerGrabWall(DesiredWall);
}

void AFPSProjectCharacter::WhileOnWall()
{

}



bool AFPSProjectCharacter::PlayerCanWallRide()
{
	//Is Falling & Has Wall Jumps -> return true
	if(bIsOnWall)
		return true;
	PlayerSpeed = GetVelocity().Length();
	if(!GetMovementComponent()->IsFalling()|| WallJumpsLeft <= 0||PlayerSpeed < 520.0f) 
		return false;
	return true;
}

FHitResult AFPSProjectCharacter::CheckWallInDirection(bool CheckRightWall)
{
	FVector Direction;
	if(CheckRightWall)
		Direction = GetActorRightVector();
	else
		Direction = GetActorRightVector()*-1;

	FHitResult WallHit;
	FVector StartLoc = GetActorLocation();
	UKismetSystemLibrary::LineTraceSingle(
	GetWorld(),
	StartLoc,
	StartLoc+Direction *120.0f,
	UEngineTypes::ConvertToTraceType(ECC_Visibility),
	true,
	{},
	EDrawDebugTrace::ForDuration,
	WallHit,
	true,
	FLinearColor::Red,
	FLinearColor::Green,
	5);
	return WallHit;
}

bool AFPSProjectCharacter::PlayerGrabWall(FHitResult Wall)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if(!PlayerController)
		return false;
	
	if (!bIsOnWall)
	{
		//Player Movement Functions
		GetCharacterMovement()->GravityScale = 0;
		GetCharacterMovement()->Velocity.Z = 0;
		bIsOnWall = true;
		WallTilt(bRightWall);
	}
	bMovementLocked = true;
	CurrentWall = Wall;
	RotateTowardsForward(GetWallForwardVector(Wall));
	//Disable Player Movement Whilst on Wall
	//Make Player Hug Wall
	
	//Camera Functions
	PlayerController->PlayerCameraManager->ViewYawMax =  GetActorRotation().Yaw + 30;
	PlayerController->PlayerCameraManager->ViewYawMin = GetActorRotation().Yaw -30;
	return true;
}

FVector AFPSProjectCharacter::GetWallForwardVector(FHitResult Wall)
{
	/*Invert the quarternion rotation so that the player will alway have a forward */
	int Invert = 1;
	if(bRightWall)
		Invert = -1;

	const FVector ForwardDir = Wall.Normal.Rotation().Quaternion().GetRightVector() *-1 * Invert;
	return ForwardDir;

	
	/*
	-------------------[Debug For Line Traces (Using Quaternions, May need these later)]-------------------

	//Wall Normal
	DrawDebugLine(GetWorld(),Wall.ImpactPoint,Wall.ImpactPoint + Wall.Normal * 50,FColor::Green,true,10,0,4);

	//Forward
	FVector ForwardDir = Wall.Normal.Rotation().Quaternion().GetRightVector() *-1 * Invert;
	DrawDebugLine(GetWorld(),Wall.ImpactPoint,Wall.ImpactPoint + ForwardDir * 50 ,FColor::Turquoise,false,10,0,4);
	UE_LOG(LogTemp,Warning,TEXT("Rotation = %s"),*ForwardDir.Rotation().ToString());

	//Back
	FVector BackDir =  Wall.Normal.Rotation().Quaternion().GetRightVector() *bInvert;
	DrawDebugLine(GetWorld(),Wall.ImpactPoint,Wall.ImpactPoint + BackDir * 50,FColor::Red,false,10,0,4);

	//Up
	FVector UpDir =  Wall.Normal.Rotation().Quaternion().GetUpVector();
	DrawDebugLine(GetWorld(),Wall.ImpactPoint,Wall.ImpactPoint + UpDir * 50,FColor::Blue,false,10,0,4);

	//Player
	FVector PlayerRot = Wall.ImpactPoint + GetActorForwardVector() * 50;
	DrawDebugLine(GetWorld(),Wall.ImpactPoint, Wall.ImpactPoint + GetActorForwardVector() *50,FColor::Yellow,false,10,0,4);

	-------------------------------------------------------------------------------------------------------
	*/
	
}


void AFPSProjectCharacter::DetatchFromWall()
{
	if(!bIsOnWall)
		return;
	bMovementLocked = false;
	bIsOnWall = false;
	if(WallJumpsLeft > 0)
	{
		FVector Speed = GetActorForwardVector();
		bRightWall ? Speed = GetActorRightVector() *-1 : Speed = GetActorRightVector();
		Speed.Normalize(0.01f);
		Speed.X *= WallJumpDistance;
		Speed.Y *= WallJumpDistance;
		Speed.Z = WallJumpHeight;
		LaunchCharacter(Speed,false,true);
		WallJumpsLeft--;
	}

	GetCharacterMovement()->GravityScale = 1;

	//Reset Camera to the Defaults
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if(!PlayerController)
		return;
	PlayerController->PlayerCameraManager->ViewYawMin = 0;
	PlayerController->PlayerCameraManager->ViewYawMax =359.998993;
	CancelWallTilt();
}

void AFPSProjectCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	DetatchFromWall();
	WallJumpsLeft = WallJumps;
}


void AFPSProjectCharacter::Interact()
{
	if(_InteractComp != nullptr)
		_InteractComp->Interact();
}

void AFPSProjectCharacter::SetRifle(bool bNewHasRifle, AWeapon* Weapon)
{
	if(!bHasRifle)
		MyWeapon = Weapon;
	bHasRifle = bNewHasRifle;
}

bool AFPSProjectCharacter::GetHasRifle()
{
	return bHasRifle;
}

void AFPSProjectCharacter::UseWeapon()
{
	if(bHasRifle && UKismetSystemLibrary::DoesImplementInterface(MyWeapon,UFireable::StaticClass()))
		IFireable::Execute_Fire(MyWeapon);
}

void AFPSProjectCharacter::ReloadWeapon()
{
	if(bHasRifle && UKismetSystemLibrary::DoesImplementInterface(MyWeapon,UFireable::StaticClass()))
		IFireable::Execute_Reload(MyWeapon);
}