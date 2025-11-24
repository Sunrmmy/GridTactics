// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDisplacementRequest.h"
#include "GridManager.generated.h"

class UPathPlanner;
class UConflictResolver;
UCLASS()
class GRIDTACTICS_API AGridManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGridManager();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool ReserveGrid(AActor* Requester, FIntPoint TargetGrid);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ForceReserveGrid(AActor* Requester, FIntPoint TargetGrid);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ReleaseGrid(FIntPoint TargetGrid);

    UFUNCTION(BlueprintCallable, Category = "Grid|Displacement")
    void RequestDash(AActor* Requester, FIntPoint Direction, int32 Distance,
        bool bCanKnockback = false, int32 KnockbackDist = 1
    );

    UFUNCTION(BlueprintCallable, Category = "Grid|Displacement")
    void RequestTeleport(AActor* Requester, FIntPoint TargetGrid);

    UFUNCTION(BlueprintCallable, Category = "Grid|Displacement")
    void RequestKnockback(AActor* Target, FIntPoint Direction, int32 Distance);

    UFUNCTION(BlueprintCallable, Category = "Grid|Displacement")
    void ProcessDisplacements();

    // --- 新增接口 ---

    // 提交自定义请求（高级用法）
    UFUNCTION(BlueprintCallable, Category = "Grid|Displacement")
    void SubmitCustomRequest(const FGridDisplacementRequest& Request);

    // 批量处理（异步，返回完成事件）
    UFUNCTION(BlueprintCallable, Category = "Grid|Displacement")
    void ProcessDisplacementsAsync();

    // 工具函数
    UFUNCTION(BlueprintPure, Category = "Grid")
    bool IsGridValid(FIntPoint Grid) const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    bool IsGridWalkable(FIntPoint Grid) const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    AActor* GetActorAtGrid(FIntPoint Grid) const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    FIntPoint GetActorCurrentGrid(AActor* Actor) const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    FVector GridToWorld(FIntPoint Grid) const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    FIntPoint WorldToGrid(FVector WorldPos) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
    UPROPERTY()
    TMap<FIntPoint, TObjectPtr<AActor>> GridReservations;

    UPROPERTY()
    TArray<FGridDisplacementRequest> PendingDisplacements;

    // 递归深度限制（防止无限链式击退）
    UPROPERTY(EditDefaultsOnly, Category = "Displacement")
    int32 MaxKnockbackRecursionDepth = 3;

    int32 CurrentRecursionDepth = 0;

    // --- 核心管线 ---
    void PlanAllPaths();
    void ResolveAllConflicts();
    void ExecuteAllDisplacements();

    // 辅助函数
    void HandleKnockbackFailure(AActor* Target, FIntPoint BlockedGrid,
        EKnockbackBlockReason Reason);
    void ProcessKnockbackQueue(TArray<FGridDisplacementRequest>& KnockbackQueue);
};
