// Fill out your copyright notice in the Description page of Project Settings.


#include "PController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FPSProjectCharacter.h"
#include "HealthComponent.h"
#include "WidgetHUD.h"

void APController::SetupInputComponent()
{
	Super::SetupInputComponent();
	MyPlayerCharacter = Cast<AFPSProjectCharacter>(this->GetPawn());
	if(MyPlayerCharacter != nullptr)
	{
		if(UEnhancedInputComponent* EIP = CastChecked<UEnhancedInputComponent>(InputComponent))
		{
			EIP->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APController::CallJumpingStart);
			EIP->BindAction(JumpAction, ETriggerEvent::Completed, this, &APController::CallJumpingEnd);
			EIP->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APController::CallMove);
			EIP->BindAction(SprintAction,ETriggerEvent::Triggered, this , &APController::CallSprintStart);
			EIP->BindAction(SprintAction,ETriggerEvent::Completed, this , &APController::CallSprintEnd);
			EIP->BindAction(CrouchAction,ETriggerEvent::Triggered,this,&APController::CallCrouchStart);
			EIP->BindAction(CrouchAction,ETriggerEvent::Completed,this,&APController::CallCrouchEnd);
			EIP->BindAction(DashAction,ETriggerEvent::Completed,this,&APController::CallDash);
			EIP->BindAction(LookAction, ETriggerEvent::Triggered, this, &APController::CallLook);
			EIP->BindAction(InteractAction,ETriggerEvent::Started,this,&APController::CallInteract);
			EIP->BindAction(FireAction,ETriggerEvent::Triggered,this,&APController::CallFireStart);
			EIP->BindAction(ReloadAction,ETriggerEvent::Triggered,this,&APController::CallReload);
			
		}

	}
}

void APController::HandleHealthUpdate(float newHealth,float maxHealth,float healthChange)
{
	_HUDWidget->UpdateHealth(newHealth/maxHealth);
}

void APController::HandleAmmoCountersUpdate(int CurrentAmmo, int ClipSize, int CurrentClip)
{
	_HUDWidget->UpdateAmmoCounters(CurrentAmmo,ClipSize,CurrentClip);
}

void APController::HandleDashUpdate(int DashesLeft, int MaxDashes)
{
	_HUDWidget->UpdateDash(DashesLeft,MaxDashes);
}

void APController::BeginPlay()
{
	Super::BeginPlay();
	SetupInputComponent();
		//Adding Mapping Context & Subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	
	if(_HUDWidgetClass)
	{
		UE_LOG(LogTemp,Warning,TEXT("WidgetCreated"));
		_HUDWidget = CreateWidget<UWidgetHUD,APController*>(this,_HUDWidgetClass.Get());
		_HUDWidget->AddToViewport();
	}
	
	if(UHealthComponent* HealthComp = MyPlayerCharacter->GetHealthComponent())
	{
		HealthComp->OnHealthComponentDamaged.AddUniqueDynamic(this,&APController::HandleHealthUpdate);
	}
	if(MyPlayerCharacter)
		MyPlayerCharacter->OnDashUpdate.AddUniqueDynamic(this,&APController::HandleDashUpdate);
}

void APController::AddWeaponMappings(UInputMappingContext* InFireMappingContext, AWeapon* Weapon)
{
	if(MyPlayerCharacter == nullptr)
		return;
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InFireMappingContext, 1);
		FireMappingContext = InFireMappingContext;
		Weapon->OnAmmoCountersUpdate.AddUniqueDynamic(this,&APController::HandleAmmoCountersUpdate);			
	}
	
}

void APController::RemoveWeaponMappings(AWeapon* Weapon)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(FireMappingContext);
		FireMappingContext = nullptr;
		Weapon->OnAmmoCountersUpdate.RemoveDynamic(this,&APController::HandleAmmoCountersUpdate);
	}
}

void APController::CallMove(const FInputActionValue& Value) {MyPlayerCharacter->Move(Value);}
void APController::CallLook(const FInputActionValue& Value) {MyPlayerCharacter->Look(Value);}
void APController::CallSprintStart() {MyPlayerCharacter->SprintStart();}
void APController::CallSprintEnd() {MyPlayerCharacter->SprintStop();}
void APController::CallCrouchStart() {MyPlayerCharacter->StartCrouch();}
void APController::CallCrouchEnd() {MyPlayerCharacter->StopCrouch();}
void APController::CallJumpingStart() {MyPlayerCharacter->Jump();}
void APController::CallJumpingEnd() {MyPlayerCharacter->StopJumping();}
void APController::CallDash() {MyPlayerCharacter->Dash();}

void APController::CallSlide()
{
	MyPlayerCharacter->Slide();
}

void APController::CallInteract() {MyPlayerCharacter->Interact();}

void APController::CallFireStart() {MyPlayerCharacter->UseWeapon();}
void APController::CallReload() {MyPlayerCharacter->ReloadWeapon();}







