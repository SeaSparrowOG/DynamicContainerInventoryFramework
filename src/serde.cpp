#include "serde.h"
#include "containerManager.h"

namespace Serialization {
	void SaveCallback(SKSE::SerializationInterface* a_intfc) {
		if (a_intfc->OpenRecord(_byteswap_ulong('MAPR'), 0)) {
			auto size = ContainerManager::ContainerManager::GetSingleton()->handledContainers.size();
			a_intfc->WriteRecordData(&size, sizeof(size));
			for (auto& container : ContainerManager::ContainerManager::GetSingleton()->handledContainers) {
				a_intfc->WriteRecordData(&container.first->formID, sizeof(container.first->formID));
				a_intfc->WriteRecordData(&container.second.first, sizeof(container.second.first));
				a_intfc->WriteRecordData(&container.second.second, sizeof(container.second.second));
			}
		}
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc) {
		std::uint32_t type;
		std::uint32_t size;
		std::uint32_t version;

		while (a_intfc->GetNextRecordInfo(type, size, version)) {
			if (version >= 0) {
				if (type == _byteswap_ulong('MAPR')) {
					std::size_t recordSize;
					a_intfc->ReadRecordData(&recordSize, sizeof(recordSize));

					for (; recordSize > 0; --recordSize) {
						RE::FormID refBuffer = 0;
						a_intfc->ReadRecordData(&refBuffer, sizeof(refBuffer));
						RE::FormID newRef = 0;
						if (a_intfc->ResolveFormID(refBuffer, newRef)) {
							float dayAttached = -1.0f;
							bool clearedLoc = false;

							a_intfc->ReadRecordData(&clearedLoc, sizeof(clearedLoc));
							a_intfc->ReadRecordData(&dayAttached, sizeof(dayAttached));
							if (dayAttached > 0) {
								auto* foundForm = RE::TESForm::LookupByID(newRef);
								auto* foundReference = foundForm ? foundForm->As<RE::TESObjectREFR>() : nullptr;
								if (foundReference) {
									auto newVal = std::make_pair(clearedLoc, dayAttached);
									ContainerManager::ContainerManager::GetSingleton()->handledContainers[foundReference] = newVal;
								}
							}
						}
					}
				}
			}
		}
	}
}