#ifndef BUILD_AND_BATTLE_H
#define BUILD_AND_BATTLE_H

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>


namespace Urho3D
{

class Node;
class Scene;
class Sprite;

}

class EditorCameraController;
class InputTranslator;
class InputMap;
struct input_context;

const float TOUCH_SENSITIVITY = 2.0f;

class Build_And_Battle : public Urho3D::Object
{
    // Enable type information.
    URHO3D_OBJECT(Build_And_Battle, Urho3D::Object);

public:
    
    Build_And_Battle(Urho3D::Context* context);

    ~Build_And_Battle();

    int run();

    void init_mouse_mode(Urho3D::MouseMode mode);

private:

    bool init();

    void release();

    void setup_global_keys(input_context * ctxt);

    void create_visuals();

    Urho3D::Engine * engine;

    Urho3D::Scene * scene;
    
    Urho3D::Node * cam_node;

    InputTranslator * input_translator;

    EditorCameraController * camera_controller;

    InputMap * input_map;

    void handle_input_event(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);

    void handle_scene_update(Urho3D::StringHash event_type, Urho3D::VariantMap& event_data);

};

#endif