#include "Settings/Settings.h"

[[nodiscard]]
std::string FormatFailureMessage(const std::string_view reason) {
    return fmt::format("{}."
                       "Check the log at Documents/My Games/Skyrim Special Edition/SKSE/{}.log for more information."sv, 
                       reason, Plugin::NAME
    );
}

static void MessageEventCallback(SKSE::MessagingInterface::Message* msg) {
    switch (msg->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        break;
    default:
        break;
    }
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v{};
	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("SeaSparrow"sv);
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	return v;
}();

SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		REX::CRITICAL("Loaded in editor, marking as incompatible"sv);
		return false;
	}
	return true;
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface * a_skse)
{
	SKSE::InitInfo info;
	info.log = true;
	info.hook = true;
	info.trampoline = true;
	info.trampolineSize = 28u;
	SKSE::Init(a_skse, info);

#ifdef SKYRIM_GOG
	static constexpr std::array<REL::Version, 4> supported = 
	{
		SKSE::RUNTIME_SSE_1_6_1130,
		SKSE::RUNTIME_SSE_1_6_1170,
		SKSE::RUNTIME_SSE_1_6_1179,
		REL::Version(1, 6, 1179, 1) // no idea what this is still
	};
#else
	static constexpr std::array<REL::Version, 2> supported = 
	{
		SKSE::RUNTIME_SSE_1_7_104,
		SKSE::RUNTIME_SSE_1_7_99
	};	
#endif

	const auto ver = a_skse->RuntimeVersion();
	if ((ver < SKSE::RUNTIME_SSE_LATEST) && (!std::ranges::contains(supported, ver))) {
		REX::CRITICAL("Game Version: {}"sv, ver.string());
		REX::CRITICAL("Supported Versions:"sv);
		for (const auto& allowed : supported) {
			REX::CRITICAL("  - {}"sv, allowed.string());
		}
		REX::FAIL(FormatFailureMessage("You are using a version not supported by this plugin"sv));
	}

	if (!Settings::Initialize()) {
		REX::FAIL(
			FormatFailureMessage("Failed to initialize settings"sv)
		);
	}

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);
	return true;
}