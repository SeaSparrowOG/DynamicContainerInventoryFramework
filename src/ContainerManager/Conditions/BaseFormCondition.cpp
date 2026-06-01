#include "BaseFormCondition.h"

namespace ContainerManager::Conditions
{
	static void CreateBaseFormConditionImpl(const Json::Value& from,
		std::string& path,
		BaseFormCondition& condition,
		BaseFormConditionFailure& failure) 
	{
		if (from.isArray()) {
			auto trimTo = path.size();
			for (Json::ArrayIndex i = 0u; i < from.size(); ++i) {
				path += "[" + std::to_string(i) + "]";
				CreateBaseFormConditionImpl(from[i], path, condition, failure);
				path.resize(trimTo);
			}
			return;
		}
		if (from.isString()) {
			auto parsed = GetFormFromString<RE::TESObjectCONT>(from.asString());
			switch (parsed.status) {
			case QueryResult::Success:
				if (parsed.value.has_value()) { 
					// EDID search can fail without a specified reason, since no mod is specified.
					condition.AppendBaseForm(parsed.value.value());
				}
				break;
			case QueryResult::FileNotFound:
			case QueryResult::FormNotInFile:
				break;
			case QueryResult::WrongFormtype:
				failure.AddWrongFormType(path, from.asString());
				break;
			case QueryResult::MissingPo3Tweaks:
				failure.AddMissingPO3Tweaks(path, from.asString());
				break;
			case QueryResult::GenericFailure:
			case QueryResult::FormatError:
				failure.AddBadFormatString(path, from.asString());
				break;
			default:
				logger::critical("Unspecified error when parsing {}"sv, from.asString());
				break;
			}
		}
		else {
			failure.AddWrongFieldType(path, "String or Array", GetJSONTypeAsString(from));
			return;
		}
	}

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

	std::expected<BaseFormCondition, BaseFormConditionFailure> CreateBaseFormCondition(const Json::Value& from, std::string& path, bool inverted)
	{
		BaseFormCondition condition;
		condition.SetInverted(inverted);
		BaseFormConditionFailure failure;

		CreateBaseFormConditionImpl(from, path, condition, failure);
		if (condition.Empty()) {
			failure.MarkConditionsEmpty(path);
		}
		if (failure.Errored()) {
			return std::unexpected(failure);
		}
		return condition;
	}

	void BaseFormConditionFailure::Report(const std::string& a_prefix) const
	{
		if (!_emptyCondition.empty()) {
			logger::error("{}  - Condition {} resolved as empty."sv, a_prefix, _emptyCondition);
		}
		else if (!_wrongFieldType.empty()) {
			for (const auto& err : _wrongFieldType) {
				logger::error("{}  - {}"sv, a_prefix, err);
			}
		}
		else if (!_wrongFormTypes.empty()) {
			for (const auto& err : _wrongFormTypes) {
				logger::error("{}  - {}"sv, a_prefix, err);
			}
		}
		else if (!_badStringFormat.empty()) {
			for (const auto& err : _badStringFormat) {
				logger::error("{}  - {}"sv, a_prefix, err);
			}
		}
		else if (!_formsRelyingOnTweaks.empty()) {
			for (const auto& err : _formsRelyingOnTweaks) {
				logger::error("{}  - {}"sv, a_prefix, err);
			}
		}
	}

	void BaseFormConditionFailure::Preamble(const std::string& a_prefix) const
	{
		logger::error("{}Base Form Condition Errors:"sv, a_prefix);
	}

	void BaseFormConditionFailure::AddWrongFieldType(const std::string& path,
		const std::string& expected, 
		const std::string& received) 
	{
		std::string error = fmt::format("{} is of type {}, expected {}."sv, path, received, expected);
		_wrongFieldType.emplace_back(std::move(error));
	}

	void BaseFormConditionFailure::AddWrongFormType(const std::string& path, 
		const std::string& rawForm)
	{
		std::string error = fmt::format("{} is not a Container."sv, path, rawForm);
		_wrongFormTypes.emplace_back(std::move(error));
	}

	void BaseFormConditionFailure::AddMissingPO3Tweaks(const std::string& path, 
		const std::string& rawForm)
	{
		std::string error = fmt::format("{} requires PO3's tweaks to resolve (string: {})."sv, path, rawForm);
		_formsRelyingOnTweaks.emplace_back(std::move(error));
	}

	void BaseFormConditionFailure::AddBadFormatString(const std::string& path, 
		const std::string& rawForm)
	{
		std::string error = fmt::format("{}: {} failed to resolve."sv, path, rawForm);
		_badStringFormat.emplace_back(std::move(error));
	}

	void BaseFormConditionFailure::MarkConditionsEmpty(const std::string& path) {
		_emptyCondition = path;
	}
}