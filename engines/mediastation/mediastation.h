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

#ifndef MEDIASTATION_H
#define MEDIASTATION_H

#include "audio/mixer.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/fs.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/serializer.h"
#include "common/events.h"
#include "common/util.h"
#include "engines/engine.h"
#include "engines/savestate.h"

#include "mediastation/detection.h"
#include "mediastation/datafile.h"
#include "mediastation/boot.h"
#include "mediastation/context.h"
#include "mediastation/asset.h"
#include "mediastation/cursors.h"
#include "mediastation/graphics.h"

namespace MediaStation {

struct MediaStationGameDescription;
class Hotspot;
class Bitmap;

// Most Media Station titles follow this file structure from the root directory
// of the CD-ROM:
// - [TITLE].EXE (main game executable, name vares based on game)
// - DATA/ (subdirectory that holds actual game data including bytecode)
//   - 100.CXT
//   - ... other CXTs, varies per title
static const char *const directoryGlobs[] = {
	"DATA", // For most titles
	"program", // For D.W. the Picky Eater
	"PZDATA", // For Puzzle Castle demo
	nullptr
};

class MediaStationEngine : public Engine {
public:
	MediaStationEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~MediaStationEngine() override;

	uint32 getFeatures() const;
	Common::String getGameId() const;
	Common::Platform getPlatform() const;
	const char *getAppName() const;
	bool hasFeature(EngineFeature f) const override {
		return
		    (f == kSupportsReturnToLauncher);
	};

	bool isFirstGenerationEngine();
	void processEvents();
	void refreshActiveHotspot();
	void addDirtyRect(const Common::Rect &rect) { _dirtyRects.push_back(rect); }
	void draw();

	void registerAsset(Asset *assetToAdd);
	void scheduleScreenBranch(uint screenId);
	void scheduleContextRelease(uint contextId);

	Asset *getAssetById(uint assetId);
	Asset *getAssetByChunkReference(uint chunkReference);
	Function *getFunctionById(uint functionId);
	ScriptValue *getVariable(uint variableId);
	VideoDisplayManager *getDisplayManager() { return _displayManager; }

	ScriptValue callBuiltInFunction(BuiltInFunction function, Common::Array<ScriptValue> &args);
	Common::RandomSource _randomSource;

	Context *_currentContext = nullptr;

	Common::Point _mousePos;
	bool _needsHotspotRefresh = false;

protected:
	Common::Error run() override;

private:
	Common::Event _event;
	Common::FSNode _gameDataDir;
	const ADGameDescription *_gameDescription;
	Common::Array<Common::Rect> _dirtyRects;

	// In Media Station, only the cursors are stored in the executable; everything
	// else is in the Context (*.CXT) data files.
	CursorManager *_cursor;
	void setCursor(uint id);

	VideoDisplayManager *_displayManager = nullptr;

	Boot *_boot = nullptr;
	Common::Array<Asset *> _assets;
	Common::SortedArray<SpatialEntity *, const SpatialEntity *> _spatialEntities;
	Common::HashMap<uint, Context *> _loadedContexts;
	Asset *_currentHotspot = nullptr;
	uint _requestedScreenBranchId = 0;
	Common::Array<uint> _requestedContextReleaseId;

	void doBranchToScreen();
	Context *loadContext(uint32 contextId);
	void releaseContext(uint32 contextId);
	Asset *findAssetToAcceptMouseEvents();

	static int compareAssetByZIndex(const SpatialEntity *a, const SpatialEntity *b);
};

extern MediaStationEngine *g_engine;
#define SHOULD_QUIT ::MediaStation::g_engine->shouldQuit()

} // End of namespace MediaStation

#endif // MEDIASTATION_H
