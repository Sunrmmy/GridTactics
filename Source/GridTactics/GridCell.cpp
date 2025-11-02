// Fill out your copyright notice in the Description page of Project Settings.


#include "GridCell.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// Sets default values
AGridCell::AGridCell()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // 创建碰撞盒（100x100x10 cm）
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetBoxExtent(FVector(GridSizeCM * 0.5f, GridSizeCM * 0.5f, 5.0f));
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);     // 明确设置为动态物体查询
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
    RootComponent = CollisionBox;

    //创建可视化网格
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AGridCell::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGridCell::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AGridCell::IsWalkable() const
{
    return CellType == EGridCellType::Walkable;
}

#if WITH_EDITOR
void AGridCell::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    CollisionBox->SetBoxExtent(FVector(GridSizeCM * 0.5f, GridSizeCM * 0.5f, 5.0f));
}

void AGridCell::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    // 可在此根据 CellType 动态更换材质或颜色（通过 VisualMesh）
}
#endif