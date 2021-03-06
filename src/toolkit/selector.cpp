#include <Urho3D/Scene/Node.h>

#include <urho_common.h>
#include <string>

#include "selector.h"

namespace bbtk
{

Selector::Selector(Urho3D::Context * context)
    : Urho3D::StaticModel(context)
{}

void Selector::set_selected(bool select)
{
    Urho3D::Scene * scn = GetScene();
    if (scn == nullptr)
        return;

    Urho3D::StaticModel * comp = node_->GetComponent<StaticModel>();

    if ((select && is_selected()) || (!select && !is_selected()))
        return;

    if (comp != nullptr)
        comp->SetEnabled(!select);

    SetEnabled(select);
}

bool Selector::is_selected()
{
    return IsEnabled();
}

void Selector::toggle_selected()
{
    set_selected(!is_selected());
}

void Selector::register_context(Urho3D::Context * context)
{
    context->RegisterFactory<Selector>();
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, Urho3D::AM_DEFAULT);
}
}