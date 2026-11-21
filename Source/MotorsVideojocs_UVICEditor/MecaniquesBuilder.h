#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MecaniquesBuilder.generated.h"

// Eina d'editor: crea els Blueprints BP_TimePickup, BP_FallingPlatform i BP_JumpPad amb els seus nodes.
// Es crida des de Python: unreal.MecaniquesBuilder.construir_mecaniques()
UCLASS()
class UMecaniquesBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TimeIsOut")
	static bool ConstruirMecaniques(const FString& Carpeta = TEXT("/Game/0_TimeIsOut/Mecaniques"));
};
