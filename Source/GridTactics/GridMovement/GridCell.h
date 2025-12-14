// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridType.h"
#include "GridCell.generated.h"
UCLASS()
class GRIDTACTICS_API AGridCell : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	AGridCell();

    // 网格类型（可行走、阻挡、水、岩浆等）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    EGridCellType CellType = EGridCellType::Walkable;

    // 网格逻辑坐标（用于快速索引）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    FIntPoint GridCoordinate;

    // 网格尺寸（厘米）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    float GridSizeCM = 100.0f;

    // 是否可行走（蓝图/C++ 查询接口）
    UFUNCTION(BlueprintPure, Category = "Grid")
    bool IsWalkable() const;

    // 可视化：在编辑器中根据类型显示不同颜色
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void OnConstruction(const FTransform& Transform) override;
#endif

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 碰撞盒子：仅用于查询交互
    UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    // 用于可视化
    UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UStaticMeshComponent* VisualMesh;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
