#include "Commands/UnrealMCPEditorCommands.h"
#include "Camera/CameraActor.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EditorSubsystem.h"
#include "EditorViewportClient.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DirectionalLight.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PointLight.h"
#include "Engine/Selection.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "HighResScreenshot.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Landscape.h"
#include "LandscapeEditorUtils.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#include "LevelEditorViewport.h"
#include "Misc/FileHelper.h"
#include "Subsystems/EditorActorSubsystem.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands() {}

TSharedPtr<FJsonObject>
FUnrealMCPEditorCommands::HandleCommand(const FString &CommandType,
                                        const TSharedPtr<FJsonObject> &Params) {
  // Actor manipulation commands
  if (CommandType == TEXT("get_actors_in_level")) {
    return HandleGetActorsInLevel(Params);
  } else if (CommandType == TEXT("find_actors_by_name")) {
    return HandleFindActorsByName(Params);
  } else if (CommandType == TEXT("spawn_actor") ||
             CommandType == TEXT("create_actor")) {
    if (CommandType == TEXT("create_actor")) {
      UE_LOG(LogTemp, Warning,
             TEXT("'create_actor' command is deprecated and will be removed in "
                  "a future version. Please use 'spawn_actor' instead."));
    }
    return HandleSpawnActor(Params);
  } else if (CommandType == TEXT("delete_actor")) {
    return HandleDeleteActor(Params);
  } else if (CommandType == TEXT("set_actor_transform")) {
    return HandleSetActorTransform(Params);
  } else if (CommandType == TEXT("get_actor_properties")) {
    return HandleGetActorProperties(Params);
  } else if (CommandType == TEXT("set_actor_property")) {
    return HandleSetActorProperty(Params);
  }
  // Blueprint actor spawning
  else if (CommandType == TEXT("spawn_blueprint_actor")) {
    return HandleSpawnBlueprintActor(Params);
  }
  // Editor viewport commands
  else if (CommandType == TEXT("focus_viewport")) {
    return HandleFocusViewport(Params);
  } else if (CommandType == TEXT("take_screenshot")) {
    return HandleTakeScreenshot(Params);
  } else if (CommandType == TEXT("create_landscape")) {
    return HandleCreateLandscape(Params);
  } else if (CommandType == TEXT("get_current_level_name")) {
    return HandleGetCurrentLevelName(Params);
  } else if (CommandType == TEXT("run_python")) {
    return HandleRunPython(Params);
  }

  return FUnrealMCPCommonUtils::CreateErrorResponse(
      FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(
    const TSharedPtr<FJsonObject> &Params) {
  TArray<AActor *> AllActors;
  UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(),
                                        AllActors);

  TArray<TSharedPtr<FJsonValue>> ActorArray;
  for (AActor *Actor : AllActors) {
    if (Actor) {
      ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
    }
  }

  TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
  ResultObj->SetArrayField(TEXT("actors"), ActorArray);

  return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(
    const TSharedPtr<FJsonObject> &Params) {
  FString Pattern;
  if (!Params->TryGetStringField(TEXT("pattern"), Pattern)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'pattern' parameter"));
  }

  TArray<AActor *> AllActors;
  UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(),
                                        AllActors);

  TArray<TSharedPtr<FJsonValue>> MatchingActors;
  for (AActor *Actor : AllActors) {
    if (Actor && Actor->GetName().Contains(Pattern)) {
      MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
    }
  }

  TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
  ResultObj->SetArrayField(TEXT("actors"), MatchingActors);

  return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(
    const TSharedPtr<FJsonObject> &Params) {
  // Get required parameters
  FString ActorType;
  if (!Params->TryGetStringField(TEXT("type"), ActorType)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'type' parameter"));
  }

  // Get actor name (required parameter)
  FString ActorName;
  if (!Params->TryGetStringField(TEXT("name"), ActorName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'name' parameter"));
  }

  // Get optional transform parameters
  FVector Location(0.0f, 0.0f, 0.0f);
  FRotator Rotation(0.0f, 0.0f, 0.0f);
  FVector Scale(1.0f, 1.0f, 1.0f);

  if (Params->HasField(TEXT("location"))) {
    Location =
        FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
  }
  if (Params->HasField(TEXT("rotation"))) {
    Rotation =
        FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
  }
  if (Params->HasField(TEXT("scale"))) {
    Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
  }

  // Create the actor based on type
  AActor *NewActor = nullptr;
  UWorld *World = GEditor->GetEditorWorldContext().World();

  if (!World) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Failed to get editor world"));
  }

  // Check if an actor with this name already exists
  TArray<AActor *> AllActors;
  UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(),
                                        AllActors);
  for (AActor *Actor : AllActors) {
    if (Actor && Actor->GetName() == ActorName) {
      return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
          TEXT("Actor with name '%s' already exists"), *ActorName));
    }
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.Name = *ActorName;

  if (ActorType == TEXT("StaticMeshActor")) {
    NewActor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
  } else if (ActorType == TEXT("PointLight")) {
    NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(),
                                              Location, Rotation, SpawnParams);
  } else if (ActorType == TEXT("SpotLight")) {
    NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(),
                                             Location, Rotation, SpawnParams);
  } else if (ActorType == TEXT("DirectionalLight")) {
    NewActor = World->SpawnActor<ADirectionalLight>(
        ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
  } else if (ActorType == TEXT("CameraActor")) {
    NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),
                                               Location, Rotation, SpawnParams);
  } else {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
  }

  if (NewActor) {
    // Set scale (since SpawnActor only takes location and rotation)
    FTransform Transform = NewActor->GetTransform();
    Transform.SetScale3D(Scale);
    NewActor->SetActorTransform(Transform);

    // Return the created actor's details
    return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
  }

  return FUnrealMCPCommonUtils::CreateErrorResponse(
      TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(
    const TSharedPtr<FJsonObject> &Params) {
  FString ActorName;
  if (!Params->TryGetStringField(TEXT("name"), ActorName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'name' parameter"));
  }

  TArray<AActor *> AllActors;
  UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(),
                                        AllActors);

  for (AActor *Actor : AllActors) {
    if (Actor && Actor->GetName() == ActorName) {
      // Store actor info before deletion for the response
      TSharedPtr<FJsonObject> ActorInfo =
          FUnrealMCPCommonUtils::ActorToJsonObject(Actor);

      // Delete the actor
      Actor->Destroy();

      TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
      ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
      return ResultObj;
    }
  }

  return FUnrealMCPCommonUtils::CreateErrorResponse(
      FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(
    const TSharedPtr<FJsonObject> &Params) {
  // Get actor name
  FString ActorName;
  if (!Params->TryGetStringField(TEXT("name"), ActorName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'name' parameter"));
  }

  // Find the actor
  AActor *TargetActor = nullptr;
  TArray<AActor *> AllActors;
  UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(),
                                        AllActors);

  for (AActor *Actor : AllActors) {
    if (Actor && Actor->GetName() == ActorName) {
      TargetActor = Actor;
      break;
    }
  }

  if (!TargetActor) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Actor not found: %s"), *ActorName));
  }

  // Get transform parameters
  FTransform NewTransform = TargetActor->GetTransform();

  if (Params->HasField(TEXT("location"))) {
    NewTransform.SetLocation(
        FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
  }
  if (Params->HasField(TEXT("rotation"))) {
    NewTransform.SetRotation(FQuat(
        FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
  }
  if (Params->HasField(TEXT("scale"))) {
    NewTransform.SetScale3D(
        FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
  }

  // Set the new transform
  TargetActor->SetActorTransform(NewTransform);

  // Return updated actor info
  return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(
    const TSharedPtr<FJsonObject> &Params) {
  // Get actor name
  FString ActorName;
  if (!Params->TryGetStringField(TEXT("name"), ActorName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'name' parameter"));
  }

  // Find the actor
  AActor *TargetActor = nullptr;
  TArray<AActor *> AllActors;
  UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(),
                                        AllActors);

  for (AActor *Actor : AllActors) {
    if (Actor && Actor->GetName() == ActorName) {
      TargetActor = Actor;
      break;
    }
  }

  if (!TargetActor) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Actor not found: %s"), *ActorName));
  }

  // Always return detailed properties for this command
  return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(
    const TSharedPtr<FJsonObject> &Params) {
  // Get actor name
  FString ActorName;
  if (!Params->TryGetStringField(TEXT("name"), ActorName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'name' parameter"));
  }

  // Find the actor
  AActor *TargetActor = nullptr;
  TArray<AActor *> AllActors;
  UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(),
                                        AllActors);

  for (AActor *Actor : AllActors) {
    if (Actor && Actor->GetName() == ActorName) {
      TargetActor = Actor;
      break;
    }
  }

  if (!TargetActor) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Actor not found: %s"), *ActorName));
  }

  // Get property name
  FString PropertyName;
  if (!Params->TryGetStringField(TEXT("property_name"), PropertyName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'property_name' parameter"));
  }

  // Get property value
  if (!Params->HasField(TEXT("property_value"))) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'property_value' parameter"));
  }

  TSharedPtr<FJsonValue> PropertyValue =
      Params->Values.FindRef(TEXT("property_value"));

  // Set the property using our utility function
  FString ErrorMessage;
  if (FUnrealMCPCommonUtils::SetObjectProperty(TargetActor, PropertyName,
                                               PropertyValue, ErrorMessage)) {
    // Property set successfully
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor"), ActorName);
    ResultObj->SetStringField(TEXT("property"), PropertyName);
    ResultObj->SetBoolField(TEXT("success"), true);

    // Also include the full actor details
    ResultObj->SetObjectField(
        TEXT("actor_details"),
        FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
    return ResultObj;
  } else {
    return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
  }
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(
    const TSharedPtr<FJsonObject> &Params) {
  // Get required parameters
  FString BlueprintName;
  if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'blueprint_name' parameter"));
  }

  FString ActorName;
  if (!Params->TryGetStringField(TEXT("actor_name"), ActorName)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'actor_name' parameter"));
  }

  // Find the blueprint
  if (BlueprintName.IsEmpty()) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Blueprint name is empty"));
  }

  FString Root = TEXT("/Game/Blueprints/");
  FString AssetPath = Root + BlueprintName;

  if (!FPackageName::DoesPackageExist(AssetPath)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
        TEXT(
            "Blueprint '%s' not found – it must reside under /Game/Blueprints"),
        *BlueprintName));
  }

  UBlueprint *Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
  if (!Blueprint) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
  }

  // Get transform parameters
  FVector Location(0.0f, 0.0f, 0.0f);
  FRotator Rotation(0.0f, 0.0f, 0.0f);
  FVector Scale(1.0f, 1.0f, 1.0f);

  if (Params->HasField(TEXT("location"))) {
    Location =
        FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
  }
  if (Params->HasField(TEXT("rotation"))) {
    Rotation =
        FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
  }
  if (Params->HasField(TEXT("scale"))) {
    Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
  }

  // Spawn the actor
  UWorld *World = GEditor->GetEditorWorldContext().World();
  if (!World) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Failed to get editor world"));
  }

  FTransform SpawnTransform;
  SpawnTransform.SetLocation(Location);
  SpawnTransform.SetRotation(FQuat(Rotation));
  SpawnTransform.SetScale3D(Scale);

  FActorSpawnParameters SpawnParams;
  SpawnParams.Name = *ActorName;

  AActor *NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass,
                                               SpawnTransform, SpawnParams);
  if (NewActor) {
    return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
  }

  return FUnrealMCPCommonUtils::CreateErrorResponse(
      TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(
    const TSharedPtr<FJsonObject> &Params) {
  // Get target actor name if provided
  FString TargetActorName;
  bool HasTargetActor =
      Params->TryGetStringField(TEXT("target"), TargetActorName);

  // Get location if provided
  FVector Location(0.0f, 0.0f, 0.0f);
  bool HasLocation = false;
  if (Params->HasField(TEXT("location"))) {
    Location =
        FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    HasLocation = true;
  }

  // Get distance
  float Distance = 1000.0f;
  if (Params->HasField(TEXT("distance"))) {
    Distance = Params->GetNumberField(TEXT("distance"));
  }

  // Get orientation if provided
  FRotator Orientation(0.0f, 0.0f, 0.0f);
  bool HasOrientation = false;
  if (Params->HasField(TEXT("orientation"))) {
    Orientation =
        FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
    HasOrientation = true;
  }

  // Get the active viewport
  FLevelEditorViewportClient *ViewportClient =
      (FLevelEditorViewportClient *)GEditor->GetActiveViewport()->GetClient();
  if (!ViewportClient) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Failed to get active viewport"));
  }

  // If we have a target actor, focus on it
  if (HasTargetActor) {
    // Find the actor
    AActor *TargetActor = nullptr;
    TArray<AActor *> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(),
                                          AllActors);

    for (AActor *Actor : AllActors) {
      if (Actor && Actor->GetName() == TargetActorName) {
        TargetActor = Actor;
        break;
      }
    }

    if (!TargetActor) {
      return FUnrealMCPCommonUtils::CreateErrorResponse(
          FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
    }

    // Focus on the actor
    ViewportClient->SetViewLocation(TargetActor->GetActorLocation() -
                                    FVector(Distance, 0.0f, 0.0f));
  }
  // Otherwise use the provided location
  else if (HasLocation) {
    ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
  } else {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Either 'target' or 'location' must be provided"));
  }

  // Set orientation if provided
  if (HasOrientation) {
    ViewportClient->SetViewRotation(Orientation);
  }

  // Force viewport to redraw
  ViewportClient->Invalidate();

  TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
  ResultObj->SetBoolField(TEXT("success"), true);
  return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(
    const TSharedPtr<FJsonObject> &Params) {
  // Get file path parameter
  FString FilePath;
  if (!Params->TryGetStringField(TEXT("filepath"), FilePath)) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Missing 'filepath' parameter"));
  }

  // Ensure the file path has a proper extension
  if (!FilePath.EndsWith(TEXT(".png"))) {
    FilePath += TEXT(".png");
  }

  // Get the active viewport
  if (GEditor && GEditor->GetActiveViewport()) {
    FViewport *Viewport = GEditor->GetActiveViewport();
    TArray<FColor> Bitmap;
    FIntRect ViewportRect(0, 0, Viewport->GetSizeXY().X,
                          Viewport->GetSizeXY().Y);

    if (Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect)) {
      TArray<uint8> CompressedBitmap;
      FImageUtils::CompressImageArray(Viewport->GetSizeXY().X,
                                      Viewport->GetSizeXY().Y, Bitmap,
                                      CompressedBitmap);

      if (FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath)) {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("filepath"), FilePath);
        return ResultObj;
      }
    }
  }

  return FUnrealMCPCommonUtils::CreateErrorResponse(
      TEXT("Failed to take screenshot"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCreateLandscape(
    const TSharedPtr<FJsonObject> &Params) {
  // Default parameters based on user request (8km x 8km)
  int32 SectionSize = 63;
  int32 SectionsPerComponent = 1;
  int32 ComponentsX = 64;
  int32 ComponentsY = 64;
  FVector Location(0.0f, 0.0f, 0.0f);
  FRotator Rotation(0.0f, 0.0f, 0.0f);
  FVector Scale(200.0f, 200.0f, 100.0f);

  // Override with JSON params
  Params->TryGetNumberField(TEXT("section_size"), SectionSize);
  Params->TryGetNumberField(TEXT("sections_per_component"),
                            SectionsPerComponent);
  Params->TryGetNumberField(TEXT("components_x"), ComponentsX);
  Params->TryGetNumberField(TEXT("components_y"), ComponentsY);

  if (Params->HasField(TEXT("location")))
    Location =
        FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
  if (Params->HasField(TEXT("rotation")))
    Rotation =
        FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
  if (Params->HasField(TEXT("scale")))
    Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));

  UWorld *World = GEditor->GetEditorWorldContext().World();
  if (!World) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Failed to get editor world"));
  }

  int32 QuadsPerSection = SectionSize;
  int32 QuadsPerComponent = QuadsPerSection * SectionsPerComponent;
  int32 SizeX = ComponentsX * QuadsPerComponent + 1;
  int32 SizeY = ComponentsY * QuadsPerComponent + 1;

  TArray<uint16> HeightData;
  HeightData.Init(32768, SizeX * SizeY);

  TMap<FGuid, TArray<uint16>> HeightmapDataPerLayers;
  HeightmapDataPerLayers.Add(FGuid(), HeightData);

  TMap<FGuid, TArray<FLandscapeImportLayerInfo>> ImportLayerInfosPerLayers;
  ImportLayerInfosPerLayers.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

  ALandscape *Landscape = World->SpawnActor<ALandscape>(
      ALandscape::StaticClass(), Location, Rotation);
  if (!Landscape) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Failed to spawn Landscape actor"));
  }

  Landscape->SetActorScale3D(Scale);
  Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1,
                    SectionsPerComponent, SectionSize, HeightmapDataPerLayers,
                    nullptr, ImportLayerInfosPerLayers,
                    ELandscapeImportAlphamapType::Additive);

  Landscape->CreateLandscapeInfo();

  TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
  ResultObj->SetBoolField(TEXT("success"), true);
  ResultObj->SetStringField(TEXT("name"), Landscape->GetName());
  return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetCurrentLevelName(
    const TSharedPtr<FJsonObject> &Params) {
  UWorld *World = GEditor->GetEditorWorldContext().World();
  if (!World) {
    return FUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Failed to get editor world"));
  }

  TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
  ResultObj->SetStringField(TEXT("level_name"), World->GetMapName());
  ResultObj->SetStringField(TEXT("full_path"),
                            World->GetOutermost()->GetName());
  ResultObj->SetBoolField(TEXT("success"), true);
  return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleRunPython(
    const TSharedPtr<FJsonObject> &Params) {
  FString ScriptPath;
  if (Params->TryGetStringField(TEXT("script_path"), ScriptPath)) {
    FString Cmd = FString::Printf(TEXT("py \"%s\""), *ScriptPath);
    if (GEditor) {
      GEditor->Exec(GEditor->GetEditorWorldContext().World(), *Cmd);
      TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
      ResultObj->SetBoolField(TEXT("success"), true);
      return ResultObj;
    }
  }
  return FUnrealMCPCommonUtils::CreateErrorResponse(
      TEXT("Missing script_path or GEditor is null"));
}