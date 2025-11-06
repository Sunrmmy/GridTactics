// Fill out your copyright notice in the Description page of Project Settings.


#include "GridTacticsPlayerController.h"

void AGridTacticsPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 显示鼠标光标
	bShowMouseCursor = true;

	// 设置输入模式为“游戏和UI”（玩家输入同时被游戏和UI接收）
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);	// 允许鼠标移出游戏窗口
	InputMode.SetHideCursorDuringCapture(false);	// 拖动时不隐藏光标
	SetInputMode(InputMode);
}