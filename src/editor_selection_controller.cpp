#include <editor_selection_controller.h>
#include <input_translator.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/StaticModelGroup.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/OctreeQuery.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Context.h>

#include <editor_selector.h>
#include <string>

using namespace Urho3D;

EditorSelectionController::EditorSelectionController(Urho3D::Context * context)
    : Object(context), cam_comp(nullptr), scene(nullptr)
{}

EditorSelectionController::~EditorSelectionController()
{}

void EditorSelectionController::init()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(EditorSelectionController, handle_update));
    SubscribeToEvent(E_INPUT_TRIGGER,
                     URHO3D_HANDLER(EditorSelectionController, handle_input_event));
}

void EditorSelectionController::set_camera(Urho3D::Camera * cam)
{
    cam_comp = cam;
}

void EditorSelectionController::set_scene(Urho3D::Scene * scn)
{
    scene = scn;
}

Urho3D::Camera * EditorSelectionController::get_camera()
{
    return cam_comp;
}

Urho3D::Scene * EditorSelectionController::get_scene()
{
    return scene;
}

void EditorSelectionController::release()
{
    UnsubscribeFromAllEvents();
}

void EditorSelectionController::handle_update(Urho3D::StringHash event_type,
                                              Urho3D::VariantMap & event_data)
{}

void EditorSelectionController::setup_input_context(input_context * ctxt)
{
    trigger_condition tc;
    input_action_trigger * it;

    tc.key = 0;
    tc.mouse_button = MOUSEB_LEFT;
    it = ctxt->create_trigger(tc);
    it->name = "SelectObject";
    it->trigger_state = t_begin;
    it->mb_required = 0;
    it->mb_allowed = 0;
    it->qual_required = 0;
    it->qual_allowed = QUAL_ANY;
}

void EditorSelectionController::handle_input_event(Urho3D::StringHash event_type,
                                                   Urho3D::VariantMap & event_data)
{
    StringHash name = event_data[InputTrigger::P_TRIGGER_NAME].GetStringHash();
    int state = event_data[InputTrigger::P_TRIGGER_STATE].GetInt();
    Vector2 norm_mpos = event_data[InputTrigger::P_NORM_MPOS].GetVector2();
    Vector2 norm_mdelta = event_data[InputTrigger::P_NORM_MDELTA].GetVector2();
    int wheel = event_data[InputTrigger::P_MOUSE_WHEEL].GetInt();

    if (name == StringHash("SelectObject"))
    {
        Octree * oct = scene->GetComponent<Octree>();

        PODVector<RayQueryResult> res;
        RayOctreeQuery q(res, cam_comp->GetScreenRay(norm_mpos.x_, norm_mpos.y_));
        oct->RaycastSingle(q);

        for (int i = 0; i < res.Size(); ++i)
        {
            RayQueryResult & cr = res[i];

            StaticModelGroup * smg = nullptr;
            StaticModel * sm = nullptr;
            
            if (cr.drawable_->IsInstanceOf<StaticModelGroup>())
                smg = static_cast<StaticModelGroup *>(cr.drawable_);

            EditorSelector * es = cr.node_->GetComponent<EditorSelector>();
            Node * nd = cr.node_;

            if (smg != nullptr)
                nd = smg->GetInstanceNode(cr.subObject_);

            if (es != nullptr)
            {
                es->set_selected(nd, true);

                // Node * nd = smg->GetInstanceNode(cr.subObject_);
                // if (nd != nullptr)
                // {
                //     smg->RemoveInstanceNode(nd);
                //     //auto ires = selection.insert(nd);
                //     //if (ires.second)
                // }
            }
            else
            {
                URHO3D_LOGINFO("Editor Selector is null");
            }
        }
    }
}