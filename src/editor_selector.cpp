#include <editor_selector.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/StaticModelGroup.h>
#include <Urho3D/IO/Log.h>
#include <string>

using namespace Urho3D;

EditorSelector::EditorSelector(Urho3D::Context * context)
    : Component(context), id_of_component_to_control(-1), sel_render_comp(nullptr)
{}

void EditorSelector::set_selected(Urho3D::Node * node, bool select)
{
    Scene * scn = GetScene();
    if (scn == nullptr)
        return;
    StaticModel * comp = static_cast<StaticModel *>(scn->GetComponent(id_of_component_to_control));
    if (comp == nullptr)
        return;

    if (node == node_)
    {
        comp->SetEnabled(!select);
        sel_render_comp->SetEnabled(select);
    }
    else
    {
        StaticModelGroup * gp = static_cast<StaticModelGroup *>(comp);
        StaticModelGroup * ogp = static_cast<StaticModelGroup *>(sel_render_comp);
        if (select)
        {
            URHO3D_LOGINFO("Adding node " + node->GetName() + " (" +  String(uint64_t(node)) + ") to selection");
            gp->RemoveInstanceNode(node);
            ogp->AddInstanceNode(node);


            int cnt = ogp->GetNumInstanceNodes();
            for (int i = 0; i < cnt; ++i)
            {
                Node * nd = ogp->GetInstanceNode(i);
                URHO3D_LOGINFO("The selected node address is " + String((uint64_t)nd));
            }
        }
        else
        {
            URHO3D_LOGINFO("Adding node " + node->GetName() + " as normal obj instance and removing from selection");
            ogp->RemoveInstanceNode(node);
            gp->AddInstanceNode(node);
        }
    }
}

bool EditorSelector::is_selected(Urho3D::Node * node)
{
    if (node == node_)
    {
        return sel_render_comp->IsEnabled();
    }
    else
    {
        URHO3D_LOGINFO("The passed in node is " + String((uint64_t)node));
        StaticModelGroup * mg = static_cast<StaticModelGroup *>(sel_render_comp);
        int cnt = mg->GetNumInstanceNodes();
        for (int i = 0; i < cnt; ++i)
        {
            Node * nd = mg->GetInstanceNode(i);
            URHO3D_LOGINFO("The selected node address is " + String((uint64_t)nd));
            if (mg->GetInstanceNode(i) == node)
            {
                return true;
            }
        }
        return false;
    }
}

void EditorSelector::toggle_selected(Urho3D::Node * node)
{
    set_selected(node, !is_selected(node));
}

void EditorSelector::set_render_component_to_control(int comp_id)
{
    if (comp_id == id_of_component_to_control)
        return;

    id_of_component_to_control = comp_id;

    if (sel_render_comp != nullptr)
        node_->RemoveComponent(sel_render_comp);

    if (id_of_component_to_control == -1)
        sel_render_comp = nullptr;
    else
    {
        Component * cmp = GetScene()->GetComponent(comp_id);
        StaticModel * smcmp = static_cast<StaticModel *>(cmp);
        ResourceCache * cache = GetSubsystem<ResourceCache>();
        Material * mat = cache->GetResource<Material>(selected_mat);

        sel_render_comp = static_cast<StaticModel *>(node_->CreateComponent(cmp->GetType()));
        sel_render_comp->SetModel(smcmp->GetModel());
        sel_render_comp->SetMaterial(mat);
    }
}

void EditorSelector::set_selection_material(const Urho3D::String & name)
{
    selected_mat = name;
    if (sel_render_comp != nullptr)
    {
        ResourceCache * cache = GetSubsystem<ResourceCache>();
        Material * mat = cache->GetResource<Material>(selected_mat);
        sel_render_comp->SetMaterial(mat);
    }
}

const Urho3D::String & EditorSelector::selection_material()
{
    return selected_mat;
}