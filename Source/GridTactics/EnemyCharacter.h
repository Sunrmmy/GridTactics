// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GridTactics/BT/EnemyAIConfig.h"
#include "EnemyCharacter.generated.h"

class UGridMovementComponent;
class USkillComponent;
class AEnemyAIController;
class UAttributesComponent;
class UWidgetComponent;
class USoundBase;

UCLASS()
class GRIDTACTICS_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // 组件访问器
    UGridMovementComponent* GetGridMovementComponent() const { return GridMovementComponent; }
    USkillComponent* GetSkillComponent() const { return SkillComponent; }
    UAttributesComponent* GetAttributesComponent() const { return AttributesComponent; }

    /** AI 配置组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    TObjectPtr<UEnemyAIConfig> AIConfig;

    UFUNCTION(BlueprintPure, Category = "AI")
    UEnemyAIConfig* GetAIConfig() const { return AIConfig; }

    UFUNCTION(BlueprintPure, Category = "Animation")
    float GetCurrentActualSpeed() const;

    // 动画状态接口（供动画蓝图使用）
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsHit() const { return bIsHit; }

    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsDead() const { return bIsDead; }

protected:
    virtual void BeginPlay() override;

    // 受伤回调
    UFUNCTION()
    void OnTakeDamage(float Damage);

    // 死亡回调
    UFUNCTION()
    void OnDeath(AActor* DeadActor);

    // 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UGridMovementComponent> GridMovementComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkillComponent> SkillComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAttributesComponent> AttributesComponent;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> HealthBarWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

    // ========================================
    // 动画状态变量（供动画蓝图读取）
    // ========================================

    /** 是否正在受击 */
    UPROPERTY(BlueprintReadOnly, Category = "Animation State")
    bool bIsHit = false;

    /** 是否已经死亡 */
    UPROPERTY(BlueprintReadOnly, Category = "Animation State")
    bool bIsDead = false;

    // ========================================
    // 音效配置
    // ========================================

    /** 受击音效 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> HitSound;

    /** 死亡音效 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> DeathSound;

    /** 死亡后尸体保留时间（秒） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
    float CorpseLifetime = 3.0f;

    /** 受击动画持续时间（自动重置 bIsHit） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float HitReactionDuration = 0.5f;
};
