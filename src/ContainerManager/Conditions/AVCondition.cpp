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
			bool isANDCondition = true;
			AVConditionError error{};
			std::vector<AVRequirement> resolvedRequirements{};

			Json::Value resolved;
			if (a_template.isObject()) {
				auto members = a_template.getMemberNames();
				bool hasValues = false;
				for (const auto& member : members) {
					const auto& value = a_template[member];
					if (member == OBJECT_TYPE) {
						const auto& matchAll = a_template[OBJECT_TYPE.data()];
						if (!matchAll.isString()) {
							error.typeError = true;
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
								error.typeError = true;
							}
						}
					}
					else if (member == OBJECT_INVERT) {
						const auto& isInverted = a_template[OBJECT_INVERT.data()];
						if (!isInverted.isBool()) {
							error.inversionError = true;
						}
						invert = isInverted.asBool();
					}
					else if (member == OBJECT_VALUES) {
						resolved = value;
						hasValues = true;
					}
					else {
						error.unknownFields.emplace_back(member);
					}
				}
				if (!hasValues) {
					error.missingValuesField = true;
					return std::unexpected(error);
				}
			}

			if (resolved.isNull()) {
				resolved = a_template;
			}

			std::vector<std::string> avs;
			avs.reserve(resolved.size());

			auto parseResult = Settings::JSON::LoadFormStrings(resolved, avs);
			if (parseResult != Settings::JSON::JsonParseResult::Success) {
				switch (parseResult) {
				case Settings::JSON::JsonParseResult::NonHomogenousArray:
					error.nonHomogenousArray = true;
					break;
				case Settings::JSON::JsonParseResult::NotStringOrArray:
					error.invalidObjectType = true;
					break;
				default:
					std::unreachable();
				}
				return std::unexpected(error);
			}

			if (avs.empty()) {
				error.empty = true;
				return std::unexpected(error);
			}

			for (const auto& rawAV : avs) {
				bool hasMax = false;
				float min = 0.0f;
				float max = 0.0f;
				auto failed = FailedAV();

				auto parts = clib_util::string::split(rawAV, "|");
				// TODO: Potentially set a proper range elsewhere.
				// Parts: AV|Min or AV|Min|Max.
				//    AV|Min|Max is the suggested format, but AV|Max|Min is also supported.
				if (parts.size() < 2 || parts.size() > 3) {
					failed.text = rawAV;
					failed.incorrectSize = true;
					error.errors.emplace_back(std::move(failed));
					continue;
				}

				auto asName = clib_util::string::trim_copy(parts.at(0));
				auto asAV = RE::ActorValueList::LookupActorValueByName(asName.c_str());
				if (asAV == RE::ActorValue::kNone) {
					failed.invalidAV = true;
				}
				auto asMin = clib_util::string::trim_copy(parts.at(1));
				std::string asMax = "";
				if (parts.size() == 3) {
					asMax = clib_util::string::trim_copy(parts.at(2));
					hasMax = true;
				}

				try {
					min = clib_util::string::to_num<float>(asMin);
				}
				catch (const std::exception&) {
					failed.invalidMin = true;
				}
				if (hasMax) {
					try {
						max = clib_util::string::to_num<float>(asMax);
					}
					catch (const std::exception&) {
						failed.invalidMax = true;
					}
				}

				if (failed.incorrectSize || failed.invalidAV ||
					failed.invalidMin || failed.invalidMax)
				{
					failed.text = rawAV;
					error.errors.emplace_back(std::move(failed));
					continue;
				}

				// Note - min == max perhaps should be considered an error. Currently, 1 mod uses that, so I support it.
				if (max < min) {
					float temp_min = min; // Technically unecessary but I am not doing an interview ffs.
					min = max;
					max = temp_min;
				}

				AVRequirement parsedRequirement = AVRequirement();
				parsedRequirement.av = asAV;
				parsedRequirement.min = min;
				parsedRequirement.max = max;
				parsedRequirement.respectMax = hasMax;
				resolvedRequirements.emplace_back(std::move(parsedRequirement));
			}
			if (error.Errored()) {
				return std::unexpected(error);
			}

			return AVCondition(resolvedRequirements, isANDCondition, invert);
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
	}
}