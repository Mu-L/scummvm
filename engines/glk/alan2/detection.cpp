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

#include "glk/alan2/detection.h"
#include "glk/alan2/detection_tables.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/md5.h"
#include "engines/game.h"

namespace Glk {
namespace Alan2 {

void Alan2MetaEngine::getSupportedGames(PlainGameList &games) {
	for (const PlainGameDescriptor *pd = ALAN2_GAME_LIST; pd->gameId; ++pd) {
		games.push_back(*pd);
	}
}

const GlkDetectionEntry* Alan2MetaEngine::getDetectionEntries() {
	return ALAN2_GAMES;
}

GameDescriptor Alan2MetaEngine::findGame(const char *gameId) {
	for (const PlainGameDescriptor *pd = ALAN2_GAME_LIST; pd->gameId; ++pd) {
		if (!strcmp(gameId, pd->gameId))
			return *pd;
	}

	return GameDescriptor::empty();
}

bool Alan2MetaEngine::detectGames(const Common::FSList &fslist, DetectedGames &gameList) {
	// Loop through the files of the folder
	for (Common::FSList::const_iterator file = fslist.begin(); file != fslist.end(); ++file) {
		// Check for a recognised filename
		if (file->isDirectory())
			continue;
		Common::String filename = file->getName();
		bool hasExt = filename.hasSuffixIgnoreCase(".acd");
		if (!hasExt)
			continue;

		// Open up the file and calculate the md5
		Common::File gameFile;
		if (!gameFile.open(*file))
			continue;
		uint32 version = gameFile.readUint32BE();
		if (version != MKTAG(2, 8, 1, 0) && version != MKTAG(2, 6, 0, 0))
			continue;

		gameFile.seek(0);
		Common::String md5 = Common::computeStreamMD5AsString(gameFile, 5000);
		size_t filesize = gameFile.size();
		gameFile.close();

		// Check for known games
		const GlkDetectionEntry *p = ALAN2_GAMES;
		while (p->_gameId && (md5 != p->_md5 || filesize != p->_filesize))
			++p;

		if (!p->_gameId) {
			const PlainGameDescriptor &desc = ALAN2_GAME_LIST[0];
			gameList.push_back(GlkDetectedGame(desc.gameId, desc.description, filename, md5, filesize));
		} else {
			PlainGameDescriptor gameDesc = findGame(p->_gameId);
			gameList.push_back(GlkDetectedGame(p->_gameId, gameDesc.description, p->_extra, filename, p->_language));
		}
	}

	return !gameList.empty();
}

void Alan2MetaEngine::detectClashes(Common::StringMap &map) {
	for (const PlainGameDescriptor *pd = ALAN2_GAME_LIST; pd->gameId; ++pd) {
		if (map.contains(pd->gameId))
			error("Duplicate game Id found - %s", pd->gameId);
		map[pd->gameId] = "";
	}
}

} // End of namespace Alan2
} // End of namespace Glk
