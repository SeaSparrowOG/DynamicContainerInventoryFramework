#pragma once

namespace Cache
{
	class VendorContainerCache : public REX::Singleton<VendorContainerCache>
	{
	public:
		bool Initialize();
		bool IsVendorContainer(RE::TESObjectREFR* a_container);

	private:
		std::unordered_set<RE::FormID> vendorContainers{};
	};
}