// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillEffect.h"
#include "SkillEffect_VFX.generated.h"

class UNiagaraSystem;
class USoundCue;
class UNiagaraComponent;

/**
 * 视觉/音效效果 - 用于播放粒子特效和音效
 * 可配置：
 * 1. 在施法者位置播放特效/音效
 * 2. 在目标位置播放特效/音效
 * 3. 在所有受影响目标位置播放特效/音效
 */
UENUM(BlueprintType)
enum class EVFXSpawnLocation : uint8
{
    Caster          UMETA(DisplayName = "Caster Location"),
    TargetGrid      UMETA(DisplayName = "Target Grid"),
    AffectedActors  UMETA(DisplayName = "All Affected Actors"),
    CasterToTarget  UMETA(DisplayName = "Caster To Target")
};
/**
 * 弹道目标模式
 */
UENUM(BlueprintType)
enum class EProjectileTargetMode : uint8
{
    ToTargetGrid    UMETA(DisplayName = "To Target Grid (使用技能目标格子)"),
    ToDirection     UMETA(DisplayName = "To Direction (沿角色朝向飞行)")
};
UCLASS(Blueprintable, EditInlineNew, meta = (DisplayName = "VFX/Sound Effect"))
class GRIDTACTICS_API USkillEffect_VFX : public USkillEffect
{
    GENERATED_BODY()

public:
    USkillEffect_VFX();

    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors) override;
    virtual bool CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const override;

    // ========================================
    // 粒子特效配置
    // ========================================

    /** Niagara 粒子系统 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Particle")
    TObjectPtr<UNiagaraSystem> NiagaraEffect;

    /** 特效生成位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Particle")
    EVFXSpawnLocation ParticleSpawnLocation = EVFXSpawnLocation::Caster;

    /** 特效位置偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Particle")
    FVector ParticleOffset = FVector::ZeroVector;

    /** 特效旋转偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Particle")
    FRotator ParticleRotation = FRotator::ZeroRotator;

    /** 特效缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Particle")
    FVector ParticleScale = FVector::OneVector;

    /** 是否附加到目标（跟随移动） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Particle")
    bool bAttachToTarget = false;

    /** 附加的骨骼名称（如果附加到角色） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Particle", meta = (EditCondition = "bAttachToTarget"))
    FName AttachSocketName = NAME_None;

    // ========================================
    // 音效配置
    // ========================================

/** 要播放的音效（支持 SoundCue、SoundWave 等所有类型） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Sound")
    TObjectPtr<USoundBase> Sound;

    /** 音效生成位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Sound")
    EVFXSpawnLocation SoundSpawnLocation = EVFXSpawnLocation::Caster;

    /** 音效音量倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float VolumeMultiplier = 1.0f;

    /** 音效音调倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Sound", meta = (ClampMin = "0.5", ClampMax = "2.0"))
    float PitchMultiplier = 1.0f;

    // ========================================
    // 高级配置
    // ========================================

    /** 延迟播放时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Timing", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float PlayDelay = 0.0f;

    // 新增：是否启用弹道移动
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Movement")
    bool bEnableProjectileMovement = false;

    /** 新增：弹道类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Movement", meta = (EditCondition = "bEnableProjectileMovement"))
    EProjectileTargetMode ProjectileTargetMode = EProjectileTargetMode::ToDirection;

    /** 新增：飞行距离（网格数，仅在 ToDirection 模式使用） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Movement", meta = (EditCondition = "bEnableProjectileMovement && ProjectileTargetMode == EProjectileTargetMode::ToDirection", ClampMin = "1", ClampMax = "20"))
    int32 ProjectileGridDistance = 5;

    // 新增：弹道速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Movement", meta = (EditCondition = "bEnableProjectileMovement"))
    float ProjectileSpeed = 1000.0f;

    // 新增：弹道弧度（0 = 直线，1 = 高抛物线）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Movement", meta = (EditCondition = "bEnableProjectileMovement", ClampMin = "0.0", ClampMax = "1.0"))
    float ProjectileArc = 0.3f;

protected:
    /** 执行 VFX（内部调用） */
    void ExecuteVFX(UWorld* World, AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors);

    /** 在指定位置播放特效 */
    void SpawnParticleAtLocation(UWorld* World, FVector Location, FRotator Rotation, AActor* AttachTarget = nullptr);

    /** 在指定位置播放音效 */
    void PlaySoundAtLocation(UWorld* World, FVector Location);

    /** 获取生成位置列表 */
    TArray<FVector> GetSpawnLocations(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors, EVFXSpawnLocation SpawnType) const;

    /** 新增：计算弹道目标位置 */
    FVector CalculateProjectileEndLocation(AActor* Instigator, FIntPoint TargetGrid) const;

    // 新增：更新弹道位置
    UFUNCTION()
    void UpdateProjectileMovement(float DeltaTime);

    /** 缓存的 Timer Handle */
    FTimerHandle ProjectileTimerHandle;

    // 缓存生成的 Niagara Component
    UPROPERTY()
    TObjectPtr<UNiagaraComponent> ActiveNiagaraComponent;

    FVector ProjectileStartLocation;
    FVector ProjectileEndLocation;
    float ProjectileElapsedTime = 0.0f;
    float ProjectileDuration = 0.0f;
};
