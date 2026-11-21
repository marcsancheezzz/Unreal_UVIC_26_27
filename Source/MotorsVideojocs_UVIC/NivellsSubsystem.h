#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NivellsSubsystem.generated.h"

// Guarda quin és l'últim nivell desbloquejat. Arribar al Level_N vol dir que has superat l'anterior.
UCLASS()
class MOTORSVIDEOJOCS_UVIC_API UNivellsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "TimeIsOut")
	int32 GetNivellDesbloquejat() const { return NivellDesbloquejat; }

	UFUNCTION(BlueprintCallable, Category = "TimeIsOut")
	void Desbloquejar(int32 Nivell);

private:
	void OnMapaCarregat(UWorld* Mon);

	int32 NivellDesbloquejat = 1;
	FDelegateHandle MapaCarregatHandle;
};
