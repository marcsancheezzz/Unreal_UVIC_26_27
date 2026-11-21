#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

// GameMode del mapa Lvl_MainMenu: sense personatge, mostra el menú i el ratolí.
UCLASS()
class MOTORSVIDEOJOCS_UVIC_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

protected:
	virtual void BeginPlay() override;
};
