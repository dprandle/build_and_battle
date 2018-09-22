#include <hex_tile_grid.h>
#include <tile_occupier.h>
#include <mtdebug_print.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;

Hex_Tile_Grid::Hex_Tile_Grid(Urho3D::Context * context) : Component(context)
{
    SubscribeToEvent(E_COMPONENTADDED, URHO3D_HANDLER(Hex_Tile_Grid, handle_component_added));
    SubscribeToEvent(E_COMPONENTREMOVED, URHO3D_HANDLER(Hex_Tile_Grid, handle_component_removed));

    init();
}

Hex_Tile_Grid::~Hex_Tile_Grid()
{
    release();
}

void Hex_Tile_Grid::release()
{
    m_world_map.Clear();
}

void Hex_Tile_Grid::init()
{
    m_world_map.Resize(QUADRANT_COUNT);
    for (uint32_t i = 0; i < QUADRANT_COUNT; ++i)
    {
        m_world_map[i].Resize(DEFAULT_GRID_SIZE);
        for (uint32_t j = 0; j < DEFAULT_GRID_SIZE; ++j)
        {
            m_world_map[i][j].Resize(DEFAULT_GRID_SIZE);
            for (uint32_t k = 0; k < DEFAULT_GRID_SIZE; ++k)
                m_world_map[i][j][k].Resize(DEFAULT_GRID_SIZE);
        }
    }
}

void Hex_Tile_Grid::add(const Tile_Item & pItem, const fvec3 & pPos)
{
    return add(pItem, ivec3(), pPos);
}

void Hex_Tile_Grid::add(const Tile_Item & pItem, const ivec3 & pSpace, const fvec3 & pOrigin)
{
    ivec3 space = pSpace + world_to_grid(pOrigin);
    Map_Index ind = grid_to_index(space);

    // If space is out of bounds, allocate more memory to accomadate
    if (!_check_bounds(ind))
        _resize_for_space(ind);

    if (!m_world_map[ind.quad_index][ind.z_][ind.y_][ind.x_].Contains(pItem))
        m_world_map[ind.quad_index][ind.z_][ind.y_][ind.x_].Push(pItem);
}

void Hex_Tile_Grid::add(const Tile_Item & pItem,
                        const Urho3D::Vector<ivec3> & pSpaces,
                        const fvec3 & pOrigin)
{
    for (uint32_t i = 0; i < pSpaces.Size(); ++i)
        add(pItem, pSpaces[i], pOrigin);
}

Urho3D::Vector<Hex_Tile_Grid::Tile_Space>
Hex_Tile_Grid::get_spaces_with_item(const Tile_Item & item)
{
    Urho3D::Vector<Hex_Tile_Grid::Tile_Space> ret;
    for (uint32_t i = 0; i < QUADRANT_COUNT; ++i)
    {
        for (uint32_t z = 0; z < m_world_map[i].Size(); ++z)
        {
            for (uint32_t y = 0; y < m_world_map[i][z].Size(); ++y)
            {
                for (uint32_t x = 0; x < m_world_map[i][z][y].Size(); ++x)
                {
                    if (!m_world_map[i][z][y][x].Empty())
                        ret.Push(m_world_map[i][z][y][x]);
                }
            }
        }
    }
    return ret;
}

const Hex_Tile_Grid::Tile_Space & Hex_Tile_Grid::at(const Map_Index & pSpace) const
{
    if (!_check_bounds(pSpace))
        return dummy_ret;

    return m_world_map[pSpace.quad_index][pSpace.z_][pSpace.y_][pSpace.x_];
}

Hex_Tile_Grid::Tile_Space Hex_Tile_Grid::get(const fvec3 & pPos) const
{
    return get(ivec3(), pPos);
}

Hex_Tile_Grid::Tile_Space Hex_Tile_Grid::get(const ivec3 & pSpace, const fvec3 & pOrigin) const
{
    if (!occupied(pSpace, pOrigin)) // also will check bounds here and return false if out of bounds
        return Tile_Space();

    ivec3 space = pSpace + world_to_grid(pOrigin);
    Map_Index ind = grid_to_index(space);

    // safe to raw index access because we know the indices are in bounds of the quadrant
    return m_world_map[ind.quad_index][ind.z_][ind.y_][ind.x_];
}

int32_t Hex_Tile_Grid::min_layer()
{
    int32_t minVal = 0;
    for (uint32_t i = 3; i < 8; ++i)
    {
        int32_t size = static_cast<int32_t>(m_world_map[i].Size());
        size *= -1;
        if (size < minVal)
            minVal = size;
    }
    return minVal;
}

int32_t Hex_Tile_Grid::max_layer()
{
    int32_t maxVal = 0;
    for (uint32_t i = 0; i < 4; ++i)
    {
        int32_t size = static_cast<int32_t>(m_world_map[i].Size());
        size -= 1;
        if (size > maxVal)
            maxVal = size;
    }
    return maxVal;
}

int32_t Hex_Tile_Grid::min_y()
{
    int32_t minY = 0;

    for (uint32_t i = 0; i < m_world_map[2].Size(); ++i)
    {
        int32_t lowestIndex = static_cast<int32_t>(m_world_map[2][i].Size()) * -1;
        if (lowestIndex < minY)
            minY = lowestIndex;
    }

    for (uint32_t i = 0; i < m_world_map[3].Size(); ++i)
    {
        int32_t lowestIndex = static_cast<int32_t>(m_world_map[3][i].Size()) * -1;
        if (lowestIndex < minY)
            minY = lowestIndex;
    }

    for (uint32_t i = 0; i < m_world_map[6].Size(); ++i)
    {
        int32_t lowestIndex = static_cast<int32_t>(m_world_map[6][i].Size()) * -1;
        if (lowestIndex < minY)
            minY = lowestIndex;
    }

    for (uint32_t i = 0; i < m_world_map[7].Size(); ++i)
    {
        int32_t lowestIndex = static_cast<int32_t>(m_world_map[7][i].Size()) * -1;
        if (lowestIndex < minY)
            minY = lowestIndex;
    }
    return minY;
}

int32_t Hex_Tile_Grid::max_y()
{
    int32_t maxY = 0;

    for (uint32_t i = 0; i < m_world_map[0].Size(); ++i)
    {
        int32_t highestIndex = static_cast<int32_t>(m_world_map[0][i].Size()) - 1;
        if (highestIndex > maxY)
            maxY = highestIndex;
    }

    for (uint32_t i = 0; i < m_world_map[1].Size(); ++i)
    {
        int32_t highestIndex = static_cast<int32_t>(m_world_map[1][i].Size()) - 1;
        if (highestIndex > maxY)
            maxY = highestIndex;
    }

    for (uint32_t i = 0; i < m_world_map[4].Size(); ++i)
    {
        int32_t highestIndex = static_cast<int32_t>(m_world_map[4][i].Size()) - 1;
        if (highestIndex > maxY)
            maxY = highestIndex;
    }

    for (uint32_t i = 0; i < m_world_map[5].Size(); ++i)
    {
        int32_t highestIndex = static_cast<int32_t>(m_world_map[5][i].Size()) - 1;
        if (highestIndex > maxY)
            maxY = highestIndex;
    }
    return maxY;
}

int32_t Hex_Tile_Grid::min_x()
{
    int32_t minX = 0;

    for (uint32_t i = 0; i < m_world_map[1].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[1][i].Size(); ++j)
        {
            int32_t lowestIndex = static_cast<int32_t>(m_world_map[1][i][j].Size()) * -1;
            if (lowestIndex < minX)
                minX = lowestIndex;
        }
    }

    for (uint32_t i = 0; i < m_world_map[3].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[3][i].Size(); ++j)
        {
            int32_t lowestIndex = static_cast<int32_t>(m_world_map[3][i][j].Size()) * -1;
            if (lowestIndex < minX)
                minX = lowestIndex;
        }
    }

    for (uint32_t i = 0; i < m_world_map[5].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[5][i].Size(); ++j)
        {
            int32_t lowestIndex = static_cast<int32_t>(m_world_map[5][i][j].Size()) * -1;
            if (lowestIndex < minX)
                minX = lowestIndex;
        }
    }

    for (uint32_t i = 0; i < m_world_map[7].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[7][i].Size(); ++j)
        {
            int32_t lowestIndex = static_cast<int32_t>(m_world_map[7][i][j].Size()) * -1;
            if (lowestIndex < minX)
                minX = lowestIndex;
        }
    }
    return minX;
}

int32_t Hex_Tile_Grid::max_x()
{
    int32_t maxX = 0;

    for (uint32_t i = 0; i < m_world_map[0].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[0][i].Size(); ++j)
        {
            int32_t highestIndex = static_cast<int32_t>(m_world_map[0][i][j].Size()) - 1;
            if (highestIndex > maxX)
                maxX = highestIndex;
        }
    }

    for (uint32_t i = 0; i < m_world_map[2].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[2][i].Size(); ++j)
        {
            int32_t highestIndex = static_cast<int32_t>(m_world_map[2][i][j].Size()) - 1;
            if (highestIndex > maxX)
                maxX = highestIndex;
        }
    }

    for (uint32_t i = 0; i < m_world_map[4].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[4][i].Size(); ++j)
        {
            int32_t highestIndex = static_cast<int32_t>(m_world_map[4][i][j].Size()) - 1;
            if (highestIndex > maxX)
                maxX = highestIndex;
        }
    }

    for (uint32_t i = 0; i < m_world_map[6].Size(); ++i)
    {
        for (uint32_t j = 0; j < m_world_map[6][i].Size(); ++j)
        {
            int32_t highestIndex = static_cast<int32_t>(m_world_map[6][i][j].Size()) - 1;
            if (highestIndex > maxX)
                maxX = highestIndex;
        }
    }
    return maxX;
}

Hex_Tile_Grid::Grid_Bounds Hex_Tile_Grid::occupied_bounds()
{
    Grid_Bounds g;
    for (uint32_t i = 0; i < 8; ++i)
    {
        for (uint32_t z = 0; z < m_world_map[i].Size(); ++z)
        {
            for (uint32_t y = 0; y < m_world_map[i][z].Size(); ++y)
            {
                for (uint32_t x = 0; x < m_world_map[i][z][y].Size(); ++x)
                {
                    if (!m_world_map[i][z][y][x].Empty())
                    {
                        ivec3 gridPos = index_to_grid(Map_Index(i, x, y, z));
                        if (gridPos.x_ > g.max_space.x_)
                            g.max_space.x_ = gridPos.x_;
                        if (gridPos.y_ > g.max_space.y_)
                            g.max_space.y_ = gridPos.y_;
                        if (gridPos.z_ > g.max_space.z_)
                            g.max_space.z_ = gridPos.z_;

                        if (gridPos.x_ < g.min_space.x_)
                            g.min_space.x_ = gridPos.x_;
                        if (gridPos.y_ < g.min_space.y_)
                            g.min_space.y_ = gridPos.y_;
                        if (gridPos.z_ < g.min_space.z_)
                            g.min_space.z_ = gridPos.z_;
                    }
                }
            }
        }
    }
    return g;
}

bool Hex_Tile_Grid::occupied(const fvec3 & pPos,
                             const Urho3D::Vector<Tile_Item> & allowed_items) const
{
    return occupied(ivec3(), pPos, allowed_items);
}

bool Hex_Tile_Grid::occupied(const ivec3 & pSpace,
                             const fvec3 & pOrigin,
                             const Urho3D::Vector<Tile_Item> & allowed_items) const
{
    ivec3 space = pSpace + world_to_grid(pOrigin);
    Map_Index ind = grid_to_index(space);

    if (!_check_bounds(ind))
        return false;

    const Urho3D::Vector<Tile_Item> & item_vec =
        m_world_map[ind.quad_index][ind.z_][ind.y_][ind.x_];
    for (int i = 0; i < item_vec.Size(); ++i)
    {
        if (!allowed_items.Contains(item_vec[i]))
            return true;
    }
    return false;
}

Urho3D::Vector<int> Hex_Tile_Grid::occupied(const Urho3D::Vector<ivec3> & pSpaces,
                                            const fvec3 & pOrigin,
                                            const Urho3D::Vector<Tile_Item> & allowed_items) const
{
    Urho3D::Vector<int> ret;
    for (uint32_t i = 0; i < pSpaces.Size(); ++i)
    {
        if (occupied(pSpaces[i], pOrigin, allowed_items))
            ret.Push(i);
    }
    return ret;
}

bool Hex_Tile_Grid::remove(const fvec3 & pPos, const Urho3D::Vector<Tile_Item> & items)
{
    return remove(ivec3(), pPos, items);
}

bool Hex_Tile_Grid::remove(const ivec3 & pSpace,
                           const fvec3 & pOrigin,
                           const Urho3D::Vector<Tile_Item> & items)
{
    bool ret = false;

    ivec3 space = pSpace + world_to_grid(pOrigin);
    Map_Index ind = grid_to_index(space);

    if (!_check_bounds(ind))
        return ret;

    Tile_Space & tile_space = m_world_map[ind.quad_index][ind.z_][ind.y_][ind.x_];

    if (items.Empty())
        tile_space.Clear();
    else
    {
        for(auto & item : items)
            ret = ret || tile_space.Remove(item);
    }
        
    return ret;
}

bool Hex_Tile_Grid::remove(const fvec3 & pos_, const Tile_Item & tile_item)
{
    Urho3D::Vector<Tile_Item> titems;
    if (tile_item.node_id_ != -1)
        titems.Push(tile_item);
    return remove(pos_, titems);
}

bool Hex_Tile_Grid::remove(const ivec3 & space_, const fvec3 & origin_, const Tile_Item & tile_item)
{
    Urho3D::Vector<Tile_Item> titems;
    if (tile_item.node_id_ != -1)
        titems.Push(tile_item);
    return remove(space_, origin_, titems);
}

Urho3D::Vector<int> Hex_Tile_Grid::remove(const Urho3D::Vector<ivec3> & spaces_,
                                          const fvec3 & origin_,
                                          const Tile_Item & tile_item)
{
    Urho3D::Vector<Tile_Item> titems;
    if (tile_item.node_id_ != -1)
        titems.Push(tile_item);
    return remove(spaces_, origin_, titems);
}

/*!
Remove multiple tiles from the grid - each space will be shifted by the grid position of the origin
*/
Urho3D::Vector<int> Hex_Tile_Grid::remove(const Urho3D::Vector<ivec3> & pSpaces,
                                          const fvec3 & pOrigin,
                                          const Urho3D::Vector<Tile_Item> & items)
{
    Urho3D::Vector<int> ret;
    for (uint32_t i = 0; i < pSpaces.Size(); ++i)
    {
        if (remove(pSpaces[i], pOrigin, items))
            ret.Push(i);
    }
    return ret;
}

void Hex_Tile_Grid::id_change(const Tile_Item & oldid, const Tile_Item newid)
{
    // Go through entire grid and remove any entrees with entity ID equal to above
    Grid_Bounds g;
    for (uint32_t i = 0; i < QUADRANT_COUNT; ++i)
    {
        for (uint32_t z = 0; z < m_world_map[i].Size(); ++z)
        {
            for (uint32_t y = 0; y < m_world_map[i][z].Size(); ++y)
            {
                for (uint32_t x = 0; x < m_world_map[i][z][y].Size(); ++x)
                {
                    m_world_map[i][z][y][x].Remove(oldid);
                    m_world_map[i][z][y][x].Push(newid);
                }
            }
        }
    }
}

void Hex_Tile_Grid::OnSceneSet(Urho3D::Scene * scene)
{
    if (GetScene() != scene && GetScene() != nullptr)
    {
        release();
        init();
    }

    Component::OnSceneSet(scene);

    PODVector<Node *> node_vec;
    scene->GetChildrenWithComponent<Tile_Occupier>(node_vec, true);
    for (auto nd : node_vec)
        _add_component(nd->GetComponent<Tile_Occupier>());
}

Urho3D::Vector<Hex_Tile_Grid::Tile_Space> Hex_Tile_Grid::bounded_set(const fvec3 & pPoint1,
                                                                     const fvec3 & pPoint2)
{
    Urho3D::Vector<Tile_Space> retSet;
    fvec3 min(pPoint1);
    fvec3 max(pPoint2);

    // Make sure we get the minimum and maximum points so that we can loop through
    // from the min to the max without having an infinite loop
    if (min.z_ > max.z_)
    {
        max.z_ = min.z_;
        min.z_ = pPoint2.z_;
    }
    if (min.y_ > max.y_)
    {
        max.y_ = min.y_;
        min.y_ = pPoint2.y_;
    }
    if (min.x_ > max.x_)
    {
        max.x_ = min.x_;
        min.x_ = pPoint2.x_;
    }

    ivec3 minGrid = world_to_grid(min);
    ivec3 maxGrid = world_to_grid(max);

    // Now loop through the grid and check if occupied - if occupied then add
    // the id to the id set
    for (; minGrid.z_ <= maxGrid.z_; ++minGrid.z_)
    {
        for (; minGrid.y_ <= maxGrid.y_; ++minGrid.y_)
        {
            for (; minGrid.x_ <= maxGrid.x_; ++minGrid.x_)
            {
                Map_Index ind = grid_to_index(minGrid);
                const Tile_Space & id = _get_id(ind);
                if (id.Empty())
                    continue;
                retSet.Push(id);
            }
        }
    }
    return retSet;
}

int32_t Hex_Tile_Grid::index_x(float pX, bool pOffset)
{
    if (pOffset)
        return int32_t(std::round(0.5f * (pX - X_GRID) / X_GRID));

    return int32_t(std::round(0.5f * pX / X_GRID));
}

int32_t Hex_Tile_Grid::index_y(float pY)
{
    return int32_t(std::round(pY / Y_GRID));
}

int32_t Hex_Tile_Grid::index_z(float pZ)
{
    return int32_t(std::round(pZ / Z_GRID));
}

Hex_Tile_Grid::Map_Index Hex_Tile_Grid::grid_to_index(const ivec3 & pSpace)
{
    Map_Index index;
    if (pSpace.z_ < 0)
    {
        index.quad_index = Quadrant_Index(index.quad_index + 4);
        index.z_ = -1 * pSpace.z_;
        index.z_ -= 1;
    }
    else
        index.z_ = pSpace.z_;
    if (pSpace.y_ < 0)
    {
        index.quad_index = Quadrant_Index(index.quad_index + 2);
        index.y_ = -1 * pSpace.y_;
        index.y_ -= 1;
    }
    else
        index.y_ = pSpace.y_;
    if (pSpace.x_ < 0)
    {
        index.quad_index = Quadrant_Index(index.quad_index + 1);
        index.x_ = -1 * pSpace.x_;
        index.x_ -= 1;
    }
    else
        index.x_ = pSpace.x_;
    return index;
}

fvec3 Hex_Tile_Grid::grid_to_world(const ivec3 & pSpace, const fvec3 & pOrigin)
{
    ivec3 space = pSpace + world_to_grid(pOrigin);
    fvec3 pos(space.x_ * 2.0f * X_GRID, space.y_ * Y_GRID, space.z_ * Z_GRID);
    if (space.y_ % 2 != 0)
        pos.x_ += X_GRID;
    return pos;
}

ivec3 Hex_Tile_Grid::index_to_grid(const Map_Index & pIndex)
{
    ivec3 grid;

    if (pIndex.quad_index < 4)
        grid.z_ = pIndex.z_;
    else
        grid.z_ = -1 * (pIndex.z_ + 1);

    if (pIndex.quad_index % 4 < 2)
        grid.y_ = pIndex.y_;
    else
        grid.y_ = -1 * (pIndex.y_ + 1);

    if (pIndex.quad_index % 2 == 0)
        grid.x_ = pIndex.x_;
    else
        grid.x_ = -1 * (pIndex.x_ + 1);

    return grid;
}

fvec3 Hex_Tile_Grid::index_to_world(const Map_Index & pIndex)
{
    return grid_to_world(index_to_grid(pIndex));
}

void Hex_Tile_Grid::snap_to_grid(fvec3 & pToSnap)
{
    pToSnap = grid_to_world(world_to_grid(pToSnap));
}

Hex_Tile_Grid::Map_Index Hex_Tile_Grid::world_to_index(const fvec3 & pWorldPos)
{
    return grid_to_index(world_to_grid(pWorldPos));
}

ivec3 Hex_Tile_Grid::world_to_grid(const fvec3 & pWorldPos)
{
    ivec3 gPos;

    gPos.y_ = index_y(pWorldPos.y_);
    bool offset = (gPos.y_ % 2 != 0);
    gPos.x_ = index_x(pWorldPos.x_, offset);
    gPos.z_ = index_z(pWorldPos.z_);
    return gPos;
}

bool Hex_Tile_Grid::_check_bounds(const Map_Index & pIndex) const
{
    if (pIndex.z_ >= m_world_map[pIndex.quad_index].Size())
        return false;

    if (pIndex.y_ >= m_world_map[pIndex.quad_index][pIndex.z_].Size())
        return false;

    if (pIndex.x_ >= m_world_map[pIndex.quad_index][pIndex.z_][pIndex.y_].Size())
        return false;

    return true;
}

void Hex_Tile_Grid::_resize_for_space(const Map_Index & pIndex)
{
    uint32_t old_size = 0;
    uint32_t new_size = 0;

    old_size = m_world_map[pIndex.quad_index].Size();
    if (pIndex.z_ >= old_size)
    {
        new_size = pIndex.z_ + TILE_GRID_RESIZE_PAD;
        m_world_map[pIndex.quad_index].Resize(new_size);
        iout << "Resizing tile grid z layer from" << old_size << "to" << new_size;

        //Resize the x and y dimensions for all new layers
        for (uint32_t i = old_size-1; i < new_size; ++i)
        {
            m_world_map[pIndex.quad_index][i].Resize(DEFAULT_GRID_SIZE);
            for (uint32_t j = 0; j < DEFAULT_GRID_SIZE; ++j)
                m_world_map[pIndex.quad_index][i][j].Resize(DEFAULT_GRID_SIZE);
        }
    }

    old_size = m_world_map[pIndex.quad_index][pIndex.z_].Size();
    if (pIndex.y_ >= old_size)
    {
        new_size = pIndex.y_ + TILE_GRID_RESIZE_PAD;
        m_world_map[pIndex.quad_index][pIndex.z_].Resize(new_size);
        iout << "Resizing tile grid y layer from" << old_size << "to" << new_size;

        // Resize all the x dimensions for the current layer/row
        for (uint32_t i = old_size-1; i < new_size; ++i)
            m_world_map[pIndex.quad_index][pIndex.z_][i].Resize(DEFAULT_GRID_SIZE);
    }

    old_size = m_world_map[pIndex.quad_index][pIndex.z_][pIndex.y_].Size();
    if (pIndex.x_ >= old_size)
    {
        new_size = pIndex.x_ + TILE_GRID_RESIZE_PAD;
        m_world_map[pIndex.quad_index][pIndex.z_][pIndex.y_].Resize(new_size);
        iout << "Resizing tile grid x layer from" << old_size << "to" << new_size;
    }
}

void Hex_Tile_Grid::register_context(Urho3D::Context * context)
{
    context->RegisterFactory<Hex_Tile_Grid>();
    //   URHO3D_ATTRIBUTE("Map_World", Map_World, m_world_map, Map_World(), AM_DEFAULT);
}

const Hex_Tile_Grid::Tile_Space & Hex_Tile_Grid::_get_id(const Map_Index & pIndex)
{
    return m_world_map[pIndex.quad_index][pIndex.z_][pIndex.y_][pIndex.x_];
}

void Hex_Tile_Grid::handle_component_added(Urho3D::StringHash eventType,
                                           Urho3D::VariantMap & eventData)
{
    Scene * scn = static_cast<Scene *>(eventData[NodeRemoved::P_SCENE].GetPtr());
    if (GetScene() != scn)
        return;
    Component * comp = static_cast<Component *>(eventData[ComponentRemoved::P_COMPONENT].GetPtr());
    if (comp->IsInstanceOf<Tile_Occupier>())
        _add_component(static_cast<Tile_Occupier *>(comp));
}

void Hex_Tile_Grid::handle_component_removed(Urho3D::StringHash eventType,
                                             Urho3D::VariantMap & eventData)
{
    Scene * scn = static_cast<Scene *>(eventData[ComponentRemoved::P_SCENE].GetPtr());
    if (GetScene() != scn)
        return;
    Component * comp = static_cast<Component *>(eventData[ComponentRemoved::P_COMPONENT].GetPtr());
    if (comp->IsInstanceOf<Tile_Occupier>())
        _remove_component(static_cast<Tile_Occupier *>(comp));
}

void Hex_Tile_Grid::_add_component(Tile_Occupier * occ)
{
    scene_occ_comps.Insert(occ);
    add(Tile_Item(occ->GetNode()->GetID()), occ->tile_spaces(), occ->GetNode()->GetPosition());
}

void Hex_Tile_Grid::_remove_component(Tile_Occupier * occ)
{
    scene_occ_comps.Erase(occ);
    fvec3 fpos = occ->GetNode()->GetPosition();
    remove(occ->tile_spaces(), fpos, Tile_Item(occ->GetNode()->GetID()));
}

void Hex_Tile_Grid::DrawDebugGeometry(bool depth)
{
    Scene * scn = GetScene();
    if (scn == nullptr)
        return;

    DebugRenderer * deb = scn->GetComponent<DebugRenderer>();
    if (deb == nullptr)
        return;

    for (uint32_t i = 0; i < QUADRANT_COUNT; ++i)
    {
        for (uint32_t z = 0; z < m_world_map[i].Size(); ++z)
        {
            for (uint32_t y = 0; y < m_world_map[i][z].Size(); ++y)
            {
                for (uint32_t x = 0; x < m_world_map[i][z][y].Size(); ++x)
                {
                    for (int item_ind = 0; item_ind < m_world_map[i][z][y][x].Size(); ++item_ind)
                    {
                        BoundingBox bb;
                        Map_Index ind;

                        float mod = -1.0f * (item_ind % 2);
                        fvec3 pos = index_to_world(Map_Index(i, x, y, z)) +
                                    fvec3(mod * 0.1f, mod * 0.1f, 0.0f);
                        bb.min_ = fvec3(-0.25f, -0.25f, -0.15f) + pos;
                        bb.max_ = fvec3(0.25f, 0.25f, 0.15f) + pos;
                        deb->AddBoundingBox(bb, Color(1.0f, 0.0f, 0.0f), depth, false);
                    }
                }
            }
        }
    }

    auto occ_iter = scene_occ_comps.Begin();
    while (occ_iter != scene_occ_comps.End())
    {
        if ((*occ_iter)->debug_enabled())
            (*occ_iter)->DrawDebugGeometry(depth);
        ++occ_iter;
    }
}
