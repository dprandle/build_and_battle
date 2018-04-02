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
    delete input_translator;
    delete camera_controller;
    delete input_map;
}

bool Build_And_Battle::init()
{
    VariantMap params;

    params[EP_WINDOW_TITLE] = GetTypeName();
    params[EP_LOG_NAME] = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
    params[EP_FULL_SCREEN] = false;
    params[EP_HEADLESS] = false;
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

    create_visuals();

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

    Input * in = GetSubsystem<Input>();
    in->SetMouseVisible(true);


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
}


void Build_And_Battle::handle_key_up(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyUp;

    int key = eventData[P_KEY].GetInt();

    // Close console (if open) or exit when ESC is pressed
    if (key == KEY_ESCAPE) 
    {
        Console* console = GetSubsystem<Console>();
        if (console->IsVisible())
            console->SetVisible(false);
        else 
            engine->Exit();
    }
}

void Build_And_Battle::handle_key_down(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyDown;

    int key = eventData[P_KEY].GetInt();

    // Toggle console with F1
    if (key == KEY_F1)
        GetSubsystem<Console>()->Toggle();

    // Toggle debug HUD with F2
    else if (key == KEY_F2)
        GetSubsystem<DebugHud>()->ToggleAll();

    // Common rendering quality controls, only when UI has no focused element
    else if (!GetSubsystem<UI>()->GetFocusElement())
    {
        Renderer* renderer = GetSubsystem<Renderer>();

        // Texture quality
        if (key == '1') {
            int quality = renderer->GetTextureQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetTextureQuality(quality);
        }

        // Material quality
        else if (key == '2') {
            int quality = renderer->GetMaterialQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetMaterialQuality(quality);
        }

        // Specular lighting
        else if (key == '3')
            renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

        // Shadow rendering
        else if (key == '4')
            renderer->SetDrawShadows(!renderer->GetDrawShadows());

        // Shadow map resolution
        else if (key == '5') {
            int shadowMapSize = renderer->GetShadowMapSize();
            shadowMapSize *= 2;
            if (shadowMapSize > 2048)
                shadowMapSize = 512;
            renderer->SetShadowMapSize(shadowMapSize);
        }

        // Shadow depth and filtering quality
        else if (key == '6') {
            ShadowQuality quality = renderer->GetShadowQuality();
            quality = (ShadowQuality)(quality + 1);
            if (quality > SHADOWQUALITY_BLUR_VSM)
                quality = SHADOWQUALITY_SIMPLE_16BIT;
            renderer->SetShadowQuality(quality);
        }

        // Occlusion culling
        else if (key == '7') {
            bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
            occlusion = !occlusion;
            renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
        }

        // Instancing
        else if (key == '8')
            renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());

        // Take screenshot
        else if (key == '9') 
        {
            Graphics* graphics = GetSubsystem<Graphics>();
            Image screenshot(context_);
            graphics->TakeScreenShot(screenshot);
            // Here we save in the Data folder with date and time appended
            screenshot.SavePNG(
                GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" + Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
        }
    }
}

void Build_And_Battle::handle_scene_update(StringHash /*eventType*/,
    VariantMap& eventData)
{
}

// If the user clicks the canvas, attempt to switch to relative mouse mode on
// web platform
void Build_And_Battle::handle_mouse_mode_request(StringHash /*eventType*/,
    VariantMap& eventData)
{
}

void Build_And_Battle::handle_mouse_mode_change(StringHash /*eventType*/,
    VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
    input->SetMouseVisible(!mouseLocked);
}
