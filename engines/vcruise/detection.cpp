/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "base/plugins.h"

#include "common/config-manager.h"
#include "common/language.h"

#include "engines/advancedDetector.h"

#include "vcruise/detection.h"

static const PlainGameDescriptor g_vcruiseGames[] = {
	{"ad2044", "A.D. 2044"},
	{"reah", "Reah: Face the Unknown"},
	{"schizm", "Schizm: Mysterious Journey"},
	{nullptr, nullptr}
};

static const char *const g_vcruiseDirectoryGlobs[] = {
	"Sfx",
	"Log",
	"Waves-12",
	"Waves-22",
	"WAVE-01",
	nullptr
};

#include "vcruise/detection_tables.h"

class VCruiseMetaEngineDetection : public AdvancedMetaEngineDetection<VCruise::VCruiseGameDescription> {
public:
	VCruiseMetaEngineDetection() : AdvancedMetaEngineDetection(VCruise::gameDescriptions, g_vcruiseGames) {
		_guiOptions = GUIO5(GAMEOPTION_FAST_ANIMATIONS, GAMEOPTION_INCREASE_DRAG_DISTANCE, GAMEOPTION_LAUNCH_DEBUG, GAMEOPTION_SKIP_MENU, GAMEOPTION_PRELOAD_SOUNDS);
		_maxScanDepth = 3;
		_directoryGlobs = g_vcruiseDirectoryGlobs;
		_flags = kADFlagCanPlayUnknownVariants;
	}

	const char *getName() const override {
		return "vcruise";
	}

	const char *getEngineName() const override {
		return "V-Cruise";
	}

	const char *getOriginalCopyright() const override {
		return "V-Cruise (C) LK Avalon";
	}

	DetectedGame toDetectedGame(const ADDetectedGame &adGame, ADDetectedGameExtraInfo *extraInfo) const override {
		DetectedGame game = AdvancedMetaEngineDetection::toDetectedGame(adGame, extraInfo);

		VCruise::VCruiseGameID gameID = reinterpret_cast<const VCruise::VCruiseGameDescription *>(adGame.desc)->gameID;

		if ((adGame.desc->flags & VCruise::VCRUISE_GF_FORCE_LANGUAGE) == 0) {
			if (gameID == VCruise::GID_AD2044) {
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::EN_ANY));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::PL_POL));
			} else if (gameID == VCruise::GID_REAH) {
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::EN_ANY));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::NL_NLD));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::FR_FRA));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::IT_ITA));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::DE_DEU));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::PL_POL));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::ES_ESP));
			} else if (gameID == VCruise::GID_SCHIZM) {
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::EN_USA));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::EN_GRB));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::NL_NLD));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::FR_FRA));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::IT_ITA));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::DE_DEU));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::PL_POL));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::ES_ESP));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::EL_GRC));
				game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::RU_RUS));

				if (adGame.desc->flags & VCruise::VCRUISE_GF_BUL_LANGUAGE)
					game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::BG_BUL));

				if (adGame.desc->flags & VCruise::VCRUISE_GF_TWN_LANGUAGE)
					game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::ZH_TWN));

				if (adGame.desc->flags & VCruise::VCRUISE_GF_JPN_LANGUAGE)
					game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::JA_JPN));

				if (adGame.desc->flags & VCruise::VCRUISE_GF_HUN_LANGUAGE)
					game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::HU_HUN));

				if (adGame.desc->flags & VCruise::VCRUISE_GF_CHN_LANGUAGE)
					game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::ZH_CHN));

				if (adGame.desc->flags & VCruise::VCRUISE_GF_CZE_LANGUAGE)
					game.appendGUIOptions(Common::getGameGUIOptionsDescriptionLanguage(Common::CS_CZE));
			}
		}

		return game;
	}
};

REGISTER_PLUGIN_STATIC(VCRUISE_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, VCruiseMetaEngineDetection);
