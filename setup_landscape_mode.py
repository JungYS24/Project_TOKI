import unreal

def log(msg):
    # Print with a distinct prefix for easy visibility in Output Log
    print(f"### [LANDSCAPE_SETUP] {msg}")

def setup_landscape():
    # ---------------------------------------------------------
    # Configuration
    # ---------------------------------------------------------
    # Camera
    CAM_LOC = unreal.Vector(-1100.0, 0.0, 1300.0)
    CAM_ROT = unreal.Rotator(-55.0, 0.0, 0.0)
    CAM_FOV = 45.0
    ASPECT_RATIO = 1.777
    
    # Floor
    FLOOR_SCALE = unreal.Vector(15.0, 25.0, 1.0)
    
    level_path = "/Game/Project_TOKI/01_Maps/Map_Main"
    ell = unreal.EditorLevelLibrary
    eal = unreal.EditorAssetLibrary

    # ---------------------------------------------------------
    # 1. Load Level
    # ---------------------------------------------------------
    if not eal.does_asset_exist(level_path):
        log(f"CRITICAL ERROR: Level not found at {level_path}")
        return

    # Force load (this might prompt to save if changes exist, usually loads cleanly in Editor)
    success = ell.load_level(level_path)
    if not success:
        log("Error: Failed to load level.")
        return
        
    world = ell.get_editor_world()
    
    # Clear selection to avoid transforming selected objects by accident
    ell.set_selected_level_actors([])

    # ---------------------------------------------------------
    # 2. Camera Setup (FixedCamera_Main)
    # ---------------------------------------------------------
    camera_label = "FixedCamera_Main"
    
    # Clean up ANY duplicate cameras with this name or old names to ensure single source of truth
    all_actors = ell.get_all_level_actors()
    existing_cameras = [a for a in all_actors if a.get_actor_label() == camera_label]
    
    camera_actor = None
    if len(existing_cameras) > 0:
        camera_actor = existing_cameras[0]
        # Destroy duplicates if any
        for i in range(1, len(existing_cameras)):
            ell.destroy_actor(existing_cameras[i])
            log("Removed duplicate FixedCamera_Main")
    else:
        # Spawn new
        camera_actor = ell.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0, 0, 0))
        camera_actor.set_actor_label(camera_label)
        log("Spawned NEW FixedCamera_Main.")
        
    # Apply Transform
    camera_actor.set_actor_location(CAM_LOC, False, False)
    camera_actor.set_actor_rotation(CAM_ROT, False)
    
    # Apply Camera Settings
    cam_comp = camera_actor.camera_component
    cam_comp.set_editor_property("aspect_ratio", ASPECT_RATIO)
    cam_comp.set_editor_property("constrain_aspect_ratio", True)
    cam_comp.set_editor_property("field_of_view", CAM_FOV)
    
    log(f"FixedCamera_Main set to Loc:{CAM_LOC} Rot:{CAM_ROT} FOV:{CAM_FOV}")

    # ---------------------------------------------------------
    # 3. Floor Setup (Floor_Guide)
    # ---------------------------------------------------------
    floor_label = "Floor_Guide"
    floor_actor = None
    
    # Aggressive search: Find "Battle_Ground", "Stage_Floor", "Floor_Guide"
    # To prevent duplicates: find ALL matching candidates, pick one, delete rest.
    candidates = []
    all_actors = ell.get_all_level_actors() # Refresh list
    for actor in all_actors:
        lbl = actor.get_actor_label()
        if lbl in ["Floor_Guide", "Battle_Ground", "Stage_Floor"]:
            candidates.append(actor)
            
    if len(candidates) > 0:
        floor_actor = candidates[0]
        floor_actor.set_actor_label(floor_label)
        # Destroy others
        for i in range(1, len(candidates)):
            ell.destroy_actor(candidates[i])
            log(f"Removed old floor actor: {candidates[i].get_actor_label()}")
    else:
        floor_actor = ell.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0,0,0))
        floor_actor.set_actor_label(floor_label)
        log("Spawned NEW Floor_Guide.")

    # Mesh & Material
    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube")
    if cube_mesh:
        floor_actor.static_mesh_component.set_static_mesh(cube_mesh)
        
    mat_path = "/Game/Project_TOKI/04_Art/Materials/M_DarkGrey"
    if eal.does_asset_exist(mat_path):
        dark_mat = unreal.load_asset(mat_path)
        floor_actor.static_mesh_component.set_material(0, dark_mat)

    # Transform
    floor_actor.set_actor_location(unreal.Vector(0,0,0), False, False)
    floor_actor.set_actor_scale3d(FLOOR_SCALE)
    
    log(f"Floor_Guide set to Scale {FLOOR_SCALE}")

    # ---------------------------------------------------------
    # 4. PostProcess
    # ---------------------------------------------------------
    pp_vol = None
    pps = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PostProcessVolume)
    if len(pps) > 0:
        pp_vol = pps[0]
    else:
        pp_vol = ell.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0,0,0))
        pp_vol.set_actor_label("PostProcessVolume")
        
    pp_vol.set_editor_property("unbound", True)
    pp_settings = pp_vol.settings
    pp_settings.bloom_intensity = 1.5
    pp_settings.override_bloom_intensity = True
    pp_vol.settings = pp_settings
    
    log("PostProcess Bloom updated.")

    # ---------------------------------------------------------
    # Save
    # ---------------------------------------------------------
    ell.save_current_level()
    log("Level Saved Successfully.")
    
    print(">>> 가로 모드(16:9) 카메라 및 전장 세팅 완료")

if __name__ == "__main__":
    setup_landscape()
