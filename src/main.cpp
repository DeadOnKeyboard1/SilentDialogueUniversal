// Copyright (C) 2026 DeadOnKeyboard
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Config.h"
#include "DialogueHook.h"

#include <spdlog/sinks/basic_file_sink.h>

namespace
{
	void InitializeLogging()
	{
		auto logDirectory = SKSE::log::log_directory();
		if (!logDirectory) {
			SKSE::stl::report_and_fail("SKSE log directory is unavailable");
		}

		*logDirectory /= "SilentDialogueUniversal.log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logDirectory->string(), true);
		auto logger = std::make_shared<spdlog::logger>("global log", std::move(sink));
		spdlog::set_default_logger(std::move(logger));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::info);
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	InitializeLogging();

	const auto runtime = REL::Module::get().version();
	SKSE::log::info("Silent Dialogue Universal 1.0.0 loading for Skyrim {}", runtime.string());
	SKSE::log::info("Detected runtime family: {}", REL::Module::IsSE() ? "SE" : "AE");

	SilentDialogue::LoadSettings();
	if (!SilentDialogue::InstallDialogueHook()) {
		SKSE::log::critical("Plugin initialization failed; no game code was patched");
		return false;
	}

	SKSE::log::info("Silent Dialogue Universal loaded successfully");
	return true;
}
