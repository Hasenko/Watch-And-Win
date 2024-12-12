// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/TextRenderComponent.h"  // Include the TextRenderComponent header

#include "ProjectYOIM/Stats/StatsComponent.h"

#include "InputCharacter.generated.h"

UCLASS()
class PROJECTYOIM_API AInputCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputMappingContext* InputMapping;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* ResetGyroscopeAction;

public:
	// Sets default values for this character's properties
	AInputCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// In InputCharacter.h
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStatsComponent* StatsComponent;

	// Running (normal)
	void Move(const FInputActionValue& InputValue);

	// Looking
	UFUNCTION(BlueprintCallable, Category = "Look")
	void OnLook(float DeltaTime);
	void ResetGyro();

	float TimeLookingAtChoice = 0.0f;
	float TimeToLookAtChoice = 3.0f;

	bool bIsOilGameStarted = false;
	bool bIsEnemyGameStarted = false;
	bool bIsHardcoreEnabled = false;

	void StartEnemyGame();
	void QuitEnemyGame();

	void StartOilGame();
	void QuitOilGame();

	void SetVisibilityOils(bool bVisible);
	void ResetOils();
	void setEnableEnemy(bool bEnable);
	void ResetEnemyLocation();

	float TimerForEnemy;

	// Function to update text values
	void UpdateScore(FString ScoreText);
	void UpdateTimer(FString TimerText);

	bool bIsEndOfGameDisplayed = false;
	float TimeEndOfGameDisplayed = 0.0f;
	float TimeToDisplayEndOfGameText = 5.0f;
	void UpdateEndOfGameText(FString Text, float time);

	int Score = 0;
	float ElapsedTime = 0.0f; // Time in seconds

	UTextRenderComponent* TextScore;
	UTextRenderComponent* TextTimer;
	UTextRenderComponent* TextEndOfGame;
	UTextRenderComponent* Cursor;

	void ResetPlayerLocation();
	FVector TargetLocation = FVector(0.0f, 0.0f, 90.150102f);
	float MoveSpeed = 5.0f; // Adjust this value for speed
	bool bIsMoving = false; // Flag to check if moving
};