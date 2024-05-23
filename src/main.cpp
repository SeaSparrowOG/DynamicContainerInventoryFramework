#include <spdlog/sinks/basic_file_sink.h>
#include "containerManager.h"
#include "eventManager.h"
#include "serde.h"
#include "settings.h"

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");

    auto pluginName = Version::NAME;
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));

    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);

    //Pattern
    spdlog::set_pattern("%v");
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
    switch (a_message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        Events::ContainerLoadedEvent::GetSingleton()->RegisterListener();
        ContainerManager::ContainerManager::GetSingleton()->InitializeData();
        Settings::ReadSettings();
        break;
    default:
        break;
    }
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;
    v.PluginVersion({ Version::MAJOR, Version::MINOR, Version::PATCH });
    v.PluginName(Version::NAME);
    v.AuthorName(Version::PROJECT_AUTHOR);
    v.UsesAddressLibrary();
    v.UsesUpdatedStructs();
    v.CompatibleVersions({
        SKSE::RUNTIME_1_6_1130,
        _1_6_1170 });
    return v;
    }();
#else 
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "EnchantmentArtExtender";
    a_info->version = 1;

    const auto ver = a_skse->RuntimeVersion();
    if (ver
#	ifndef SKYRIMVR
        < SKSE::RUNTIME_1_5_39
#	else
        > SKSE::RUNTIME_VR_1_4_15_1
#	endif
        ) {
        SKSE::log::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
        return false;
    }

    return true;
}
#endif

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse) {
    SetupLog();
    _loggerInfo("Starting up {}.", Version::NAME);
    _loggerInfo("Plugin Version: {}.", Version::VERSION);
    _loggerInfo("Version build:");

#ifdef SKYRIM_AE
    _loggerInfo("    >Latest Version.");
#else
    _loggerInfo("    >1.5 Version. Do not report ANY issues with this version.");
#endif
    _loggerInfo("----------------------------------------------");
    SKSE::Init(a_skse);
    const auto serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID(_byteswap_ulong('SSCD'));
    serialization->SetSaveCallback(Serialization::SaveCallback);
    serialization->SetLoadCallback(Serialization::LoadCallback);

    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);
    return true;
}