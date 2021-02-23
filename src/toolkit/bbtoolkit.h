#pragma once

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

class Input_Translator;
class Input_Map;
struct Input_Context;

namespace bbtk
{

class Selection_Controller;
class Camera_Controller;

const float TOUCH_SENSITIVITY = 2.0f;

class BBToolkit : public Object
{
    // Enable type information.
    URHO3D_OBJECT(BBToolkit, Object);

public:
    
    BBToolkit(Context* context);

    ~BBToolkit();

    int run();

    void init_mouse_mode(MouseMode mode);

private:

    void handle_input_event(StringHash eventType, VariantMap& eventData);

    void handle_ui_event(StringHash eventType, VariantMap& eventData);

    void handle_log_message(StringHash event_type, Urho3D::VariantMap & event_data);

    void handle_scene_update(StringHash event_type, VariantMap& event_data);

    void handle_post_render_update(StringHash event_type, VariantMap& event_data);

    bool init();

    void release();

    void setup_global_keys(Input_Context * ctxt);

    void create_visuals();

    Engine * engine_;

    Scene * scene_;
    
    Node * cam_node_;

    Input_Translator * input_translator_;

    Camera_Controller * camera_controller_;

    Input_Map * input_map_;

    bool draw_debug_;

};

}