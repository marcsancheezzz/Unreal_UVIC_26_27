#include "MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NivellsSubsystem.h"

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Fons = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Fons"));
		Fons->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 1.f));
		WidgetTree->RootWidget = Fons;

		UVerticalBox* Caixa = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Caixa"));
		Fons->SetContent(Caixa);
		if (UBorderSlot* FonsSlot = Cast<UBorderSlot>(Caixa->Slot))
		{
			FonsSlot->SetHorizontalAlignment(HAlign_Center);
			FonsSlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* Titol = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Titol"));
		Titol->SetText(FText::FromString(TEXT("TIME IS OUT")));
		FSlateFontInfo Font = Titol->GetFont();
		Font.Size = 64;
		Titol->SetFont(Font);
		Titol->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TitolSlot = Caixa->AddChildToVerticalBox(Titol))
		{
			TitolSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));
			TitolSlot->SetHorizontalAlignment(HAlign_Center);
		}

		BotonsNivell.Reset();
		for (int32 i = 1; i <= 3; ++i)
		{
			BotonsNivell.Add(AfegirBoto(Caixa, FString::Printf(TEXT("Nivell %d"), i)));
		}
		BotoSortir = AfegirBoto(Caixa, TEXT("Sortir"));
	}
	return Super::RebuildWidget();
}

UButton* UMainMenuWidget::AfegirBoto(UVerticalBox* Caixa, const FString& Text)
{
	UButton* Boto = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Etiqueta = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Etiqueta->SetText(FText::FromString(Text));
	Etiqueta->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	FSlateFontInfo Font = Etiqueta->GetFont();
	Font.Size = 28;
	Etiqueta->SetFont(Font);
	Boto->SetContent(Etiqueta);

	if (UVerticalBoxSlot* BotoSlot = Caixa->AddChildToVerticalBox(Boto))
	{
		BotoSlot->SetPadding(FMargin(0.f, 8.f));
		BotoSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	return Boto;
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const UNivellsSubsystem* Nivells = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNivellsSubsystem>() : nullptr;
	const int32 Desbloquejat = Nivells ? Nivells->GetNivellDesbloquejat() : 1;

	for (int32 i = 0; i < BotonsNivell.Num(); ++i)
	{
		const bool bObert = (i + 1) <= Desbloquejat;
		BotonsNivell[i]->SetIsEnabled(bObert);
		if (UTextBlock* Etiqueta = Cast<UTextBlock>(BotonsNivell[i]->GetContent()))
		{
			Etiqueta->SetText(FText::FromString(bObert ? FString::Printf(TEXT("Nivell %d"), i + 1) : FString::Printf(TEXT("Nivell %d  (bloquejat)"), i + 1)));
		}
	}

	BotonsNivell[0]->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnNivell1);
	BotonsNivell[1]->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnNivell2);
	BotonsNivell[2]->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnNivell3);
	BotoSortir->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnSortir);
}

void UMainMenuWidget::AnarANivell(FName Nivell)
{
	// Torna el control al joc: si no, el nou nivell es queda en mode menú i el personatge no es mou.
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	RemoveFromParent();
	UGameplayStatics::OpenLevel(this, Nivell);
}

void UMainMenuWidget::OnNivell1() { AnarANivell(TEXT("Level_1")); }
void UMainMenuWidget::OnNivell2() { AnarANivell(TEXT("Level_2")); }
void UMainMenuWidget::OnNivell3() { AnarANivell(TEXT("Level_3")); }

void UMainMenuWidget::OnSortir()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
