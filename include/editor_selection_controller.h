#ifndef EDITOR_SELECTION_CONTROLLER_H
#define EDITOR_SELECTION_CONTROLLER_H

#include <Urho3D/Core/Object.h>
#include <typedefs.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/OcclusionBuffer.h>

#define SEL_OBJ_NAME "SelectObject"
#define DRAG_SELECTED_OBJECT "DragSelectedObject"
#define EXTEND_SEL_OBJ_NAME "ExtendObjectSelection"
#define DRAGGING "Dragging"
#define ENABLE_SELECTION_RECT "EnableSelectionRect"

namespace Urho3D
{
class Node;
class Camera;
class Scene;
class UIElement;
class BorderImage;
}

struct input_context;

class EditorSelectionController : public Urho3D::Object
{
    URHO3D_OBJECT(EditorSelectionController, Urho3D::Object)

  public:
    EditorSelectionController(Urho3D::Context * context);
    ~EditorSelectionController();

    void init();

    void release();

    void clear_selection();

    void move_selection(const fvec3 & amount);

    void remove_from_selection(Urho3D::Node* node);

    void set_camera(Urho3D::Camera * cam);

    void set_scene(Urho3D::Scene * scn);

    Urho3D::Camera * get_camera();

    Urho3D::Scene * get_scene();

    void handle_update(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);
    void handle_input_event(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);

    void setup_input_context(input_context * ctxt);

  private:

    HashMap<Urho3D::Node*, Vector<Urho3D::Node*>> selection;

    Vector<StringHash> hashes;

    Urho3D::Camera * cam_comp;

    Urho3D::Scene * scene;

    Urho3D::UIElement * ui_root;

    Urho3D::BorderImage * ui_selection_rect;

    fvec3 frame_translation;

    fvec4 drag_point;
    
    fvec4 selection_rect;

};
#endif
