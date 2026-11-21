#include "MainMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MainMenuWidget.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	UMainMenuWidget* Menu = CreateWidget<UMainMenuWidget>(PC, UMainMenuWidget::StaticClass());
	if (!Menu) return;
	Menu->SetIsFocusable(true);
	Menu->AddToViewport();

	PC->SetShowMouseCursor(true);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu->TakeWidget());
	PC->SetInputMode(Mode);
}
