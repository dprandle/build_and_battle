#ifndef BUILD_AND_BATTLE_H
#define BUILD_AND_BATTLE_H

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Math/Vector3.h>

using namespace Urho3D;

namespace Urho3D
{
class Node;
class Scene;
class Sprite;
}

class EditorSelectionController;
class EditorCameraController;
class InputTranslator;
class InputMap;
struct input_context;

const float TOUCH_SENSITIVITY = 2.0f;

class Build_And_Battle : public Object
{
    // Enable type information.
    URHO3D_OBJECT(Build_And_Battle, Object);

public:
    
    Build_And_Battle(Context* context);

    ~Build_And_Battle();

    int run();

    void init_mouse_mode(MouseMode mode);

private:

    void handle_input_event(StringHash eventType, VariantMap& eventData);

    void handle_ui_event(StringHash eventType, VariantMap& eventData);

    void handle_scene_update(StringHash event_type, VariantMap& event_data);

    void handle_post_render_update(StringHash event_type, VariantMap& event_data);

    bool init();

    void release();

    void setup_global_keys(input_context * ctxt);

    void create_visuals();

    Engine * engine;

    Scene * scene;
    
    Node * cam_node;

    InputTranslator * input_translator;

    EditorCameraController * camera_controller;

    InputMap * input_map;

    bool m_draw_debug;

};

#endif