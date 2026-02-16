#include "VendorContainerCache.h"

namespace Cache
{
	bool VendorContainerCache::Initialize()
	{
		return true;
	}

	bool VendorContainerCache::IsVendorContainer(RE::TESObjectREFR* a_container)
	{
		return a_container && vendorContainers.contains(a_container->GetFormID());
	}
}