// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectYOIM/Stats/StatsComponent.h"

// Sets default values for this component's properties
UStatsComponent::UStatsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	// Health
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
}

// Called when the game starts
void UStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

// Called every frame
void UStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// -------------------------------------------------------------------------------------------------------------------------------------------------

void UStatsComponent::UpdateHealth(float HealthToLose)
{
	CurrentHealth -= HealthToLose;

	if (CurrentHealth <= 0 && bIsAlive)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black, "You are dead !");
		CurrentHealth = 0;
		bIsAlive = false;
	}
	else if (CurrentHealth > 0)
	{
		bIsAlive = true;
	}
}