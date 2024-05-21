#pragma once

namespace Utility {
	bool IsHex(std::string const& s);
	bool IsModPresent(std::string a_modName);
	RE::FormID StringToFormID(std::string a_str);

	template <typename T>
	T* GetObjectFromMod(std::string a_id, std::string a_mod) {
		T* response = nullptr;

		//Is mod present?
		if (!IsModPresent(a_mod)) return response;
		//Is the string a hex?
		if (!IsHex(a_id)) return response;

		RE::FormID formID = StringToFormID(a_id);
		response = RE::TESDataHandler::GetSingleton()->LookupForm<T>(formID, a_mod);
		return response;
	}

	RE::TESForm* GetFormFromMod(std::string a_id, std::string a_mod);
}