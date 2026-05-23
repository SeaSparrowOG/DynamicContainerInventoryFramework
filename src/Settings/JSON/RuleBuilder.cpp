#include "RuleBuilder.h"

#include "ContainerManager/Conditions/AVCondition.h"
#include "Settings/JSON/JSONSettings.h"

namespace Settings::JSON
{
	bool RuleParser::BuildRule() {
		bool hasUnknown = false;
		bool hasMissing = false;
		bool hasInvalid = false;

		auto members = _rule.getMemberNames();
		for (const auto& member : members) {
			if (!_knownFields.contains(member)) {
				_unknownFields.AddUnknownField(member);
				hasUnknown = true;
			}
		}

		const auto& changes = _rule[RULE_CHANGES];
		if (!changes) {
			hasMissing = true;
			_missingFields.AddMissingField(RULE_CHANGES);
		}

		const auto& conditions = _rule[RULE_CONDITIONS];
		if (conditions) {
			if (!conditions.isObject()) {
				hasInvalid = true;
				_invalidFields.AddInvalidField(RULE_CONDITIONS);
				goto ConditionsEnd;
			}

			members = conditions.getMemberNames();
			for (const auto& member : members) {
				if (!_knownConditionFields.contains(member)) {
					_unknownFields.AddUnknownField(member);
					hasUnknown = true;
				}
			}

			const auto& allowVendorsField = conditions[CONDITIONS_ALLOW_VENDORS];
			const auto& onlyVendorsField = conditions[CONDITIONS_ONLY_VENDORS];
			const auto& randomAddField = conditions[CONDITIONS_RANDOM_ADD];
			const auto& allowNoResetField = conditions[CONDITIONS_ALLOW_NO_RESET];
			const auto& bypassUnsafeContainersField = conditions[CONDITIONS_ALLOW_NO_RESET_OLD];
			
			if (allowVendorsField) {
				if (!allowVendorsField.isBool()) {
					hasInvalid = true;
					std::string erroredField = _configName + "|" + CONDITIONS_ALLOW_VENDORS;
					_invalidFields.AddInvalidField(erroredField);
					goto CheckOnlyVendors;
				}
				_allowVendors = allowVendorsField.asBool();
			}
		CheckOnlyVendors:
			if (onlyVendorsField) {
				if (!onlyVendorsField.isBool()) {
					hasInvalid = true;
					std::string erroredField = _configName + "|" + CONDITIONS_ONLY_VENDORS;
					_invalidFields.AddInvalidField(erroredField);
					goto CheckRandomAdd;
				}
				bool doOnlyVendors = onlyVendorsField.asBool();
				if (doOnlyVendors) {
					_allowVendors = true;
					_onlyVendors = true;
				}
			}
		CheckRandomAdd:
			if (randomAddField) {
				if (!randomAddField.isBool()) {
					hasInvalid = true;
					std::string erroredField = _configName + "|" + CONDITIONS_RANDOM_ADD;
					_invalidFields.AddInvalidField(erroredField);
					goto CheckNoReset;
				}
				_randomAdd = randomAddField.asBool();
			}
		CheckNoReset:
			if (allowNoResetField) {
				if (!allowNoResetField.isBool()) {
					hasInvalid = true;
					std::string erroredField = _configName + "|" + CONDITIONS_ALLOW_NO_RESET;
					_invalidFields.AddInvalidField(erroredField);
					goto CheckNoReset;
				}
				_allowNoReset = allowNoResetField.asBool();
			}
			else if (bypassUnsafeContainersField) {
				if (!bypassUnsafeContainersField.isBool()) {
					hasInvalid = true;
					std::string erroredField = _configName + "|" + CONDITIONS_ALLOW_NO_RESET_OLD;
					_invalidFields.AddInvalidField(erroredField);
					goto CheckNoReset;
				}
				_allowNoReset = bypassUnsafeContainersField.asBool();
			}
		}

	ConditionsEnd:

		if (hasInvalid) {
			std::unique_ptr<ContainerManager::ParseFailure> failure =
				std::make_unique<ContainerManager::InvalidFieldTypeFailure>(_invalidFields);
			_failures.emplace_back(std::move(failure));
		}
		if (hasMissing) {
			std::unique_ptr<ContainerManager::ParseFailure> failure =
				std::make_unique<ContainerManager::MissingFieldFailure>(_missingFields);
			_failures.emplace_back(std::move(failure));
		}
		if (hasUnknown) {
			std::unique_ptr<ContainerManager::ParseFailure> failure =
				std::make_unique<ContainerManager::UnknownFieldFailure>(_unknownFields);
			_failures.emplace_back(std::move(failure));
		}

		if (!_failures.empty()) {
			for (const auto& failure : _failures) {
				if (!failure->Recoverable()) {
					_errored = true;
					return false;
				}
			}
		}
		return true;
	}

	RuleParser::RuleParser(const Json::Value& canonicalizedRule,
		const std::string& configName) :
		_rule{ canonicalizedRule },
		_configName{ configName }
	{}

	RuleParser::~RuleParser() {
		auto* containerManager = ContainerManager::InventorySwapper::GetSingleton();
		if (_errored) {
			for (auto& failure : _failures) {
				containerManager->RegisterFailure(std::move(failure));
			}
			return;
		}
		std::vector<size_t> conditionIDs;
		if (!_conditions.empty()) {
			conditionIDs.reserve(_conditions.size());
			for (auto& condition : _conditions) {
				auto pos = containerManager->RegisterCondition(std::move(condition));
				conditionIDs.emplace_back(pos);
			}
		}
		// changes guaranteed not empty
		for (auto& change : _changes) {
			change->DefineConditions(conditionIDs);
			change->SetAllowNoReset(_allowNoReset);
			change->SetAllowVendors(_allowVendors);
			change->SetOnlyVendors(_onlyVendors);
			change->SetRandomAdd(_randomAdd);
			containerManager->RegisterChange(std::move(change));
		}

		// absolutely none of this is needed
		_rule.clear();
		_configName.clear();
		_changes.clear();
		_conditions.clear();
		_failures.clear();
	}

	bool ParseArray(const Json::Value& array, std::string& path) {
		auto trimTo = path.size();
		bool success = true;
		for (Json::ArrayIndex i = 0; i < array.size(); ++i) {
			path.resize(trimTo);
			path += "[" + std::to_string(i) + "]";
			const auto& arrayElement = array[i];
			if (!arrayElement.isObject()) {
				logger::critical("  - Found non-object element in: {}"sv, path);
				success = false;
				continue;
			}
			success &= ParseObject(arrayElement, path);
		}
		return true;
	}

	bool ParseObject(const Json::Value& object, const std::string& name) {
		const auto& version = object["version"];
		if (version) {
			if (!version.isUInt()) {
				logger::critical("    Config has invalid Version field."sv);
				return false;
			}
			auto required = version.asUInt();
			if (required > PARSER_VERSION) {
				logger::critical("    Config requires version {}, but CDF is version {}. Update CDF on Nexus."sv, required, PARSER_VERSION);
				return false;
			}
		}
		// Legacy: Until CDF 3.0, configs were structured like this: { "rules": [ ... ] }.
		const auto& rules = object["rules"];
		if (rules && rules.isArray()) {
			std::string arrayPath = name;
			return ParseArray(rules, arrayPath);
		}
		auto parser = RuleParser(object, name);
		return parser.BuildRule();
	}

	bool ParseSuccessfulConfigs() {
		logger::info("Gathering and parsing canonicalized configs..."sv);
		auto* settingsHolder = Settings::JSON::Holder::GetSingleton();
		if (!settingsHolder) {
			logger::critical("  >Failed to get internal settings holder, aborting load..."sv);
			return false;
		}
		
		bool success = true;
		const auto& configMap= settingsHolder->GetConfigs();
		if (!configMap.empty()) {
			logger::info("Found {} configuration files."sv, configMap.size());
		}
		else {
			logger::info("Didn't find any configuration files."sv);
			return true;
		}

		for (const auto& [name, config] : configMap) {
			logger::info("  - Parsing {}..."sv, name);
			if (config.isObject()) {
				success &= ParseObject(config, name);
			}
			else if (config.isArray()) {
				std::string arrayPath = name;
				success &= ParseArray(config, arrayPath);
			}
			else {
				logger::critical("    Found non Object or Array element.");
				success = false;
			}
		}

		if (!success) {
			logger::critical("Parsing failed with unrecoverable errors."sv);
			auto* containerManager = ContainerManager::InventorySwapper::GetSingleton();
			containerManager->Report();
		}
		else {
			logger::info("Parsing finished successfully."sv);
		}
		return success;
	}
}