#include "AddRule.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager::Changes
{
	void AddChange::Apply(RE::TESObjectREFR* a_target, ContainerDeltas& a_deltas) const {
		auto& counts = a_deltas.counts;
		if (randomAdd) {
			auto seed = clib_util::RNG();
			std::size_t upper = m_additions.size() - 1u;
			for (std::size_t i = 0u; i < count; ++i) {
				const auto index = seed.generate((std::size_t)0u, upper);
				auto* item = m_additions.at(index);

				auto it = counts.find(item);
				if (it == counts.end()) {
					counts.emplace(item, 1u);
				}
				else {
					(*it).second += 1u;
				}
			}
			return;
		}

		for (auto* item : m_additions) {
			auto it = counts.find(item);
			if (it == counts.end()) {
				counts.emplace(item, count);
			}
			else {
				(*it).second += count;
			}
		}
	}

	void AddChange::Report(const std::string& a_prefix) const {
		logger::info("{}Add Rule: {}"sv, a_prefix, m_name);
		logger::info("{}  Info:"sv, a_prefix);
		logger::info("{}    >Count: {}"sv, a_prefix, count);
		logger::info("{}    >Random Add: {}"sv, a_prefix, randomAdd ? "TRUE"sv : "FALSE"sv);
		logger::info("{}  Items:"sv, a_prefix);
		for (auto* item : m_additions) {
			auto* charName = item->GetName();
			if (strcmp(charName, "") == 0) {
				static auto* po3 = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");
				if (po3) {
					logger::info("{}    >{}"sv, a_prefix, clib_util::editorID::get_editorID(item));
				}
				else {
					logger::info("{}    >{:08X}"sv, a_prefix, item->GetFormID());
				}
			}
			else {
				logger::info("{}    >{}"sv, a_prefix, charName);
			}
		}

		if (!ids.empty()) {
			std::string conditionIDs = fmt::format("{}", ids.at(0u));
			std::size_t idCount = ids.size();

			if (idCount > 1u) {
				for (std::size_t i = 1u; i < idCount; ++i) {
					conditionIDs += fmt::format(" - {}", ids.at(i));
				}
			}
			logger::info("{}  Conditions: [{}]"sv, a_prefix, conditionIDs);
		}
		else {
			logger::info("{}  Conditions: [NONE]"sv, a_prefix);
		}
	}

	void AddChange::AddObject(RE::TESBoundObject* a_obj) { m_additions.emplace_back(a_obj); }
	void AddChange::SetRandomAdd() { randomAdd = true; }
	void AddChange::SetCount(std::uint16_t a_count) { count = a_count; }
	void AddChange::SetName(const std::string& a_name) { m_name = a_name; }

	void AddChangeFailure::Report(const std::string& a_prefix) const {
		logger::info("{}Add Change Failure:"sv, a_prefix);
		if (empty) {
			logger::error("{}  >Add forms resolved as empty."sv, a_prefix);
		}
		if (mixedArrayTypes) {
			logger::error("{}  >{} field has several different JSON types."sv, a_prefix, ADD_FIELD);
		}
		if (addFieldType != "") {
			logger::error("{}  >{} is of type {}."sv, a_prefix, ADD_FIELD, addFieldType);
		}
		if (countFieldType != "") {
			logger::error("{}  >{} is of type {}."sv, a_prefix, COUNT_FIELD, countFieldType);
		}
		if (randomAddFieldReason != "") {
			logger::error("{}  >{} is of type {}."sv, a_prefix, RANDOM_ADD_FIELD, randomAddFieldReason);
		}
		if (!badForms.empty()) {
			logger::error("{}  >The following forms resolved incorrectly:"sv, a_prefix);
			for (const auto& reason : badForms) {
				logger::error("{}    {}"sv, a_prefix, reason);
			}
		}
	}

	void AddChangeFailure::FlagMixedArray() { mixedArrayTypes = true; }
	void AddChangeFailure::FlagRandomAddField(const std::string& a_reason) { randomAddFieldReason = a_reason; }
	void AddChangeFailure::FlagCountField(const std::string& a_type) { countFieldType = a_type; }
	void AddChangeFailure::FlagAddField(const std::string& a_type) { addFieldType = a_type; }
	void AddChangeFailure::FlagEmpty() { empty = true; }

	void AddChangeFailure::FlagBadForm(const std::string& a_rawString, const std::string& a_reason) {
		badForms.emplace_back(fmt::format("{} - {}", a_rawString, a_reason));
	}

	bool AddChangeFailure::Errored() const {
		if (addFieldType != "") {
			return true;
		}
		if (countFieldType != "") {
			return true;
		}
		if (randomAddFieldReason != "") {
			return true;
		}
		return empty || mixedArrayTypes;
	}

	std::expected<AddChange, AddChangeFailure> CreateAddRule(const Json::Value& a_add, bool a_hasCount, const Json::Value& a_count) {
		return AddChange();
	}
}