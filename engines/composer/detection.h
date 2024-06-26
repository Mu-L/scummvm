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

#ifndef COMPOSER_DETECTION_H
#define COMPOSER_DETECTION_H

#include "engines/advancedDetector.h"

namespace Composer {

enum GameType {
	GType_ComposerV1,
	GType_ComposerV2
};

enum GameFileTypes {
	GAME_CONFIGFILE     = 1 << 0,    // Game configuration
	GAME_SCRIPTFILE     = 1 << 1,    // Game script
	GAME_EXECUTABLE     = 1 << 2     // Game executable
};

struct ComposerGameDescription {
	AD_GAME_DESCRIPTION_HELPERS(desc);

	ADGameDescription desc;

	int gameType;
};

} // End of namespace Composer

#endif // COMPOSER_DETECTION_H
