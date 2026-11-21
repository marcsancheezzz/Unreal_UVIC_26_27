#include "NivellsSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UObjectGlobals.h"

void UNivellsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MapaCarregatHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UNivellsSubsystem::OnMapaCarregat);
}

void UNivellsSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapaCarregatHandle);
	Super::Deinitialize();
}

void UNivellsSubsystem::Desbloquejar(int32 Nivell)
{
	NivellDesbloquejat = FMath::Max(NivellDesbloquejat, Nivell);
}

void UNivellsSubsystem::OnMapaCarregat(UWorld* Mon)
{
	if (!Mon) return;

	// "Level_2" -> 2. Al menú o en altres mapes no fa res.
	FString Nom = Mon->GetMapName();
	Nom.RemoveFromStart(Mon->StreamingLevelsPrefix);
	if (Nom.RemoveFromStart(TEXT("Level_")) && Nom.IsNumeric())
	{
		Desbloquejar(FCString::Atoi(*Nom));

		// Per si de cas: en un nivell de joc, el control sempre és del joc.
		if (APlayerController* PC = Mon->GetFirstPlayerController())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
		}
	}
}
