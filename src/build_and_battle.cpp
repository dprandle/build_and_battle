//#include <Urho3D/Urho3DAll.h>
#include <Urho3D/Input/InputEvents.h>
#include <mtdebug_print.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/UIEvents.h>

#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/CollisionShape.h>

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/EngineDefs.h>

#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Physics/PhysicsEvents.h>

#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModelGroup.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Technique.h>

#include <tile_occupier.h>
#include <hex_tile_grid.h>
#include <editor_selector.h>
#include <editor_selection_controller.h>
#include <camera_controller.h>
#include <input_translator.h>
#include <build_and_battle.h>

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
      engine(new Engine(context)),
      scene(nullptr),
      cam_node(nullptr),
      input_translator(new InputTranslator(context)),
      camera_controller(new EditorCameraController(context)),
      input_map(new InputMap()),
      m_draw_debug(false)
{
}

Build_And_Battle::~Build_And_Battle()
{
    //delete engine;
    //    delete input_translator;
    //    delete camera_controller;
    delete input_map;
}

void Build_And_Battle::handle_post_render_update(StringHash event_type, VariantMap &event_data)
{
    if (m_draw_debug)
    {
        if (scene != nullptr)
        {
            PhysicsWorld *pw = scene->GetComponent<PhysicsWorld>();
            if (pw != nullptr)
                pw->DrawDebugGeometry(false);
            Octree *oc = scene->GetComponent<Octree>();
            if (oc != nullptr)
                oc->DrawDebugGeometry(true);
            EditorSelectionController *esc = scene->GetComponent<EditorSelectionController>();
            if (esc != nullptr)
                esc->DrawDebugGeometry(true);
            Hex_Tile_Grid *tg = scene->GetComponent<Hex_Tile_Grid>();
            if (tg != nullptr)
                tg->DrawDebugGeometry(false);
        }
    }
}

bool Build_And_Battle::init()
{
    VariantMap params;
    params[EP_WINDOW_TITLE] = GetTypeName();
    params[EP_LOG_NAME] = GetTypeName() + ".log";
    params[EP_FULL_SCREEN] = false;
    params[EP_WINDOW_RESIZABLE] = true;
    params[EP_SOUND] = true;
    params[EP_WINDOW_WIDTH] = 1920;
    params[EP_WINDOW_HEIGHT] = 1080;
    params[EP_HIGH_DPI] = true;

    // Register custom systems
    context_->RegisterSubsystem(input_translator);
    context_->RegisterSubsystem(camera_controller);

    // Scene root components
    EditorSelectionController::register_context(context_);
    Hex_Tile_Grid::register_context(context_);

    // Per node components
    Tile_Occupier::register_context(context_);
    EditorSelector::register_context(context_);

    if (!engine->Initialize(params))
    {
        // Do some logging
        return false;
    }

    input_translator->init();
    camera_controller->init();

    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Build_And_Battle, handle_scene_update));
    SubscribeToEvent(E_INPUT_TRIGGER, URHO3D_HANDLER(Build_And_Battle, handle_input_event));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Build_And_Battle, handle_post_render_update));
    SubscribeToEvent(E_CLICK, URHO3D_HANDLER(Build_And_Battle, handle_ui_event));

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
    Graphics *graphics = GetSubsystem<Graphics>();
    graphics->SetWindowTitle("Build and Battle");

    // Get default style
    ResourceCache *cache = GetSubsystem<ResourceCache>();
    XMLFile *xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Create console
    Console *console = engine->CreateConsole();
    console->SetDefaultStyle(xmlFile);
    console->GetBackground()->SetOpacity(0.8f);
    console->GetBackground()->SetOpacity(0.8f);

    auto ui = GetSubsystem<UI>();
    ui->SetScale(2.0);

    // Create debug HUD.
    DebugHud *debugHud = engine->CreateDebugHud();
    debugHud->SetDefaultStyle(xmlFile);

    UIElement *root = ui->GetRoot();
    Button *btn = new Button(context_);    
    btn->SetName("Button1");
    btn->SetSize(IntVector2(1920, 1080));
    btn->SetColor(Color(0.7f, 0.6f, 0.1f, 0.4f));
    root->AddChild(btn);

    //Button *
    Context *ctxt = GetContext();
    scene = new Scene(ctxt);
    scene->CreateComponent<DebugRenderer>();
    scene->CreateComponent<Octree>();
    PhysicsWorld *phys = scene->CreateComponent<PhysicsWorld>();
    Hex_Tile_Grid *tg = scene->CreateComponent<Hex_Tile_Grid>();
    EditorSelectionController *editor_selection = scene->CreateComponent<EditorSelectionController>();
    phys->SetGravity(fvec3(0.0f, 0.0f, -9.81f));

    Node *editor_cam_node = new Node(ctxt);
    Camera *editor_cam = editor_cam_node->CreateComponent<Camera>();
    editor_cam_node->SetPosition(Vector3(8, -8, 5));
    editor_cam_node->SetDirection(Vector3(0, 0, -1));
    editor_cam_node->Pitch(-70.0f);

    //editor_cam_node->Rot
    camera_controller->set_camera(editor_cam);
    editor_selection->set_camera(editor_cam);

    Renderer *rnd = GetSubsystem<Renderer>();

    Viewport *vp = new Viewport(ctxt, scene, editor_cam);
    vp->SetDrawDebug(true);

    rnd->SetViewport(0, vp);
    RenderPath *rp = new RenderPath;
    rp->Load(cache->GetResource<XMLFile>("RenderPaths/DeferredWithOutlines.xml"));
    vp->SetRenderPath(rp);

    Node *skyNode = scene->CreateChild("Sky");
    skyNode->Rotate(Quaternion(90.0f, Vector3(1, 0, 0)));
    Skybox *skybox = skyNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a directional light
    Node *light_node = scene->CreateChild("Dir_Light");
    light_node->SetDirection(Vector3(-0.0f, -0.5f, -1.0f));
    Light *light = light_node->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(1.0f, 1.0f, 1.0f));
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(1.0f);

    Technique *tech = cache->GetResource<Technique>("Techniques/DiffNormal.xml");
    Technique *tech_outline = cache->GetResource<Technique>("Techniques/DiffNormalOutline.xml");

    // Create StaticModelGroups in the scene
    StaticModelGroup *lastGroup = nullptr;
    Material *grass_tile = cache->GetResource<Material>("Materials/Tiles/Grass.xml");
    Material *grass_tile_selected = cache->GetResource<Material>("Materials/Tiles/GrassSelected.xml");
    grass_tile_selected->SetShaderParameter("OutlineEnable", true);

    //<parameter name="OutlineWidth" value="0.01" />
    //<parameter name="OutlineColor" value="1 1 0 1" />
    Model *mod = cache->GetResource<Model>("Models/Tiles/Grass.mdl");

    input_context *in_context = input_map->create_context("global_context");
    camera_controller->setup_input_context(in_context);
    editor_selection->setup_input_context(in_context);
    input_translator->push_context(in_context);
    setup_global_keys(in_context);

    int cnt = 0;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 20; ++x)
        {
            for (int z = 0; z < 2; ++z)
            {

                Node *tile_node = scene->CreateChild("Grass_Tile_" + String(cnt));

                StaticModel *modc = tile_node->CreateComponent<StaticModel>();
                modc->SetModel(mod);
                modc->SetMaterial(grass_tile);

                EditorSelector *sel = tile_node->CreateComponent<EditorSelector>();
                sel->set_selection_material("Materials/Tiles/GrassSelected.xml");
                sel->set_selected(tile_node, false);

                CollisionShape *cs = tile_node->CreateComponent<CollisionShape>();
                const BoundingBox &bb = modc->GetBoundingBox();
                cs->SetBox(bb.Size());

                RigidBody *rb = tile_node->CreateComponent<RigidBody>();
                rb->SetMass(0.0f);
                // if (z == 1)
                //     rb->SetMass(10.0f);
                // rb->SetFriction(0.1f);
                // rb->SetRestitution(0.9f);

                Tile_Occupier *occ = tile_node->CreateComponent<Tile_Occupier>();
                tile_node->AddListener(occ);

                tile_node->SetPosition(Hex_Tile_Grid::grid_to_world(ivec3(x, y, z * 30.0f)));
                tile_node->SetRotation(Quaternion(90.0f, fvec3(1.0f, 0.0f, 0.0f)));

                //lastGroup->AddInstanceNode(tile_node);
                ++cnt;
            }
        }
    }
}

void Build_And_Battle::setup_global_keys(input_context *ctxt)
{
    input_context *input_ctxt = input_map->get_context(StringHash("global_context"));

    if (ctxt == nullptr)
        return;

    input_action_trigger it;

    it.condition.key = KEY_ESCAPE;
    it.condition.mouse_button = 0;
    it.name = "CloseWindow";
    it.trigger_state = t_end;
    it.mb_required = 0;
    it.mb_allowed = MOUSEB_ANY;
    it.qual_required = 0;
    it.qual_allowed = QUAL_ANY;
    ctxt->create_trigger(it);

    it.condition.key = KEY_F1;
    it.name = "ToggleConsole";
    it.trigger_state = t_begin;
    ctxt->create_trigger(it);

    it.condition.key = KEY_F2;
    it.name = "ToggleDebugHUD";
    ctxt->create_trigger(it);

    it.condition.key = KEY_F9;
    it.name = "TakeScreenshot";
    ctxt->create_trigger(it);

    it.condition.key = KEY_F3;
    it.name = "ToggleDebugGeometry";
    ctxt->create_trigger(it);
}

void Build_And_Battle::handle_scene_update(StringHash /*eventType*/, VariantMap &event_data)
{
}

void Build_And_Battle::handle_input_event(StringHash event_type,
                                          VariantMap &event_data)
{
    StringHash name = event_data[InputTrigger::P_TRIGGER_NAME].GetStringHash();
    int state = event_data[InputTrigger::P_TRIGGER_STATE].GetInt();
    Vector2 norm_mpos = event_data[InputTrigger::P_NORM_MPOS].GetVector2();
    Vector2 norm_mdelta = event_data[InputTrigger::P_NORM_MDELTA].GetVector2();
    int wheel = event_data[InputTrigger::P_MOUSE_WHEEL].GetInt();

    if (name == StringHash("CloseWindow"))
    {
        Console *console = GetSubsystem<Console>();
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
    else if (name == StringHash("ToggleDebugGeometry"))
    {
        m_draw_debug = !m_draw_debug;
    }
    else if (name == StringHash("TakeScreenshot"))
    {
        Graphics *graphics = GetSubsystem<Graphics>();
        Image screenshot(context_);
        graphics->TakeScreenShot(screenshot);
        // Here we save in the Data folder with date and time appended
        screenshot.SavePNG(
            GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
            Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
    }
}