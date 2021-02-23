#pragma once

#include <Urho3D/Graphics/StaticModel.h>

namespace Urho3D
{
class Node;
class Context;
class StaticModel;
} // namespace Urho3D

//using namespace Urho3D;

namespace bbtk
{

class Selector : public Urho3D::StaticModel
{
    URHO3D_OBJECT(Selector, Component);

  public:
    /// Construct.
    Selector(Urho3D::Context * context);

    void set_selected(bool select);

    bool is_selected();

    void toggle_selected();

    static void register_context(Urho3D::Context * context);

  protected:
    //void OnNodeSet(Node * node) override;

  private:
};
}