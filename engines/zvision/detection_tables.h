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

#ifndef ZVISION_DETECTION_TABLES_H
#define ZVISION_DETECTION_TABLES_H

#include "common/translation.h"

namespace ZVision {

static const PlainGameDescriptor zVisionGames[] = {
	{ "znemesis", "Zork Nemesis: The Forbidden Lands" },
	{ "zgi", "Zork: Grand Inquisitor" },
	{ 0, 0 }
};

static const char *const directoryGlobs[] = {
	"znemscr",
	"taunts", // zgi mac
	0
};

static const ZVisionGameDescription gameDescriptions[] = {

	{
		// Zork Nemesis English version
		{
			"znemesis",
			0,
			{
				{ "CSCR.ZFS", 0, "88226e51a205d2e50c67a5237f3bd5f2", 2397741 },
				{ "ASCR.ZFS", 0, "9a1e1a48a56cf12a22bad2d2e47f6c92", 917452 },
				AD_LISTEND
			},
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_ENABLE_VENUS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_NEMESIS
	},

	{
		// Zork Nemesis French version
		{
			"znemesis",
			0,
			AD_ENTRY2s("CSCR.ZFS",		"f04113357b4748c13efcb58b4629887c", 2577873,
					   "NEMESIS.STR",	"333bcb17bbb7f57cae742fbbe44f56f3", 9219),
			Common::FR_FRA,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_ENABLE_VENUS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_NEMESIS
	},

	{
		// Zork Nemesis German version
		{
			"znemesis",
			0,
			AD_ENTRY2s("CSCR.ZFS",		"f04113357b4748c13efcb58b4629887c", 2577873,
					   "NEMESIS.STR",	"3d1a12b907751653866cffc6d4dfb331", 9505),
			Common::DE_DEU,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_ENABLE_VENUS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_NEMESIS
	},

	{
		// Zork Nemesis Italian version
		{
			"znemesis",
			0,
			AD_ENTRY2s("CSCR.ZFS",		"f04113357b4748c13efcb58b4629887c", 2577873,
					   "NEMESIS.STR",	"7c568feca8d9f9ae855c47183612c305", 9061),
			Common::IT_ITA,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_ENABLE_VENUS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_NEMESIS
	},

	{
		// Zork Nemesis Korean version
		{
			"znemesis",
			0,
			{
				{ "CSCR.ZFS", 0, "88226e51a205d2e50c67a5237f3bd5f2", 2397741 },
				{ "ASCR.ZFS", 0, "127f59f96be3d13eafac665eeede080d", 765413 },
				AD_LISTEND
			},
			Common::KO_KOR,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_ENABLE_VENUS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_NEMESIS
	},

	{
		// Zork Nemesis English Mac version
		// Bugreport #11755
		{
			"znemesis",
			MetaEngineDetection::GAME_NOT_IMPLEMENTED, // Reason for being unsupported
			{
				{ "CSCR.ZFS", 0, "d:ce26cbb17bfbaa774742b3187262a7c0", 2597635 },
				{ "ASCR.ZFS", 0, "d:5ee98db1bf73983eb8148da231342085", 929931 },
				AD_LISTEND
			},
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_UNSUPPORTED,
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_ENABLE_VENUS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_NEMESIS
	},

	{
		// Zork Nemesis English demo version
		{
			"znemesis",
			"Demo",
			AD_ENTRY1s("SCRIPTS.ZFS", "64f1e881394e9462305104f99513c833", 380539),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_DEMO,
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_ENABLE_VENUS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_NEMESIS
	},

	{
		// Zork Grand Inquisitor English CD version
		{
			"zgi",
			"CD",
			AD_ENTRY1s("SCRIPTS.ZFS", "81efd40ecc3d22531e211368b779f17f", 8336944),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO5(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_GRANDINQUISITOR
	},

	{
		// Zork Grand Inquisitor French CD version, reported by ulrichh on IRC
		{
			"zgi",
			"CD",
			AD_ENTRY1s("SCRIPTS.ZFS", "4d1ec4ade7ecc9ee9ec591d43ca3d213", 8338133),
			Common::FR_FRA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO5(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_GRANDINQUISITOR
	},

	{
		// Zork Grand Inquisitor German CD version, reported by breit in bug #6760
		{
			"zgi",
			"CD",
			AD_ENTRY1s("SCRIPTS.ZFS", "b7ac7e331b9b7f884590b0b325b560c8", 8338133),
			Common::DE_DEU,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO5(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_GRANDINQUISITOR
	},

	{
		// Zork Grand Inquisitor Spanish CD version, reported by dianiu in bug #6764
		{
			"zgi",
			"CD",
			AD_ENTRY1s("SCRIPTS.ZFS", "5cdc4b99c1134053af135aae71326fd1", 8338141),
			Common::ES_ESP,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO5(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_GRANDINQUISITOR
	},

	{
		// Zork Grand Inquisitor Mac CD version, reported by macca8 in bug #11756
		{
			"zgi",
			"CD",
			AD_ENTRY2s("SCRIPTS.ZFS",  "81efd40ecc3d22531e211368b779f17f", 8336944,
					   "G0LPH10P.RAW", "c0b1f28b1cd1aaeb83c1a3985401bb14", 24462),
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_NO_FLAGS,
			GUIO5(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_GRANDINQUISITOR
	},

	{
		// Zork Grand Inquisitor English DVD version
		{
			"zgi",
			"DVD",
			AD_ENTRY1s("SCRIPTS.ZFS", "03157a3399513bfaaf8dc6d5ab798b36", 8433326),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_DVD,
#if defined(USE_MPEG2) && defined(USE_A52)
			GUIO6(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_USE_HIRES_MPEG_MOVIES, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
#else
			GUIO5(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
#endif
		},
		GID_GRANDINQUISITOR
	},

	{
		// Zork Grand Inquisitor English demo version
		{
			"zgi",
			"Demo",
			AD_ENTRY1s("SCRIPTS.ZFS", "71a2494fd2fb999347deb13401e9b998", 304239),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_DEMO,
			GUIO5(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_DOUBLE_FPS, GAMEOPTION_DISABLE_ANIM_WHILE_TURNING, GAMEOPTION_ENABLE_WIDESCREEN, GAMEOPTION_HQ_PANORAMA)
		},
		GID_GRANDINQUISITOR
	},

	{
		AD_TABLE_END_MARKER,
		GID_NONE
	}
};

} // End of namespace ZVision

#endif
