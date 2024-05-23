#include "utility.h"

namespace Utility {
	bool IsModPresent(std::string a_modName) {
		auto* found = RE::TESDataHandler::GetSingleton()->LookupLoadedLightModByName(a_modName);
		if (!found) { found = RE::TESDataHandler::GetSingleton()->LookupLoadedModByName(a_modName); }
		if (found) return true;
		return false;
	}

	bool IsHex(std::string const& s) {
		return s.compare(0, 2, "0x") == 0
			&& s.size() > 2
			&& s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
	}

	RE::FormID StringToFormID(std::string a_str) {
		RE::FormID result;
		std::istringstream ss{ a_str };
		ss >> std::hex >> result;
		return result;
	}

	RE::TESForm* GetFormFromMod(std::string a_id, std::string a_mod) {
		RE::TESForm* response = nullptr;

		//Is mod present?
		if (!IsModPresent(a_mod)) return response;
		//Is the string a hex?
		if (!IsHex(a_id)) return response;

		RE::FormID formID = StringToFormID(a_id);
		response = RE::TESDataHandler::GetSingleton()->LookupForm(formID, a_mod);
		return response;
	}

	RE::FormID ParseFormID(const std::string& a_identifier) {
		if (const auto splitID = clib_util::string::split(a_identifier, "|"); splitID.size() == 2) {
			const auto  formID = clib_util::string::to_num<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
		}
		return static_cast<RE::FormID>(0);
	}
}