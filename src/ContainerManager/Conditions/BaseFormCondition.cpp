#include "BaseFormCondition.h"

namespace ContainerManager::Conditions
{
	bool BaseFormCondition::IsValid(const ConditionCheckParams& a_params) const {
		const bool hasMatch = _data.contains(a_params.baseformID);
		return _inverted ? !hasMatch : hasMatch;
	}

	void BaseFormCondition::PrintCondition(const std::string& a_pref) const {
		logger::info("{}Base Form Condition:"sv, a_pref);
		static auto* tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");
		for (const auto id : _data) {
			auto* cont = RE::TESForm::LookupByID<RE::TESObjectCONT>(id);
			if (cont) {
				logger::info("{}  - {}{}{}{}."sv, 
					a_pref,
					_inverted ? "[NOT]<" : "",
					cont->GetName(),
					tweaks ?  fmt::format<std::string>(" ({})", clib_util::editorID::get_editorID(cont)) : "",
					_inverted ? ">" : ""
				);
			}
		}
	}

	void BaseFormCondition::AppendBaseForm(RE::TESBoundObject* base) {
		if (!base) {
			return;
		}
		_data.insert(base->GetFormID());
	}

	std::expected<condition_ptr, error_ptrs> TryCreateBaseFormCondition(const Json::Value& from, 
		std::string& path, 
		bool inverted)
	{
		BaseFormCondition baseFormCondition;
		baseFormCondition.SetInverted(inverted);

		condition_ptr result = std::make_unique<BaseFormCondition>(baseFormCondition);
		return result;
	}
}