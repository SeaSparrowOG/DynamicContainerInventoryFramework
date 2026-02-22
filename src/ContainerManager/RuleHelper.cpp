#include "RuleHelper.h"

#include "Conditions/AVCondition.h"
#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager
{
	bool BuildConditions() {
		logger::info("Building conditions..."sv);
		const auto* configManager = Settings::JSON::ConfigHolder::GetSingleton();
		if (!configManager) {
			logger::critical("  Failed to get config holder singleton."sv);
			return false;
		}

		bool result = true;
		const auto& configs = configManager->GetConfigs();
		if (configs.empty()) {
			logger::info("  No configs found to parse."sv);
			return result;
		}

		for (const auto& [name, config] : configs) {
			constexpr std::string_view root = "Root"sv;
			logger::info("  Parsing {}..."sv, name);

			if (config.empty()) {
				logger::warn("    >Empty."sv);
				continue;
			}

			auto helper = RuleHelper(config, name);
		}
		return result;
	}

	RuleHelper::~RuleHelper() {
		logger::error("    Failed to create requested rule."sv);
		if (!valid) {
			errors.PrintErrors("      ");
			return;
		}

		auto* handler = InventorySwapper::GetSingleton();
		std::vector<std::size_t> ids{};

		if (!conditions.empty()) {
			ids.reserve(conditions.size());

			for (auto& condition : conditions) {
				auto id = handler->RegisterCondition(std::move(condition));
				ids.emplace_back(id);
			}
		}

		for (auto& change : changes) {
			change->DefineConditions(ids);
			handler->RegisterChange(std::move(change));
		}
	}

	RuleHelper::RuleHelper(const Json::Value& a_normalizedJSON, const std::string& a_configName) {
		if (a_normalizedJSON.isObject()) {
			std::vector<std::string> path{ "Root" };
			ParseObject(a_normalizedJSON, path);
		}
		else if (a_normalizedJSON.isArray()) {
			auto size = a_normalizedJSON.size();
			for (unsigned int i = 0; i < size; ++i) {
				const auto& arrayVal = a_normalizedJSON[i];
				if (arrayVal.empty()) {
					std::string absolutePath = fmt::format("Root[{}]", i);
					errors.invalidTopLevelObjects.emplace_back("{} - Empty."sv, absolutePath);
					valid = false;
					continue;
				}
				else if (!arrayVal.isObject()) {
					std::string absolutePath = fmt::format("Root[{}]", i);
					errors.invalidTopLevelObjects.emplace_back("{} - {}."sv, absolutePath, Settings::JSON::GetFieldType(arrayVal));
					valid = false;
					continue;
				}

				std::vector<std::string> path{ fmt::format("Root[{}]", i) };
				ParseObject(arrayVal, path);
				path.pop_back();
			}
		}
		else {
			errors.invalidTopLevelObjects.emplace_back(fmt::format("Root - {}", Settings::JSON::GetFieldType(a_normalizedJSON)));
			valid = false;
		}
	}

	void RuleHelper::ParseObject(const Json::Value& a_value, std::vector<std::string>& a_path) {
		auto members = a_value.getMemberNames();
		bool hasChanges = false;
		bool hasConditions = false;
		for (const auto& member : members) {
			if (member == TOP_LEVEL_CHANGES) {
				hasChanges = true;
			}
			else if (member == TOP_LEVEL_CONDITIONS) {
				hasConditions = true;
			}
			else {
				std::string absolutePath = "";
				for (const auto& partialPath : a_path) {
					absolutePath.append(partialPath);
				}
				absolutePath.append(fmt::format("|{}", member));

				valid = false;
				errors.unknownTopLevelObjects.push_back(absolutePath);
				continue;
			}
		}

		if (hasChanges) {
			const auto& changes = a_value[TOP_LEVEL_CHANGES.data()];
			if (changes.isObject()) {
				if (changes.empty()) {
					std::string absolutePath = "";
					for (const auto& partialPath : a_path) {
						absolutePath.append(partialPath);
					}

					valid = false;
					errors.emptyChangesFields.push_back(absolutePath);
				}
				else {

				}
			}
			else if (changes.isArray()) {
				auto size = changes.size();
				for (unsigned int i = 0u; i < size; ++i) {
					const auto& arrayVal = changes[i];
					if (arrayVal.empty()) {
						std::string absolutePath = "";
						for (const auto& partialPath : a_path) {
							absolutePath.append(partialPath);
						}
						absolutePath.append(fmt::format("[{}]"), i);

						valid = false;
						errors.emptyChangesFields.push_back(absolutePath);
						continue;
					}
					a_path.emplace_back(fmt::format("[{}]", i));
					ParseChange(arrayVal, a_path);
					a_path.pop_back();
				}
			}
			else {
				std::string absolutePath = "";
				for (const auto& partialPath : a_path) {
					absolutePath.append(partialPath);
				}
				absolutePath.append("|");
				absolutePath.append(Settings::JSON::GetFieldType(changes));

				valid = false;
				errors.invalidChangesFields.push_back(absolutePath);
			}
		}
		else {
			std::string absolutePath = "";
			for (const auto& partialPath : a_path) {
				absolutePath.append(partialPath);
			}
			errors.missingChangesFields.push_back(absolutePath);
		}

		if (hasConditions) {
			const auto& conditions = a_value[TOP_LEVEL_CONDITIONS.data()];
			if (conditions.empty()) {
				std::string absolutePath = "";
				for (const auto& partialPath : a_path) {
					absolutePath.append(partialPath);
				}
				absolutePath.append(TOP_LEVEL_CONDITIONS.data());

				valid = false;
				errors.emptyConditionsFields.emplace_back(absolutePath);
			}
			else if (!conditions.isObject()) {

			}
			else {
				a_path.emplace_back(fmt::format("|{}", TOP_LEVEL_CONDITIONS));
				ParseCondition(conditions, a_path);
				a_path.pop_back();
			}
		}
	}

	void RuleHelper::ParseChange(const Json::Value& a_value, std::vector<std::string>& a_path) {
	}

	void RuleHelper::ParseCondition(const Json::Value& a_value, std::vector<std::string>& a_path) {
	}

	void RuleHelper::StructuredErrorMessages::PrintErrors(const std::string& a_prefix) const {
		(void)a_prefix;
	}
}