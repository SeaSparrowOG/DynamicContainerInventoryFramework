#include "serde.h"
#include "containerManager.h"

namespace Serialization {
	void SaveCallback(SKSE::SerializationInterface* a_intfc) {
		if (a_intfc->OpenRecord(MapRecord, SaveVersion)) {
			auto* map = ContainerManager::ContainerManager::GetSingleton()->GetHandledContainers();
			auto size = map->size();
			a_intfc->WriteRecordData(&size, sizeof(size));
			for (auto& container : *map) {
				a_intfc->WriteRecordData(&container.first, sizeof(container.first));
				a_intfc->WriteRecordData(&container.second.first, sizeof(container.second.first));
				a_intfc->WriteRecordData(&container.second.second, sizeof(container.second.second));
			}
		}

		if (a_intfc->OpenRecord(UnsafeMapRecord, SaveVersion)) {
			auto* map = ContainerManager::ContainerManager::GetSingleton()->GetHandledUnsafeContainers();
			auto size = map->size();
			a_intfc->WriteRecordData(&size, sizeof(size));
			for (auto& container : *map) {
				a_intfc->WriteRecordData(&container.first, sizeof(container.first));
				a_intfc->WriteRecordData(&container.second, sizeof(container.second));
			}
		}
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc) {
		std::uint32_t type;
		std::uint32_t size;
		std::uint32_t version;
		auto* map = ContainerManager::ContainerManager::GetSingleton()->GetHandledContainers();
		map->clear();

		auto* unsafeMap = ContainerManager::ContainerManager::GetSingleton()->GetHandledUnsafeContainers();
		unsafeMap->clear();

		while (a_intfc->GetNextRecordInfo(type, version, size)) {
			if (type == MapRecord) {
				if (version == SaveVersion) {
					std::size_t recordSize;
					a_intfc->ReadRecordData(&recordSize, sizeof(recordSize));
					recordSize--;
					for (; recordSize > 0; --recordSize) {
						RE::FormID refBuffer = 0;
						RE::FormID newRef = 0;
						float dayAttached = -1.0f;
						bool clearedLoc = false;

						a_intfc->ReadRecordData(&refBuffer, sizeof(refBuffer));
						a_intfc->ResolveFormID(refBuffer, newRef);
						a_intfc->ReadRecordData(&clearedLoc, sizeof(clearedLoc));
						a_intfc->ReadRecordData(&dayAttached, sizeof(dayAttached));
						if (newRef > 0) {
							auto newVal = std::make_pair(clearedLoc, dayAttached);
							(*map)[newRef] = newVal;
						}
					}
				}
			}
			else if (type == UnsafeMapRecord) {
				if (version == SaveVersion) {
					std::size_t recordSize;
					a_intfc->ReadRecordData(&recordSize, sizeof(recordSize));
					recordSize--;
					for (; recordSize > 0; --recordSize) {
						RE::FormID refBuffer = 0;
						RE::FormID newRef = 0;
						bool used = false;

						a_intfc->ReadRecordData(&refBuffer, sizeof(refBuffer));
						a_intfc->ResolveFormID(refBuffer, newRef);
						a_intfc->ReadRecordData(&used, sizeof(used));
						if (newRef > 0) {
							(*unsafeMap)[newRef] = used;
						}
					}
				}
			}
		}
	}
}