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
		if (Errored()) {
			return;
		}

		auto* handler = InventorySwapper::GetSingleton();
		std::vector<std::size_t> ids{};

		if (!pendingConditions.empty()) {
			ids.reserve(pendingConditions.size());

			for (auto& condition : pendingConditions) {
				auto id = handler->RegisterCondition(std::move(condition));
				ids.emplace_back(id);
			}
		}

		for (auto& change : pendingChanges) {
			change->DefineConditions(ids);
			handler->RegisterChange(std::move(change));
		}
	}

	RuleHelper::RuleHelper(const Json::Value& a_normalizedJSON, const std::string& a_configName) {
		if (a_normalizedJSON.isObject()) {
			std::vector<std::string> path{ fmt::format("{}|Root", a_configName) };
			ParseObject(a_normalizedJSON, path);
		}
		else if (a_normalizedJSON.isArray()) {
			auto size = a_normalizedJSON.size();
			for (unsigned int i = 0; i < size; ++i) {
				const auto& arrayVal = a_normalizedJSON[i];
				if (arrayVal.empty()) {
					std::string absolutePath = fmt::format("Root[{}]", i);
					errors.invalidTopLevelObjects.emplace_back(fmt::format("{} - Empty."sv, absolutePath));
					continue;
				}
				else if (!arrayVal.isObject()) {
					std::string absolutePath = fmt::format("Root[{}]", i);
					errors.invalidTopLevelObjects.emplace_back(fmt::format("{} - {}."sv, absolutePath, Settings::JSON::GetFieldType(arrayVal)));
					continue;
				}

				std::vector<std::string> path{ fmt::format("Root[{}]", i) };
				ParseObject(arrayVal, path);
				path.pop_back();
			}
		}
		else {
			errors.invalidTopLevelObjects.emplace_back(fmt::format("Root - {}", Settings::JSON::GetFieldType(a_normalizedJSON)));
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
			else if (member == TOP_LEVEL_VERSION) {
				const auto& minVersionMember = a_value[member];
				if (minVersionMember.isNumeric()) {
					const auto requiredVer = minVersionMember.asInt();
					if (requiredVer > PARSER_VERSION) {
						errors.invalidVersion;
					}
				}
				else if (minVersionMember.isString()) {
					const auto rawVer = minVersionMember.asString();
					try {
						const auto requiredVer = std::stoi(rawVer);
						if (requiredVer > PARSER_VERSION) {
							errors.invalidVersion = true;
						}
					}
					catch (...) {
						errors.invalidVersion = true;
					}
				}
				else {
					errors.invalidVersion = true;
				}
			}
			else {
				std::string absolutePath = "";
				for (const auto& partialPath : a_path) {
					absolutePath.append(partialPath);
				}
				absolutePath.append(fmt::format("|{}", member));

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

					errors.emptyChangesFields.push_back(absolutePath);
				}
				else {
					a_path.emplace_back("|Changes");
					ParseChange(changes, a_path);
					a_path.pop_back();
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
						absolutePath.append(fmt::format("[{}]", i));

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
			if (!conditions.isObject()) {
				std::string absolutePath = "";
				for (const auto& partialPath : a_path) {
					absolutePath.append(partialPath);
				}
				absolutePath.append(TOP_LEVEL_CONDITIONS.data());

				errors.emptyConditionsFields.emplace_back(absolutePath);
			}
			else if (conditions.empty()) {
				std::string absolutePath = "";
				for (const auto& partialPath : a_path) {
					absolutePath.append(partialPath);
				}
				absolutePath.append(TOP_LEVEL_CONDITIONS.data());

				errors.emptyConditionsFields.emplace_back(absolutePath);
			}
			else {
				a_path.emplace_back(fmt::format("|{}", TOP_LEVEL_CONDITIONS));
				ParseCondition(conditions, a_path);
				a_path.pop_back();
			}
		}
	}

	void RuleHelper::ParseChange(const Json::Value& a_value, std::vector<std::string>& a_path) {
		(void)a_value;
		(void)a_path;
	}

	void RuleHelper::ParseCondition(const Json::Value& a_value, std::vector<std::string>& a_path) {
		const auto members = a_value.getMemberNames();
		std::string absolutePath = "";
		for (const auto& partPath : a_path) {
			absolutePath.append(partPath);
		}

		for (const auto& member : members) {
			std::string currentPath = absolutePath + "|" + member;

			bool invert = member.starts_with("!");
			const auto type = ConditionTypeFromString(member);
			const auto& value = a_value[member];

			if (type == ConditionType::Invalid) {
				errors.invalidConditionsFields.emplace_back(currentPath);
				continue;
			}

			switch (type) {
			case ConditionType::PlayerSkills:
				AddPlayerSkillCondition(value, invert);
				break;
			default:
				std::unreachable();
			}
		}
	}

	void RuleHelper::StructuredErrorMessages::PrintErrors(const std::string& a_prefix) const {
		(void)a_prefix;
	}

	bool RuleHelper::Errored() const {
		if (!errors.emptyChangesFields.empty()) {
			return true;
		}
		if (!errors.emptyConditionsFields.empty()) {
			return true;
		}
		if (!errors.invalidChangesFields.empty()) {
			return true;
		}
		if (!errors.invalidConditionsFields.empty()) {
			return true;
		}
		if (!errors.invalidTopLevelObjects.empty()) {
			return true;
		}
		if (!errors.missingChangesFields.empty()) {
			return true;
		}
		if (!errors.missingPlugins.empty()) {
			return true;
		}
		if (!errors.unknownTopLevelObjects.empty()) {
			return true;
		}
		return !errors.emptyConfig && !errors.invalidVersion;
	}

	void RuleHelper::AddPlayerSkillCondition(const Json::Value& a_condition, bool a_negate) {
		auto result = Conditions::CreateAVCondition(a_condition, a_negate);
		if (!result) {
			erroredPlayerSkillConditions.emplace_back(std::move(result.error()));
			return;
		}
		std::unique_ptr<Condition> avCondition =
			std::make_unique<Conditions::AVCondition>(result.value());
		pendingConditions.emplace_back(std::move(avCondition));
	}
}