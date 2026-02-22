#include "AVCondition.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager
{
	namespace Conditions
	{
		bool AVRequirement::MeetsCondition(const RE::ActorValueOwner* a_actor) const {
			const auto level = a_actor->GetActorValue(av);
			if (respectMax && level <= max) {
				return false;
			}
			if (level < min) {
				return false;
			}
			return true;
		}

		void AVRequirement::PrintCondition(const std::string& a_pref) const
		{
			logger::info("{}AV: {}"sv, a_pref, RE::ActorValueToString(av));
			logger::info("{}  >Minimum: {}"sv, a_pref, min);
			if (respectMax) {
				logger::info("{}  >Maximum: {}"sv, a_pref, max);
			}
		}

		AVCondition::AVCondition(std::vector<AVRequirement> a_requirements, bool a_and, bool invert) {
			if (a_requirements.empty()) {
				throw std::runtime_error("AV Condition resolved empty");
			}
			inverted = invert;
			m_requirements = std::move(a_requirements);
			andCondition = a_and;
		}

		void AVCondition::PrintCondition(const std::string& a_pref) const
		{
			logger::info("{}Condition Type: Player Skill."sv, a_pref);
			logger::info("{}  >Inverted: {}"sv, a_pref, inverted ? "TRUE"sv : "FALSE"sv);
			logger::info("{}  >Operand: {}"sv, a_pref, andCondition ? "AND"sv : "OR"sv);
			logger::info("{}  >Contents:"sv, a_pref);
			for (const auto& av : m_requirements) {
				av.PrintCondition(fmt::format("  {}"sv, a_pref));
			}
		}

		bool AVCondition::IsValid(const ConditionCheckParams& a_params) const {
			auto* owner = a_params.playerOwner;
			if (inverted) {
				for (const auto& requirement : m_requirements) {
					if (requirement.MeetsCondition(owner)) {
						return false;
					}
				}
			}
			for (const auto& requirement : m_requirements) {
				if (!requirement.MeetsCondition(owner)) {
					return false;
				}
			}
			return true;
		}

		AVConditionResult CreateAVCondition(const Json::Value& a_template, bool invert) {
			bool isANDCondition = true;
			auto response = AVConditionResult();
			response.errors.push_back("AV Condition Resolved Incorrectly:");
			std::vector<AVRequirement> resolvedRequirements{};

			Json::Value resolved = Json::Value();
			if (a_template.isObject()) {
				const auto& matchAll = a_template[OBJECT_TYPE.data()];
				if (matchAll) {
					if (!matchAll.isString()) {
						response.errored = true;
						response.errors.push_back(fmt::format("  Incorrectly formatted object - {} is not a String field."sv, OBJECT_TYPE));
					}
					else {
						const auto mode = clib_util::string::tolower(matchAll.asString());
						if (mode == OBJECT_AND) {
							isANDCondition = true;
						}
						else if (mode == OBJECT_OR) {
							isANDCondition = false;
						}
						else {
							response.errored = true;
							response.errors.push_back(fmt::format("  Unexpected {} setting in AV Condition field {}."sv, mode, OBJECT_TYPE));
						}
					}
				}

				const auto& isInverted = a_template[OBJECT_INVERT.data()];
				if (isInverted) {
					if (!isInverted.isBool()) {
						response.errors.push_back(fmt::format("  Incorrectly formatted object - {} is not a Bool field."sv, OBJECT_INVERT));
						response.errored = true;
					}
					else {
						invert = isInverted.asBool();
					}
				}
				if (!a_template.isMember(OBJECT_VALUES.data())) {
					response.errored = true;
					response.errors.push_back(fmt::format("  Incorrectly formatted Object - no {} field found."sv, OBJECT_VALUES));
					return response;
				}
				auto& values = a_template[OBJECT_VALUES.data()];
				resolved = values;
			}

			if (resolved.isNull()) {
				resolved = a_template;
			}

			std::vector<std::string> avs{};
			avs.reserve(resolved.size());

			auto parseResult = Settings::JSON::LoadFormStrings(resolved, avs);
			if (parseResult != Settings::JSON::JsonParseResult::Success) {
				response.errored = true;
				switch (parseResult) {
				case Settings::JSON::JsonParseResult::NonHomogenousArray:
					response.errors.push_back(fmt::format("  Incorrect value type passed to {}. Array contains non-string element."sv, OBJECT_VALUES));
					break;
				case Settings::JSON::JsonParseResult::NotStringOrArray:
					response.errors.push_back(fmt::format("  Incorrect value type passed to {}. Expected string or array."sv, OBJECT_VALUES));
					break;
				default:
					response.errors.push_back(fmt::format("  Unexpected error encounted while parsing {}."sv, OBJECT_VALUES));
					break;
				}
				return response;
			}

			if (avs.empty()) {
				response.errored = true;
				response.errors.push_back("  AV array resolved as empty.");
				return response;
			}

			for (const auto& rawAV : avs) {
				bool hasMax = false;
				float min = 0.0f;
				float max = 0.0f;

				auto parts = clib_util::string::split(rawAV, "|");
				if (parts.size() <= 1) {
					response.errored = true;
					response.errors.push_back(fmt::format("  Error while parsing {}. Delimiter likely missing, value: {}", OBJECT_VALUES, rawAV));
				}

				auto asName = parts.at(0);
				auto asAV = RE::ActorValueList::LookupActorValueByName(asName.c_str());
				if (asAV == RE::ActorValue::kNone) {
					response.errored = true;
					response.errors.push_back(fmt::format("  Error in {}. Provided actor value ({}) does not exist.", OBJECT_VALUES, asName));
				}
				auto& asMin = parts.at(1);
				std::string asMax = "";
				if (parts.size() == 3) {
					asMax = parts.at(2);
					hasMax = true;
				}

				try {
					min = clib_util::string::to_num<float>(asMin);
				}
				catch (const std::exception& e) {
					response.errored = true;
					response.errors.push_back(fmt::format("  Error in {}. Provided actor value ({})'s minimum value is not a number.", OBJECT_VALUES, asName, asMin));
				}
				if (hasMax) {
					try {
						max = clib_util::string::to_num<float>(asMax);
					}
					catch (const std::exception& e) {
						response.errored = true;
						response.errors.push_back(fmt::format("  Error in {}. Provided actor value ({})'s maximum value is not a number.", OBJECT_VALUES, asName, asMax));
					}
				}

				AVRequirement parsedRequirement = AVRequirement();
				parsedRequirement.av = asAV;
				parsedRequirement.min = min;
				parsedRequirement.max = max;
				parsedRequirement.respectMax = hasMax;
				resolvedRequirements.emplace_back(std::move(parsedRequirement));
			}
			if (response.errored) {
				return response;
			}

			auto condition = AVCondition(resolvedRequirements, isANDCondition, invert);
			response.result = std::move(condition);
			return response;
		}
	}
}