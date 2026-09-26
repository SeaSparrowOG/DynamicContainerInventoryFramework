#include "hooks/hooks.h"
#include "settings/INISettings.h"
#include "settings/JSONSettings.h"
#include "merchantCache/merchantCache.h"

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
	{
		SKSE::PluginVersionData v{};

		v.PluginVersion(Plugin::VERSION);
		v.PluginName(Plugin::NAME);
		v.AuthorName("SeaSparrow"sv);
		v.UsesAddressLibrary();
		v.UsesUpdatedStructs();

		return v;
	}();

SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		return false;
	}

	return true;
}

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		Hooks::ContainerManager::GetSingleton()->WarmCache();
		MerchantCache::MerchantCache::GetSingleton()->BuildCache();
		REX::INFO("If there are any config errors, they'll show here:");
		Settings::JSON::Read();
		REX::INFO("=================================================");
		Hooks::ContainerManager::GetSingleton()->PrettyPrint();
		break;
	default:
		break;
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	constexpr std::size_t allocSize = 14u * 5u + 33u * 2u;
	SKSE::InitInfo info;
	info.hook = true;
	info.log = true;
	info.logLevel = REX::ELogLevel::Trace;
	info.logName = Plugin::NAME.data();
	info.logPattern = "[%T.%e] [%=5t] [%L] %v";
	info.trampoline = true;
	info.trampolineSize = allocSize;

	SKSE::Init(a_skse, info);

	REX::INFO("=================================================");
	REX::INFO("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());
	REX::INFO("Author: SeaSparrow");
	REX::INFO("=================================================");

	const auto ver = a_skse->RuntimeVersion();

#ifdef SKYRIM_GOG
	static constexpr std::array<REL::Version, 2> supported = 
	{
		SKSE::RUNTIME_SSE_1_6_1170,
		SKSE::RUNTIME_SSE_1_6_1179
	};

	if (!std::ranges::contains(supported, ver)) {
		REX::CRITICAL("Game Version: {}"sv, ver.string());
		REX::CRITICAL("Supported Versions:"sv);
		for (const auto& allowed : supported) {
			REX::CRITICAL("  - {}"sv, allowed.string());
		}
		REX::FAIL(
			fmt::format("You are using a version not supported by this plugin. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME)
		);
	}
#else
	static constexpr std::array<REL::Version, 2> supported = 
	{
		SKSE::RUNTIME_SSE_1_7_104,
		SKSE::RUNTIME_SSE_1_7_99
	};

	if ((ver < SKSE::RUNTIME_SSE_LATEST) && (!std::ranges::contains(supported, ver))) {
		REX::CRITICAL("Game Version: {}"sv, ver.string());
		REX::CRITICAL("Supported Versions:"sv);
		for (const auto& allowed : supported) {
			REX::CRITICAL("  - {}"sv, allowed.string());
		}
		REX::FAIL(
			fmt::format("You are using a version not supported by this plugin. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME)
		);
	}
#endif

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	Hooks::Install();
	Settings::INI::Read();
	return true;
}