#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
class Node;
class Context;
class StaticModel;
}

class EditorSelector : public Urho3D::Component
{
    URHO3D_OBJECT(EditorSelector, Urho3D::Component);

  public:

    /// Construct.
    EditorSelector(Urho3D::Context * context);

    void set_selected(Urho3D::Node * node, bool select);

    bool is_selected(Urho3D::Node * node);

    void toggle_selected(Urho3D::Node * node);

    void set_selection_material(const Urho3D::String & name);

    void set_render_component_to_control(int comp_id);

    const Urho3D::String & selection_material();

  protected:

    //void OnNodeSet(Urho3D::Node * node) override;

  private:

    int id_of_component_to_control;
    Urho3D::StaticModel * sel_render_comp;

    Urho3D::String selected_mat;

};