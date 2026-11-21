#include "ProvaAutomatica.h"

#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MainMenuWidget.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NivellsSubsystem.h"
#include "UObject/UObjectIterator.h"

bool UProvaAutomatica::ShouldCreateSubsystem(UObject* Outer) const
{
	return FParse::Param(FCommandLine::Get(), TEXT("ProvaTimeIsOut"));
}

void UProvaAutomatica::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UProvaAutomatica::Tick));
}

void UProvaAutomatica::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

void UProvaAutomatica::Comprova(bool bOk, const FString& Que)
{
	Resultats.Add(FString::Printf(TEXT("%s  %s"), bOk ? TEXT("OK   ") : TEXT("FALLA"), *Que));
	if (!bOk) ++Errors;
	UE_LOG(LogTemp, Display, TEXT("ProvaTimeIsOut: %s"), *Resultats.Last());
}

void UProvaAutomatica::Acabar()
{
	if (bAcabat) return;
	bAcabat = true;
	Resultats.Add(FString::Printf(TEXT("RESULTAT: %s (%d errors)"), Errors == 0 ? TEXT("TOT OK") : TEXT("HI HA ERRORS"), Errors));
	FFileHelper::SaveStringArrayToFile(Resultats, *(FPaths::ProjectSavedDir() / TEXT("ProvaTimeIsOut.txt")));
	UE_LOG(LogTemp, Display, TEXT("ProvaTimeIsOut: %s"), *Resultats.Last());
	if (UWorld* Mon = GetGameInstance()->GetWorld())
	{
		UKismetSystemLibrary::QuitGame(Mon, nullptr, EQuitPreference::Quit, false);
	}
}

APawn* UProvaAutomatica::Jugador() const
{
	UWorld* Mon = GetGameInstance()->GetWorld();
	APlayerController* PC = Mon ? Mon->GetFirstPlayerController() : nullptr;
	return PC ? PC->GetPawn() : nullptr;
}

AActor* UProvaAutomatica::Buscar(const FString& Classe, int32 Index) const
{
	int32 Trobats = 0;
	for (TActorIterator<AActor> It(GetGameInstance()->GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetClass()->GetName() == Classe && Trobats++ == Index) return *It;
	}
	return nullptr;
}

AActor* UProvaAutomatica::VictoryZoneCap(FName Nivell) const
{
	for (TActorIterator<AActor> It(GetGameInstance()->GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName() != TEXT("VictoryZone_C")) continue;
		if (FNameProperty* Prop = FindFProperty<FNameProperty>(It->GetClass(), TEXT("NextLevelName")))
		{
			if (Prop->GetPropertyValue_InContainer(*It) == Nivell) return *It;
		}
	}
	return nullptr;
}

void UProvaAutomatica::Teleport(const FVector& On) const
{
	if (ACharacter* Personatge = Cast<ACharacter>(Jugador()))
	{
		Personatge->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		Personatge->SetActorLocation(On, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UProvaAutomatica::PremerW(bool bPremer) const
{
	// Entra per Slate, com un teclat de veritat: si el joc es queda en "mode menú", el personatge no es mourà.
	FSlateApplication& Slate = FSlateApplication::Get();
	const FKeyEvent Tecla(EKeys::W, Slate.GetModifierKeys(), 0, false, 0, 0);
	if (bPremer) Slate.ProcessKeyDownEvent(Tecla);
	else Slate.ProcessKeyUpEvent(Tecla);
}

bool UProvaAutomatica::ClicarNivell(int32 Index) const
{
	for (TObjectIterator<UMainMenuWidget> It; It; ++It)
	{
		if (It->GetWorld() == GetGameInstance()->GetWorld() && It->IsInViewport())
		{
			const TArray<TObjectPtr<UButton>>& Botons = It->GetBotonsNivell();
			if (!Botons.IsValidIndex(Index) || !Botons[Index]->GetIsEnabled()) return false;
			Botons[Index]->OnClicked.Broadcast();
			return true;
		}
	}
	return false;
}

double UProvaAutomatica::TempsRestant() const
{
	AActor* Porta = Buscar(TEXT("InitialDoor_C"));
	FDoubleProperty* Prop = Porta ? FindFProperty<FDoubleProperty>(Porta->GetClass(), TEXT("TotalTime")) : nullptr;
	return Prop ? Prop->GetPropertyValue_InContainer(Porta) : -1.0;
}

int32 UProvaAutomatica::Comptar(const FString& Classe) const
{
	int32 N = 0;
	for (TActorIterator<AActor> It(GetGameInstance()->GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetClass()->GetName() == Classe) ++N;
	}
	return N;
}

double UProvaAutomatica::TerraSota(const AActor* Actor) const
{
	// Distància fins al terra just a sota (-1 si no n'hi ha en 400 cm).
	FHitResult Impacte;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Actor);
	if (const APawn* P = Jugador()) Params.AddIgnoredActor(P);
	const FVector Inici = Actor->GetActorLocation();
	if (GetGameInstance()->GetWorld()->LineTraceSingleByChannel(Impacte, Inici, Inici - FVector(0, 0, 400), ECC_Visibility, Params))
	{
		return Impacte.Distance;
	}
	return -1.0;
}

FTransform UProvaAutomatica::PortaFinal() const
{
	if (AActor* Porta = Buscar(TEXT("FinalDoor_C")))
	{
		TArray<UStaticMeshComponent*> Meshes;
		Porta->GetComponents(Meshes);
		for (UStaticMeshComponent* M : Meshes)
		{
			if (M->GetName() == TEXT("Door")) return M->GetComponentTransform();
		}
	}
	return FTransform::Identity;
}

void UProvaAutomatica::ComprovaTerraRellotges(const FString& Nivell)
{
	for (int32 i = 0; AActor* R = Buscar(TEXT("BP_TimePickup_C"), i); ++i)
	{
		const double D = TerraSota(R);
		Comprova(D >= 0.0 && D < 250.0, FString::Printf(TEXT("%s: el rellotge %d te terra a sota (%.0f cm)"), *Nivell, i + 1, D));
	}
}

int32 UProvaAutomatica::Desbloquejat() const
{
	const UNivellsSubsystem* Nivells = GetGameInstance()->GetSubsystem<UNivellsSubsystem>();
	return Nivells ? Nivells->GetNivellDesbloquejat() : -1;
}

bool UProvaAutomatica::Tick(float DeltaTime)
{
	UWorld* Mon = GetGameInstance()->GetWorld();
	if (bAcabat || !Mon) return true;

	FString Ara = Mon->GetMapName();
	Ara.RemoveFromStart(Mon->StreamingLevelsPrefix);
	if (Ara != Mapa)
	{
		Mapa = Ara;
		TempsPas = 0.0; // les esperes compten des que s'ha carregat el mapa
		UE_LOG(LogTemp, Display, TEXT("ProvaTimeIsOut: mapa %s (pas %d)"), *Mapa, Pas);
	}
	TempsPas += DeltaTime;

	if (TempsPas > 40.0)
	{
		Comprova(false, FString::Printf(TEXT("El pas %d s'ha quedat encallat al mapa %s"), Pas, *Mapa));
		Acabar();
		return true;
	}

	auto Espera = [&](const TCHAR* MapaEsperat, double Segons) { return Mapa == MapaEsperat && TempsPas >= Segons; };
	auto Moviment = [&](const FString& On)
	{
		const APawn* P = Jugador();
		const double Dist = P ? FVector::Dist2D(P->GetActorLocation(), PosInicial) : 0.0;
		Comprova(P && Dist > 50.0, FString::Printf(TEXT("%s: el personatge es mou amb la W (%.0f cm)"), *On, Dist));
	};

	switch (Pas)
	{
	case 0: // Menú inicial
		if (!Espera(TEXT("Lvl_MainMenu"), 1.5)) break;
		Comprova(Desbloquejat() == 1, TEXT("Menu: nomes el Nivell 1 desbloquejat al principi"));
		Comprova(!ClicarNivell(2), TEXT("Menu: el boto Nivell 3 esta bloquejat"));
		Comprova(ClicarNivell(0), TEXT("Menu: clic a Nivell 1"));
		Seguent();
		break;
	case 1:
		if (!Espera(TEXT("Level_1"), 3.0)) break;
		Comprova(Jugador() != nullptr, TEXT("Nivell 1: hi ha personatge"));
		PosInicial = Jugador() ? Jugador()->GetActorLocation() : FVector::ZeroVector;
		PremerW(true);
		Seguent();
		break;
	case 2:
		if (TempsPas < 1.0) break;
		PremerW(false);
		Moviment(TEXT("Nivell 1"));
		if (AActor* VZ = VictoryZoneCap(TEXT("Level_2"))) Teleport(VZ->GetActorLocation());
		else Comprova(false, TEXT("Nivell 1: hi ha VictoryZone cap al Level_2"));
		Seguent();
		break;
	case 3:
		if (!Espera(TEXT("Level_2"), 3.0)) break;
		Comprova(true, TEXT("Nivell 1 -> Nivell 2"));
		Comprova(Desbloquejat() >= 2, TEXT("Nivell 2 desbloquejat"));
		PosInicial = Jugador() ? Jugador()->GetActorLocation() : FVector::ZeroVector;
		PremerW(true);
		Seguent();
		break;
	case 4: // Nivell 2: rellotges
		if (TempsPas < 1.0) break;
		PremerW(false);
		Moviment(TEXT("Nivell 2"));
		Comprova(Comptar(TEXT("Hammer_C")) == 2, FString::Printf(TEXT("Nivell 2: queden 2 martells (%d)"), Comptar(TEXT("Hammer_C"))));
		Comprova(Comptar(TEXT("BP_TimePickup_C")) == 2, TEXT("Nivell 2: hi ha 2 rellotges"));
		ComprovaTerraRellotges(TEXT("Nivell 2"));
		TempsAbans = TempsRestant();
		Objectiu = Buscar(TEXT("BP_TimePickup_C"));
		if (Objectiu.IsValid()) Teleport(Objectiu->GetActorLocation());
		Seguent();
		break;
	case 5:
		if (TempsPas < 0.5) break;
		Comprova(!Objectiu.IsValid() && TempsRestant() > TempsAbans + 8.0, FString::Printf(TEXT("Nivell 2: el rellotge suma temps (%.1f -> %.1f s)"), TempsAbans, TempsRestant()));
		PortaAbans = PortaFinal();
		Objectiu = Buscar(TEXT("Hammer_C"), 0);
		if (Objectiu.IsValid()) Teleport(Objectiu->GetActorLocation());
		Seguent();
		break;
	case 6:
		if (TempsPas < 1.5) break;
		{
			AActor* Segon = Buscar(TEXT("Hammer_C"), 0);
			if (Segon == Objectiu.Get()) Segon = Buscar(TEXT("Hammer_C"), 1);
			Objectiu = Segon;
		}
		if (Objectiu.IsValid()) Teleport(Objectiu->GetActorLocation());
		Seguent();
		break;
	case 7:
		if (TempsPas < 4.0) break;
		Comprova(Comptar(TEXT("Hammer_C")) == 0, TEXT("Nivell 2: s'han recollit els 2 martells"));
		// La porta s'obre quan t'hi acostes (la seva Box) amb tots els martells.
		if (AActor* Porta = Buscar(TEXT("FinalDoor_C")))
		{
			TArray<UBoxComponent*> Caixes;
			Porta->GetComponents(Caixes);
			if (Caixes.Num() > 0) Teleport(Caixes[0]->GetComponentLocation());
		}
		Seguent();
		break;
	case 8:
		if (TempsPas < 3.0) break;
		Comprova(!PortaFinal().Equals(PortaAbans, 1.0), TEXT("Nivell 2: la porta final s'obre amb els 2 martells"));
		if (AActor* VZ = VictoryZoneCap(TEXT("Level_3"))) Teleport(VZ->GetActorLocation());
		else Comprova(false, TEXT("Nivell 2: hi ha VictoryZone cap al Level_3"));
		Seguent();
		break;
	case 9:
		if (!Espera(TEXT("Level_3"), 3.0)) break;
		Comprova(true, TEXT("Nivell 2 -> Nivell 3"));
		Comprova(Desbloquejat() >= 3, TEXT("Nivell 3 desbloquejat"));
		PosInicial = Jugador() ? Jugador()->GetActorLocation() : FVector::ZeroVector;
		PremerW(true);
		Seguent();
		break;
	case 10: // Mecànica 1: rellotge
		if (TempsPas < 1.0) break;
		PremerW(false);
		Moviment(TEXT("Nivell 3"));
		TempsAbans = TempsRestant();
		Comprova(TempsAbans > 0.0, FString::Printf(TEXT("Nivell 3: hi ha compte enrere (%.1f s)"), TempsAbans));
		ComprovaTerraRellotges(TEXT("Nivell 3"));
		Objectiu = Buscar(TEXT("BP_TimePickup_C"));
		if (Objectiu.IsValid()) Teleport(Objectiu->GetActorLocation());
		else Comprova(false, TEXT("Nivell 3: hi ha BP_TimePickup"));
		Seguent();
		break;
	case 11: // Mecànica 2: plataforma
		if (TempsPas < 0.5) break;
		Comprova(!Objectiu.IsValid(), TEXT("Rellotge: desapareix en tocar-lo"));
		Comprova(TempsRestant() > TempsAbans + 8.0, FString::Printf(TEXT("Rellotge: suma temps (%.1f -> %.1f s)"), TempsAbans, TempsRestant()));
		Objectiu = Buscar(TEXT("BP_FallingPlatform_C"), 1);
		if (Objectiu.IsValid()) Teleport(Objectiu->GetActorLocation() + FVector(0, 0, 100));
		else Comprova(false, TEXT("Nivell 3: hi ha BP_FallingPlatform"));
		Seguent();
		break;
	case 12:
		if (TempsPas < 1.6) break;
		Comprova(Objectiu.IsValid() && Objectiu->IsHidden() && !Objectiu->GetActorEnableCollision(), TEXT("Plataforma: desapareix 1 s despres de trepitjar-la"));
		if (AActor* Inici = Buscar(TEXT("PlayerStart"))) Teleport(Inici->GetActorLocation());
		Seguent();
		break;
	case 13: // Mecànica 3: trampolí
		if (TempsPas < 3.0) break;
		Comprova(Objectiu.IsValid() && !Objectiu->IsHidden() && Objectiu->GetActorEnableCollision(), TEXT("Plataforma: torna a apareixer 3 s despres"));
		Objectiu = Buscar(TEXT("BP_JumpPad_C"));
		if (Objectiu.IsValid()) Teleport(Objectiu->GetActorLocation() + FVector(0, 0, 100));
		else Comprova(false, TEXT("Nivell 3: hi ha BP_JumpPad"));
		Seguent();
		break;
	case 14:
		if (TempsPas < 0.15) break;
		Comprova(Jugador() && Jugador()->GetVelocity().Z > 600.0, FString::Printf(TEXT("Trampoli: llanca el jugador amunt (vel Z %.0f)"), Jugador() ? Jugador()->GetVelocity().Z : 0.0));
		Seguent();
		break;
	case 15:
		if (TempsPas < 3.0) break;
		Comprova(Jugador() && Jugador()->GetActorLocation().Z > 1650.0, FString::Printf(TEXT("Trampoli: arriba a dalt de la paret (Z %.0f)"), Jugador() ? Jugador()->GetActorLocation().Z : 0.0));
		if (AActor* VZ = VictoryZoneCap(TEXT("Lvl_MainMenu"))) Teleport(VZ->GetActorLocation());
		else Comprova(false, TEXT("Nivell 3: hi ha VictoryZone cap al menu"));
		Seguent();
		break;
	case 16: // Tornada al menú
		if (!Espera(TEXT("Lvl_MainMenu"), 1.5)) break;
		Comprova(true, TEXT("Nivell 3 -> Menu"));
		Comprova(Desbloquejat() == 3, TEXT("Menu: els 3 nivells desbloquejats"));
		Comprova(ClicarNivell(1), TEXT("Menu: clic a Nivell 2 (ara desbloquejat)"));
		Seguent();
		break;
	case 17:
		if (!Espera(TEXT("Level_2"), 3.0)) break;
		PosInicial = Jugador() ? Jugador()->GetActorLocation() : FVector::ZeroVector;
		PremerW(true);
		Seguent();
		break;
	case 18:
		if (TempsPas < 1.0) break;
		PremerW(false);
		Moviment(TEXT("Nivell 2 entrant des del menu"));
		Acabar();
		break;
	}
	return true;
}
