#ifndef EDITOR_SELECTION_CONTROLLER_H
#define EDITOR_SELECTION_CONTROLLER_H

#include <math_utils.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/OcclusionBuffer.h>
#include <Urho3D/Container/HashSet.h>
#include <Urho3D/Scene/Component.h>

#define SEL_OBJ_NAME "SelectObject"
#define DRAG_SELECTED_OBJECT "DragSelectedObject"
#define EXTEND_SEL_OBJ_NAME "ExtendObjectSelection"
#define DRAGGING "Dragging"
#define ENABLE_SELECTION_RECT "EnableSelectionRect"
#define Z_MOVE_HELD "ZMoveHeld"
#define X_MOVE_HELD "XMoveHeld"
#define Y_MOVE_HELD "YMoveHeld"
#define TOGGLE_OCC_DEBUG "ToggleOccDebug"

const Urho3D::Color SEL_RECT_BORDER_COL = Urho3D::Color(0.0f, 0.0f, 0.7f, 0.6f);
const int BORDER_SIZE = 1;
const Urho3D::Color SEL_RECT_COL = Urho3D::Color(0.0f, 0.0f, 0.7f, 0.2f);

namespace Urho3D
{
class Node;
class Camera;
class Scene;
class UIElement;
class BorderImage;
}
class EditorSelector;

const int Z_MOVE_FLAG = 1;
const int X_MOVE_FLAG = 2;
const int Y_MOVE_FLAG = 4;

struct input_context;

class EditorSelectionController : public Urho3D::Component
{
    URHO3D_OBJECT(EditorSelectionController, Urho3D::Component)

  public:
    EditorSelectionController(Urho3D::Context * context);
    ~EditorSelectionController();

    void clear_selection();

    bool is_selected(Urho3D::Node * obj_node, Urho3D::Node * sub_obj_node = nullptr);

    void snap_selection();

    void translate_selection(const fvec3 & translation);

    void toggle_occ_debug_selection();

    void remove_from_selection(Urho3D::Node * node);

    void set_camera(Urho3D::Camera * cam);

    Urho3D::Camera * get_camera();

    void DrawDebugGeometry(bool depthTest);

    void handle_update(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);
    void handle_input_event(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);
    void handle_component_added(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);
    void handle_component_removed(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);

    void setup_input_context(input_context * ctxt);

    static void register_context(Urho3D::Context * ctxt);

  protected:

    void OnSceneSet(Urho3D::Scene * scene) override;

  private:

    void add_to_selection_from_rect();

    HashMap<Urho3D::Node *, Vector<Urho3D::Node *>> selection;

    HashMap<Urho3D::Node *, Vector<Urho3D::Node *>> sel_rect_selection;

    Vector<StringHash> hashes;

    Urho3D::Camera * cam_comp;

    Urho3D::Scene * scene;

    Urho3D::UIElement * ui_root;

    Urho3D::UIElement * ui_selection_rect;

    HashMap<Urho3D::Node *, bool> cached_raycasts;

    Urho3D::HashSet<EditorSelector *> scene_sel_comps;

    fvec3 frame_translation;

    fvec4 drag_point;

    fvec4 selection_rect;

    fvec3 total_drag_translation;

    bool move_allowed;

    bool do_snap;

    int movement_flag;
};
#endif
