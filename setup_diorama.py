import unreal

def log(msg):
    print(f"### [DIORAMA] {msg}")

def setup_diorama():
    level_path = "/Game/Project_TOKI/01_Maps/Map_Main"
    eal = unreal.EditorAssetLibrary
    ell = unreal.EditorLevelLibrary
    
    if not eal.does_asset_exist(level_path):
        log("Error: Map_Main not found.")
        return

    ell.load_level(level_path)
    world = ell.get_editor_world()
    
    # 1. Floor
    floor_label = "Stage_Floor"
    # Clean old
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
        if actor.get_actor_label().startswith("Stage_"):
            ell.destroy_actor(actor)

    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube")
    
    floor = ell.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0,0,0))
    floor.set_actor_label(floor_label)
    floor.static_mesh_component.set_static_mesh(cube_mesh)
    floor.set_actor_scale3d(unreal.Vector(20.0, 12.0, 1.0))
    
    # Material for Floor (Dark Grey)
    mat_path = "/Game/Project_TOKI/04_Art/Materials/M_DarkGrey"
    if not eal.does_asset_exist(mat_path):
        base_mat = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")
        at = unreal.AssetToolsHelpers.get_asset_tools()
        # Ensure folder exists
        if not eal.does_directory_exist("/Game/Project_TOKI/04_Art/Materials"):
            eal.make_directory("/Game/Project_TOKI/04_Art/Materials")
        
        # Create Material Instance
        factory = unreal.MaterialInstanceConstantFactoryNew()
        mi = at.create_asset("M_DarkGrey", "/Game/Project_TOKI/04_Art/Materials", unreal.MaterialInstanceConstant, factory)
        mi.set_editor_property("parent", base_mat)
        
        # Set Color (Note: BasicShapeMaterial uses 'Color' parameter usually)
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(0.2, 0.2, 0.2, 1.0))
        eal.save_asset(mat_path)
    
    dark_mat = unreal.load_asset(mat_path)
    floor.static_mesh_component.set_material(0, dark_mat)
    log("Floor placed and material applied.")

    # 2. Walls (N, S, E, W)
    # Floor is 20m x 12m (Scale 20x12). 1 unit scale = 100 units (1m).
    # Bounds: X from -1000 to 1000, Y from -600 to 600.
    wall_height_scale = 0.5
    wall_thickness = 0.2
    
    walls = [
        ("Stage_Wall_N", unreal.Vector(1000, 0, 25), unreal.Vector(wall_thickness, 12.0, wall_height_scale)),
        ("Stage_Wall_S", unreal.Vector(-1000, 0, 25), unreal.Vector(wall_thickness, 12.0, wall_height_scale)),
        ("Stage_Wall_E", unreal.Vector(0, 600, 25), unreal.Vector(20.0, wall_thickness, wall_height_scale)),
        ("Stage_Wall_W", unreal.Vector(0, -600, 25), unreal.Vector(20.0, wall_thickness, wall_height_scale)),
    ]
    
    for name, loc, scale in walls:
        w = ell.spawn_actor_from_class(unreal.StaticMeshActor, loc)
        w.set_actor_label(name)
        w.static_mesh_component.set_static_mesh(cube_mesh)
        w.set_actor_scale3d(scale)
    log("Boundary walls placed.")

    # 3. Atmosphere (ExponentialHeightFog)
    # Clean old fog
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExponentialHeightFog):
        ell.destroy_actor(actor)
        
    fog_actor = ell.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0,0,0))
    fog_comp = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_comp.set_editor_property("fog_density", 0.05)
    # Purple: (0.3, 0.1, 0.5)
    fog_comp.set_editor_property("fog_inscattering_color", unreal.LinearColor(0.3, 0.1, 0.5, 1.0))
    log("Purple fog added.")

    eal.save_asset(level_path)
    ell.save_current_level()
    log("Diorama Setup Successful.")

if __name__ == "__main__":
    setup_diorama()
