// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectYOIM/Input/InputCharacter.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include <Kismet/GameplayStatics.h>
#include "Camera/CameraComponent.h"

#include "ProjectYOIM/Stats/StatsComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include <ProjectYOIM/Enemy/Enemy.h>

// Sets default values
AInputCharacter::AInputCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StatsComponent = CreateDefaultSubobject<UStatsComponent>(TEXT("StatsComponent"));
}

// Called when the game starts or when spawned
void AInputCharacter::BeginPlay()
{
	Super::BeginPlay();
    setEnableEnemy(false);

    // Get references to specific TextRender components by class
    TArray<UTextRenderComponent*> TextRenderComponents;
    GetComponents<UTextRenderComponent>(TextRenderComponents); // Get all TextRender components

    for (UTextRenderComponent* TextComponent : TextRenderComponents)
    {
        if (TextComponent->GetName() == TEXT("TextScore"))
        {
            TextScore = TextComponent; // Assign the reference to TextScore
            UE_LOG(LogTemp, Warning, TEXT("TextScore get"));
        }
        else if (TextComponent->GetName() == TEXT("TextTimer"))
        {
            TextTimer = TextComponent; // Assign the reference to TextTimer
            UE_LOG(LogTemp, Warning, TEXT("TextTimer get"));

        }
        else if (TextComponent->GetName() == TEXT("TextEndOfGame"))
        {
            TextEndOfGame = TextComponent;
            UE_LOG(LogTemp, Warning, TEXT("TextEndOfGame get"));
        }
        else if (TextComponent->GetName() == TEXT("CURSOR"))
        {
            Cursor = TextComponent;
            UE_LOG(LogTemp, Warning, TEXT("CURSOR get"));
        }
    }
}

// Called every frame
void AInputCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    OnLook(DeltaTime);

    TArray<AActor*> ActorsWithTag;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Enemy", ActorsWithTag);

    for (AActor* Actor : ActorsWithTag)
    {
        if (Actor)
        {
            UEnemy* EnemyComponent = Actor->FindComponentByClass<UEnemy>();
            if (EnemyComponent)
            {
                if (EnemyComponent->bIsPlayerLooking)
                {
                    // MOVE AWAY
                    if (UFunction* TriggerFunction = Actor->FindFunction(TEXT("RunAway")))
                    {
                        // Create a buffer just in case (if we send a null buffer, the system will crash if the event has parameters).
                        // (Check the codebase to see examples sending params.)
                        uint8* ParamsBuffer = static_cast<uint8*>(FMemory_Alloca(TriggerFunction->ParmsSize));
                        FMemory::Memzero(ParamsBuffer, TriggerFunction->ParmsSize);
                        Actor->ProcessEvent(TriggerFunction, ParamsBuffer);
                    }
                }

                ACharacter* EnemyCharacter = Cast<ACharacter>(Actor);
                if (EnemyCharacter && bIsEnemyGameStarted && DeltaTime > TimerForEnemy)
                {
                    // Get the velocity of the character
                    FVector Velocity = EnemyCharacter->GetCharacterMovement()->Velocity;

                    UE_LOG(LogTemp, Display, TEXT("Timer after start : %f"), DeltaTime);
                    // Check if the magnitude (speed) is close to zero
                    if (Velocity.Size() <= KINDA_SMALL_NUMBER) // KINDA_SMALL_NUMBER is a small threshold to account for float precision
                    {
                        UE_LOG(LogTemp, Warning, TEXT("You lose !"));
                        UpdateEndOfGameText("YOU LOSE !", 5.0f);
                        QuitEnemyGame();
                    }
                }
            }
        }

    }

    if (bIsOilGameStarted || bIsEnemyGameStarted)
    {
        ElapsedTime += DeltaTime;

        int32 Minutes = FMath::FloorToInt(ElapsedTime / 60.0f);
        int32 Seconds = FMath::FloorToInt(ElapsedTime) % 60; // Get seconds remainder

        FString TimerString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);

        UpdateTimer(TimerString);
    }

    if (bIsEndOfGameDisplayed)
    {
        TimeEndOfGameDisplayed += DeltaTime;

        if (TimeEndOfGameDisplayed >= TimeToDisplayEndOfGameText)
        {
            TimeEndOfGameDisplayed = 0.0f;
            bIsEndOfGameDisplayed = false;
            TextEndOfGame->SetText(FText::FromString(""));
            TextEndOfGame->SetVisibility(false);

            UE_LOG(LogTemp, Display, TEXT("Removing text lose"));
        }
    }

    if (Score >= 18)
    {
        Score = 0;
        UpdateEndOfGameText("YOU WON !<br>GG", 5.0f);
        QuitOilGame();
    }

    if (bIsMoving)
    {
        FVector CurrentLocation = GetActorLocation();
        FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, MoveSpeed);
        SetActorLocation(NewLocation);

        // Check if close enough to stop moving
        if (FVector::Dist(NewLocation, TargetLocation) < 1.0f)
        {
            bIsMoving = false; // Stop moving if close enough
        }
    }
}

// Called to bind functionality to input
void AInputCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	if (UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AInputCharacter::Move);
        Input->BindAction(ResetGyroscopeAction, ETriggerEvent::Triggered, this, &AInputCharacter::ResetGyro);
	}

}

void AInputCharacter::Move(const FInputActionValue& InputValue)
{
	FVector2D InputVector = InputValue.Get<FVector2D>();

	if (IsValid(Controller))
	{
        UE_LOG(LogTemp, Display, TEXT("Moving"));
		// Movement
		const FRotator Rotation = Controller->GetControlRotation(); // 1 = forward, -1 = back
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardBackwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightLeftDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardBackwardDirection, InputVector.Y);
		AddMovementInput(RightLeftDirection, InputVector.X);
	}
}

void AInputCharacter::OnLook(float DeltaTime)
{
    UCameraComponent* CameraComponent = FindComponentByClass<UCameraComponent>();

    // Get the camera component's location and rotation
    FVector CameraLocation = CameraComponent->GetComponentLocation();
    FRotator CameraRotation = CameraComponent->GetComponentRotation();

    // Define the trace start point and direction
    FVector Start = CameraLocation;
    float TraceDistance = 10000.0f;  // Adjust this to a smaller value to bring the point closer
    FVector End = CameraLocation + (CameraRotation.Vector() * TraceDistance);  // Reduced distance

    FHitResult HitResult;
    FCollisionQueryParams TraceParams(FName(TEXT("OnLookTrace")), true, this);
    TraceParams.bTraceComplex = true;
    TraceParams.bReturnPhysicalMaterial = false;

    // Perform the line trace to a single point
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, Start, End, ECC_Visibility, TraceParams
    );

    if (bHit)
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor)
        {
            if (HitActor->ActorHasTag(FName("Hardcore")) && !bIsEnemyGameStarted && !bIsOilGameStarted)
            {
                TimeLookingAtChoice += DeltaTime;

                if (TimeLookingAtChoice >= TimeToLookAtChoice)
                {
                    bIsHardcoreEnabled = !bIsHardcoreEnabled;
                    TimeLookingAtChoice = 0;
                    if (bIsHardcoreEnabled)
                    {
                        UpdateEndOfGameText("Hardcore mod<br>ENABLED", 1.0f);
                        Cursor->SetVisibility(false);
                    }
                    else
                    {
                        UpdateEndOfGameText("Hardcore mod<br>DISABLED", 1.0f);
                        Cursor->SetVisibility(true);
                    }
                }
            }

            else if (HitActor->ActorHasTag(FName("Enemy_Game_Start")) && !bIsEnemyGameStarted && !bIsOilGameStarted)
            {
                TimeLookingAtChoice += DeltaTime;

                if (TimeLookingAtChoice >= TimeToLookAtChoice)
                {
                    TimerForEnemy = DeltaTime;
                    UE_LOG(LogTemp, Warning, TEXT("Starting Enemy Game"));
                    UE_LOG(LogTemp, Display, TEXT("Timer on start : %f"), DeltaTime);
                    UpdateEndOfGameText("Enemy game started !", 2.0f);
                    StartEnemyGame();
                }

            }
            else if (HitActor->ActorHasTag(FName("Oil_Game_Start")) && !bIsEnemyGameStarted && !bIsOilGameStarted)
            {
                TimeLookingAtChoice += DeltaTime;

                if (TimeLookingAtChoice >= TimeToLookAtChoice)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Starting Oil Game"));
                    UpdateEndOfGameText("Oil game started !", 2.0f);
                    StartOilGame();
                }
            }
            else if (HitActor->ActorHasTag(FName("Oil_Quit")) && bIsOilGameStarted)
            {
                TimeLookingAtChoice += DeltaTime;

                if (TimeLookingAtChoice >= TimeToLookAtChoice)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Quit oil"));
                    UpdateEndOfGameText("ragequit ??", 5.0f);
                    QuitOilGame();
                }
            }

            else if (HitActor->ActorHasTag(FName("Oil")) && bIsOilGameStarted && !HitActor->ActorHasTag(FName("Cleaned")))
            {
                UE_LOG(LogTemp, Warning, TEXT("looking at oils"));

                UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(HitActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
                if (MeshComponent)
                {
                    UMaterialInstanceDynamic* MaterialInstance = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
                    if (MaterialInstance)
                    {
                        float OpacityValue;
                        MaterialInstance->GetScalarParameterValue(FName("Opacity"), OpacityValue);
                        UE_LOG(LogTemp, Warning, TEXT("Current Opacity: %f"), OpacityValue);

                        // Check if transparency (opacity) is less than or equal to 1.0 (fully transparent)
                        if (OpacityValue <= 1.0f && OpacityValue > 0)
                        {
                            // Decrease the opacity by 0.3f (making the object less transparent)
                            float NewOpacityValue = FMath::Clamp(OpacityValue - 0.03f, 0.0f, 1.0f);
                            MaterialInstance->SetScalarParameterValue(FName("Opacity"), NewOpacityValue);

                            // Log that the opacity is decreasing
                            UE_LOG(LogTemp, Warning, TEXT("Actor '%s': Decreased Opacity to %f"), *HitActor->GetName(), NewOpacityValue);
                        }
                        else
                        {
                            // If opacity is at the maximum, hide the actor and mark it as "Cleaned"
                            HitActor->SetActorHiddenInGame(true);
                            HitActor->SetActorTickEnabled(false);

                            UpdateScore(FString::FromInt(++Score));
                            UpdateEndOfGameText("+ 1", 0.5f);

                            // Add the "Cleaned" tag to prevent re-triggering
                            HitActor->Tags.Add(FName("Cleaned"));

                            UE_LOG(LogTemp, Warning, TEXT("Actor '%s' cleaned: hiding the actor."), *HitActor->GetName());
                        }
                    }
                }
            }
            else if (HitActor->ActorHasTag(FName("Enemy")) && bIsEnemyGameStarted)
            {
                UEnemy* EnemyComponent = HitResult.GetActor()->FindComponentByClass<UEnemy>();

                if (EnemyComponent)
                {
                    EnemyComponent->bIsPlayerLooking = true;
                }

            }
            else
            {
                TimeLookingAtChoice = 0;

                TArray<AActor*> ActorsWithTag;
                UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Enemy", ActorsWithTag);

                for (AActor* Actor : ActorsWithTag)
                {
                    if (Actor)
                    {
                        UEnemy* EnemyComponent = Actor->FindComponentByClass<UEnemy>();
                        if (EnemyComponent)
                        {
                            EnemyComponent->bIsPlayerLooking = false;
                        }
                    }
                }
            }
        }
    }
    else
    {
        TimeLookingAtChoice = 0;
    }

}

void AInputCharacter::ResetGyro()
{
    UE_LOG(LogTemp, Display, TEXT("RESET GYRO TRIGERRED"));

    UCameraComponent* CameraComponent = FindComponentByClass<UCameraComponent>();

    // Check if the CameraComponent exists
    if (CameraComponent)
    {
        // Set the camera's rotation to 0 degrees
        FRotator CameraRotation = CameraComponent->GetComponentRotation();
        CameraRotation.Pitch = 0.0f; // Set Pitch to 0

        CameraComponent->SetWorldRotation(CameraRotation);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CameraComponent is not found!"));
    }
}

void AInputCharacter::StartEnemyGame()
{
    /*
        Enable Enemies

        Hide Enemy Start
        Hide Oil Start

        Create Timer
    */

    Score = 0;
    UpdateScore(FString::FromInt(Score));

    ElapsedTime = 0.0f;
    UpdateTimer("00:00");

    bIsEnemyGameStarted = true;
    bIsOilGameStarted = false;

    setEnableEnemy(true);
    // this->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
    ResetPlayerLocation();
}

void AInputCharacter::QuitEnemyGame()
{
    /*
        Disable Enemies

        Show Enemy Start
        Show Oil Start

        Destroy Timer
    */

    bIsEnemyGameStarted = false;
    bIsOilGameStarted = false;

    setEnableEnemy(false);
}

void AInputCharacter::StartOilGame()
{
    Score = 0;
    UpdateScore(FString::FromInt(Score));

    ElapsedTime = 0.0f;
    UpdateTimer("00:00");

    bIsEnemyGameStarted = false;
    bIsOilGameStarted = true;

    // Show wall + oil
    SetVisibilityOils(true);

    /*
        Create Timer
        Create Oil counter
    */
    // this->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
    ResetPlayerLocation();
};

void AInputCharacter::QuitOilGame()
{
    bIsEnemyGameStarted = false;
    bIsOilGameStarted = false;

    // Hide wall + oil
    ResetOils();
    SetVisibilityOils(false);

    /*
        Destroy Timer
        Destroy Oil counter

        Show recap of the game (timer + oil counter)
    */
};

void AInputCharacter::SetVisibilityOils(bool bVisible)
{
    TArray<AActor*> ActorsWithTag;

    // Find all actors with the specified tag
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Oil_Game", ActorsWithTag);

    // Iterate over the found actors and log their names
    for (AActor* Actor : ActorsWithTag)
    {
        if (Actor)
        {
            // Find the static mesh component (or any other visible component)
            UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
            if (MeshComponent)
            {
                // Set visibility to true (or false depending on bVisible)
                MeshComponent->SetVisibility(bVisible, true);  // true for propagate to children
                UE_LOG(LogTemp, Warning, TEXT("Set visibility of '%s' to %s"), *Actor->GetName(), bVisible ? TEXT("true") : TEXT("false"));
            }
        }
    }
}

void AInputCharacter::ResetOils()
{
    TArray<AActor*> ActorsWithTag;

    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Oil", ActorsWithTag);

    for (AActor* Actor : ActorsWithTag)
    {
        if (Actor)
        {
            UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
            if (MeshComponent)
            {
                UMaterialInstanceDynamic* MaterialInstance = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
                if (MaterialInstance)
                {
                    float OpacityValue;
                    MaterialInstance->GetScalarParameterValue(FName("Opacity"), OpacityValue);
                    UE_LOG(LogTemp, Warning, TEXT("Current Opacity: %f"), OpacityValue);
                    MaterialInstance->SetScalarParameterValue(FName("Opacity"), 1.0f);
                    Actor->SetActorHiddenInGame(false);
                    Actor->SetActorTickEnabled(true);
                    Actor->Tags.Remove(FName("Cleaned"));
                }
            }
        }
    }
}

void AInputCharacter::setEnableEnemy(bool bEnable)
{
    ResetEnemyLocation();
    TArray<AActor*> ActorsWithTag;

    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Enemy", ActorsWithTag);

    for (AActor* Actor : ActorsWithTag)
    {
        if (Actor)
        {
            // Disable the actor if bEnable is false
            if (!bEnable)
            {
                // Hide the actor in the game
                Actor->SetActorHiddenInGame(true);

                // Disable the actor's movement or other interactions
                ACharacter* Character = Cast<ACharacter>(Actor);
                if (Character)
                {
                    Character->GetCharacterMovement()->DisableMovement();
                    Character->SetActorTickEnabled(false); // Stop ticking if necessary
                }
                else
                {
                    // If it's not a character, disable any other components that allow interaction
                    Actor->SetActorTickEnabled(false);
                }
            }
            else
            {
                // Enable the actor if bEnable is true
                Actor->SetActorHiddenInGame(false);

                // Enable movement if it is a character
                if (ACharacter* Character = Cast<ACharacter>(Actor))
                {
                    Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
                    Character->SetActorTickEnabled(true); // Resume ticking if necessary
                }
                else
                {
                    // Enable any other necessary components
                    Actor->SetActorTickEnabled(true);
                }
            }
        }
    }
}

void AInputCharacter::ResetEnemyLocation()
{
    /*
        Set location of every enemy to his initial position
    */

    // Define the initial positions for each enemy
    TMap<FName, FVector> InitialPositions = {
        { "Enemy1", FVector(0.0f, 2400.0f, 108.000200f) },
        { "Enemy2", FVector(500.0f, 2300.0f, 108.000200f) },
        { "Enemy3", FVector(-500.0f, 2300.0f, 108.000200f) }
    };

    // Loop over each tag and reset the location of the corresponding enemy
    for (const auto& Elem : InitialPositions)
    {
        // Find actor(s) with the specified tag
        TArray<AActor*> FoundEnemies;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), Elem.Key, FoundEnemies);

        for (AActor* Enemy : FoundEnemies)
        {
            if (Enemy)
            {
                // Set the enemy's location to its initial position
                Enemy->SetActorLocation(Elem.Value);
            }
        }
    }
}

void AInputCharacter::UpdateScore(FString ScoreText)
{
    if (TextScore)
    {
        TextScore->SetText(FText::FromString("Score : " + ScoreText));
    }
}

void AInputCharacter::UpdateTimer(FString TimerText)
{
    if (TextTimer)
    {
        TextTimer->SetText(FText::FromString(TimerText));
    }
}

void AInputCharacter::UpdateEndOfGameText(FString Text, float time)
{
    if (TextEndOfGame)
    {
        TimeToDisplayEndOfGameText = time;
        bIsEndOfGameDisplayed = true;
        TextEndOfGame->SetText(FText::FromString(Text));
        TextEndOfGame->SetVisibility(true);
    }
}

void AInputCharacter::ResetPlayerLocation()
{
    bIsMoving = true;
}