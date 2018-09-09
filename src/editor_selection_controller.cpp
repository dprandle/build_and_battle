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
    hashes.Push(StringHash(SEL_OBJ_NAME));
    hashes.Push(StringHash(EXTEND_SEL_OBJ_NAME));

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(EditorSelectionController, handle_update));
    SubscribeToEvent(E_INPUT_TRIGGER,
                     URHO3D_HANDLER(EditorSelectionController, handle_input_event));
}

void EditorSelectionController::clear_selection() 
{
    auto iter = selection.Begin();
    while (iter != selection.End())
    {
        EditorSelector * es = iter->first_->GetComponent<EditorSelector>();
        es->set_selected(iter->first_, false);
        ++iter;
    }
    selection.Clear();
}

void EditorSelectionController::remove_from_selection(Urho3D::Node* node)
{
    auto iter = selection.Begin();
    while (iter != selection.End())
    {
        EditorSelector * es = iter->first_->GetComponent<EditorSelector>();
        if (node == iter->first_)
        {
            selection.Erase(iter);
            es->set_selected(node, false);
            return;
        }
        bool fnd = iter->second_.Remove(node);
        if (fnd)
        {
            es->set_selected(node, false);
            return;
        }
        ++iter;
    }
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
    input_action_trigger it;

    it.condition.key = 0;
    it.condition.mouse_button = MOUSEB_LEFT;
    it.trigger_state = t_begin;
    it.mb_required = 0;
    it.mb_allowed = 0;
    it.qual_required = 0;
    it.qual_allowed = 0;
    it.name = SEL_OBJ_NAME;
    ctxt->create_trigger(it);
    
    it.name = EXTEND_SEL_OBJ_NAME;
    it.qual_required = QUAL_CTRL;
    ctxt->create_trigger(it);
}

void EditorSelectionController::handle_input_event(Urho3D::StringHash event_type,
                                                   Urho3D::VariantMap & event_data)
{
    StringHash name = event_data[InputTrigger::P_TRIGGER_NAME].GetStringHash();
    int state = event_data[InputTrigger::P_TRIGGER_STATE].GetInt();
    Vector2 norm_mpos = event_data[InputTrigger::P_NORM_MPOS].GetVector2();
    Vector2 norm_mdelta = event_data[InputTrigger::P_NORM_MDELTA].GetVector2();
    int wheel = event_data[InputTrigger::P_MOUSE_WHEEL].GetInt();

    if (hashes.Contains(name))
    {
        Octree * oct = scene->GetComponent<Octree>();
        PODVector<RayQueryResult> res;
        // Get the closest object for selection
        RayOctreeQuery q(res, cam_comp->GetScreenRay(norm_mpos.x_, norm_mpos.y_));
        oct->RaycastSingle(q);

        // Go through the results from the query - there really should only be one result since we are only doing 
        // a single raycast
        for (int i = 0; i < res.Size(); ++i)
        {
            RayQueryResult & cr = res[i];
            Node * nd = cr.node_;

            StaticModelGroup * smg = nullptr;
            StaticModel * sm = nullptr;
            
            // If the object hit is a static model group then get the instance node
            if (cr.drawable_->IsInstanceOf<StaticModelGroup>())
                nd = static_cast<StaticModelGroup *>(cr.drawable_)->GetInstanceNode(cr.subObject_);

            EditorSelector * es = cr.node_->GetComponent<EditorSelector>();
            if (es != nullptr)
            {
                if (name == hashes[0])
                {
                    clear_selection();
                    es->set_selected(nd, true);
                    if (!selection[cr.node_].Contains(nd) && cr.node_ != nd)
                        selection[cr.node_].Push(nd);
                }
                if (name == hashes[1])
                {
                    if (es->is_selected(nd))
                    {
                        remove_from_selection(nd);
                    }
                    else
                    {
                        es->set_selected(nd, true);
                        if (!selection[cr.node_].Contains(nd) && cr.node_ != nd)
                            selection[cr.node_].Push(nd);
                    }
                }
            }
        }
    }
}

