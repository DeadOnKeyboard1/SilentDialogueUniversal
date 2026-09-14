// Copyright (C) 2026 DeadOnKeyboard
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Config.h"

#include <REX/W32/KERNEL32.h>

#include <charconv>
#include <filesystem>

namespace SilentDialogue
{
	namespace
	{
		Settings g_settings;

		std::string ReadValue(const std::filesystem::path& a_path, const char* a_section, const char* a_key, const char* a_fallback)
		{
			std::array<char, 64> value{};
			REX::W32::GetPrivateProfileStringA(
				a_section,
				a_key,
				a_fallback,
				value.data(),
				static_cast<std::uint32_t>(value.size()),
				a_path.string().c_str());
			return value.data();
		}

		template <class T>
		T ParseNumber(const std::string& a_value, T a_fallback)
		{
			T parsed{};
			const auto result = std::from_chars(a_value.data(), a_value.data() + a_value.size(), parsed);
			return result.ec == std::errc{} && result.ptr == a_value.data() + a_value.size() ? parsed : a_fallback;
		}

		bool ParseBool(std::string a_value, bool a_fallback)
		{
			std::ranges::transform(a_value, a_value.begin(), [](unsigned char a_char) {
				return static_cast<char>(std::tolower(a_char));
			});
			if (a_value == "1" || a_value == "true" || a_value == "yes" || a_value == "on") {
				return true;
			}
			if (a_value == "0" || a_value == "false" || a_value == "no" || a_value == "off") {
				return false;
			}
			return a_fallback;
		}
	}

	const Settings& GetSettings() noexcept
	{
		return g_settings;
	}

	void LoadSettings()
	{
		const auto path = std::filesystem::absolute("Data/SKSE/Plugins/SilentDialogueUniversal.ini");

		g_settings.wordsPerSecond = std::clamp(
			ParseNumber(ReadValue(path, "Timing", "WordsPerSecond", "2.0"), 2.0F),
			0.5F,
			10.0F);
		g_settings.minimumSeconds = std::clamp(
			ParseNumber<std::uint32_t>(ReadValue(path, "Timing", "MinimumSeconds", "1"), 1),
			1U,
			10U);
		g_settings.maximumSeconds = std::clamp(
			ParseNumber<std::uint32_t>(ReadValue(path, "Timing", "MaximumSeconds", "10"), 10),
			g_settings.minimumSeconds,
			10U);
		g_settings.forceSubtitles = ParseBool(ReadValue(path, "Subtitles", "ForceForUnvoiced", "1"), true);
		g_settings.verboseLogging = ParseBool(ReadValue(path, "Logging", "Verbose", "0"), false);
		if (g_settings.verboseLogging) {
			spdlog::set_level(spdlog::level::debug);
		}

		SKSE::log::info(
			"Settings: WordsPerSecond={}, MinimumSeconds={}, MaximumSeconds={}, ForceForUnvoiced={}, Verbose={}",
			g_settings.wordsPerSecond,
			g_settings.minimumSeconds,
			g_settings.maximumSeconds,
			g_settings.forceSubtitles,
			g_settings.verboseLogging);
	}
}
