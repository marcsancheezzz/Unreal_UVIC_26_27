#include "MecaniquesBuilder.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

namespace
{
	const UEdGraphSchema_K2* Schema() { return GetDefault<UEdGraphSchema_K2>(); }

	UBlueprint* CrearBlueprint(const FString& Carpeta, const FString& Nom)
	{
		UPackage* Paquet = CreatePackage(*(Carpeta / Nom));
		UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Paquet, *Nom, BPTYPE_Normal,
			UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
		FAssetRegistryModule::AssetCreated(BP);
		Paquet->MarkPackageDirty();
		return BP;
	}

	template <typename T>
	T* AfegirComponent(UBlueprint* BP, FName Nom, USCS_Node* Pare = nullptr, USCS_Node** OutNode = nullptr)
	{
		USCS_Node* Node = BP->SimpleConstructionScript->CreateNode(T::StaticClass(), Nom);
		if (Pare) Pare->AddChildNode(Node);
		else BP->SimpleConstructionScript->AddNode(Node);
		if (OutNode) *OutNode = Node;
		return CastChecked<T>(Node->ComponentTemplate);
	}

	UK2Node_Event* EventOverlap(UBlueprint* BP, UEdGraph* Graf)
	{
		const FName NomEvent = GET_FUNCTION_NAME_CHECKED(AActor, ReceiveActorBeginOverlap);
		UK2Node_Event* Ev = FBlueprintEditorUtils::FindOverrideForFunction(BP, AActor::StaticClass(), NomEvent);
		if (!Ev)
		{
			int32 Y = 0;
			Ev = FKismetEditorUtilities::AddDefaultEventNode(BP, Graf, NomEvent, AActor::StaticClass(), Y);
		}
		Ev->SetEnabledState(ENodeEnabledState::Enabled, false);
		Ev->NodeComment.Empty();
		Ev->bCommentBubbleVisible = false;
		Ev->NodePosX = 0;
		Ev->NodePosY = 0;
		return Ev;
	}

	UK2Node_CallFunction* Funcio(UEdGraph* Graf, UClass* Classe, FName Nom, int32 X, int32 Y)
	{
		FGraphNodeCreator<UK2Node_CallFunction> Creador(*Graf);
		UK2Node_CallFunction* Node = Creador.CreateNode();
		Node->SetFromFunction(Classe->FindFunctionByName(Nom));
		Node->NodePosX = X;
		Node->NodePosY = Y;
		Creador.Finalize();
		return Node;
	}

	UK2Node_DynamicCast* CastA(UEdGraph* Graf, UClass* Classe, int32 X, int32 Y)
	{
		FGraphNodeCreator<UK2Node_DynamicCast> Creador(*Graf);
		UK2Node_DynamicCast* Node = Creador.CreateNode();
		Node->TargetType = Classe;
		Node->NodePosX = X;
		Node->NodePosY = Y;
		Creador.Finalize();
		return Node;
	}

	template <typename T>
	T* Variable(UEdGraph* Graf, FName Nom, UClass* Classe, int32 X, int32 Y)
	{
		FGraphNodeCreator<T> Creador(*Graf);
		T* Node = Creador.CreateNode();
		if (Classe) Node->VariableReference.SetExternalMember(Nom, Classe);
		else Node->VariableReference.SetSelfMember(Nom);
		Node->NodePosX = X;
		Node->NodePosY = Y;
		Creador.Finalize();
		return Node;
	}

	UEdGraphPin* Pin(UEdGraphNode* Node, FName Nom, EEdGraphPinDirection Dir = EGPD_Input)
	{
		UEdGraphPin* P = Node->FindPin(Nom, Dir);
		ensureMsgf(P, TEXT("Pin %s no trobat a %s"), *Nom.ToString(), *Node->GetName());
		return P;
	}

	void Connectar(UEdGraphPin* A, UEdGraphPin* B)
	{
		if (A && B) ensureMsgf(Schema()->TryCreateConnection(A, B), TEXT("No s'han pogut connectar %s i %s"), *A->PinName.ToString(), *B->PinName.ToString());
	}

	void Valor(UEdGraphPin* P, const FString& V) { if (P) Schema()->TrySetDefaultValue(*P, V); }
	void Objecte(UEdGraphPin* P, UObject* O) { if (P) Schema()->TrySetDefaultObject(*P, O); }

	UEdGraphPin* Exec(UEdGraphNode* N) { return N->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); }
	UEdGraphPin* Then(UEdGraphNode* N) { return N->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); }

	bool Compilar(UBlueprint* BP)
	{
		FKismetEditorUtilities::CompileBlueprint(BP);
		UE_LOG(LogTemp, Display, TEXT("Mecaniques: %s compilat, estat %d"), *BP->GetName(), (int32)BP->Status);
		return BP->Status != BS_Error;
	}
}

bool UMecaniquesBuilder::ConstruirMecaniques(const FString& Carpeta)
{
	UClass* Jugador = LoadObject<UClass>(nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
	UClass* PortaInicial = LoadObject<UClass>(nullptr, TEXT("/Game/0_TimeIsOut/Actors/InitialDoor.InitialDoor_C"));
	UStaticMesh* Cub = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cilindre = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* Or = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
	UMaterialInterface* Pedra = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Brick_Cut_Stone.M_Brick_Cut_Stone"));
	UMaterialInterface* Tech = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Tech_Hex_Tile_Pulse.M_Tech_Hex_Tile_Pulse"));
	USoundBase* SoRellotge = LoadObject<USoundBase>(nullptr, TEXT("/Game/0_TimeIsOut/Sounds/Hammer-Pickup.Hammer-Pickup"));
	USoundBase* SoSalt = LoadObject<USoundBase>(nullptr, TEXT("/Game/0_TimeIsOut/Sounds/Button.Button"));
	if (!Jugador || !PortaInicial) return false;

	bool bTotOk = true;

	// ---------- BP_TimePickup: toques el rellotge -> +10 s al compte enrere de la InitialDoor ----------
	{
		UBlueprint* BP = CrearBlueprint(Carpeta, TEXT("BP_TimePickup"));
		USCS_Node* Arrel = nullptr;
		USphereComponent* Zona = AfegirComponent<USphereComponent>(BP, TEXT("Zona"), nullptr, &Arrel);
		Zona->InitSphereRadius(80.f);
		Zona->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		UStaticMeshComponent* Rellotge = AfegirComponent<UStaticMeshComponent>(BP, TEXT("Rellotge"), Arrel);
		Rellotge->SetStaticMesh(Cilindre);
		Rellotge->SetMaterial(0, Or);
		Rellotge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Rellotge->SetRelativeRotation_Direct(FRotator(0.f, 0.f, 90.f));
		Rellotge->SetRelativeScale3D_Direct(FVector(0.6f, 0.6f, 0.12f));
		URotatingMovementComponent* Gir = AfegirComponent<URotatingMovementComponent>(BP, TEXT("Gir"));
		Gir->RotationRate = FRotator(0.f, 120.f, 0.f);

		FEdGraphPinType TipusReal(UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Double, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
		FBlueprintEditorUtils::AddMemberVariable(BP, TEXT("SegonsExtra"), TipusReal, TEXT("10.0"));
		FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(BP, TEXT("SegonsExtra"), false);

		UEdGraph* Graf = FBlueprintEditorUtils::FindEventGraph(BP);
		UK2Node_Event* Ev = EventOverlap(BP, Graf);
		UK2Node_DynamicCast* EsJugador = CastA(Graf, Jugador, 300, 0);
		UK2Node_CallFunction* BuscarPorta = Funcio(Graf, UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetActorOfClass), 650, 0);
		UK2Node_DynamicCast* EsPorta = CastA(Graf, PortaInicial, 1000, 0);
		UK2Node_VariableGet* GetTemps = Variable<UK2Node_VariableGet>(Graf, TEXT("TotalTime"), PortaInicial, 1250, 250);
		UK2Node_VariableGet* GetExtra = Variable<UK2Node_VariableGet>(Graf, TEXT("SegonsExtra"), nullptr, 1250, 350);
		UK2Node_CallFunction* Suma = Funcio(Graf, UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), 1500, 280);
		UK2Node_VariableSet* SetTemps = Variable<UK2Node_VariableSet>(Graf, TEXT("TotalTime"), PortaInicial, 1750, 0);
		UK2Node_CallFunction* So = Funcio(Graf, UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, PlaySound2D), 2050, 0);
		UK2Node_CallFunction* Destruir = Funcio(Graf, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor), 2400, 0);

		Connectar(Then(Ev), Exec(EsJugador));
		Connectar(Pin(Ev, TEXT("OtherActor"), EGPD_Output), EsJugador->GetCastSourcePin());
		Connectar(EsJugador->GetValidCastPin(), Exec(BuscarPorta));
		Objecte(Pin(BuscarPorta, TEXT("ActorClass")), PortaInicial);
		Connectar(Then(BuscarPorta), Exec(EsPorta));
		Connectar(Pin(BuscarPorta, UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output), EsPorta->GetCastSourcePin());
		Connectar(EsPorta->GetValidCastPin(), Exec(SetTemps));
		Connectar(EsPorta->GetCastResultPin(), Pin(GetTemps, UEdGraphSchema_K2::PN_Self));
		Connectar(EsPorta->GetCastResultPin(), Pin(SetTemps, UEdGraphSchema_K2::PN_Self));
		Connectar(Pin(GetTemps, TEXT("TotalTime"), EGPD_Output), Pin(Suma, TEXT("A")));
		Connectar(Pin(GetExtra, TEXT("SegonsExtra"), EGPD_Output), Pin(Suma, TEXT("B")));
		Connectar(Pin(Suma, UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output), Pin(SetTemps, TEXT("TotalTime")));
		Connectar(Then(SetTemps), Exec(So));
		Objecte(Pin(So, TEXT("Sound")), SoRellotge);
		Connectar(Then(So), Exec(Destruir));
		bTotOk &= Compilar(BP);
	}

	// ---------- BP_FallingPlatform: la trepitges -> 1 s -> desapareix -> 3 s -> torna ----------
	{
		UBlueprint* BP = CrearBlueprint(Carpeta, TEXT("BP_FallingPlatform"));
		USCS_Node* Arrel = nullptr;
		AfegirComponent<USceneComponent>(BP, TEXT("Arrel"), nullptr, &Arrel);
		UStaticMeshComponent* Plataforma = AfegirComponent<UStaticMeshComponent>(BP, TEXT("Plataforma"), Arrel);
		Plataforma->SetStaticMesh(Cub);
		Plataforma->SetMaterial(0, Pedra);
		Plataforma->SetRelativeLocation_Direct(FVector(0.f, 0.f, -20.f));
		Plataforma->SetRelativeScale3D_Direct(FVector(4.f, 4.f, 0.4f));
		UBoxComponent* Trepitjar = AfegirComponent<UBoxComponent>(BP, TEXT("Trepitjar"), Arrel);
		Trepitjar->SetRelativeLocation_Direct(FVector(0.f, 0.f, 30.f));
		Trepitjar->InitBoxExtent(FVector(190.f, 190.f, 30.f));
		Trepitjar->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

		UEdGraph* Graf = FBlueprintEditorUtils::FindEventGraph(BP);
		UK2Node_Event* Ev = EventOverlap(BP, Graf);
		UK2Node_DynamicCast* EsJugador = CastA(Graf, Jugador, 300, 0);
		UK2Node_CallFunction* Espera1 = Funcio(Graf, UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Delay), 650, 0);
		UK2Node_CallFunction* Amagar = Funcio(Graf, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorHiddenInGame), 950, 0);
		UK2Node_CallFunction* SenseCol = Funcio(Graf, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision), 1250, 0);
		UK2Node_CallFunction* Espera3 = Funcio(Graf, UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Delay), 1550, 0);
		UK2Node_CallFunction* Mostrar = Funcio(Graf, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorHiddenInGame), 1850, 0);
		UK2Node_CallFunction* AmbCol = Funcio(Graf, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision), 2150, 0);

		Connectar(Then(Ev), Exec(EsJugador));
		Connectar(Pin(Ev, TEXT("OtherActor"), EGPD_Output), EsJugador->GetCastSourcePin());
		Connectar(EsJugador->GetValidCastPin(), Exec(Espera1));
		Valor(Pin(Espera1, TEXT("Duration")), TEXT("1.0"));
		Connectar(Then(Espera1), Exec(Amagar));
		Valor(Pin(Amagar, TEXT("bNewHidden")), TEXT("true"));
		Connectar(Then(Amagar), Exec(SenseCol));
		Valor(Pin(SenseCol, TEXT("bNewActorEnableCollision")), TEXT("false"));
		Connectar(Then(SenseCol), Exec(Espera3));
		Valor(Pin(Espera3, TEXT("Duration")), TEXT("3.0"));
		Connectar(Then(Espera3), Exec(Mostrar));
		Valor(Pin(Mostrar, TEXT("bNewHidden")), TEXT("false"));
		Connectar(Then(Mostrar), Exec(AmbCol));
		Valor(Pin(AmbCol, TEXT("bNewActorEnableCollision")), TEXT("true"));
		bTotOk &= Compilar(BP);
	}

	// ---------- BP_JumpPad: hi passes per sobre -> et llança endavant i amunt ----------
	{
		UBlueprint* BP = CrearBlueprint(Carpeta, TEXT("BP_JumpPad"));
		USCS_Node* Arrel = nullptr;
		UBoxComponent* Zona = AfegirComponent<UBoxComponent>(BP, TEXT("Zona"), nullptr, &Arrel);
		Zona->InitBoxExtent(FVector(80.f, 80.f, 40.f));
		Zona->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		UStaticMeshComponent* Base = AfegirComponent<UStaticMeshComponent>(BP, TEXT("Base"), Arrel);
		Base->SetStaticMesh(Cilindre);
		Base->SetMaterial(0, Tech);
		Base->SetRelativeLocation_Direct(FVector(0.f, 0.f, -35.f));
		Base->SetRelativeScale3D_Direct(FVector(1.6f, 1.6f, 0.1f));

		UEdGraph* Graf = FBlueprintEditorUtils::FindEventGraph(BP);
		UK2Node_Event* Ev = EventOverlap(BP, Graf);
		UK2Node_DynamicCast* EsJugador = CastA(Graf, Jugador, 300, 0);
		UK2Node_CallFunction* Llancar = Funcio(Graf, ACharacter::StaticClass(), GET_FUNCTION_NAME_CHECKED(ACharacter, LaunchCharacter), 650, 0);
		UK2Node_CallFunction* So = Funcio(Graf, UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, PlaySound2D), 1000, 0);

		Connectar(Then(Ev), Exec(EsJugador));
		Connectar(Pin(Ev, TEXT("OtherActor"), EGPD_Output), EsJugador->GetCastSourcePin());
		Connectar(EsJugador->GetValidCastPin(), Exec(Llancar));
		Connectar(EsJugador->GetCastResultPin(), Pin(Llancar, UEdGraphSchema_K2::PN_Self));
		Valor(Pin(Llancar, TEXT("LaunchVelocity")), TEXT("500.0,0.0,1200.0"));
		Valor(Pin(Llancar, TEXT("bXYOverride")), TEXT("true"));
		Valor(Pin(Llancar, TEXT("bZOverride")), TEXT("true"));
		Connectar(Then(Llancar), Exec(So));
		Objecte(Pin(So, TEXT("Sound")), SoSalt);
		bTotOk &= Compilar(BP);
	}

	return bTotOk;
}
