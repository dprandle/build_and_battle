#include <urho_common.h>

#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/CollisionShape.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Technique.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/AngelScript/Script.h>
#include <Urho3D/LuaScript/LuaScript.h>

#include <tile_occupier.h>
#include <hex_tile_grid.h>
#include <input_translator.h>
#include <mtdebug_print.h>
#include <input_translator.h>

#include "build_and_battle.h"


int main(int argc, char **argv)
{
    ParseArguments(argc, argv);
    Context *context(new Context());
    Build_And_Battle *bb(new Build_And_Battle(context));
    int ret = bb->run();
    delete bb;
    delete context;
    return ret;
}

Build_And_Battle::Build_And_Battle(Context *context)
    : Object(context),
      engine_(new Engine(context)),
      scene_(nullptr),
      cam_node_(nullptr),
      input_translator_(new Input_Translator(context)),
      input_map_(new Input_Map()),
      draw_debug_(false)
{
}

Build_And_Battle::~Build_And_Battle()
{
    //delete engine;
    //    delete input_translator;
    //    delete camera_controller;
    delete input_map_;
}

void Build_And_Battle::handle_post_render_update(StringHash event_type, VariantMap &event_data)
{
    if (draw_debug_)
    {
        if (scene_ != nullptr)
        {
            PhysicsWorld * pw = scene_->GetComponent<PhysicsWorld>();
            if (pw != nullptr)
                pw->DrawDebugGeometry(false);
            Octree * oc = scene_->GetComponent<Octree>();
            if (oc != nullptr)
                oc->DrawDebugGeometry(true);
            Hex_Tile_Grid * tg = scene_->GetComponent<Hex_Tile_Grid>();
            if (tg != nullptr)
                tg->DrawDebugGeometry(false);
        }
    }
}

void Build_And_Battle::handle_log_message(StringHash event_type, VariantMap & event_data)
{}

bool Build_And_Battle::init()
{
    VariantMap engine_params;
    engine_params[EP_FRAME_LIMITER] = false;
    String str = GetSubsystem<FileSystem>()->GetProgramDir() + "build_and_battle.log";
    engine_params[EP_LOG_NAME] = str;
    str = GetSubsystem<FileSystem>()->GetCurrentDir() + ";" + GetSubsystem<FileSystem>()->GetProgramDir();
    engine_params[EP_RESOURCE_PREFIX_PATHS] = str;
    engine_params[EP_FULL_SCREEN] = false;
    engine_params[EP_WINDOW_WIDTH] = 1920;
    engine_params[EP_WINDOW_HEIGHT] = 1080;
    engine_params[EP_HIGH_DPI] = true;
    engine_params[EP_WINDOW_RESIZABLE] = true;

    // Scene root components
    Hex_Tile_Grid::register_context(context_);

    // Per node components
    Tile_Occupier::register_context(context_);

    if (!engine_->Initialize(engine_params))
    {
        // Do some logging
        return false;
    }

    context_->RegisterSubsystem(new Script(context_));
    context_->RegisterSubsystem(new LuaScript(context_));

    input_translator_->init();

    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Build_And_Battle, handle_scene_update));
    SubscribeToEvent(E_INPUT_TRIGGER, URHO3D_HANDLER(Build_And_Battle, handle_input_event));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Build_And_Battle, handle_post_render_update));
    SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(Build_And_Battle, handle_log_message));

    create_visuals();
    return true;
}

void Build_And_Battle::handle_ui_event(StringHash eventType, VariantMap &eventData)
{
}

int Build_And_Battle::run()
{
    if (!init())
        return -1;

    while (!engine_->IsExiting())
        engine_->RunFrame();

    release();

    return 0;
}

void Build_And_Battle::release()
{
    input_translator_->release();

    context_->RemoveSubsystem<Input_Translator>();
}

void Build_And_Battle::create_visuals()
{

    Graphics * graphics = GetSubsystem<Graphics>();
    graphics->SetWindowTitle("Build and Battle");
    
    // Get default style
    ResourceCache * cache = GetSubsystem<ResourceCache>();
    XMLFile * xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    UI * usi = GetSubsystem<UI>();
    usi->SetScale(2.0);
    UIElement * root = usi->GetRoot();
    root->SetDefaultStyle(xmlFile);

    

    // Create console
    Console * console = engine_->CreateConsole();
    // Create debug HUD.
    DebugHud * debugHud = engine_->CreateDebugHud();
    //debugHud->SetDefaultStyle(xmlFile);

    Context * ctxt = GetContext();
    scene_ = new Scene(ctxt);
    scene_->CreateComponent<DebugRenderer>();
    scene_->CreateComponent<Octree>();
    PhysicsWorld * phys = scene_->CreateComponent<PhysicsWorld>();
    Hex_Tile_Grid * tg = scene_->CreateComponent<Hex_Tile_Grid>();
    phys->SetGravity(fvec3(0.0f, 0.0f, -9.81f));

    Node * editor_cam_node = new Node(ctxt);
    Camera * editor_cam = editor_cam_node->CreateComponent<Camera>();
    editor_cam_node->SetPosition(Vector3(8, -8, 5));
    editor_cam_node->SetDirection(Vector3(0, 0, -1));
    editor_cam_node->Pitch(-70.0f);

    Renderer * rnd = GetSubsystem<Renderer>();

    Viewport * vp = new Viewport(ctxt, scene_, editor_cam);
    vp->SetDrawDebug(true);
    rnd->SetViewport(0, vp);

    RenderPath * rp = new RenderPath;
    rp->Load(cache->GetResource<XMLFile>("RenderPaths/DeferredWithOutlines.xml"));
    vp->SetRenderPath(rp);

    Node * skyNode = scene_->CreateChild("Sky");
    skyNode->Rotate(Quaternion(90.0f, Vector3(1, 0, 0)));
    Skybox * skybox = skyNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a directional light
    Node * light_node = scene_->CreateChild("Dir_Light");
    light_node->SetDirection(Vector3(-0.0f, -0.5f, -1.0f));
    Light * light = light_node->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(1.0f, 1.0f, 1.0f));
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(1.0f);


    Technique * tech = cache->GetResource<Technique>("Techniques/DiffNormal.xml");
    Material * grass_tile = cache->GetResource<Material>("Materials/Tiles/Grass.xml");
    Model * mod = cache->GetResource<Model>("Models/Tiles/Grass.mdl");
    Input_Context * in_context = input_map_->create_context("global_context");

    input_translator_->push_context(in_context);
    setup_global_keys(in_context);

    int cnt = 0;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 20; ++x)
        {
            for (int z = 0; z < 2; ++z)
            {
                Node * tile_node = scene_->CreateChild("Grass_Tile");

                StaticModel * modc = tile_node->CreateComponent<StaticModel>();
                modc->SetModel(mod);
                modc->SetMaterial(grass_tile);

                CollisionShape * cs = tile_node->CreateComponent<CollisionShape>();
                const BoundingBox & bb = modc->GetBoundingBox();
                cs->SetBox(bb.Size());

                // RigidBody * rb = tile_node->CreateComponent<RigidBody>();
                // rb->SetMass(0.0f);
                // if (z == 1)
                //     rb->SetMass(10.0f);
                // rb->SetFriction(0.1f);
                // rb->SetRestitution(0.9f);

                Tile_Occupier * occ = tile_node->CreateComponent<Tile_Occupier>();
                tile_node->AddListener(occ);

                tile_node->SetPosition(Hex_Tile_Grid::grid_to_world(ivec3(x, y, z)));
                tile_node->SetRotation(Quaternion(90.0f, fvec3(1.0f, 0.0f, 0.0f)));

                ++cnt;
            }
        }
    }
}

void Build_And_Battle::setup_global_keys(Input_Context *ctxt)
{
    if (ctxt == nullptr)
        return;

    Input_Action_Trigger it;

    it.condition_.mouse_button_ = 0;
    it.mb_required_ = 0;
    it.mb_allowed_ = MOUSEB_ANY;
    it.qual_required_ = 0;
    it.qual_allowed_ = QUAL_ANY;
    it.condition_.key_ = KEY_F1;
    it.name_ = "ToggleConsole";
    it.trigger_state_ = T_BEGIN;
    ctxt->create_trigger(it);

    it.condition_.key_ = KEY_F2;
    it.name_ = "ToggleDebugHUD";
    ctxt->create_trigger(it);

    it.condition_.key_ = KEY_F9;
    it.name_ = "TakeScreenshot";
    ctxt->create_trigger(it);

    it.condition_.key_ = KEY_F3;
    it.name_ = "ToggleDebugGeometry";
    ctxt->create_trigger(it);

    it.condition_.key_ = KEY_DELETE;
    it.name_ = "DeleteSelection";
    ctxt->create_trigger(it);
}

void Build_And_Battle::handle_scene_update(StringHash /*eventType*/, VariantMap &event_data)
{
}

void Build_And_Battle::handle_input_event(StringHash event_type,
                                          VariantMap &event_data)
{
    StringHash name = event_data[Input_Trigger::P_TRIGGER_NAME].GetStringHash();
    int state = event_data[Input_Trigger::P_TRIGGER_STATE].GetInt();
    Vector2 norm_mpos = event_data[Input_Trigger::P_NORM_MPOS].GetVector2();
    Vector2 norm_mdelta = event_data[Input_Trigger::P_NORM_MDELTA].GetVector2();
    int wheel = event_data[Input_Trigger::P_MOUSE_WHEEL].GetInt();

    if (name == StringHash("ToggleDebugHUD"))
    {
        GetSubsystem<DebugHud>()->ToggleAll();
    }
    else if (name == StringHash("ToggleConsole"))
    {
        GetSubsystem<Console>()->Toggle();
    }
    else if (name == StringHash("ToggleDebugGeometry"))
    {
        draw_debug_ = !draw_debug_;
    }
    else if (name == StringHash("TakeScreenshot"))
    {
        Graphics * graphics = GetSubsystem<Graphics>();
        Image screenshot(context_);
        graphics->TakeScreenShot(screenshot);
        // Here we save in the Data folder with date and time appended
        screenshot.SavePNG(
            GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
            Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
    }
}