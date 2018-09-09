#include <editor_selector.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/StaticModelGroup.h>
#include <Urho3D/IO/Log.h>
#include <string>


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
    
    if ((select && is_selected(node)) || (!select && !is_selected(node)))
        return;

    StaticModelGroup * ogp = nullptr;
    StaticModelGroup * gp = static_cast<StaticModelGroup *>(comp);
    if (sel_render_comp->IsInstanceOf<StaticModelGroup>())
        ogp = static_cast<StaticModelGroup *>(sel_render_comp);

    URHO3D_LOGINFO("The selection node is " + String(uint64_t(node_)) + " and the passed in node is " + String(uint64_t(node)));
    // If node passed in is the same node that this component is attached to, it means either the node is a StaticModel
    // and we should disable our static model and enable the normal static model, or it means we are a static model group
    // and we should remove all instance nodes and add them back to the normal static model
    if (node == node_)
    {
        if (ogp == nullptr)
        {
            comp->SetEnabled(!select);
            ogp->SetEnabled(select);
        }
        else if (!select)
        {
            int cnt = ogp->GetNumInstanceNodes();
            for (int i = 0; i < cnt; ++i)
            {
                Node * nd = ogp->GetInstanceNode(i);
                gp->AddInstanceNode(nd);                
            }
            ogp->RemoveAllInstanceNodes();
        }
    }
    else if (ogp != nullptr)
    {
        if (select)
        {
            // Remove the instance node from the normal model and add it to the selection model
            gp->RemoveInstanceNode(node);
            ogp->AddInstanceNode(node);
        }
        else
        {
            ogp->RemoveInstanceNode(node);
            gp->AddInstanceNode(node);
        }
    }
}

bool EditorSelector::is_selected(Urho3D::Node * node)
{
    // If node is our node - it means we are not a static model group and so do not need to worry
    // about sub objects
    if (node == node_)
    {
        return sel_render_comp->IsEnabled();
    }
    else
    {
        // The node passed in is not the same node our static model is attached to (or static model group)
        // This means either the node is a sub object (if it is selected) or not if it is not selected
        StaticModelGroup * mg = static_cast<StaticModelGroup *>(sel_render_comp);
        int cnt = mg->GetNumInstanceNodes();
        for (int i = 0; i < cnt; ++i)
        {
            Node * nd = mg->GetInstanceNode(i);
            if (mg->GetInstanceNode(i) == node)
                return true;
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