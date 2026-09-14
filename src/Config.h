// Copyright (C) 2026 DeadOnKeyboard
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace SilentDialogue
{
	struct Settings
	{
		float wordsPerSecond{ 2.0F };
		std::uint32_t minimumSeconds{ 1 };
		std::uint32_t maximumSeconds{ 10 };
		bool forceSubtitles{ true };
		bool verboseLogging{ false };
	};

	[[nodiscard]] const Settings& GetSettings() noexcept;
	void LoadSettings();
}
