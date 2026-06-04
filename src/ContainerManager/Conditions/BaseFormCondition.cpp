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
					tweaks ? fmt::format<std::string>(" ({})", clib_util::editorID::get_editorID(cont)) : "",
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

	static void ProcessElement(const Json::Value& val,
		std::string& path,
		BaseFormCondition& condition,
		error& errorHolder)
	{

	}

	std::expected<condition, error> TryCreateBaseFormCondition(const Json::Value& from,
		std::string& path,
		bool inverted)
	{
		error errorHolder;
		BaseFormCondition baseFormCondition;
		baseFormCondition.SetInverted(inverted);

		// Expected format:
		// {
		//   "IsOr": Bool
		//   "Data": Sting or Array
		// }
		// OR
		// String
		// OR
		// Array (of srings or objects)

		condition result = std::make_unique<BaseFormCondition>(baseFormCondition);
		return result;
	}
}