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
#include <Urho3D/UI/View3D.h>
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

#include "bbtoolkit.h"
#include "selector.h"
#include "selection_controller.h"
#include "camera_controller.h"
#include "selection_controller.h"
#include "camera_controller.h"

int main(int argc, char **argv)
{
    ParseArguments(argc, argv);
    Context *context(new Context());
    bbtk::BBToolkit *bb(new bbtk::BBToolkit(context));
    int ret = bb->run();
    delete bb;
    delete context;
    return ret;
}

namespace bbtk
{

    BBToolkit::BBToolkit(Context *context)
        : Object(context),
          engine_(new Engine(context)),
          scene_(nullptr),
          cam_node_(nullptr),
          input_translator_(new Input_Translator(context)),
          camera_controller_(new bbtk::Camera_Controller(context)),
          input_map_(new Input_Map()),
          draw_debug_(false)
    {
    }

    BBToolkit::~BBToolkit()
    {
        //delete engine;
        //    delete input_translator;
        //    delete camera_controller;
        delete input_map_;
    }

    void BBToolkit::handle_post_render_update(StringHash event_type, VariantMap &event_data)
    {
        if (draw_debug_)
        {
            if (scene_ != nullptr)
            {
                PhysicsWorld *pw = scene_->GetComponent<PhysicsWorld>();
                if (pw != nullptr)
                    pw->DrawDebugGeometry(false);
                Octree *oc = scene_->GetComponent<Octree>();
                if (oc != nullptr)
                    oc->DrawDebugGeometry(true);
                Selection_Controller *esc = scene_->GetComponent<Selection_Controller>();
                if (esc != nullptr)
                    esc->DrawDebugGeometry(true);
                Hex_Tile_Grid *tg = scene_->GetComponent<Hex_Tile_Grid>();
                if (tg != nullptr)
                    tg->DrawDebugGeometry(false);
            }
        }
    }

    void BBToolkit::handle_log_message(StringHash event_type, VariantMap &event_data)
    {
        // String msg = event_data[LogMessage::P_MESSAGE].GetString();
        // int state = event_data[LogMessage::P_LEVEL].GetInt();
        // String color;
        // switch(state)
        // {
        //     case(LOG_TRACE):
        //         color = "blue";
        //         break;
        //     case(LOG_WARNING):
        //         color = "yellow";
        //         break;
        //     case(LOG_ERROR):
        //         color = "red";
        //         break;
        //     case(LOG_DEBUG):
        //         color = "white";
        //         break;
        //     case(LOG_INFO):
        //         color = "green";
        //         break;
        // }
        // QString formatted_str("<font color=" + color + ">" + msg.CString() + "</font>");
        //bbtk.ui->console->append(formatted_str);
    }

    bool BBToolkit::init()
    {
        VariantMap engine_params;
        engine_params[EP_FRAME_LIMITER] = false;
        String str = GetSubsystem<FileSystem>()->GetProgramDir() + "bbtoolkit.log";
        engine_params[EP_LOG_NAME] = str;
        str = GetSubsystem<FileSystem>()->GetCurrentDir() + ";" + GetSubsystem<FileSystem>()->GetProgramDir();
        engine_params[EP_RESOURCE_PREFIX_PATHS] = str;
        engine_params[EP_FULL_SCREEN] = false;
        engine_params[EP_WINDOW_WIDTH] = 1920 / 2;
        engine_params[EP_WINDOW_HEIGHT] = 1080 / 2;
        engine_params[EP_HIGH_DPI] = true;
        engine_params[EP_WINDOW_RESIZABLE] = true;

        // Register custom systems
        context_->RegisterSubsystem(camera_controller_);
        context_->RegisterSubsystem(input_translator_);

        // Scene root components
        Selection_Controller::register_context(context_);
        Hex_Tile_Grid::register_context(context_);

        // Per node components
        Tile_Occupier::register_context(context_);
        Selector::register_context(context_);

        if (!engine_->Initialize(engine_params))
        {
            // Do some logging
            return false;
        }

        context_->RegisterSubsystem(new Script(context_));
        context_->RegisterSubsystem(new LuaScript(context_));

        input_translator_->init();
        camera_controller_->init();

        SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(BBToolkit, handle_scene_update));
        SubscribeToEvent(E_INPUT_TRIGGER, URHO3D_HANDLER(BBToolkit, handle_input_event));
        SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(BBToolkit, handle_post_render_update));
        SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(BBToolkit, handle_log_message));

        GetSubsystem<Renderer>()->SetThreadedOcclusion(true);

        create_visuals();
        return true;
    }

    void BBToolkit::handle_ui_event(StringHash eventType, VariantMap &eventData)
    {
    }

    int BBToolkit::run()
    {
        if (!init())
            return -1;

        while (!engine_->IsExiting())
            engine_->RunFrame();

        release();
        return 0;
    }

    void BBToolkit::release()
    {
        input_translator_->release();
        camera_controller_->release();

        context_->RemoveSubsystem<Camera_Controller>();
        context_->RemoveSubsystem<Input_Translator>();
    }

    void BBToolkit::create_visuals()
    {

        Graphics *graphics = GetSubsystem<Graphics>();
        graphics->SetWindowTitle("BBToolkit");

        // Get default style
        ResourceCache *cache = GetSubsystem<ResourceCache>();
        XMLFile *xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
        UI *usi = GetSubsystem<UI>();
        //usi->SetScale(2.0);
        UIElement *root = usi->GetRoot();
        root->SetDefaultStyle(xmlFile);

        // Create console
        Console *console = engine_->CreateConsole();
        console->SetDefaultStyle(xmlFile);

        // Create debug HUD.
        DebugHud *debugHud = engine_->CreateDebugHud();
        debugHud->SetDefaultStyle(xmlFile);

        // Button * btn = new Button(context_);

        // btn->SetName("Button1");
        // btn->SetSize(IntVector2(50, 10));
        // btn->SetPosition(root->GetWidth()/2-25,root->GetHeight()/2-10);
        // //btn->SetColor(Color(0.7f, 0.6f, 0.1f, 0.4f));
        // btn->SetBorder(IntRect(2,4,6,8));

        // root->AddChild(btn);

        //Button *

        scene_ = new Scene(context_);
        scene_->CreateComponent<DebugRenderer>();
        scene_->CreateComponent<Octree>();
        PhysicsWorld *phys = scene_->CreateComponent<PhysicsWorld>();
        Hex_Tile_Grid *tg = scene_->CreateComponent<Hex_Tile_Grid>();
        Selection_Controller *editor_selection = scene_->CreateComponent<Selection_Controller>();
        phys->SetGravity(fvec3(0.0f, 0.0f, -9.81f));

        Node *editor_cam_node = new Node(context_);
        Camera *editor_cam = editor_cam_node->CreateComponent<Camera>();
        editor_cam_node->SetPosition(Vector3(8, -8, 5));
        editor_cam_node->SetDirection(Vector3(0, 0, -1));
        editor_cam_node->Pitch(-70.0f);

        View3D *main_view = new View3D(context_);
        main_view->SetView(scene_, editor_cam, true);
        Viewport *vp = main_view->GetViewport();
        RenderPath *rp = new RenderPath;
        rp->Load(cache->GetResource<XMLFile>("RenderPaths/DeferredWithOutlines.xml"));
        vp->SetRenderPath(rp);
        root->AddChild(main_view);
        main_view->SetEnableAnchor(true);
        main_view->SetMinAnchor(fvec2(0.25f, 0.25f));
        main_view->SetMaxAnchor(fvec2(0.75f, 0.75f));
        Viewport_Data vpd;
        vpd.view_element = main_view;
        vpd.vp = vp;
        input_translator_->push_viewport(vpd);

        editor_selection->create_selection_rect(main_view);

        // Set up rendering stuff
        Renderer *rnd = GetSubsystem<Renderer>();
        //Scene * dummy_scene = new Scene(context_);
        //dummy_scene->CreateComponent<Octree>();
        //Node * dummy_cam_node = new Node(context_);
        //Camera * dummy_cam = dummy_cam_node->CreateComponent<Camera>(Urho3D::LOCAL);
        Viewport *main_vp = new Viewport(context_, nullptr, nullptr); //dummy_scene, dummy_cam);
        rnd->SetViewport(0, main_vp);

        RenderPath *rp_main = new RenderPath;
        rp_main->Load(cache->GetResource<XMLFile>("RenderPaths/ToolkitMain.xml"));
        main_vp->SetRenderPath(rp_main);

        camera_controller_->add_viewport(0);
        editor_selection->set_viewport(vp);
        camera_controller_->set_camera(editor_cam);
        editor_selection->set_camera(editor_cam);

        Node *skyNode = scene_->CreateChild("Sky");
        skyNode->Rotate(Quaternion(90.0f, Vector3(1, 0, 0)));
        Skybox *skybox = skyNode->CreateComponent<Skybox>();
        skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

        // Create a directional light
        Node *light_node = scene_->CreateChild("Dir_Light");
        light_node->SetDirection(Vector3(-0.0f, -0.5f, -1.0f));
        Light *light = light_node->CreateComponent<Light>();
        light->SetLightType(LIGHT_DIRECTIONAL);
        light->SetColor(Color(1.0f, 1.0f, 1.0f));
        light->SetSpecularIntensity(5.0f);
        light->SetBrightness(1.0f);

        Technique *tech = cache->GetResource<Technique>("Techniques/DiffNormal.xml");
        Technique *tech_outline = cache->GetResource<Technique>("Techniques/DiffNormalOutline.xml");

        Material *grass_tile = cache->GetResource<Material>("Materials/Tiles/Grass.xml");
        Material *grass_tile_selected =
            cache->GetResource<Material>("Materials/Tiles/GrassSelected.xml");
        grass_tile_selected->SetShaderParameter("OutlineEnable", true);

        //<parameter name="OutlineWidth" value="0.01" />
        //<parameter name="OutlineColor" value="1 1 0 1" />
        Model *mod = cache->GetResource<Model>("Models/Tiles/Grass.mdl");

        Input_Context *in_context = input_map_->create_context("global_context");
        camera_controller_->setup_input_context(in_context);
        editor_selection->setup_input_context(in_context);
        input_translator_->push_context(in_context);
        setup_global_keys(in_context);

        int cnt = 0;
        for (int y = 0; y < 20; ++y)
        {
            for (int x = 0; x < 20; ++x)
            {
                for (int z = 0; z < 2; ++z)
                {
                    Node *tile_node = scene_->CreateChild("Grass_Tile");

                    StaticModel *modc = tile_node->CreateComponent<StaticModel>();
                    modc->SetModel(mod);
                    modc->SetMaterial(grass_tile);

                    Selector *sel = tile_node->CreateComponent<Selector>();
                    sel->SetModel(mod);
                    sel->SetMaterial(grass_tile_selected);
                    sel->set_selected(false);

                    CollisionShape *cs = tile_node->CreateComponent<CollisionShape>();
                    const BoundingBox &bb = modc->GetBoundingBox();
                    cs->SetBox(bb.Size());

                    // RigidBody * rb = tile_node->CreateComponent<RigidBody>();
                    // rb->SetMass(0.0f);
                    // if (z == 1)
                    //     rb->SetMass(10.0f);
                    // rb->SetFriction(0.1f);
                    // rb->SetRestitution(0.9f);

                    Tile_Occupier *occ = tile_node->CreateComponent<Tile_Occupier>();
                    tile_node->AddListener(occ);

                    tile_node->SetPosition(Hex_Tile_Grid::grid_to_world(ivec3(x, y, z)));
                    tile_node->SetRotation(Quaternion(90.0f, fvec3(1.0f, 0.0f, 0.0f)));

                    if (x == 0 && y == 0 && z == 0)
                    {
                        File prefab_file(context_, "grass_tile.json", FILE_WRITE);
                        tile_node->SaveJSON(prefab_file);
                        prefab_file.Close();
                        //tile_node->SaveJSON()
                    }

                    ++cnt;
                }
            }
        }
    }

    void BBToolkit::setup_global_keys(Input_Context *ctxt)
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

    void BBToolkit::handle_scene_update(StringHash /*eventType*/, VariantMap &event_data)
    {
    }

    void BBToolkit::handle_input_event(StringHash event_type,
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
        else if (name == StringHash("DeleteSelection"))
        {
            //bbtk.ui->details->clear_selection();
            scene_->GetComponent<Selection_Controller>()->delete_selection();
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
            Graphics *graphics = GetSubsystem<Graphics>();
            Image screenshot(context_);
            graphics->TakeScreenShot(screenshot);
            // Here we save in the Data folder with date and time appended
            screenshot.SavePNG(
                GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
                Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
        }
    }
}