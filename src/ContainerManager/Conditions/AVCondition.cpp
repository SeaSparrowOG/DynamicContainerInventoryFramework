#include "AVCondition.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager
{
	namespace Conditions
	{
		bool AVRequirement::MeetsCondition(const RE::ActorValueOwner* a_actor) const {
			const auto level = a_actor->GetActorValue(av);
			if (respectMax && level > max) {
				return false;
			}
			if (level < min) {
				return false;
			}
			return true;
		}

		void AVRequirement::PrintCondition(const std::string& a_pref) const {
			logger::info("{}AV: {}"sv, a_pref, RE::ActorValueToString(av));
			logger::info("{}  >Minimum: {}"sv, a_pref, min);
			if (respectMax) {
				logger::info("{}  >Maximum: {}"sv, a_pref, max);
			}
		}

		AVCondition::AVCondition(std::vector<AVRequirement> a_requirements, bool a_and, bool invert) : 
			andCondition(a_and),
			inverted(invert),
			m_requirements(std::move(a_requirements)) 
		{
			if (m_requirements.empty()) {
				throw std::runtime_error("AV Condition resolved empty");
			}
		}

		void AVCondition::PrintCondition(const std::string& a_pref) const {
			logger::info("{}Condition Type: Player Skill."sv, a_pref);
			logger::info("{}  >Inverted: {}"sv, a_pref, inverted ? "TRUE"sv : "FALSE"sv);
			logger::info("{}  >Operand: {}"sv, a_pref, andCondition ? "AND"sv : "OR"sv);
			logger::info("{}  >Contents:"sv, a_pref);
			for (const auto& av : m_requirements) {
				av.PrintCondition(fmt::format("  {}"sv, a_pref));
			}
		}

		bool AVCondition::IsValid(const ConditionCheckParams& a_params) const {
			// ConditionCheckParams guarantees that playerOwner is valid and Non-Null
			auto* owner = a_params.playerOwner;
			bool result;
			if (andCondition) {
				result = std::all_of(m_requirements.begin(), m_requirements.end(), [&](const AVRequirement& av) {
					return av.MeetsCondition(owner);
				});
			}
			else {
				result = std::any_of(m_requirements.begin(), m_requirements.end(), [&](const AVRequirement& av) {
					return av.MeetsCondition(owner);
				});
			}
			return inverted ? !result : result;
		}

		std::expected<AVCondition, AVConditionError> CreateAVCondition(const Json::Value& a_template, bool invert) {
			std::vector<AVRequirement> requirements;
			return AVCondition(requirements);
		}

		bool AVConditionError::Errored() const {
			if (!errors.empty()) {
				return true;
			}
			if (!unknownFields.empty()) {
				return true;
			}
			return empty || typeError || inversionError ||
				invalidObjectType || missingValuesField || nonHomogenousArray;
		}

		void AVConditionError::Report(const std::string& a_prefix) const {
			if (empty) {
				logger::error("{}AV Condition does not contain any AVs."sv, a_prefix);
			}
			if (typeError) {
				logger::error("{}AV Condition has Type specified, but it is not a String or String is not AND/OR."sv, a_prefix);
			}
			if (inversionError) {
				logger::error("{}AV Condition specified invertion field, but it is not a bool."sv, a_prefix);
			}
			if (invalidObjectType) {
				logger::error("{}AV Condition specified AVs are not a string or an array"sv, a_prefix);
			}
			if (missingValuesField) {
				logger::error("{}AV Condition is missing a mandatory <values> field."sv, a_prefix);
			}
			if (nonHomogenousArray) {
				logger::error("{}AV Condition is neither a string nor an array."sv, a_prefix);
			}
			if (!unknownFields.empty()) {
				logger::error("{}AV Condition has the following fields specified, but they are not supported:"sv, a_prefix);
				for (const auto& field : unknownFields) {
					logger::error("{}  >{}"sv, a_prefix, field);
				}
			}
			if (!errors.empty()) {
				logger::error("{}The following AVs resolved with errors:"sv, a_prefix);
				for (const auto& error : errors) {
					logger::error("{}  >{}"sv, a_prefix, error.text);
				}
			}
		}
	}
}