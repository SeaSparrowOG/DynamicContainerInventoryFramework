#include "RuleHelper.h"

#include "Changes/AddRule.h"
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

		auto* containerManager = InventorySwapper::GetSingleton();
		if (!containerManager) {
			logger::critical("  Failed to get internal Inventory Swapper singleton."sv);
			return false;
		}

		bool result = true;
		const auto& configs = configManager->GetConfigs();
		if (configs.empty()) {
			logger::info("  No configs found to parse."sv);
			return result;
		}

		for (const auto& [name, config] : configs) {
			logger::info("  Parsing {}..."sv, name);
			std::string path = fmt::format("{}|Root", name);
			std::size_t trimTo = path.size();
			auto error = StructuredErrorMessages();
			error.SetName(name);

			if (config.isArray()) {
				unsigned int size = config.size();

				for (unsigned int i = 0; i < size; ++i) {
					path = fmt::format("{}[{}]", path, i);

					auto constructor = RuleConstructor(path);
					const auto& arrayVal = config[i];
					if (arrayVal.isObject()) {
						if (!constructor.Build(arrayVal)) {
							result = false;
						}
					}
					else {
						error.FlagInvalidObject(path, Settings::JSON::GetFieldType(config));
					}
					path.resize(trimTo);
				}
			}
			else if (config.isObject()) {
				// Legacy: Format used to be { "rules": [ .. ] }
				const auto members = config.getMemberNames();
				if (members.size() == 1 && members.at(0) == "rules") {
					const auto& rules = config["rules"];
					path += "|Rules";
					std::size_t extendedTrim = path.size();
					unsigned int size = rules.size();

					for (unsigned int i = 0; i < size; ++i) {
						path = fmt::format("{}[{}]", path, i);

						auto constructor = RuleConstructor(path);
						const auto& arrayVal = rules[i];
						if (arrayVal.isObject()) {
							if (!constructor.Build(arrayVal)) {
								result = false;
							}
						}
						else {
							error.FlagInvalidObject(path, Settings::JSON::GetFieldType(config));
						}
						path.resize(extendedTrim);
					}
					path.resize(trimTo);
				}
				else {
					auto constructor = RuleConstructor(path);
					if (!constructor.Build(config)) {
						result = false;
					}
				}
			}
			else {
				error.FlagInvalidObject(path, Settings::JSON::GetFieldType(config));
			}

			if (error.Errored()) {
				result = false;
				std::unique_ptr<Failure> failure =
					std::make_unique<StructuredErrorMessages>(error);
				containerManager->RegisterFailure(name, std::move(failure));
			}
		}
#ifndef NDEBUG
		return true;
#else
		return result;
#endif
	}

	void StructuredErrorMessages::Report(const std::string& a_prefix) const {
		logger::error("{}Structure Errors:"sv, a_prefix);
		if (!invalidObjectTypes.empty()) {
			logger::error("{}  The following rules were not Objects:"sv, a_prefix);
			for (const auto& obj : invalidObjectTypes) {
				logger::error("{}    >{}"sv, a_prefix, obj);
			}
		}
	}

	bool StructuredErrorMessages::Errored() const {
		if (!invalidObjectTypes.empty()) {
			return true;
		}
		return empty;
	}

	void StructuredErrorMessages::FlagInvalidObject(const std::string& a_path, const std::string& a_type) {
		invalidObjectTypes.emplace_back(fmt::format("{} - {}", a_path, a_type));
	}

	RuleConstructor::RuleConstructor(const std::string& a_path) {
		path = a_path;
		errors = TopLevelErrors(path);
	}

	bool RuleConstructor::Build(const Json::Value& a_rule) {
		// Guaranteed: Non-Empty object.
		auto members = a_rule.getMemberNames();
		for (const auto& member : members) {
			if (member == TOP_LEVEL_FRIENDLY_NAME) {
				// Nothing. Check name earlier and silently skip here.
			}
			else if (member == TOP_LEVEL_CHANGES) {
				const auto& changes = a_rule[member];
				if (changes.isArray()) {
					std::string secondaryPath = fmt::format("{}|{}", path, TOP_LEVEL_CHANGES);
					auto size = changes.size();
					auto trimTo = secondaryPath.size();
					for (unsigned int i = 0u; i < size; ++i) {
						secondaryPath += fmt::format("[{}]", i);
						const auto& arrayVal = changes[i];
						RegisterChange(arrayVal, secondaryPath);
						secondaryPath.resize(trimTo);
					}
				}
				else if (changes.isObject()) {
					RegisterChange(changes, fmt::format("{}|{}", path, TOP_LEVEL_CHANGES));
				}
				else {
					errors.FlagUnknownKey(TOP_LEVEL_CHANGES.data(), Settings::JSON::GetFieldType(changes));
				}
			}
			else if (member == TOP_LEVEL_CONDITIONS) {
				CreateConditions(a_rule[member]);
			}
			else if (member == TOP_LEVEL_VERSION) {
				// Nothing. Check version EARLIER silently skip over here.
			}
			else {
				errors.FlagUnknownKey(member, Settings::JSON::GetFieldType(a_rule[member]));
			}
		}

		auto* manager = InventorySwapper::GetSingleton();
		if (errors.Errored()) {
			std::unique_ptr<Failure> asFailure =
				std::make_unique<TopLevelErrors>(std::move(errors));
			manager->RegisterFailure(path, std::move(asFailure));
			return false;
		}

		// TODO: Turn this into a proper error. Leave warn for debug.
		if (pendingChanges.empty()) {
			logger::warn("EMPTY CONDITIONS");
		}

		std::vector<std::size_t> ids{};
		if (!pendingConditions.empty()) {
			ids.reserve(pendingConditions.size());
			for (auto& pending : pendingConditions) {
				ids.push_back(manager->RegisterCondition(std::move(pending)));
			}
		}
		for (auto& pending : pendingChanges) {
			pending->DefineConditions(ids);
			manager->RegisterChange(std::move(pending));
		}
		return true;
	}

	void RuleConstructor::RegisterChange(const Json::Value& a_changes, const std::string& a_path) {
		// Guaranteed - Non-Empty Object
		bool hasAdd = false;
		bool hasRemove = false;
		bool hasRemoveByKeyword = false;
		auto members = a_changes.getMemberNames();
		for (const auto& member : members) {
			if (member == CHANGE_ADD) {
				hasAdd = true;
			}
			else if (member == CHANGE_REMOVE) {
				hasRemove = true;
			}
			else if (member == CHANGE_REMOVE_BY_KEYWORD) {
				hasRemoveByKeyword;
			}
			else if (member == CHANGE_COUNT) {
				// Nothing (count is acceptable)
			}
			else {
				errors.FlagUnknownKey(fmt::format("{}|{}", a_path, member), Settings::JSON::GetFieldType(a_changes[member]));
			}
		}

		if (hasRemoveByKeyword) {
			if (hasAdd) {

			}
			else {

			}
		}
		else {
			if (hasAdd && hasRemove) {

			}
			else if (hasRemove) {

			}
			else if (hasAdd) {
				auto ruleResult = Changes::CreateAddRule(a_changes, a_path);
				if (!ruleResult) {
					std::unique_ptr<Failure> failure =
						std::make_unique<Changes::AddChangeFailure>(ruleResult.error());
					errors.FlagBadCondition(a_path, std::move(failure));
					return;
				}
				std::unique_ptr<Change> change =
					std::make_unique<Changes::AddChange>(ruleResult.value());
				pendingChanges.emplace_back(std::move(change));
			}
		}
	}

	void RuleConstructor::CreateConditions(const Json::Value& a_condition) {
		if (!a_condition.isObject()) {
			errors.FlagUnknownKey(TOP_LEVEL_CONDITIONS.data(), Settings::JSON::GetFieldType(a_condition));
			return;
		}

		auto members = a_condition.getMemberNames();
		for (const auto& member : members) {
			auto type = ConditionTypeFromString(member);
			switch (type) {
			case ConditionType::PlayerSkills:
				AddPlayerSkillCondition(a_condition[member], member.starts_with("!"));
				break;
			default:
				errors.FlagUnknownConditionKey(member);
				break;
			}
		}
	}

	void RuleConstructor::AddPlayerSkillCondition(const Json::Value& a_condition, bool a_invert) {
		auto result = Conditions::CreateAVCondition(a_condition, a_invert);
		if (!result) {
			std::unique_ptr<Failure> parseError =
				std::make_unique<Conditions::AVConditionError>(result.error());
			if (a_invert) {
				errors.FlagBadCondition(fmt::format("!{}", CONDITION_PLAYER_SKILLS), std::move(parseError));
			}
			else {
				errors.FlagBadCondition(CONDITION_PLAYER_SKILLS.data(), std::move(parseError));
			}
			return;
		}
		std::unique_ptr<Condition> condition =
			std::make_unique<Conditions::AVCondition>(result.value());
		pendingConditions.emplace_back(std::move(condition));
	}

	void RuleConstructor::TopLevelErrors::Report(const std::string& a_prefix) const {
		logger::error("---- Rule Errors ----");
		if (missingChanges) {
			logger::error("Changes field was missing."sv);
		}
		if (!unknownValues.empty()) {
			logger::error("  The following values were unrecognized:"sv);
			for (const auto& val : unknownValues) {
				logger::error("    >{}"sv, val);
			}
		}
		if (!unknownConditionKeys.empty()) {
			logger::error("  The following condition keys were unrecognized:"sv);
			for (const auto& val : unknownConditionKeys) {
				logger::error("    >{}"sv, val);
			}
		}
		if (!failures.empty()) {
			logger::error("  The following failures occured:"sv);
			for (const auto& failure : failures) {
				failure.second->Report("    ");
			}
		}
	}

	bool RuleConstructor::TopLevelErrors::Errored() const {
		if (!unknownValues.empty()) {
			return true;
		}
		if (!unknownConditionKeys.empty()) {
			return true;
		}
		if (!failures.empty()) {
			return true;
		}
		return missingChanges;
	}
}