#include "RuleHelper.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager
{
	bool RuleHelper::AllPluginsPresent(Json::Value& a_plugins) {
		auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh) {
			valid = false;
			return false;
		}

		bool findAll = true;
		if (a_plugins.isObject()) {
			static constexpr std::array<std::string_view, 2u> expectedNames = {
				CONDITION_FLAG_VALUE,
				CONDITION_FLAG_OPERAND
			};
			static constexpr auto namesBegin = expectedNames.begin();
			static constexpr auto namesEnd = expectedNames.end();

			auto members = a_plugins.getMemberNames();
			for (const auto& member : members) {
				if (std::find(namesBegin, namesEnd, member) == namesEnd) {
					valid = false;
					errors.badFieldValueType.emplace_back(fmt::format("{} - Unknown Member: {}", CONDITION_PLUGINS, member));
					return false;
				}
			}

			auto& operand = a_plugins[CONDITION_FLAG_OPERAND.data()];
			auto& complexValue = a_plugins[CONDITION_FLAG_VALUE.data()];

			if (!complexValue) {
				errors.missingFields.emplace_back(fmt::format("{} - Missing Member: {}", CONDITION_PLUGINS, CONDITION_FLAG_VALUE));
				valid = false;
				return false;
			}
			if (operand) {
				if (!operand.isString()) {
					errors.badFieldValueType.emplace_back(fmt::format("{} - {} is not a string", CONDITION_PLUGINS, CONDITION_FLAG_OPERAND));
					valid = false;
					return false;
				}

				static constexpr std::array<std::string_view, 2u> expectedSettings = {
					CONDITION_FLAG_OPERAND_AND,
					CONDITION_FLAG_OPERAND_OR
				};
				static constexpr auto flagsBegin = expectedSettings.begin();
				static constexpr auto flagsEnd = expectedSettings.end();
				auto operandLowercase = clib_util::string::tolower(operand.asString());
				if (std::find(flagsBegin, flagsEnd, operandLowercase) == flagsEnd) {
					errors.badFieldValueFormat.emplace_back(
						fmt::format("{} - {} holds {} (expected {} or {})", 
							CONDITION_PLUGINS, 
							CONDITION_FLAG_OPERAND, 
							operandLowercase,
							CONDITION_FLAG_OPERAND_OR,
							CONDITION_FLAG_OPERAND_AND)
					);
					valid = false;
					return false;
				}
				findAll = operandLowercase == CONDITION_FLAG_OPERAND_AND;
			}
			a_plugins = std::move(complexValue);
		}

		std::vector<std::string> pluginNames{};
		auto parseResult = Settings::JSON::LoadFormStrings(a_plugins, pluginNames);
		if (parseResult != Settings::JSON::JsonParseResult::Success) {
			switch (parseResult) {
			case Settings::JSON::JsonParseResult::NonHomogenousArray:
				errors.mixedArrayTypes.emplace_back(CONDITION_PLUGINS);
				break;
			default:
				errors.badFieldValueType.emplace_back(CONDITION_PLUGINS);
				break;
			}
			valid = false;
			return false;
		}

		if (pluginNames.empty()) {
			errors.emptyRules.emplace_back(CONDITION_PLUGINS);
			valid = false;
			return false;
		}

		if (findAll) {
			return std::all_of(pluginNames.begin(), pluginNames.end(),
				[&](const auto& name) {
					return dh->LookupLoadedModByName(name) != nullptr;
				});
		}
		return std::any_of(pluginNames.begin(), pluginNames.end(),
			[&](const auto& name) {
				return dh->LookupLoadedModByName(name) != nullptr;
			});
	}
}