#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

// Menú principal: títol, un botó per nivell (bloquejat si encara no l'has desbloquejat) i Sortir.
UCLASS()
class MOTORSVIDEOJOCS_UVIC_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	const TArray<TObjectPtr<UButton>>& GetBotonsNivell() const { return BotonsNivell; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UButton* AfegirBoto(UVerticalBox* Caixa, const FString& Text);
	void AnarANivell(FName Nivell);

	UFUNCTION()
	void OnNivell1();
	UFUNCTION()
	void OnNivell2();
	UFUNCTION()
	void OnNivell3();
	UFUNCTION()
	void OnSortir();

	UPROPERTY()
	TArray<TObjectPtr<UButton>> BotonsNivell;

	UPROPERTY()
	TObjectPtr<UButton> BotoSortir;
};
