// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectYOIM/Enemy/Enemy.h"
#include "GameFramework/Character.h" // Include for ACharacter
#include "GameFramework/CharacterMovementComponent.h" // Include if using a character movement component

// Sets default values for this component's properties
UEnemy::UEnemy()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UEnemy::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UEnemy::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}