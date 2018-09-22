#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
class Node;
class Context;
class StaticModel;
}

using namespace Urho3D;

class EditorSelector : public Component
{
    URHO3D_OBJECT(EditorSelector, Component);

  public:

    /// Construct.
    EditorSelector(Context * context);

    void set_selected(Node * node, bool select);

    bool is_selected(Node * node);

    void toggle_selected(Node * node);

    void set_selection_material(const String & name);

    void set_render_component_to_control(int comp_id);

    const String & selection_material();

    static void register_context(Urho3D::Context * context);

    protected:

    void OnNodeSet(Urho3D::Node * node) override;

  protected:

    //void OnNodeSet(Node * node) override;

  private:

    int id_of_component_to_control;
    StaticModel * sel_render_comp;

    String selected_mat;

};