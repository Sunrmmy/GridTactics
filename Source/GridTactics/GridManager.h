// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

UCLASS()
class GRIDTACTICS_API AGridManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGridManager();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 请求预定一个网格用于移动（解决竞态条件）
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool ReserveGrid(AActor* Requester, FIntPoint TargetGrid);

	// 强制预定网格（用于击退或技能位移，会覆盖原有的预定）
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ForceReserveGrid(AActor* Requester, FIntPoint TargetGrid);

	// 移动完成或者取消时释放预定的网格
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ReleaseGrid(FIntPoint TargetGrid);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// 存储每个网格被哪个Actor预定了	Key: 网格坐标, Value: 预定该网格的Actor
	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<AActor>> GridReservations;

};
