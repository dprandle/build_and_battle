#ifndef EDITOR_SELECTION_CONTROLLER_H
#define EDITOR_SELECTION_CONTROLLER_H

#include <Urho3D/Core/Object.h>

namespace Urho3D
{
class Node;
class Camera;
class Scene;
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

    void set_camera(Urho3D::Camera * cam);

    void set_scene(Urho3D::Scene * scn);

    Urho3D::Camera * get_camera();

    Urho3D::Scene * get_scene();

    void handle_update(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);

    void handle_input_event(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);

    void setup_input_context(input_context * ctxt);

  private:

    Urho3D::Camera * cam_comp;

    Urho3D::Scene * scene;

};
#endif
