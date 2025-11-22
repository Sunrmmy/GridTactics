// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"

// Sets default values
AGridManager::AGridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AGridManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AGridManager::ReserveGrid(AActor* Requester, FIntPoint TargetGrid)
{
    // 检查是否已经有其他Actor预定了这个格子
    if (GridReservations.Contains(TargetGrid))
    {
        return false;
    }

    // 没有被预定，立即为请求者预定该格子
    GridReservations.Add(TargetGrid, Requester);
    return true;
}

void AGridManager::ForceReserveGrid(AActor* Requester, FIntPoint TargetGrid)
{
    // 强制覆盖预定（如果有之前的预定，直接覆盖）
    GridReservations.Add(TargetGrid, Requester);
}

void AGridManager::ReleaseGrid(FIntPoint GridToRelease)
{
    GridReservations.Remove(GridToRelease);
}