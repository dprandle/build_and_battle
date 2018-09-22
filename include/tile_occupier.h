#pragma once

#include <Urho3D/Scene/Component.h>
#include <math_utils.h>

namespace Urho3D
{
class Model;
}

class Tile_Occupier : public Urho3D::Component
{
    URHO3D_OBJECT(Tile_Occupier, Component);

  public:
    Tile_Occupier(Urho3D::Context * context);
    ~Tile_Occupier();

    void build_from_model(Urho3D::Model * model);

    void add(const ivec3 & grid);

    void remove(const ivec3 & grid);

    const Urho3D::Vector<ivec3> & tile_spaces();

    static void register_context(Urho3D::Context * ctxt);

    void enable_debug(bool enable);

    bool debug_enabled();

    void DrawDebugGeometry(bool depth);

  protected:
    
    void OnNodeSet(Urho3D::Node* node) override;

    void OnMarkedDirty(Urho3D::Node* node) override;
    
  private:

    fvec3 m_old_position;

    bool draw_debug;

    Urho3D::Vector<ivec3> spaces;
};