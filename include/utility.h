#pragma once

namespace Utility {
	bool IsHex(std::string const& s);
	bool IsModPresent(std::string a_modName);
	RE::FormID StringToFormID(std::string a_str);
	RE::TESObjectMISC* GetBoundObjectFromMod(std::string a_id, std::string a_mod);
}