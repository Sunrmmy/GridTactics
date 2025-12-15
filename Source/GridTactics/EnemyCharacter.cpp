// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "GridTactics/GridMovement/GridMovementComponent.h"
#include "GridTactics/Skills/SkillComponent.h"
#include "AttributesComponent.h"
#include "AttributesBar.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 禁用默认的角色移动旋转，以便我们通过组件精确控制
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	GridMovementComponent = CreateDefaultSubobject<UGridMovementComponent>(TEXT("GridMovementComponent"));
	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	AttributesComponent = CreateDefaultSubobject<UAttributesComponent>(TEXT("AttributesComponent"));
    // 创建 AI 配置组件
    AIConfig = CreateDefaultSubobject<UEnemyAIConfig>(TEXT("AIConfig"));

	// 设置AI控制器来默认附身这个Character
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 创建头顶血条的WidgetComponent
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidgetComponent->SetupAttachment(RootComponent);
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); // 设置为屏幕空间，它会始终面向镜头
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 调整到角色头顶位置
	HealthBarWidgetComponent->SetDrawSize(FVector2D(100.f, 20.f)); // 设置控件的绘制大小
	HealthBarWidgetComponent->SetWidget(nullptr);	// 默认为空

}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("========== EnemyCharacter::BeginPlay: %s =========="), *GetName());

	// 检查组件
	if (!GridMovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyCharacter: GridMovementComponent is NULL!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: GridMovementComponent OK"));
	}

	if (!SkillComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyCharacter: SkillComponent is NULL!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: SkillComponent OK"));
	}

	if (!AttributesComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyCharacter: AttributesComponent is NULL!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: AttributesComponent OK"));
		UE_LOG(LogTemp, Log, TEXT("  - HP: %.1f / %.1f"), 
			AttributesComponent->GetHP(), AttributesComponent->GetMaxHP());
		UE_LOG(LogTemp, Log, TEXT("  - Stamina: %.1f / %.1f"), 
			AttributesComponent->GetStamina(), AttributesComponent->GetMaxStamina());
	}

	if (!AIConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyCharacter: AIConfig is NULL!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: AIConfig OK"));
		UE_LOG(LogTemp, Log, TEXT("  - MinCombatDistance: %.1f"), AIConfig->MinCombatDistance);
		UE_LOG(LogTemp, Log, TEXT("  - MaxCombatDistance: %.1f"), AIConfig->MaxCombatDistance);
	}

	// 检查 AI 控制器
	AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyCharacter: No AIController possessed! AutoPossessAI = %d"), 
			(int32)AutoPossessAI);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyCharacter: AIController possessed successfully: %s"), 
			*AIController->GetName());
	}

	// 原有代码...
	if (HealthBarWidgetClass && HealthBarWidgetComponent)
	{
		// 将指定的UI类设置给WidgetComponent
		HealthBarWidgetComponent->SetWidgetClass(HealthBarWidgetClass);

		// 使用 AttributesBar 的接口
		if (UAttributesBar* AttributesBar = Cast<UAttributesBar>(HealthBarWidgetComponent->GetUserWidgetObject()))
		{
			AttributesBar->SetOwnerActor(this);
			UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: AttributesBar bound for %s"), *GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyCharacter: HealthBarWidgetClass is not an AttributesBar! "));
		}
	}
	else
	{
		if (!HealthBarWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyCharacter: HealthBarWidgetClass is NULL for %s! Please set it in BP_EnemyCharacter Class Defaults."), *GetName());
		}
		if (!HealthBarWidgetComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("EnemyCharacter: HealthBarWidgetComponent is NULL for %s!"), *GetName());
		}
	}

	// 绑定受伤和死亡事件
	if (AttributesComponent)
	{
		// 绑定受伤委托（AttributesComponent 中新增）
		AttributesComponent->OnDamageTaken.AddDynamic(this, &AEnemyCharacter::OnTakeDamage);

		// 绑定死亡委托
		AttributesComponent->OnCharacterDied.AddDynamic(this, &AEnemyCharacter::OnDeath);
	}
	UE_LOG(LogTemp, Warning, TEXT("========== EnemyCharacter::BeginPlay END =========="));
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float AEnemyCharacter::GetCurrentActualSpeed() const
{
	if (GridMovementComponent && GridMovementComponent->IsMoving())
	{
		return GridMovementComponent->GetCurrentActualSpeed();
	}
	return 0.0f;
}

// ========================================
// 受击处理
// ========================================

void AEnemyCharacter::OnTakeDamage(float Damage)
{
    if (bIsDead)
    {
        return;
    }

    // 设置受击状态（动画蓝图会读取这个变量）
    bIsHit = true;

    UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: %s taking damage %.1f"), *GetName(), Damage);

    // 播放受击音效
    if (HitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            HitSound,
            GetActorLocation(),
            1.0f,
            1.0f
        );
    }

    // 自动重置受击状态（避免卡在受击动画）
    FTimerHandle HitResetTimer;
    GetWorld()->GetTimerManager().SetTimer(
        HitResetTimer,
        [this]()
        {
            bIsHit = false;
        },
        HitReactionDuration,
        false
    );
}

// ========================================
// 死亡处理
// ========================================

void AEnemyCharacter::OnDeath(AActor* DeadActor)
{
    if (bIsDead)
    {
        return;
    }

    bIsDead = true;

    UE_LOG(LogTemp, Warning, TEXT("EnemyCharacter: %s died!"), *GetName());

    // 播放死亡音效
    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            DeathSound,
            GetActorLocation(),
            1.0f,
            1.0f
        );
    }

    // 禁用碰撞和 AI
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 禁用 AI
    if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
    {
        AIController->UnPossess();
    }

    // 隐藏血条
    if (HealthBarWidgetComponent)
    {
        HealthBarWidgetComponent->SetVisibility(false);
    }

    // 延迟销毁尸体
    FTimerHandle CorpseTimer;
    GetWorld()->GetTimerManager().SetTimer(
        CorpseTimer,
        [this]()
        {
            if (IsValid(this))
            {
                Destroy();
                UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: Corpse destroyed"));
            }
        },
        CorpseLifetime,
        false
    );
}