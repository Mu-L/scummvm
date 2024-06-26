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

#include "common/translation.h"

#include "engines/advancedDetector.h"
#include "gui/message.h"

#include "plumbers/plumbers.h"

namespace Plumbers {
const char *PlumbersGame::getGameId() const { return _gameDescription->gameId; }
Common::Platform PlumbersGame::getPlatform() const { return _gameDescription->platform; }
} // End of namespace Plumbers

class PlumbersMetaEngine : public AdvancedMetaEngine<ADGameDescription> {
	const char *getName() const override {
		return "plumbers";
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override;
	bool hasFeature(MetaEngineFeature f) const override;
};

Common::Error PlumbersMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	if (desc->platform == Common::kPlatform3DO) {
#ifdef USE_RGB_COLOR
		*engine = new Plumbers::PlumbersGame3DO(syst, desc);
#else
		 // I18N: Plumbers is the title of the game. 3DO is the name of platform
		GUI::MessageDialog dialog(_("3DO Plumbers requires RGB support."));
		dialog.runModal();
		return Common::kUnsupportedColorMode;
#endif
	} else
		*engine = new Plumbers::PlumbersGameWindows(syst, desc);
	return Common::kNoError;
}

bool PlumbersMetaEngine::hasFeature(MetaEngineFeature f) const {
	return false;
}

#if PLUGIN_ENABLED_DYNAMIC(PLUMBERS)
	REGISTER_PLUGIN_DYNAMIC(PLUMBERS, PLUGIN_TYPE_ENGINE, PlumbersMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(PLUMBERS, PLUGIN_TYPE_ENGINE, PlumbersMetaEngine);
#endif
