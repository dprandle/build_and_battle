#include <input_translator.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>

using namespace Urho3D;

bool input_context::add_trigger(
	input_action_trigger * trigger
	)
{
	// Make sure we have no absolute duplicates (not allowed due to stacking contexts)
	trigger_range tr = get_triggers(trigger->condition);
	while (tr.first != tr.second)
	{
		if (*(tr.first->second) == *trigger)
			return false;
		++tr.first;
	}

	t_map.emplace(trigger->condition.lookup_key,trigger);
	return true;
}

input_action_trigger * input_context::create_trigger(
	const trigger_condition & cond
	)
{
	input_action_trigger * trigger = new input_action_trigger;
	trigger->condition = cond;
	if (add_trigger(trigger))
		return trigger;
	delete trigger;
	return nullptr;
}

bool input_context::destroy_trigger(
	input_action_trigger * trigger
	)
{
	input_action_trigger * trg = remove_trigger(trigger);
	if (trg != nullptr)
	{
		delete trg;
		return true;
	}
	return false;	
}

bool input_context::destroy_triggers(
	const trigger_condition & cond
	)
{
	std::vector<input_action_trigger*> triggs = remove_triggers(cond);
	for (int i = 0; i < triggs.size(); ++i)
		delete triggs[i];
	if (triggs.empty())
		return false;
	return true;
}

input_action_trigger * input_context::remove_trigger(
	input_action_trigger * trigger
	)
{
	auto iter = t_map.begin();
	while (iter != t_map.end())
	{
		if (iter->second == trigger)
		{
			t_map.erase(iter);
			return trigger;
		}
		++iter;
	}
}

std::vector<input_action_trigger*> input_context::remove_triggers(
	const trigger_condition & cond
	)
{
	std::vector<input_action_trigger*> ret;
	trigger_range tr = get_triggers(cond);
	while (tr.first != tr.second)
	{
		ret.push_back(tr.first->second);
		tr.first = t_map.erase(tr.first);
	}
	return ret;
}
	
trigger_range input_context::get_triggers(
	const trigger_condition & cond
	)
{
	return t_map.equal_range(cond.lookup_key);
}
	
InputMap::InputMap()
{}

InputMap::InputMap(const InputMap & copy_)
{}

InputMap::~InputMap()
{}

InputMap & InputMap::operator=(InputMap rhs)
{}

bool InputMap::add_context(const Urho3D::StringHash & name, input_context * to_add)
{
	input_context * ctxt = get_context(name);
	if (ctxt == nullptr)
	{
		Pair<StringHash, input_context*> ins_pair;
		ins_pair.first_ = StringHash(name);
		ins_pair.second_ = to_add;
		m_contexts.Insert(ins_pair);
		return true;
	}
	return false;
}

input_context * InputMap::get_context(const Urho3D::StringHash & name)
{
	auto iter = m_contexts.Find(name);
	if (iter != m_contexts.End())
		return iter->second_;
	return nullptr;
}
	
input_context * InputMap::create_context(const Urho3D::StringHash & name)
{
	input_context * ctxt = new input_context;
	if (!add_context(name, ctxt))
	{
		delete ctxt;
		return nullptr;
	}
	return ctxt;
}

bool InputMap::destroy_context(const Urho3D::StringHash & name)
{
	input_context * ic = remove_context(name);
	if (ic != nullptr)
	{
		delete ic;
		return true;
	}
	return false;
}

bool InputMap::destroy_context(input_context * to_destroy)
{
	to_destroy = remove_context(to_destroy);
	if (to_destroy != nullptr)
	{
		delete to_destroy;
		return true;
	}
	return false;
}

input_context * InputMap::remove_context(const Urho3D::StringHash & name)
{
	auto iter = m_contexts.Find(name);
	if (iter != m_contexts.End())
	{
		input_context * ic = iter->second_;
		m_contexts.Erase(iter);
		return ic;
	}
	return nullptr;
}

input_context * InputMap::remove_context(input_context * to_remove)
{
	auto iter = m_contexts.Begin();
	while (iter != m_contexts.End())
	{
		if (iter->second_ == to_remove)
		{
			m_contexts.Erase(iter);
			return to_remove;
		}
		++iter;
	}
	return nullptr;
}

bool InputMap::rename_context(const Urho3D::StringHash & old_name, const Urho3D::StringHash & new_name)
{
	input_context * ic = remove_context(old_name);
	if (ic == nullptr)
		return false;
	return add_context(new_name, ic);
}

InputTranslator::InputTranslator(Urho3D::Context * context):
	Object(context)
{}

InputTranslator::~InputTranslator()
{}

void InputTranslator::init()
{
	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(InputTranslator, handle_key_down));
	SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(InputTranslator, handle_key_up));
	SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(InputTranslator, handle_mouse_down));
	SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(InputTranslator, handle_mouse_up));
	SubscribeToEvent(E_MOUSEWHEEL, URHO3D_HANDLER(InputTranslator, handle_mouse_wheel));
	SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(InputTranslator, handle_mouse_move));
	GetSubsystem<Input>()->SetMouseVisible(true);
}

void InputTranslator::release()
{
    UnsubscribeFromAllEvents();
}

void InputTranslator::handle_key_down(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	trigger_condition tc;
	tc.key = int32_t(eventData["Key"].GetInt());
	tc.mouse_button = 0;
	int32_t qualifiers(eventData["Qualifiers"].GetInt());
	int32_t mouse_buttons(eventData["Buttons"].GetInt());
	
	for (int i = m_context_stack.Size(); i > 0; --i)
	{
		input_context * cur_ic = m_context_stack[i-1];
		input_action_trigger * trig = nullptr;

		// Go through every trigger - if the current mb and quals are contained in the required qual and mb fields
		// and the allowed 
		// First find all triggers with the exact match for key and mouse qualifiers
		trigger_range tr = cur_ic->get_triggers(tc);
		
		while (tr.first != tr.second)
		{
			input_action_trigger * trig = tr.first->second;

			// Check the qualifier and mouse button required conditions
			bool pass_qual_required((trig->qual_required & qualifiers) == trig->qual_required);
			bool pass_mb_required((trig->mb_required & mouse_buttons) == trig->mb_required);

			// Check the qualifier and mouse button allowed conditions
			int32_t allowed_quals = trig->qual_required | trig->qual_allowed;
			int32_t allowed_mb = trig->mb_required | trig->mb_allowed;
			bool pass_qual_allowed(((allowed_quals & QUAL_ANY) == QUAL_ANY) || (qualifiers | allowed_quals) == allowed_quals);
			bool pass_mb_allowed(((allowed_mb & MOUSEB_ANY) == MOUSEB_ANY) || ((mouse_buttons | allowed_mb) == allowed_mb));

			// If passes all the conditions, send the event for the trigger and mark trigger as active
			if (!_trigger_already_active(trig) && pass_qual_required && pass_mb_required && pass_qual_allowed && pass_mb_allowed)
			{
				if ((trig->trigger_state & t_begin) == t_begin)
				{
					using namespace InputTrigger;
					VariantMap event_data;
					event_data[P_TRIGGER_NAME] = trig->name;
					event_data[P_TRIGGER_STATE] = t_begin;
					event_data[P_NORM_MPOS] = current_norm_mpos;
					event_data[P_NORM_MDELTA] = Vector2();
					event_data[P_MOUSE_WHEEL] = 0;
					SendEvent(E_INPUT_TRIGGER,event_data);
				}
				active_triggers.Push(trig);
			}

			++tr.first;
		}
	}	
}

bool InputTranslator::_trigger_already_active(input_action_trigger * trig)
{
	auto iter = active_triggers.Begin();
	while (iter != active_triggers.End())
	{
		if (*trig == **iter)
			return true;
		++iter;
	}
	return false;		
}

void InputTranslator::handle_key_up(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	trigger_condition tc;
	tc.key = int32_t(eventData["Key"].GetInt());
	tc.mouse_button = 0;
	int32_t mouse_buttons = int32_t(eventData["Buttons"].GetInt());
	int32_t qualifiers = int32_t(eventData["Qualifiers"].GetInt());

	auto iter = active_triggers.Begin();
	while (iter != active_triggers.End())
	{
		input_action_trigger * curt = *iter;

		// Always end the action and send the trigger when the main key is depressed - dont care
		// about any of the qualifier situation
		if (tc.key == curt->condition.key)
		{
			if ((curt->trigger_state & t_end) == t_end)
			{
				VariantMap event_data;
				using namespace InputTrigger;
				event_data[P_TRIGGER_NAME] = curt->name;
				event_data[P_TRIGGER_STATE] = t_end;
				event_data[P_NORM_MPOS] = current_norm_mpos;
				event_data[P_NORM_MDELTA] = Vector2();
				event_data[P_MOUSE_WHEEL] = 0;
				SendEvent(E_INPUT_TRIGGER,event_data);
			}
			iter = active_triggers.Erase(iter);
		}
		else
		{
			++iter;
		}
	}

}

void InputTranslator::_normalize_mpos(Vector2 & to_norm)
{
    IntVector2 size = GetSubsystem<Graphics>()->GetSize();
	to_norm.x_ /= float(size.x_);
	to_norm.y_ /= float(size.y_);
}

void InputTranslator::handle_mouse_down(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	trigger_condition tc;
	tc.key = 0;
	tc.mouse_button = eventData["Button"].GetInt();
	int32_t mouse_buttons = int32_t(eventData["Buttons"].GetInt());
	int32_t qualifiers = int32_t(eventData["Qualifiers"].GetInt());
	int32_t wheel = 0;
	Vector2 norm_mdelta;

	if (tc.mouse_button == MOUSEB_WHEEL)
		wheel = int32_t(eventData["Wheel"].GetInt());

	if (tc.mouse_button == MOUSEB_MOVE)
	{
		Vector2 current_mpos(float(eventData["X"].GetInt()),float(eventData["Y"].GetInt()));
		_normalize_mpos(current_mpos);
		norm_mdelta = current_mpos - current_norm_mpos;
		current_norm_mpos = current_mpos;
	}


	for (int i = m_context_stack.Size(); i > 0; --i)
	{
		input_context * cur_ic = m_context_stack[i-1];
		input_action_trigger * trig = nullptr;

		// Go through every trigger - if the current mb and quals are contained in the required qual and mb fields
		// and the allowed 
		// First find all triggers with the exact match for key and mouse qualifiers
		trigger_range tr = cur_ic->get_triggers(tc);
		while (tr.first != tr.second)
		{
			input_action_trigger * trig = tr.first->second;

			// Check the qualifier and mouse button required conditions
			bool pass_qual_required((trig->qual_required & qualifiers) == trig->qual_required);
			bool pass_mb_required((trig->mb_required & mouse_buttons) == trig->mb_required);

			// Check the qualifier and mouse button allowed conditions
			int32_t allowed_quals = trig->qual_required | trig->qual_allowed;
			int32_t allowed_mb = trig->mb_required | trig->mb_allowed | trig->condition.mouse_button;
			bool pass_qual_allowed(((allowed_quals & QUAL_ANY) == QUAL_ANY) || (qualifiers | allowed_quals) == allowed_quals);
			bool pass_mb_allowed(((allowed_mb & MOUSEB_ANY) == MOUSEB_ANY) || ((mouse_buttons | allowed_mb) == allowed_mb));

			// If passes all the conditions, send the event for the trigger and mark trigger as active

			if (!_trigger_already_active(trig) && pass_qual_required && pass_mb_required && pass_qual_allowed && pass_mb_allowed)
			{
				if ((trig->trigger_state & t_begin) == t_begin)
				{
					VariantMap event_data;
					using namespace InputTrigger;
					event_data[P_TRIGGER_NAME] = trig->name;
					event_data[P_TRIGGER_STATE] = t_begin;
					event_data[P_NORM_MPOS] = current_norm_mpos;
					event_data[P_NORM_MDELTA] = norm_mdelta;
					event_data[P_MOUSE_WHEEL] = wheel;
					SendEvent(E_INPUT_TRIGGER,event_data);
				}

				if (tc.mouse_button != MOUSEB_WHEEL && tc.mouse_button != MOUSEB_MOVE)
					active_triggers.Push(trig);
			}

			++tr.first;
		}
	}	

}

void InputTranslator::handle_mouse_up(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	trigger_condition tc;
	tc.key = 0;
	tc.mouse_button = eventData["Button"].GetInt();
	int32_t mouse_buttons = int32_t(eventData["Buttons"].GetInt());
	int32_t qualifiers = int32_t(eventData["Qualifiers"].GetInt());

	auto iter = active_triggers.Begin();
	while (iter != active_triggers.End())
	{
		input_action_trigger * curt = *iter;

		if (tc.mouse_button == curt->condition.mouse_button)
		{
			if (((curt->trigger_state & t_end) == t_end))
			{
				VariantMap event_data;
				using namespace InputTrigger;
				event_data[P_TRIGGER_NAME] = curt->name;
				event_data[P_TRIGGER_STATE] = t_end;
				event_data[P_NORM_MPOS] = current_norm_mpos;
				event_data[P_NORM_MDELTA] = Vector2();
				event_data[P_MOUSE_WHEEL] = 0;
				SendEvent(E_INPUT_TRIGGER,event_data);
			}
			iter = active_triggers.Erase(iter);
		}
		else
		{
			++iter;
		}
	}

}

void InputTranslator::handle_mouse_wheel(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	eventData["Button"] = MOUSEB_WHEEL;
	handle_mouse_down(eventType, eventData);
}

void InputTranslator::handle_mouse_move(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	eventData["Button"] = MOUSEB_MOVE;
	handle_mouse_down(eventType, eventData);
}


void InputTranslator::push_context(input_context * ctxt)
{
	m_context_stack.Push(ctxt);
}

void InputTranslator::pop_context()
{
	m_context_stack.Pop();		
}
