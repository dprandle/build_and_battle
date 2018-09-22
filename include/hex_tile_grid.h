#pragma once

const float X_GRID = 0.866f;
const float Y_GRID = 1.5f;
const float Z_GRID = 0.45f;
const int QUADRANT_COUNT = 8;
const int DEFAULT_GRID_SIZE = 32;
const int TILE_GRID_RESIZE_PAD = 8;

#include <Urho3D/Scene/Component.h>
#include <math_utils.h>

namespace Urho3D
{
    class Scene;
    class Node;
}

class Tile_Occupier;

class Hex_Tile_Grid : public Urho3D::Component
{
    URHO3D_OBJECT(Hex_Tile_Grid, Urho3D::Component);

  public:
    enum Quadrant_Index
    {
        top_right_front,
        top_left_front,
        top_right_back,
        top_left_back,
        bottom_right_front,
        bottom_left_front,
        bottom_right_back,
        bottom_left_back
    };

    struct Tile_Item
    {
        Tile_Item(int node_id = -1) : node_id_(node_id)
        {}

        bool operator==(const Tile_Item & rhs) const
        {
            return (node_id_ == rhs.node_id_);
        }

        bool operator!=(const Tile_Item & rhs) const
        {
            return !(*this == rhs);
        }

        int node_id_;
    };

    struct Grid_Bounds
    {
        ivec3 min_space;
        ivec3 max_space;
    };

    struct Map_Index
    {
        Map_Index(uint32_t quadrant_ = 0, uint32_t x = 0, uint32_t y = 0, uint32_t z = 0)
            : quad_index(Quadrant_Index(quadrant_)), x_(x), y_(y), z_(z)
        {}

        Quadrant_Index quad_index;
        uint32_t x_;
        uint32_t y_;
        uint32_t z_;
    };

    using Tile_Space = Urho3D::Vector<Tile_Item>;
    using Map_Layer = Urho3D::Vector<Urho3D::Vector<Tile_Space>>;
    using Map_Quadrant = Urho3D::Vector<Map_Layer>;
    using Map_World = Urho3D::Vector<Map_Quadrant>;

    Hex_Tile_Grid(Urho3D::Context * context);

    ~Hex_Tile_Grid();

    void add(const Tile_Item & item_, const fvec3 & pos_);

    void add(const Tile_Item & item_, const ivec3 & space_, const fvec3 & origin_ = fvec3());

    void add(const Tile_Item & item_,
             const Urho3D::Vector<ivec3> & spaces_,
             const fvec3 & origin_ = fvec3());

    const Tile_Space & at(const Map_Index & space_) const;

    void init();

    void release();

    Tile_Space get(const fvec3 & pos_) const;

    Tile_Space get(const ivec3 & space_, const fvec3 & origin_ = fvec3()) const;

    Urho3D::Vector<Tile_Space> get_spaces_with_item(const Tile_Item & item);

    Urho3D::Vector<Tile_Space> bounded_set(const fvec3 & point1_, const fvec3 & point2_);

    Grid_Bounds occupied_bounds();

    int32_t min_layer();

    int32_t max_layer();

    int32_t min_y();

    int32_t max_y();

    int32_t min_x();

    int32_t max_x();

    bool
    occupied(const fvec3 & pos_,
             const Urho3D::Vector<Tile_Item> & allowed_items = Urho3D::Vector<Tile_Item>()) const;

    bool
    occupied(const ivec3 & space_,
             const fvec3 & origin_ = fvec3(),
             const Urho3D::Vector<Tile_Item> & allowed_items = Urho3D::Vector<Tile_Item>()) const;

    Urho3D::Vector<int>
    occupied(const Urho3D::Vector<ivec3> & spaces_,
             const fvec3 & origin_ = fvec3(),
             const Urho3D::Vector<Tile_Item> & allowed_items = Urho3D::Vector<Tile_Item>()) const;


    bool remove(const fvec3 & pos_, const Tile_Item & tile_item = Tile_Item());

    bool remove(const ivec3 & space_, const fvec3 & origin_ = fvec3(),const Tile_Item & tile_item = Tile_Item());

    Urho3D::Vector<int> remove(const Urho3D::Vector<ivec3> & spaces_,
                               const fvec3 & origin_ = fvec3(), const Tile_Item & tile_item = Tile_Item());

    bool remove(const fvec3 & pos_, const Urho3D::Vector<Tile_Item> & items = Urho3D::Vector<Tile_Item>());

    bool remove(const ivec3 & space_, const fvec3 & origin_ = fvec3(), const Urho3D::Vector<Tile_Item> & items = Urho3D::Vector<Tile_Item>());

    Urho3D::Vector<int> remove(const Urho3D::Vector<ivec3> & spaces_,
                               const fvec3 & origin_ = fvec3(), const Urho3D::Vector<Tile_Item> & items = Urho3D::Vector<Tile_Item>());

    static int32_t index_x(float x_, bool offset_);

    static int32_t index_y(float y_);

    static int32_t index_z(float z_);

    static ivec3 world_to_grid(const fvec3 & world_);

    static Map_Index world_to_index(const fvec3 & world_);

    static Map_Index grid_to_index(const ivec3 & grid_);

    static fvec3 grid_to_world(const ivec3 & grid_, const fvec3 & origin_ = fvec3());

    static ivec3 index_to_grid(const Map_Index & index_);

    static fvec3 index_to_world(const Map_Index & index_);

    static void snap_to_grid(fvec3 & world_);

    static void register_context(Urho3D::Context * context);

    void handle_component_added(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);

    void handle_component_removed(Urho3D::StringHash eventType, Urho3D::VariantMap & eventData);

    void id_change(const Tile_Item & oldid, const Tile_Item newid);

    void DrawDebugGeometry(bool depth);

  protected:
    
    void OnSceneSet(Urho3D::Scene* scene) override;

    

  private:
    bool _check_bounds(const Map_Index & index_) const;

    void _resize_for_space(const Map_Index & index_);

    const Tile_Space & _get_id(const Map_Index & index_);

    void _add_component(Tile_Occupier * occ);

    void _remove_component(Tile_Occupier * occ);

    Tile_Space dummy_ret;

    Urho3D::HashSet<Tile_Occupier *> scene_occ_comps;

    Map_World m_world_map;
};