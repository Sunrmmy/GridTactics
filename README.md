# GridTactics

Developed with Unreal Engine 5

## GridTactics 游戏框架与系统架构说明文档

### 1. 项目概述
GridTactics 是一个基于虚幻引擎（Unreal Engine 5.5）开发的、以网格为基础的即时战术游戏 DEMO。游戏通过体力限制和技能范围设计来强调策略性，而非传统的回合制。项目采用 C++ 为主要开发语言，并利用虚幻引擎的组件化和数据驱动设计思想，构建了一个模块化、可扩展的游戏框架。

#### 核心特色
- **数据驱动**：通过 DataAsset 定义技能、AI 行为等，便于策划调整和内容扩展
- **组件化设计**：将移动、属性、技能等核心功能封装为独立的 ActorComponent，实现高内聚、低耦合
- **事件驱动**：利用委托（Delegate）在不同系统间进行通信，例如伤害事件、死亡事件、技能变更事件等
- **AI 行为树**：通过行为树（Behavior Tree）和黑板（Blackboard）实现智能的敌人 AI，支持动态决策

---

### 2. 核心项目框架
项目遵循 Unreal Engine 的标准游戏框架，并在此基础上进行了扩展：

#### 基础框架组件
- **GameMode (GridTacticsGameMode)**：负责定义游戏的核心规则，如战斗的开始与结束、角色生成、胜负条件判断等
- **PlayerController (GridTacticsPlayerController)**：处理玩家输入，并将输入事件转发给 HeroCharacter 或其组件
- **Character**：
  - **AHeroCharacter**：玩家控制的角色，负责集成输入、相机控制和与玩家相关的组件
  - **AEnemyCharacter**：AI 控制的角色，负责集成 AI 控制器、行为树配置和与敌人相关的组件
- **PlayerState (GridTacticsPlayerState)**：存储玩家的持久化状态，如 FAttributeModifier 的定义
- **GameInstance (GridTacticsGameInstance)**：存储跨关卡的全局数据和管理游戏会话

#### 核心功能组件
这是项目设计的精髓，通过组件化将角色能力解耦：

- **UAttributesComponent**：属性组件，角色的「心脏」
  - 负责管理所有核心属性（HP, MP, Stamina, Shield, Armor, MoveSpeed 等）
  - 实现 Modifier 系统，支持 Buff/Debuff 对属性的动态修改（加法/乘法）
  - 广播 OnDamageTaken 和 OnCharacterDied 事件，供其他系统监听

- **UGridMovementComponent**：网格移动组件，角色的「双腿」
  - 负责处理角色在网格上的所有移动逻辑，包括平滑插值、旋转等
  - 提供 TryMoveOneStep（常规移动）和 ExecuteDisplacementPathWithHeight（技能位移）接口

- **USkillComponent**：技能组件，角色的「技能书」
  - 管理角色拥有的所有技能实例 (FSkillEntry)
  - 维护技能状态机 (ESkillState: Idle, Aiming, Casting)
  - 处理技能的冷却、激活流程和资源消耗
  - 广播 OnSkillAdded、OnSkillReplaced 事件，通知 UI 更新

- **UEnemyAIConfig**：AI 配置组件，敌人的「大脑配置」
  - 存储 AI 的行为参数（战斗距离、技能优先级等），实现了 AI 行为的数据驱动

---

### 3. 网格移动系统 (Grid Movement System)
这是游戏的核心玩法基础，架构设计非常清晰：

#### 核心组件

- **AGridManager (核心)**：网格世界的「上帝」类，管理所有网格单元 (GridCell) 的状态
  - **功能**：
    - 存储所有网格单元，并提供查询接口（IsGridValid, IsGridWalkable）
    - 维护 Actor 与 Grid 的映射关系（GetActorAtGrid, GetActorCurrentGrid）
    - 提供坐标转换功能（WorldToGrid, GridToWorld）

- **UGridMovementComponent**：附加在 Character 上，执行具体的移动动作
  - **状态**：内部维护 EMovementState（Idle, Moving, DisplacementMoving），区分常规移动和技能位移
  - **接口**：
    - **TryMoveOneStep**：尝试向指定方向移动一格，处理碰撞和体力消耗
    - **ExecuteDisplacementPathWithHeight**：执行由技能（如传送、冲锋）发起的特殊位移，支持抛物线和高度偏移

- **UPathPlanner**：路径规划器，使用 A* 算法为 AI 或玩家长距离移动提供最优路径

- **UConflictResolver**：移动冲突解决器。当多个单位尝试同时移动到同一格子时，由它来裁决谁可以移动

#### 工作流程
1. **移动请求**：HeroCharacter 通过输入或 EnemyCharacter 通过 AI 任务，调用 GridMovementComponent::TryMoveOneStep
2. **合法性检查**：GridMovementComponent 请求 GridManager 检查目标格子是否可行走且未被占用
3. **执行移动**：如果合法，GridMovementComponent 开始逐帧更新 Actor 的位置和旋转，实现平滑移动
4. **状态更新**：移动期间，GridMovementComponent 的状态为 Moving，HeroCharacter 的 RootState 为 Busy，阻止其他操作
5. **完成移动**：到达目标后，状态恢复为 Idle

---

### 4. 技能系统 (Skill System)
这是项目中最复杂、设计最精良的系统之一，完全数据驱动且高度可扩展。

#### 核心组件

- **USkillDataAsset (数据核心)**：定义一个技能的所有静态数据
  - **内容**：
    - 基础信息：名称、描述、图标
    - 逻辑类：SkillLogicClass (指向 UBaseSkill 的子类)
    - 范围定义：RangePattern (施法范围) 和 EffectPattern (效果范围)
    - 类型：ESkillTargetType (自身、方向、目标格子、目标敌人)
    - 消耗：冷却、体力、法力、施法时间 (TimeCost)
    - 效果列表：SkillEffects (一个 TArray 的 USkillEffect 实例)

- **USkillComponent (管理器)**：附加在 Character 上，管理所有技能的运行时状态
  - **功能**：
    - 在 BeginPlay 中根据 EquippedSkillsData 实例化 UBaseSkill 对象
    - 管理每个技能的冷却 (CooldownRemaining)
    - 处理技能的瞄准 (TryStartAiming) 和确认 (TryConfirmSkill) 流程

- **UBaseSkill (运行时实例)**：执行技能的具体逻辑
  - **核心方法**：
    - **CanActivate**：检查资源和冷却
    - **Activate**：消耗资源，并调用 ExecuteSkillEffects
    - **ExecuteSkillEffects**：遍历 SkillDataAsset 中的 SkillEffects 列表，并按延迟逐个执行
    - **GetAffectedActors**：根据 EffectPattern 和角色朝向计算受影响的目标

- **USkillEffect (效果原子)**：定义一个原子化的技能效果（伤害、Buff、位移等）
  - **核心方法**：Execute_Implementation，每个子类实现自己的效果
  - **关键特性**：
    - **ExecutionDelay**：支持延迟执行，为多段技能提供了基础
    - **bRecheckRangeOnExecution**：延迟伤害检测的核心，允许效果在执行瞬间重新计算范围
  - **子类示例**：
    - USkillEffect_Damage：造成伤害
    - USkillEffect_Buff：添加/刷新 Buff/Debuff
    - USkillEffect_Teleport：执行传送，支持曼哈顿距离限制
    - USkillEffect_Burn：持续伤害效果

#### 工作流程
1. **输入**：玩家按下技能键，调用 SkillComponent::TryStartAiming
2. **瞄准**：SkillComponent 进入 Aiming 状态。HeroCharacter 在 Tick 中调用 UpdateAimingDirection，显示技能范围指示器
3. **确认**：玩家点击鼠标，调用 SkillComponent::TryConfirmSkill
4. **验证**：TryConfirmSkill 检查技能范围（针对 TargetGrid 类型）和 BaseSkill::CanActivate（资源/冷却）
5. **激活**：验证通过后，调用 BaseSkill::Activate
6. **执行效果**：Activate 调用 ExecuteSkillEffects，后者遍历 SkillDataAsset 中的 SkillEffects 列表
7. **延迟/即时**：每个 SkillEffect 根据自己的 ExecutionDelay 被立即执行或通过 FTimerManager 延迟执行
8. **伤害判定**：SkillEffect_Damage 在执行时，如果 bRecheckRangeOnExecution 为 true，会重新调用 BaseSkill::GetAffectedActors，实现精确的延迟伤害判定

---

### 5. AI 系统 (AI System)
AI 系统基于虚幻引擎标准的行为树 (Behavior Tree) 和黑板 (Blackboard) 构建。

#### 核心组件

- **AEnemyAIController**：敌人的「驾驶员」，负责运行行为树

- **UEnemyAIConfig**：存储 AI 的行为参数，如战斗距离、技能优先级等，使 AI 行为可配置

- **行为树任务 (BTTask)**：
  - **BTTask_SelectAndUseSkill**：AI 的核心攻击逻辑。检查技能范围、冷却、资源，并选择最高优先级的可用技能
  - **BTTask_CalculateKitingPosition**：AI 的移动决策逻辑。根据与玩家的距离，决定是后退、接近还是横向移动（拉扯）
  - **BTTask_MoveToGrid**：执行移动决策，调用 GridMovementComponent 移动一格
  - **BTTask_RotateToTarget**：使敌人旋转朝向玩家
  - **BTTask_FindPlayer**：寻找玩家
  - **BTTask_FindPatrolLocation**：寻找巡逻点

- **行为树服务 (BTService)**：
  - **BTService_DetectPlayer**：定期检测玩家是否在警戒范围内，并将结果写入黑板

- **黑板 (Blackboard)**：AI 的「记忆体」，存储关键数据
  - **键 (Keys)**：TargetPlayer (目标玩家), KitingPosition (移动目标点), LastSkillTime (上次技能使用时间)

#### 工作流程
1. **检测**：BTService_DetectPlayer 定期索敌，找到玩家后写入黑板
2. **决策**：行为树根据黑板中的 TargetPlayer 进入战斗分支
3. **攻击决策**：
   - BTTask_SelectAndUseSkill 尝试使用技能
   - 如果成功，执行技能并更新 LastSkillTime
   - 如果失败（距离不够、冷却中、资源不足），则进入移动决策
4. **移动决策**：
   - BTTask_CalculateKitingPosition 计算一个合适的「风筝」位置并写入黑板
   - BTTask_RotateToTarget 使敌人朝向目标
   - BTTask_MoveToGrid 读取黑板位置并执行移动
5. **循环**：行为树持续执行，使敌人表现出攻击、移动、再攻击的智能行为

---

### 6. UI 系统 (UI System)
UI 系统主要由 UHUDWidget 和 UserWidget蓝图类 负责，用于显示玩家和敌人角色的的实时状态和HUD。

#### 核心组件

- **UHUDWidget**：专为玩家角色编写，作为玩家HUD的父类，提供便捷的数据获取接口。
  - **数据绑定**：
    - 在 BeginPlay 或 NativeTick 中获取 HeroCharacter 的引用
    - 缓存 AttributesComponent 和 SkillComponent 的指针，避免每帧查找
    - 提供一系列 UFUNCTION(BlueprintPure) 函数，如 GetHPPercent, GetCombinedHealthText, GetSkillComponent 等，供 UMG 蓝图绑定
  - **事件驱动更新**：
    - 绑定到 SkillComponent 的 OnSkillAdded 和 OnSkillReplaced 委托
    - 当技能发生变化时，自动调用 RefreshSkillSlots (蓝图实现)，实现 UI 的自动刷新

#### 工作流程
1. **创建**：HeroCharacter 在 BeginPlay 或 PossessedBy 中创建 UHUDWidget 并添加到视口
2. **数据获取**：UHUDWidget 在 NativeTick 中持续更新对 AttributesComponent 等组件的引用
3. **UI 更新**：
   - UMG 蓝图中的进度条、文本等控件绑定到 UHUDWidget 提供的 Get... 函数
   - 虚幻引擎的 Slate/UMG 系统每帧调用这些函数，自动更新显示
4. **事件响应**：当玩家学习或替换技能时，SkillComponent 广播事件，UHUDWidget 监听到事件后调用蓝图函数 RefreshSkillSlots，实现技能栏的动态更新

---

### 7. 总结与展望
GridTactics 项目已经构建了健康的游戏框架。其数据驱动和组件化的设计思想使其易于维护和扩展。

#### 未来可扩展方向
- **游戏节奏控制**：可以在 GridTacticsGameMode 中添加暂停、时间缩放等功能，以增强即时战斗中的策略性
- **更复杂的资源系统**：引入除体力/法力之外的全局资源或连击点数系统
- **物品与装备**：创建 ItemDataAsset 和 EquipmentComponent，进一步扩展角色能力
- **更复杂的 AI**：引入 EQS (Environment Query System) 进行更智能的选位
- **网络同步**：为组件和关键函数添加网络复制支持，实现联机对战
