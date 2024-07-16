#pragma once

namespace Utility {
	bool IsHex(std::string const& s);
	bool IsModPresent(std::string a_modName);
	RE::FormID StringToFormID(std::string a_str);
	RE::FormID ParseFormID(const std::string& a_identifier);

	template <typename T>
	T* GetObjectFromMod(std::string a_id, std::string a_mod) {
		T* response = nullptr;
		if (!IsModPresent(a_mod)) return response;
		if (!IsHex(a_id)) return response;

		RE::FormID formID = StringToFormID(a_id);
		response = RE::TESDataHandler::GetSingleton()->LookupForm<T>(formID, a_mod);
		return response;
	}

	void GetParentChain(RE::BGSLocation* a_child, std::vector<RE::BGSLocation*>* a_parentArray);
	void ResolveLeveledList(RE::TESLeveledList* a_levItem, RE::BSScrapArray<RE::CALCED_OBJECT>* a_result, uint32_t a_count);
}