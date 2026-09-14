// Copyright (C) 2026 DeadOnKeyboard
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DialogueHook.h"

#include "Config.h"

#include <MinHook.h>
#include <RE/B/BSResourceNiBinaryStream.h>
#include <RE/D/DialogueItem.h>
#include <RE/T/TESTopicInfo.h>

#include <cmath>

namespace SilentDialogue
{
	namespace
	{
		using DialogueItemCtor = RE::DialogueItem* (*)(
			RE::DialogueItem*,
			RE::TESQuest*,
			RE::TESTopic*,
			RE::TESTopicInfo*,
			RE::TESObjectREFR*);

		DialogueItemCtor g_originalCtor{ nullptr };
		constexpr std::string_view kSilentVoiceDirectory = "Data\\Sound\\Voice\\SilentDialogueUniversal\\";

		bool EndsWithInsensitive(std::string_view a_value, std::string_view a_suffix)
		{
			if (a_value.size() < a_suffix.size()) {
				return false;
			}
			return std::ranges::equal(
				a_value.substr(a_value.size() - a_suffix.size()),
				a_suffix,
				[](unsigned char a_left, unsigned char a_right) {
					return std::tolower(a_left) == std::tolower(a_right);
				});
		}

		bool ResourceExists(std::string_view a_path)
		{
			auto tryPath = [](std::string_view a_candidate) {
				RE::BSResourceNiBinaryStream stream{ std::string(a_candidate) };
				return stream.good();
			};

			if (tryPath(a_path)) {
				return true;
			}
			constexpr std::string_view dataPrefix = "Data\\";
			if (a_path.size() > dataPrefix.size() &&
				std::equal(dataPrefix.begin(), dataPrefix.end(), a_path.begin(), [](unsigned char a_left, unsigned char a_right) {
					return std::tolower(a_left) == std::tolower(a_right);
				})) {
				return tryPath(a_path.substr(dataPrefix.size()));
			}
			return false;
		}

		bool VoiceExists(std::string_view a_basePath)
		{
			if (a_basePath.empty()) {
				return false;
			}
			if (EndsWithInsensitive(a_basePath, ".fuz") ||
				EndsWithInsensitive(a_basePath, ".xwm") ||
				EndsWithInsensitive(a_basePath, ".wav")) {
				return ResourceExists(a_basePath);
			}

			for (const auto extension : { ".fuz"sv, ".xwm"sv, ".wav"sv }) {
				std::string candidate{ a_basePath };
				candidate += extension;
				if (ResourceExists(candidate)) {
					return true;
				}
			}
			return false;
		}

		std::uint32_t DecodeUtf8(std::string_view a_text, std::size_t& a_index)
		{
			const auto first = static_cast<unsigned char>(a_text[a_index++]);
			if (first < 0x80) {
				return first;
			}

			std::uint32_t codePoint{};
			std::size_t continuationCount{};
			if ((first & 0xE0) == 0xC0) {
				codePoint = first & 0x1F;
				continuationCount = 1;
			} else if ((first & 0xF0) == 0xE0) {
				codePoint = first & 0x0F;
				continuationCount = 2;
			} else if ((first & 0xF8) == 0xF0) {
				codePoint = first & 0x07;
				continuationCount = 3;
			} else {
				return first;
			}

			if (a_index + continuationCount > a_text.size()) {
				a_index = a_text.size();
				return first;
			}
			for (std::size_t i = 0; i < continuationCount; ++i) {
				const auto next = static_cast<unsigned char>(a_text[a_index]);
				if ((next & 0xC0) != 0x80) {
					return first;
				}
				++a_index;
				codePoint = (codePoint << 6) | (next & 0x3F);
			}
			return codePoint;
		}

		bool IsCjk(std::uint32_t a_codePoint)
		{
			return (a_codePoint >= 0x3040 && a_codePoint <= 0x30FF) ||
			       (a_codePoint >= 0x3400 && a_codePoint <= 0x4DBF) ||
			       (a_codePoint >= 0x4E00 && a_codePoint <= 0x9FFF) ||
			       (a_codePoint >= 0xAC00 && a_codePoint <= 0xD7AF) ||
			       (a_codePoint >= 0xF900 && a_codePoint <= 0xFAFF);
		}

		bool IsWordCodePoint(std::uint32_t a_codePoint)
		{
			if (a_codePoint < 0x80) {
				return std::isalnum(static_cast<unsigned char>(a_codePoint)) != 0 || a_codePoint == '\'';
			}
			return !IsCjk(a_codePoint) &&
			       !(a_codePoint >= 0x2000 && a_codePoint <= 0x206F) &&
			       !(a_codePoint >= 0x3000 && a_codePoint <= 0x303F);
		}

		std::uint32_t ReadingUnits(std::string_view a_text)
		{
			std::uint32_t words{};
			std::uint32_t cjkCharacters{};
			bool inWord{};

			for (std::size_t index = 0; index < a_text.size();) {
				const auto codePoint = DecodeUtf8(a_text, index);
				if (IsCjk(codePoint)) {
					if (inWord) {
						++words;
						inWord = false;
					}
					++cjkCharacters;
				} else if (IsWordCodePoint(codePoint)) {
					inWord = true;
				} else if (inWord) {
					++words;
					inWord = false;
				}
			}
			if (inWord) {
				++words;
			}

			return words + static_cast<std::uint32_t>(std::ceil(static_cast<double>(cjkCharacters) / 2.5));
		}

		std::uint32_t SelectDuration(std::string_view a_text)
		{
			const auto units = ReadingUnits(a_text);
			if (units == 0) {
				return 0;
			}

			const auto& settings = GetSettings();
			const auto seconds = static_cast<std::uint32_t>(std::ceil(
				static_cast<double>(units) / settings.wordsPerSecond + 1.0));
			return std::clamp(seconds, settings.minimumSeconds, settings.maximumSeconds);
		}

		void ProcessDialogueItem(RE::DialogueItem* a_item)
		{
			if (!a_item) {
				return;
			}

			bool replacedAny{};
			for (auto* response : a_item->responses) {
				if (!response || !response->text.c_str()) {
					continue;
				}
				const std::string_view text{ response->text.c_str() };
				const auto duration = SelectDuration(text);
				const auto* voicePath = response->voice.c_str();
				if (duration == 0 || (voicePath && VoiceExists(voicePath))) {
					continue;
				}

				std::string replacement{ kSilentVoiceDirectory };
				replacement += "silence_";
				replacement += std::to_string(duration);
				if (!VoiceExists(replacement)) {
					SKSE::log::error("Silent voice resource is missing: {}.fuz", replacement);
					continue;
				}

				response->voice = replacement.c_str();
				response->soundLip = true;
				replacedAny = true;

				if (GetSettings().verboseLogging) {
					SKSE::log::debug("Substituted {} second silent voice for: {}", duration, text);
				}
			}

			if (replacedAny && GetSettings().forceSubtitles && a_item->info) {
				a_item->info->data.flags.set(RE::TOPIC_INFO_DATA::TOPIC_INFO_FLAGS::kForceSubtitle);
			}
		}

		RE::DialogueItem* HookedDialogueItemCtor(
			RE::DialogueItem* a_self,
			RE::TESQuest* a_quest,
			RE::TESTopic* a_topic,
			RE::TESTopicInfo* a_topicInfo,
			RE::TESObjectREFR* a_speaker)
		{
			auto* result = g_originalCtor(a_self, a_quest, a_topic, a_topicInfo, a_speaker);
			ProcessDialogueItem(result);
			return result;
		}

		const char* MinHookStatus(MH_STATUS a_status)
		{
			const auto* text = MH_StatusToString(a_status);
			return text ? text : "unknown MinHook status";
		}
	}

	bool InstallDialogueHook()
	{
		const auto initializeStatus = MH_Initialize();
		if (initializeStatus != MH_OK && initializeStatus != MH_ERROR_ALREADY_INITIALIZED) {
			SKSE::log::critical("MH_Initialize failed: {}", MinHookStatus(initializeStatus));
			return false;
		}

		REL::Relocation<std::uintptr_t> target{ REL::RelocationID(34413, 35220) };
		void* original{};
		const auto createStatus = MH_CreateHook(
			reinterpret_cast<void*>(target.address()),
			reinterpret_cast<void*>(&HookedDialogueItemCtor),
			std::addressof(original));
		if (createStatus != MH_OK) {
			SKSE::log::critical("MH_CreateHook failed at 0x{:X}: {}", target.address(), MinHookStatus(createStatus));
			return false;
		}

		g_originalCtor = reinterpret_cast<DialogueItemCtor>(original);
		const auto enableStatus = MH_EnableHook(reinterpret_cast<void*>(target.address()));
		if (enableStatus != MH_OK) {
			SKSE::log::critical("MH_EnableHook failed at 0x{:X}: {}", target.address(), MinHookStatus(enableStatus));
			return false;
		}

		SKSE::log::info("Dialogue hook installed at 0x{:X}", target.address());
		return true;
	}
}
