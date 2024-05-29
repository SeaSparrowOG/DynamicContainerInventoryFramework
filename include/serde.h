#pragma once

namespace Serialization {
	void SaveCallback(SKSE::SerializationInterface* a_intfc);
	void LoadCallback(SKSE::SerializationInterface* a_intfc);

	inline const std::uint32_t MapRecord = _byteswap_ulong('MAPR');
	inline const std::uint32_t UnsafeMapRecord = _byteswap_ulong('MAPU');
	inline const std::uint32_t SaveRecord = _byteswap_ulong('SSCD');
	inline const std::uint32_t SaveVersion = 2;
}