#include <editor_selection_controller.h>
#include <input_translator.h>
#include <mtdebug_print.h>
#include <build_and_battle.h>
#include <hex_tile_grid.h>
#include <tile_occupier.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Graphics/DebugRenderer.h>

#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/CollisionShape.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Material.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Math/Frustum.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/StaticModelGroup.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/OctreeQuery.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/BorderImage.h>

#include <editor_selector.h>
#include <string>

using namespace Urho3D;

EditorSelectionController::EditorSelectionController(Urho3D::Context * context)
    : Component(context),
      cam_comp(nullptr),
      scene(nullptr),
      selection_rect(-1.0f, -1.0f, -1.0f, -1.0f),
      movement_flag(0),
      move_allowed(true),
      do_snap(false)
{
    UI * usi = GetSubsystem<UI>();

    ui_root = usi->GetRoot();

    ui_selection_rect = ui_root->CreateChild<UIElement>("sel_rect_root");
    ui_selection_rect->SetVisible(false);
    //ui_selection_rect->SetSortChildren(false);

    BorderImage * ui_left_border_sel_rect =
        ui_selection_rect->CreateChild<BorderImage>("leftb_selection_rect");
    ui_left_border_sel_rect->SetEnableAnchor(true);
    ui_left_border_sel_rect->SetMinAnchor(0.0f, 0.0f);
    ui_left_border_sel_rect->SetMaxAnchor(0.0f, 1.0f);
    ui_left_border_sel_rect->SetMaxOffset(ivec2(BORDER_SIZE, 0));
    ui_left_border_sel_rect->SetColor(SEL_RECT_BORDER_COL);

    BorderImage * ui_right_border_sel_rect =
        ui_selection_rect->CreateChild<BorderImage>("rightb_selection_rect");
    ui_right_border_sel_rect->SetEnableAnchor(true);
    ui_right_border_sel_rect->SetMinAnchor(1.0f, 0.0f);
    ui_right_border_sel_rect->SetMaxAnchor(1.0f, 1.0f);
    ui_right_border_sel_rect->SetMinOffset(ivec2(0, 0));
    ui_right_border_sel_rect->SetMaxOffset(ivec2(BORDER_SIZE, 0));
    ui_right_border_sel_rect->SetColor(SEL_RECT_BORDER_COL);

    BorderImage * ui_top_border_sel_rect =
        ui_selection_rect->CreateChild<BorderImage>("topb_selection_rect");
    ui_top_border_sel_rect->SetEnableAnchor(true);
    ui_top_border_sel_rect->SetMinAnchor(0.0f, 0.0f);
    ui_top_border_sel_rect->SetMaxAnchor(1.0f, 0.0f);
    ui_top_border_sel_rect->SetMinOffset(ivec2(BORDER_SIZE, 0));
    ui_top_border_sel_rect->SetMaxOffset(ivec2(-BORDER_SIZE, BORDER_SIZE));
    ui_top_border_sel_rect->SetColor(SEL_RECT_BORDER_COL);

    BorderImage * ui_bottom_border_sel_rect =
        ui_selection_rect->CreateChild<BorderImage>("bottomb_selection_rect");
    ui_bottom_border_sel_rect->SetEnableAnchor(true);
    ui_bottom_border_sel_rect->SetMinAnchor(0.0f, 1.0f);
    ui_bottom_border_sel_rect->SetMaxAnchor(1.0f, 1.0f);
    ui_bottom_border_sel_rect->SetMinOffset(ivec2(BORDER_SIZE, 0));
    ui_bottom_border_sel_rect->SetMaxOffset(ivec2(-BORDER_SIZE, BORDER_SIZE));
    ui_bottom_border_sel_rect->SetColor(SEL_RECT_BORDER_COL);

    BorderImage * ui_inner_sel_rect =
        ui_selection_rect->CreateChild<BorderImage>("inner_selection_rect");
    ui_inner_sel_rect->SetEnableAnchor(true);
    ui_inner_sel_rect->SetMinAnchor(0.0f, 0.0f);
    ui_inner_sel_rect->SetMaxAnchor(1.0f, 1.0f);
    ui_inner_sel_rect->SetMinOffset(ivec2(BORDER_SIZE, BORDER_SIZE));
    ui_inner_sel_rect->SetMaxOffset(ivec2(-BORDER_SIZE, -BORDER_SIZE));
    ui_inner_sel_rect->SetColor(SEL_RECT_COL);

    hashes.Push(StringHash(SEL_OBJ_NAME));
    hashes.Push(StringHash(DRAG_SELECTED_OBJECT));
    hashes.Push(StringHash(EXTEND_SEL_OBJ_NAME));
    hashes.Push(StringHash(DRAGGING));
    hashes.Push(StringHash(ENABLE_SELECTION_RECT));
    hashes.Push(StringHash(Z_MOVE_HELD));
    hashes.Push(StringHash(X_MOVE_HELD));
    hashes.Push(StringHash(Y_MOVE_HELD));
    hashes.Push(StringHash(TOGGLE_OCC_DEBUG));

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(EditorSelectionController, handle_update));
    SubscribeToEvent(E_INPUT_TRIGGER,
                     URHO3D_HANDLER(EditorSelectionController, handle_input_event));
    SubscribeToEvent(E_COMPONENTADDED,
                     URHO3D_HANDLER(EditorSelectionController, handle_component_added));
    SubscribeToEvent(E_COMPONENTREMOVED,
                     URHO3D_HANDLER(EditorSelectionController, handle_component_removed));
}

EditorSelectionController::~EditorSelectionController()
{}

void EditorSelectionController::OnSceneSet(Urho3D::Scene * scene_)
{
    scene = scene_;
    Component::OnSceneSet(scene_);
}

void EditorSelectionController::clear_selection()
{
    selection.Clear();
    sel_rect_selection.Clear();
}

void EditorSelectionController::handle_component_added(Urho3D::StringHash eventType,
                                                       Urho3D::VariantMap & eventData)
{
    Scene * scn = static_cast<Scene *>(eventData[NodeRemoved::P_SCENE].GetPtr());
    if (scene != scn)
        return;
    Component * comp = static_cast<Component *>(eventData[ComponentRemoved::P_COMPONENT].GetPtr());
    if (comp->IsInstanceOf<EditorSelector>())
        scene_sel_comps.Insert(static_cast<EditorSelector *>(comp));
}

void EditorSelectionController::handle_component_removed(Urho3D::StringHash eventType,
                                                         Urho3D::VariantMap & eventData)
{
    Scene * scn = static_cast<Scene *>(eventData[ComponentRemoved::P_SCENE].GetPtr());
    if (scene != scn)
        return;
    Component * comp = static_cast<Component *>(eventData[ComponentRemoved::P_COMPONENT].GetPtr());
    if (comp->IsInstanceOf<EditorSelector>())
        scene_sel_comps.Erase(static_cast<EditorSelector *>(comp));
}

void EditorSelectionController::DrawDebugGeometry(bool depth_test)
{
    if (scene == nullptr)
        return;

    Urho3D::DebugRenderer * deb = scene->GetComponent<DebugRenderer>();
    if (deb == nullptr)
        return;

    auto iter = cached_raycasts.Begin();
    while (iter != cached_raycasts.End())
    {
        Node * camn = cam_comp->GetNode();
        fvec3 cam_pos = camn->GetPosition() + camn->GetDirection().Normalized() * 0.1f;
        fvec3 node_pos = iter->first_->GetPosition();
        deb->AddLine(cam_pos, node_pos, Color(1.0f, 0.0f, 0.0f), depth_test);
        ++iter;
    }
}

void EditorSelectionController::translate_selection(const fvec3 & translation)
{
    auto sel_iter = selection.Begin();
    while (sel_iter != selection.End())
    {
        sel_iter->first_->Translate(translation, TS_WORLD);
        ++sel_iter;
    }
}

void EditorSelectionController::toggle_occ_debug_selection()
{
    auto sel_iter = selection.Begin();
    while (sel_iter != selection.End())
    {
        Tile_Occupier * toc = sel_iter->first_->GetComponent<Tile_Occupier>();
        if (toc != nullptr)
            toc->enable_debug(!toc->debug_enabled());
        ++sel_iter;
    }
}

void EditorSelectionController::remove_from_selection(Urho3D::Node * node)
{
    auto iter = selection.Begin();
    while (iter != selection.End())
    {
        EditorSelector * es = iter->first_->GetComponent<EditorSelector>();
        if (node == iter->first_)
        {
            selection.Erase(iter);
            return;
        }
        bool fnd = iter->second_.Remove(node);
        if (fnd)
            return;
        ++iter;
    }
}

void EditorSelectionController::set_camera(Urho3D::Camera * cam)
{
    cam_comp = cam;
}

Urho3D::Camera * EditorSelectionController::get_camera()
{
    return cam_comp;
}

void EditorSelectionController::handle_update(Urho3D::StringHash event_type,
                                              Urho3D::VariantMap & event_data)
{
    ResourceCache * cache = GetSubsystem<ResourceCache>();
    static HashMap<Material *, bool> mat_map;
    Urho3D::Vector<Hex_Tile_Grid::Tile_Item> allowed_items;
    
    allowed_items.Resize(selection.Size());
    int i = 0;
    auto sel_iter_al = selection.Begin();
    while (sel_iter_al != selection.End())
    {
        int node_id = sel_iter_al->first_->GetID();
        allowed_items[i].node_id_ = node_id;
        ++i;
        ++sel_iter_al;
    }
    
    auto iter = selection.Begin();
    while (iter != selection.End())
    {
        EditorSelector * es = iter->first_->GetComponent<EditorSelector>();
        String str = es->selection_material();
        Material * mat = cache->GetResource<Material>(str);

        auto fiter = mat_map.Find(mat);
        if (fiter == mat_map.End())
            mat_map[mat] = true;

        if (iter->second_.Empty())
        {
            Hex_Tile_Grid * tg = scene->GetComponent<Hex_Tile_Grid>();
            Tile_Occupier * occ = iter->first_->GetComponent<Tile_Occupier>();

            uint32_t node_id = iter->first_->GetID();
            iter->first_->Translate(frame_translation, TS_WORLD);

            if (occ != nullptr)
            {
                auto occ_tiles =
                    tg->occupied(occ->tile_spaces(), iter->first_->GetPosition(), allowed_items);
                mat_map[mat] = mat_map[mat] && occ_tiles.Empty();
            }
        }
        else
        {
            for (int i = 0; i < iter->second_.Size(); ++i)
            {
                Hex_Tile_Grid * tg = scene->GetComponent<Hex_Tile_Grid>();
                Tile_Occupier * occ = iter->first_->GetComponent<Tile_Occupier>();

                uint32_t node_id = iter->first_->GetID();
                iter->second_[i]->Translate(frame_translation, TS_WORLD);

                if (occ != nullptr)
                {
                    auto occ_tiles = tg->occupied(
                        occ->tile_spaces(), iter->first_->GetPosition(), allowed_items);
                    mat_map[mat] = mat_map[mat] && occ_tiles.Empty();
                }
            }
        }
        ++iter;
    }
    total_drag_translation += frame_translation;
    frame_translation = fvec3();

    if (do_snap)
    {
        snap_selection();
        do_snap = false;
    }

    auto mat_iter = mat_map.Begin();
    while (mat_iter != mat_map.End())
    {
        move_allowed = true;
        fvec4 color(0.0f, 0.0f, 1.0f, 1.0f);
        if (!mat_iter->second_)
        {
            move_allowed = false;
            color = fvec4(1.0f, 0.0f, 0.0f, 1.0f);
            mat_iter->second_ = true;
        }

        mat_iter->first_->SetShaderParameter("OutlineColor", color);
        ++mat_iter;
    }

    auto sel_comp_iter = scene_sel_comps.Begin();
    while (sel_comp_iter != scene_sel_comps.End())
    {
        Node * nd = (*sel_comp_iter)->GetNode();
        StaticModel * md = nd->GetComponent<StaticModel>();
        if (md->IsInstanceOf<StaticModelGroup>())
        {
            StaticModelGroup * smg = static_cast<StaticModelGroup *>(md);
            for (int i = 0; i < smg->GetNumInstanceNodes(); ++i)
            {
                Node * inst_node = smg->GetInstanceNode(i);

                (*sel_comp_iter)->set_selected(inst_node, is_selected(nd, inst_node));
            }
        }
        else
        {
            (*sel_comp_iter)->set_selected(nd, is_selected(nd));
        }
        ++sel_comp_iter;
    }
}

void EditorSelectionController::snap_selection()
{
    auto sel_iter = selection.Begin();
    while (sel_iter != selection.End())
    {
        fvec3 pos = sel_iter->first_->GetPosition();
        Hex_Tile_Grid::snap_to_grid(pos);
        sel_iter->first_->SetPosition(pos);
        ++sel_iter;
    }
    sel_iter = sel_rect_selection.Begin();
    while (sel_iter != sel_rect_selection.End())
    {
        fvec3 pos = sel_iter->first_->GetPosition();
        Hex_Tile_Grid::snap_to_grid(pos);
        sel_iter->first_->SetPosition(pos);
        ++sel_iter;
    }
}

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

    it.name = DRAG_SELECTED_OBJECT;
    it.trigger_state = t_begin | t_end;
    ctxt->create_trigger(it);

    it.name = ENABLE_SELECTION_RECT;
    it.qual_required = QUAL_SHIFT;
    ctxt->create_trigger(it);

    it.name = EXTEND_SEL_OBJ_NAME;
    it.trigger_state = t_begin;
    it.qual_required = QUAL_CTRL;
    ctxt->create_trigger(it);

    it.condition.mouse_button = MOUSEB_MOVE;
    it.name = DRAGGING;
    it.mb_required = MOUSEB_LEFT;
    it.qual_required = 0;
    it.qual_allowed = QUAL_ANY;
    ctxt->create_trigger(it);

    it.condition.key = KEY_Z;
    it.condition.mouse_button = 0;
    it.trigger_state = t_begin | t_end;
    it.mb_required = 0;
    it.mb_allowed = MOUSEB_ANY;
    it.qual_required = 0;
    it.qual_allowed = 0;
    it.name = Z_MOVE_HELD;
    ctxt->create_trigger(it);

    it.condition.key = KEY_X;
    it.name = X_MOVE_HELD;
    ctxt->create_trigger(it);

    it.condition.key = KEY_Y;
    it.name = Y_MOVE_HELD;
    ctxt->create_trigger(it);

    it.condition.key = KEY_O;
    it.trigger_state = t_begin;
    it.name = TOGGLE_OCC_DEBUG;
    ctxt->create_trigger(it);
}

bool EditorSelectionController::is_selected(Urho3D::Node * obj_node, Urho3D::Node * sub_obj_node)
{
    auto fiter = selection.Find(obj_node);
    if (fiter != selection.End())
    {
        if (sub_obj_node == nullptr)
            return true;

        return fiter->second_.Contains(sub_obj_node);
    }

    auto sq_iter = sel_rect_selection.Find(obj_node);
    if (sq_iter != sel_rect_selection.End())
    {
        if (sub_obj_node == nullptr)
            return true;

        return sq_iter->second_.Contains(sub_obj_node);
    }
    return false;
}

void EditorSelectionController::add_to_selection_from_rect()
{
    Frustum f;
    ivec2 sz = ui_root->GetSize();
    ivec2 sel_pos = ui_selection_rect->GetPosition();
    ivec2 sel_sz = ui_selection_rect->GetSize();

    fvec2 norm_sel_pos(float(sel_pos.x_) / float(sz.x_), float(sel_pos.y_) / float(sz.y_));
    fvec2 norm_sz(float(sel_sz.x_) / float(sz.x_), float(sel_sz.y_) / float(sz.y_));
    fvec2 norm_offset = norm_sel_pos + 0.5f * norm_sz - fvec2(0.5f, 0.5f);

    float fov = cam_comp->GetFov();
    float ar = cam_comp->GetAspectRatio();
    float near_z = cam_comp->GetNearClip();

    float new_ar = Abs(float(sel_sz.x_) / float(sel_sz.y_));
    float new_fov = Abs(norm_sz.y_ * fov);

    if (new_fov < 1.0f || new_ar > sz.x_ || new_ar < (1.0f / float(sz.y_)))
        return;

    float ws_size_y = near_z * std::tan(radians(fov) / 2.0f);
    float ws_size_x = ar * -ws_size_y;

    fvec2 ws_sub_pos(ws_size_x * (2.0f * norm_sel_pos.x_ - 1.0f),
                     ws_size_y * (2.0f * norm_sel_pos.y_ - 1.0f));
    fvec2 ws_sub_sz(2.0f * norm_sz.x_ * ws_size_x, 2.0f * norm_sz.y_ * ws_size_y);

    fmat4 moved_proj = perspective_from(ws_sub_pos.x_,
                                        ws_sub_pos.x_ + ws_sub_sz.x_,
                                        ws_sub_pos.y_ + ws_sub_sz.y_,
                                        ws_sub_pos.y_,
                                        near_z,
                                        1000.0f);
    f.Define(moved_proj * cam_comp->GetView());
    PODVector<Drawable *> res;
    Octree * octree = scene->GetComponent<Octree>();
    FrustumOctreeQuery fq(res, f, DRAWABLE_GEOMETRY);
    octree->GetDrawables(fq);
    sel_rect_selection.Clear();
    for (int i = 0; i < res.Size(); ++i)
    {
        Node * nd = res[i]->GetNode();
        StaticModel * sm = static_cast<StaticModel *>(res[i]);
        EditorSelector * es = nd->GetComponent<EditorSelector>();

        if (es == nullptr)
            continue;

        // If the object hit is a static model group then we need to go through each instance
        // one by one and check the bounding box against the frustum
        if (sm->IsInstanceOf<StaticModelGroup>())
        {
            StaticModelGroup * smg = static_cast<StaticModelGroup *>(sm);
            const BoundingBox & bb = smg->GetModel()->GetBoundingBox();
            for (int inst_id = 0; inst_id < smg->GetNumInstanceNodes(); ++inst_id)
            {
                Node * inst_nd = smg->GetInstanceNode(inst_id);
                BoundingBox transf_bb = bb.Transformed(inst_nd->GetTransform());
                if (f.IsInside(transf_bb) == Intersection::INSIDE)
                {
                    //es->set_selected(inst_nd, true);
                    if (!selection[nd].Contains(inst_nd) && nd != inst_nd)
                        selection[nd].Push(inst_nd);
                }
            }
        }
        else
        {
            const BoundingBox & bb = sm->GetWorldBoundingBox();

            PhysicsWorld * phys = scene->GetComponent<PhysicsWorld>();

            bool left_drag = (norm_sel_pos.x_ < selection_rect.z_);

            bool res = false;
            auto fiter = cached_raycasts.Find(nd);
            if (left_drag || f.IsInside(bb) == Intersection::INSIDE)
            {
                if (fiter == cached_raycasts.End())
                {
                    // Do a raycast to the center and each point of the objects bounding box
                    PhysicsRaycastResult ray_result;
                    fvec3 direction = nd->GetPosition() - cam_comp->GetNode()->GetPosition();
                    direction.Normalize();
                    Ray cast_ray(cam_comp->GetNode()->GetPosition(), direction);
                    // Get the closest object for selection
                    phys->RaycastSingle(ray_result, cast_ray, 100.0f);
                    if (ray_result.body_ != nullptr)
                        res = (ray_result.body_->GetNode() == nd);
                    cached_raycasts[nd] = res;
                }
                else
                {
                    res = fiter->second_;
                }
            }

            if (res)
            {
                //es->set_selected(nd, true);
                sel_rect_selection[nd].Clear();
            }
        }
    }
}

void EditorSelectionController::handle_input_event(Urho3D::StringHash event_type,
                                                   Urho3D::VariantMap & event_data)
{
    StringHash name = event_data[InputTrigger::P_TRIGGER_NAME].GetStringHash();
    int state = event_data[InputTrigger::P_TRIGGER_STATE].GetInt();
    Vector2 norm_mpos = event_data[InputTrigger::P_NORM_MPOS].GetVector2();
    Vector2 norm_mdelta = event_data[InputTrigger::P_NORM_MDELTA].GetVector2();
    int wheel = event_data[InputTrigger::P_MOUSE_WHEEL].GetInt();

    if (name == hashes[8])
    {
        toggle_occ_debug_selection();
    }
    if (name == hashes[3])
    {
        // drag_point.w_ is a value used to detect if we are dragging - reset to 0 when draggin stops and set to 1 when
        // dragging starts
        if (drag_point.w_ > 0.5f)
        {
            // Project the mouse movement vector to the camera projection plane that the collision occured
            // This is dependent on screen resolution - we have the normalized mdelta
            fvec3 cam_pos = cam_comp->GetNode()->GetPosition();
            drag_point.w_ = 1.0f;
            fvec4 screen_space = cam_comp->GetProjection() * cam_comp->GetView() * drag_point;
            screen_space /= screen_space.w_;
            screen_space.x_ += norm_mdelta.x_ * 2.0f;
            screen_space.y_ -= norm_mdelta.y_ * 2.0f;
            fvec4 new_posv4 =
                (cam_comp->GetProjection() * cam_comp->GetView()).Inverse() * screen_space;
            new_posv4 /= new_posv4.w_;

            fvec3 new_pos(new_posv4.x_, new_posv4.y_, new_posv4.z_);
            fvec3 drag_pnt(drag_point.x_, drag_point.y_, drag_point.z_);
            fvec3 raw_translation = new_pos - drag_pnt;

            int state = movement_flag;
            if (state < 7)
            {
                if (state == 0)
                    state = X_MOVE_FLAG | Y_MOVE_FLAG;

                fvec3 normal = cam_comp->GetNode()->GetDirection();
                normal *= fvec3(float((state & X_MOVE_FLAG) != X_MOVE_FLAG),
                                float((state & Y_MOVE_FLAG) != Y_MOVE_FLAG),
                                float((state & Z_MOVE_FLAG) != Z_MOVE_FLAG));
                normal.Normalize();
                fvec3 cast_vec = (new_pos - cam_pos);
                fvec3 proj_vec = raw_translation.ProjectOntoPlane(fvec3(0, 0, 0), normal);
                fvec3 norm_vec = proj_vec - raw_translation;

                raw_translation +=
                    (norm_vec.LengthSquared() / norm_vec.DotProduct(cast_vec)) * cast_vec;

                raw_translation *= fvec3(float((state & X_MOVE_FLAG) == X_MOVE_FLAG),
                                         float((state & Y_MOVE_FLAG) == Y_MOVE_FLAG),
                                         float((state & Z_MOVE_FLAG) == Z_MOVE_FLAG));
            }
            drag_point += fvec4(raw_translation, 1.0f);
            frame_translation += raw_translation;
        }

        if (ui_selection_rect->IsVisible())
        {
            ivec2 sz = ui_root->GetSize();
            fvec2 anchor_point(selection_rect.z_, selection_rect.w_);
            fvec2 new_sz = norm_mpos - anchor_point;

            // Set the selection rect position by default to the anchor point (point where first clicked on started the selection
            // rect), then if the rectangle formed by this point as the upper left corner and the new mouse pos is the lower left
            // corner makes a negative size (where down and to the right are positive), then we gotta set the selection rect position
            // at the new mouse pos and make the size = abs(size)
            selection_rect.x_ = anchor_point.x_;
            selection_rect.y_ = anchor_point.y_;
            if (new_sz.x_ < 0.0f)
                selection_rect.x_ = norm_mpos.x_;
            if (new_sz.y_ < 0.0f)
                selection_rect.y_ = norm_mpos.y_;

            fvec2 norm_sel_pos(selection_rect.x_, selection_rect.y_);
            // Convert set the selection rect position and size in pixels (multiply the norm coords by root element pixel size)
            ui_selection_rect->SetSize(ivec2(Abs(new_sz.x_) * sz.x_, Abs(new_sz.y_) * sz.y_));
            ui_selection_rect->SetPosition(
                ivec2(selection_rect.x_ * sz.x_, selection_rect.y_ * sz.y_));
            add_to_selection_from_rect();
        }
    }
    else if (name == hashes[4])
    {
        if (state == t_begin)
        {
            // The Z and W components are the anchor point for the selection rectangle
            selection_rect.x_ = selection_rect.z_ = norm_mpos.x_;
            selection_rect.y_ = selection_rect.w_ = norm_mpos.y_;

            ivec2 sz = ui_root->GetSize();
            ivec2 sz_c(norm_mpos.x_ * sz.x_, norm_mpos.y_ * sz.y_);
            ui_selection_rect->SetPosition(sz_c);
            ui_selection_rect->SetSize(ivec2(0, 0));
            ui_selection_rect->SetVisible(true);
        }
        else
        {
            if (ui_selection_rect->IsVisible())
            {
                // copy our selection and clear it
                auto sel_iter = sel_rect_selection.Begin();
                while (sel_iter != sel_rect_selection.End())
                {
                    auto & node_vec = selection[sel_iter->first_];
                    for (int i = 0; i < sel_iter->second_.Size(); ++i)
                    {
                        if (!node_vec.Contains(sel_iter->second_[i]))
                            node_vec.Push(sel_iter->second_[i]);
                    }
                    ++sel_iter;
                }
                cached_raycasts.Clear();
                ui_selection_rect->SetVisible(false);
            }
        }
    }
    else if (name == hashes[5])
    {
        if (state == t_begin)
            movement_flag |= Z_MOVE_FLAG;
        else
            movement_flag &= ~Z_MOVE_FLAG;
    }
    else if (name == hashes[6])
    {
        if (state == t_begin)
            movement_flag |= X_MOVE_FLAG;
        else
            movement_flag &= ~X_MOVE_FLAG;
    }
    else if (name == hashes[7])
    {
        if (state == t_begin)
            movement_flag |= Y_MOVE_FLAG;
        else
            movement_flag &= ~Y_MOVE_FLAG;
    }
    else if (hashes.Contains(name))
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
                    if (!es->is_selected(nd))
                    {
                        clear_selection();
                        //es->set_selected(nd, true);
                        if (!selection[cr.node_].Contains(nd) && cr.node_ != nd)
                            selection[cr.node_].Push(nd);
                    }
                }
                else if (name == hashes[1])
                {
                    drag_point = fvec4();
                    if (state == t_begin)
                        drag_point = fvec4(cr.position_, 1.0f);
                    else
                    {
                        if (ui_selection_rect->IsVisible())
                        {
                            // copy our selection and clear it
                            auto sel_iter = sel_rect_selection.Begin();
                            while (sel_iter != sel_rect_selection.End())
                            {
                                auto & node_vec = selection[sel_iter->first_];
                                for (int i = 0; i < sel_iter->second_.Size(); ++i)
                                {
                                    if (!node_vec.Contains(sel_iter->second_[i]))
                                        node_vec.Push(sel_iter->second_[i]);
                                }
                                ++sel_iter;
                            }

                            cached_raycasts.Clear();
                            ui_selection_rect->SetVisible(false);
                        }
                        if (!move_allowed)
                        {
                            translate_selection(-total_drag_translation);
                        }
                        total_drag_translation = fvec3();
                        do_snap = true;
                    }
                }
                else if (name == hashes[2])
                {
                    if (es->is_selected(nd))
                    {
                        remove_from_selection(nd);
                    }
                    else
                    {
                        //es->set_selected(nd, true);
                        if (!selection[cr.node_].Contains(nd) && cr.node_ != nd)
                            selection[cr.node_].Push(nd);
                    }
                }
            }
            else if (name == hashes[0])
            {
                drag_point = fvec4();
                clear_selection();
            }
        }

        if (res.Empty())
        {
            if (name == hashes[0])
            {
                drag_point = fvec4();
                clear_selection();
            }

            if (state == t_begin)
            {
                // The Z and W components are the anchor point for the selection rectangle
                selection_rect.x_ = selection_rect.z_ = norm_mpos.x_;
                selection_rect.y_ = selection_rect.w_ = norm_mpos.y_;

                ivec2 sz = ui_root->GetSize();
                ivec2 sz_c(norm_mpos.x_ * sz.x_, norm_mpos.y_ * sz.y_);
                ui_selection_rect->SetPosition(sz_c);
                ui_selection_rect->SetSize(ivec2(0, 0));
                ui_selection_rect->SetVisible(true);
            }
            else
            {
                if (ui_selection_rect->IsVisible())
                {
                    // copy our selection and clear it
                    auto sel_iter = sel_rect_selection.Begin();
                    while (sel_iter != sel_rect_selection.End())
                    {
                        auto & node_vec = selection[sel_iter->first_];
                        for (int i = 0; i < sel_iter->second_.Size(); ++i)
                        {
                            if (!node_vec.Contains(sel_iter->second_[i]))
                                node_vec.Push(sel_iter->second_[i]);
                        }
                        ++sel_iter;
                    }

                    cached_raycasts.Clear();
                    ui_selection_rect->SetVisible(false);
                }
            }
        }
    }
}

void EditorSelectionController::register_context(Urho3D::Context * ctxt)
{
    ctxt->RegisterFactory<EditorSelectionController>();
}