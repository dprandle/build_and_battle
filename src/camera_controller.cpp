#include <camera_controller.h>
#include <input_translator.h>

#include <mtdebug_print.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Context.h>

using namespace Urho3D;

EditorCameraController::EditorCameraController(Urho3D::Context * context)
    : Object(context), cp(), cam_comp(nullptr)
{}

EditorCameraController::~EditorCameraController()
{}

void EditorCameraController::init()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(EditorCameraController, handle_update));
    SubscribeToEvent(E_INPUT_TRIGGER, URHO3D_HANDLER(EditorCameraController, handle_input_event));
}

void EditorCameraController::release()
{
    UnsubscribeFromAllEvents();
}

void EditorCameraController::set_camera(Urho3D::Camera * cam)
{
    cam_comp = cam;
}

Urho3D::Camera * EditorCameraController::get_camera()
{
    return cam_comp;
}

camera_params * EditorCameraController::get_camera_params()
{
    return &cp;
}

void EditorCameraController::handle_update(Urho3D::StringHash event_type,
                                           Urho3D::VariantMap & event_data)
{
    if (cam_comp == nullptr)
        return;

    Urho3D::Node * cam_node = cam_comp->GetNode();

    float dt = event_data["TimeStep"].GetFloat();
    static double accumulator = 0.0f;
    accumulator += dt;

    while (accumulator >= FIXED_TIMESTEP)
    {
        Vector3 resultant;
        auto iter = forces.Begin();
        while (iter != forces.End())
        {
            force_timer & ft = iter->second_;

            if (iter->first_ == StringHash("CameraForward"))
            {
                float mag = ft.force.Length();
                ft.force = cam_node->GetDirection() * mag;
            }
            else if (iter->first_ == StringHash("CameraReverse"))
            {
                float mag = ft.force.Length();
                ft.force = cam_node->GetDirection() * -mag;
            }
            else if (iter->first_ == StringHash("CameraLeft"))
            {
                float mag = ft.force.Length();
                ft.force = cam_node->GetRight() * -mag;
            }
            else if (iter->first_ == StringHash("CameraRight"))
            {
                float mag = ft.force.Length();
                ft.force = cam_node->GetRight() * mag;
            }

            resultant += ft.force;
            ft.time_left -= FIXED_TIMESTEP;
            if (ft.time_left <= 0.0f && ft.time_left > -0.9f)
                iter = forces.Erase(iter);
            else
                ++iter;
        }

        Vector3 drag = current_vel.Normalized() * current_vel.LengthSquared() * cp.drag_coef;
        resultant -= drag;

        Vector3 vh = current_vel + resultant * FIXED_TIMESTEP / (2.0f * cp.camera_mass);
        Vector3 new_pos = cam_node->GetPosition() + vh * FIXED_TIMESTEP;
        current_vel = (vh + resultant * FIXED_TIMESTEP / (2.0f * cp.camera_mass)) * 0.99f;

        // Clamp velocity down
        float vel_len = current_vel.Length();
        if (vel_len <= 0.5f && vel_len >= -0.5f)
            current_vel = Vector3();

        cam_node->SetPosition(new_pos);

        accumulator -= FIXED_TIMESTEP;
    }
}

void EditorCameraController::setup_input_context(input_context * ctxt)
{
    input_action_trigger it;

    it.condition.key = KEY_W;
    it.condition.mouse_button = 0;
    it.name = "CameraForward";
    it.trigger_state = t_begin | t_end;
    it.mb_required = 0;
    it.mb_allowed = MOUSEB_ANY;
    it.qual_required = 0;
    it.qual_allowed = QUAL_ANY;
    ctxt->create_trigger(it);

    it.condition.key = KEY_S;
    it.name = "CameraReverse";
    ctxt->create_trigger(it);

    it.condition.key = KEY_A;
    it.name = "CameraLeft";
    ctxt->create_trigger(it);

    it.condition.key = KEY_D;
    it.name = "CameraRight";
    ctxt->create_trigger(it);

    it.condition.key = 0;
    it.condition.mouse_button = MOUSEB_MOVE;
    it.name = "CameraPitchYaw";
    it.trigger_state = t_begin;
    it.mb_required = MOUSEB_RIGHT;
    it.mb_allowed = MOUSEB_RIGHT;
    it.qual_required = 0;
    it.qual_allowed = QUAL_ANY;
    ctxt->create_trigger(it);

    it.name = "CameraMove";
    it.mb_required = MOUSEB_LEFT | MOUSEB_RIGHT;
    it.mb_allowed = MOUSEB_ANY;
    ctxt->create_trigger(it);

    it.condition.mouse_button = MOUSEB_WHEEL;
    it.name = "CameraZoom";
    it.mb_required = 0;
    it.mb_allowed = MOUSEB_ANY;
    ctxt->create_trigger(it);
}

void EditorCameraController::_on_cam_pitch_yaw(const Urho3D::Vector2 & mdelta)
{
    if (cam_comp == nullptr)
        return;
    Urho3D::Node * cam_node = cam_comp->GetNode();

    Quaternion pitch(mdelta.y_ * cp.invert_cam_center.y_ * cp.pitch_yaw_sensitivity,
                     cam_node->GetRight());
    Quaternion yaw(mdelta.x_ * cp.invert_cam_center.x_ * cp.pitch_yaw_sensitivity,
                   Vector3(0.0f, 0.0f, 1.0f));
    cam_node->SetRotation(pitch * yaw * cam_node->GetRotation());
}

void EditorCameraController::_on_cam_move(const Urho3D::Vector2 & mdelta)
{
    if (cam_comp == nullptr)
        return;
    Urho3D::Node * cam_node = cam_comp->GetNode();

    Vector3 translation = cam_node->GetRight() * -mdelta.x_ * cp.invert_panning.x_ +
                          cam_node->GetUp() * mdelta.y_ * cp.invert_panning.y_;
    cam_node->SetPosition(cam_node->GetPosition() + translation * cp.movement_sensitivity);
}

void EditorCameraController::_on_cam_zoom(int wheel)
{
    force_timer ref;
    auto iter = forces.Find("CameraZoom");
    if (iter != forces.End())
        ref = iter->second_;

    ref.time_left = 0.01f;
    if (wheel > 0)
        ref.force += cam_comp->GetNode()->GetDirection() * cp.zoom_force;
    else if (wheel < 0)
        ref.force += cam_comp->GetNode()->GetDirection() * -cp.zoom_force;

    forces["CameraZoom"] = ref;
}

void EditorCameraController::handle_input_event(Urho3D::StringHash event_type,
                                                Urho3D::VariantMap & event_data)
{
    if (cam_comp == nullptr)
        return;
    Urho3D::Node * cam_node = cam_comp->GetNode();

    StringHash name = event_data[InputTrigger::P_TRIGGER_NAME].GetStringHash();
    int state = event_data[InputTrigger::P_TRIGGER_STATE].GetInt();
    Vector2 norm_mpos = event_data[InputTrigger::P_NORM_MPOS].GetVector2();
    Vector2 norm_mdelta = event_data[InputTrigger::P_NORM_MDELTA].GetVector2();
    int wheel = event_data[InputTrigger::P_MOUSE_WHEEL].GetInt();

    force_timer ft;
    ft.time_left = 0.0f;

    if (name == StringHash("CameraForward"))
    {
        ft.time_left = -1.0f; // do not auto remove this force
        ft.force = cam_node->GetDirection() * cp.move_force;
        if (state == t_begin)
            forces[name] = ft;
        else
            forces.Erase(name);
    }
    else if (name == StringHash("CameraReverse"))
    {
        ft.time_left = -1.0f; // do not auto remove this force
        ft.force = cam_node->GetDirection() * -cp.move_force;
        if (state == t_begin)
            forces[name] = ft;
        else
            forces.Erase(name);
    }
    else if (name == StringHash("CameraLeft"))
    {
        ft.time_left = -1.0f; // do not auto remove this force
        ft.force = cam_node->GetRight() * -cp.move_force;
        if (state == t_begin)
            forces[name] = ft;
        else
            forces.Erase(name);
    }
    else if (name == StringHash("CameraRight"))
    {
        ft.time_left = -1.0f; // do not auto remove this force
        ft.force = cam_node->GetRight() * cp.move_force;
        if (state == t_begin)
            forces[name] = ft;
        else
            forces.Erase(name);
    }
    else if (name == StringHash("CameraPitchYaw"))
    {
        _on_cam_pitch_yaw(norm_mdelta);
    }
    else if (name == StringHash("CameraMove"))
    {
        _on_cam_move(norm_mdelta);
    }
    else if (name == StringHash("CameraZoom"))
    {
        _on_cam_zoom(wheel);
    }
}