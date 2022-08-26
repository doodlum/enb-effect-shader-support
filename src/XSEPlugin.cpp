
#include "EffectShaderSupport.h"
#include <API/ENBSeriesAPI.h>

ENB_API::ENBSDKALT1001* g_ENB = nullptr;

static void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		g_ENB = reinterpret_cast<ENB_API::ENBSDKALT1001*>(ENB_API::RequestENBAPI(ENB_API::SDKVersion::V1001));
		if (g_ENB) {
			logger::info("Obtained ENB API");
			g_ENB->SetCallbackFunction([](ENBCallbackType calltype) {
				switch (calltype) {
				case ENBCallbackType::ENBCallback_PostLoad:
					EffectShaderSupport::GetSingleton()->LoadJSON();
					EffectShaderSupport::GetSingleton()->UpdateUI();
					break;
				case ENBCallbackType::ENBCallback_PreSave:
					EffectShaderSupport::GetSingleton()->SaveJSON();
					break;
				case ENBCallbackType::ENBCallback_BeginFrame:
					EffectShaderSupport::GetSingleton()->Update();
					break;
				case ENBCallbackType::ENBCallback_PostReset:
					EffectShaderSupport::GetSingleton()->UpdateUI();
					break;
				}
			});
		} else
			logger::info("Unable to acquire ENB API");

		break;
	case SKSE::MessagingInterface::kDataLoaded:
		EffectShaderSupport::GetSingleton()->GetFireFXForms();
		break;
	}

}

void Init()
{
	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", MessageHandler);
	EffectShaderSupport::InstallHooks();
}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto       sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();

	logger::info("Loaded plugin");

	SKSE::Init(a_skse);

	Init();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName("PluginName");
	v.PluginVersion({ 1, 0, 0, 0 });
	v.UsesAddressLibrary(true);
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}
