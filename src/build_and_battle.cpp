#include <Urho3D/Core/Timer.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/Urho3DAll.h>

#include <camera_controller.h>
#include <input_translator.h>
#include <build_and_battle.h>

using namespace Urho3D;

int main(int argc, char** argv)
{
    Urho3D::ParseArguments(argc, argv);
    Urho3D::Context * context(new Urho3D::Context());
    Build_And_Battle * bb(new Build_And_Battle(context));
    int ret = bb->run();
    delete bb;
    delete context;
    return ret;
}

Build_And_Battle::Build_And_Battle(Context* context)
    : Object(context)
    , engine(new Engine(context))
    , scene(nullptr)
    , cam_node(nullptr)
    , input_translator(new InputTranslator(context))
    , camera_controller(new EditorCameraController(context))
    , input_map(new InputMap())
{}

Build_And_Battle::~Build_And_Battle()
{
    //delete engine;
//    delete input_translator;
//    delete camera_controller;
    delete input_map;
}

bool Build_And_Battle::init()
{
    VariantMap params;

    GetSubsystem<FileSystem>()->SetCurrentDir(GetSubsystem<FileSystem>()->GetProgramDir());
    params[EP_WINDOW_TITLE] = GetTypeName();
    params[EP_LOG_NAME] = GetTypeName() + ".log";
    params[EP_FULL_SCREEN] = false;
    params[EP_WINDOW_RESIZABLE] = true;
    params[EP_SOUND] = true;

    // Register custom systems
    context_->RegisterSubsystem(input_translator);
    context_->RegisterSubsystem(camera_controller);
    

    if (!engine->Initialize(params))
    {
        // Do some logging
        return false;
    }

    input_translator->init();
    camera_controller->init();


    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Build_And_Battle, handle_scene_update));
	SubscribeToEvent(E_INPUT_TRIGGER, URHO3D_HANDLER(Build_And_Battle, handle_input_event));

    create_visuals();

    input_context* in_context = input_map->create_context("global_context");
    camera_controller->setup_input_context(in_context);
    input_translator->push_context(in_context);

    setup_global_keys(in_context);

    return true;
}

int Build_And_Battle::run()
{
    if (!init())
        return -1;

    while (!engine->IsExiting())
        engine->RunFrame();
    
    release();

    return 0;
}

void Build_And_Battle::release()    
{
    input_translator->release();
    camera_controller->release();

    context_->RemoveSubsystem<EditorCameraController>();
    context_->RemoveSubsystem<InputTranslator>();
}

void Build_And_Battle::create_visuals()
{
    Graphics* graphics = GetSubsystem<Graphics>();
    graphics->SetWindowTitle("Build and Battle");

    // Get default style
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Create console
    Console* console = engine->CreateConsole();
    console->SetDefaultStyle(xmlFile);
    console->GetBackground()->SetOpacity(0.8f);

    // Create debug HUD.
    DebugHud* debugHud = engine->CreateDebugHud();
    debugHud->SetDefaultStyle(xmlFile);

    UI * usi = GetSubsystem<UI>();
    UIElement * root = usi->GetRoot();
    Button * btn = new Button(context_);
    btn->SetName("Button1");
    root->AddChild(btn);
    btn->SetColor(Color(0.7f,0.6f,0.1f,1.0f));

    //Button *
    Context * ctxt = GetContext();
    Scene * scene = new Scene(ctxt);
    scene->CreateComponent<Octree>();
    scene->CreateComponent<DebugRenderer>();

    Node * editor_cam_node = new Node(ctxt);
    Camera* editor_cam = editor_cam_node->CreateComponent<Camera>();
    editor_cam_node->SetPosition(Vector3(0, 0, 5));
    editor_cam_node->SetDirection(Vector3(0, 0, -1));
    camera_controller->set_camera(editor_cam);

    Renderer* rnd = GetSubsystem<Renderer>();
    Viewport* vp = new Viewport(ctxt, scene, editor_cam);

    rnd->SetViewport(0, vp);
    RenderPath* rp = new RenderPath;
    rp->Load(cache->GetResource<XMLFile>("RenderPaths/Deferred.xml"));
    vp->SetRenderPath(rp);

    Node* skyNode = scene->CreateChild("Sky");
    skyNode->Rotate(Quaternion(90.0f, Vector3(1, 0, 0)));
    Skybox* skybox = skyNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a directional light
    Node* light_node = scene->CreateChild("Dir_Light");
    light_node->SetDirection(Vector3(-0.0f, -0.5f, -1.0f));
    Light* light = light_node->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(1.0f, 1.0f, 1.0f));
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(1.0f);

    // Create StaticModelGroups in the scene
    StaticModelGroup* lastGroup = nullptr;
    Material* grass_tile = cache->GetResource<Material>("Materials/Tiles/Grass.xml");
    // Texture2D * diff_grass =
    // cache->GetResource<Texture2D>("Textures/Tiles/diffuseGrass.png");
    // Texture2D * norm_grass =
    // cache->GetResource<Texture2D>("Textures/Tiles/normalGrass.png");
    // grass_tile->SetTexture(Urho3D::TU_DIFFUSE, diff_grass);
    // grass_tile->SetTexture(Urho3D::TU_NORMAL, norm_grass);
    // grass_tile->SetTechnique(0,
    // cache->GetResource<Technique>("Techniques/DiffNormal.xml"));

    for (int y = -50; y < 50; ++y) {
        for (int x = -50; x < 50; ++x) {
            if (!lastGroup || lastGroup->GetNumInstanceNodes() >= 25 * 25) {
                Node* tile_group_node = scene->CreateChild("Grass_Tile_Group");
                lastGroup = tile_group_node->CreateComponent<StaticModelGroup>();
                lastGroup->SetModel(
                    cache->GetResource<Model>("Models/Tiles/Grass.mdl"));
                lastGroup->SetMaterial(grass_tile);
            }

            Node* tile_node = scene->CreateChild("Grass_Tile");
            tile_node->SetPosition(Vector3(x, y, 0));
            tile_node->SetScale(0.5f);
            tile_node->SetRotation(Quaternion(90.0f, Vector3(1.0f, 0.0f, 0.0f)));
            lastGroup->AddInstanceNode(tile_node);
        }
    }
}

void Build_And_Battle::setup_global_keys(input_context * ctxt)
{
    input_context* input_ctxt = input_map->get_context(StringHash("global_context"));

    if (ctxt == nullptr)
        return;
    
    trigger_condition tc;
    input_action_trigger * it = nullptr;

	tc.key = KEY_ESCAPE;
	tc.mouse_button = 0;

	it = ctxt->create_trigger(tc);
 	it->name = "CloseWindow";
	it->trigger_state = t_end;
	it->mb_required = 0;
	it->mb_allowed = MOUSEB_ANY;
	it->qual_required = 0;
	it->qual_allowed = QUAL_ANY;

	tc.key = KEY_F1;
	tc.mouse_button = 0;

	it = ctxt->create_trigger(tc);
 	it->name = "ToggleConsole";
	it->trigger_state = t_begin;
	it->mb_required = 0;
	it->mb_allowed = MOUSEB_ANY;
	it->qual_required = 0;
	it->qual_allowed = QUAL_ANY;

	tc.key = KEY_F2;
	tc.mouse_button = 0;

	it = ctxt->create_trigger(tc);
 	it->name = "ToggleDebugHUD";
	it->trigger_state = t_begin;
	it->mb_required = 0;
	it->mb_allowed = MOUSEB_ANY;
	it->qual_required = 0;
	it->qual_allowed = QUAL_ANY;


	tc.key = KEY_F9;
	tc.mouse_button = 0;

	it = ctxt->create_trigger(tc);
 	it->name = "TakeScreenshot";
	it->trigger_state = t_begin;
	it->mb_required = 0;
	it->mb_allowed = MOUSEB_ANY;
	it->qual_required = 0;
	it->qual_allowed = QUAL_ANY;

}

void Build_And_Battle::handle_scene_update(StringHash /*eventType*/,
    VariantMap& event_data)
{
}

void Build_And_Battle::handle_input_event(Urho3D::StringHash event_type, Urho3D::VariantMap& event_data)
{
    StringHash name = event_data[InputTrigger::P_TRIGGER_NAME].GetStringHash();
    int state = event_data[InputTrigger::P_TRIGGER_STATE].GetInt();
    Vector2 norm_mpos = event_data[InputTrigger::P_NORM_MPOS].GetVector2();
    Vector2 norm_mdelta = event_data[InputTrigger::P_NORM_MDELTA].GetVector2();
    int wheel = event_data[InputTrigger::P_MOUSE_WHEEL].GetInt();

    if (name == StringHash("CloseWindow"))
    {
        Console* console = GetSubsystem<Console>();
        if (console->IsVisible())
            console->SetVisible(false);
        else
            engine->Exit();
    }
    else if (name == StringHash("ToggleDebugHUD"))
    {
        GetSubsystem<DebugHud>()->ToggleAll();
    }
    else if (name == StringHash("ToggleConsole"))
    {
        GetSubsystem<Console>()->Toggle();
    }
    else if (name == StringHash("TakeScreenshot"))
    {
        Graphics* graphics = GetSubsystem<Graphics>();
        Image screenshot(context_);
        graphics->TakeScreenShot(screenshot);
        // Here we save in the Data folder with date and time appended
        screenshot.SavePNG(
            GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" + Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
    }
}