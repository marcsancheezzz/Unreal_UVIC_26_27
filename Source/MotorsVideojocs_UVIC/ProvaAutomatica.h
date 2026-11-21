#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProvaAutomatica.generated.h"

// Prova que juga tota la partida sola: menú -> N1 -> N2 -> N3 (mecàniques) -> menú -> N2.
// Només s'activa arrencant el joc amb -ProvaTimeIsOut. Escriu el resultat a Saved/ProvaTimeIsOut.txt.
UCLASS()
class MOTORSVIDEOJOCS_UVIC_API UProvaAutomatica : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool Tick(float DeltaTime);

	void Comprova(bool bOk, const FString& Que);
	void Seguent() { ++Pas; TempsPas = 0.0; }
	void Acabar();

	APawn* Jugador() const;
	AActor* Buscar(const FString& Classe, int32 Index = 0) const;
	AActor* VictoryZoneCap(FName Nivell) const;
	void Teleport(const FVector& On) const;
	void PremerW(bool bPremer) const;
	bool ClicarNivell(int32 Index) const;
	double TempsRestant() const;
	int32 Desbloquejat() const;
	int32 Comptar(const FString& Classe) const;
	double TerraSota(const AActor* Actor) const;
	FTransform PortaFinal() const;
	void ComprovaTerraRellotges(const FString& Nivell);

	FTSTicker::FDelegateHandle TickHandle;
	TArray<FString> Resultats;
	int32 Errors = 0;
	int32 Pas = 0;
	double TempsPas = 0.0;
	FString Mapa;
	FVector PosInicial = FVector::ZeroVector;
	double TempsAbans = 0.0;
	TWeakObjectPtr<AActor> Objectiu;
	FTransform PortaAbans;
	bool bAcabat = false;
};
