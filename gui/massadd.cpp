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

#include "engines/metaengine.h"
#include "common/algorithm.h"
#include "common/config-manager.h"
#include "common/debug.h"
#include "common/system.h"
#include "common/taskbar.h"
#include "common/translation.h"

#include "engines/advancedDetector.h"

#include "gui/massadd.h"

#ifndef DISABLE_MASS_ADD
namespace GUI {

/*
TODO:
- Themify this dialog
- Add a ListWidget showing all the games we are going to add, and update it live
- Add a 'busy' mouse cursor (animated?) which indicates to the user that
  something is in progress, and show this cursor while we scan
*/

enum {
	// Upper bound (im milliseconds) we want to spend in handleTickle.
	// Setting this low makes the GUI more responsive but also slows
	// down the scanning.
	kMaxScanTime = 50
};

enum {
	kOkCmd = 'OK  ',
	kCancelCmd = 'CNCL'
};



MassAddDialog::MassAddDialog(const Common::FSNode &startDir)
	: Dialog("MassAdd"),
	_dirsScanned(0),
	_oldGamesCount(0),
	_dirTotal(0),
	_okButton(nullptr),
	_dirProgressText(nullptr),
	_gameProgressText(nullptr) {

	Common::U32StringArray l;

	// The dir we start our scan at
	_scanStack.push(startDir);

	// Removed for now... Why would you put a title on mass add dialog called "Mass Add Dialog"?
	// new StaticTextWidget(this, "massadddialog_caption", "Mass Add Dialog");

	_dirProgressText = new StaticTextWidget(this, "MassAdd.DirProgressText",
						_("... progress ..."));

	_gameProgressText = new StaticTextWidget(this, "MassAdd.GameProgressText",
						_("... progress ..."));

	_dirProgressText->setAlign(Graphics::kTextAlignCenter);
	_gameProgressText->setAlign(Graphics::kTextAlignCenter);

	_list = new MassAddListWidget(this, "MassAdd.GameList");
	_list->setEditable(false);
	_list->setNumberingMode(kListNumberingOff);
	_list->setList(l);

	_okButton = new ButtonWidget(this, "MassAdd.Ok", _("OK"), Common::U32String(), kOkCmd, Common::ASCII_RETURN);
	_okButton->setEnabled(false);

	new ButtonWidget(this, "MassAdd.Cancel", _("Cancel"), Common::U32String(), kCancelCmd, Common::ASCII_ESCAPE);

	// Build a map from all configured game paths to the targets using them
	const Common::ConfigManager::DomainMap &domains = ConfMan.getGameDomains();
	Common::ConfigManager::DomainMap::const_iterator iter;
	for (iter = domains.begin(); iter != domains.end(); ++iter) {
		Common::Path path(Common::Path::fromConfig(iter->_value.getVal("path")));

		// Remove trailing slash, so that "/foo" and "/foo/" match.
		// This works around a bug in the POSIX FS code (and others?)
		// where paths are not normalized (so FSNodes referring to identical
		// FS objects may return different values in path()).
		path.removeTrailingSeparators();
		if (!path.empty()) {
			_pathToTargets[path].push_back(iter->_key);
		}
	}
}

struct GameTargetLess {
	bool operator()(const DetectedGame &x, const DetectedGame &y) const {
		return x.preferredTarget.compareToIgnoreCase(y.preferredTarget) < 0;
	}
};

struct GameDescLess {
	bool operator()(const DetectedGame &x, const DetectedGame &y) const {
		return x.description.compareToIgnoreCase(y.description) < 0;
	}
};


void MassAddDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
#if defined(USE_TASKBAR)
	// Remove progress bar and count from taskbar
	g_system->getTaskbarManager()->setProgressState(Common::TaskbarManager::kTaskbarNoProgress);
	g_system->getTaskbarManager()->setCount(0);
#endif

	// FIXME: It's a really bad thing that we use two arbitrary constants
	if (cmd == kOkCmd) {
		// Sort the detected games. This is not strictly necessary, but nice for
		// people who want to edit their config file by hand after a mass add.
		Common::sort(_games.begin(), _games.end(), GameTargetLess());
		// Add all the detected games to the config
		for (auto &game : _games) {
			// Make sure the game is selected
			if (game.isSelected) {
				debug(1, "  Added gameid '%s', desc '%s'",
					game.gameId.c_str(),
					game.description.c_str());
				game.gameId = EngineMan.createTargetForGame(game);
			}
		}

		// Write everything to disk
		ConfMan.flushToDisk();

		// And scroll to first detected game
		if (!_games.empty()) {
			Common::sort(_games.begin(), _games.end(), GameDescLess());
			ConfMan.set("temp_selection", _games.front().gameId);
		}

		close();
	} else if (cmd == kCancelCmd) {
		// User cancelled, so we don't do anything and just leave.
		_games.clear();
		close();
	} else if (cmd == kListSelectionChangedCmd) {
		// Select / unselect game from list
		int curretScrollPos = _list->getCurrentScrollPos();
		_games[_list->getSelected()].isSelected = !_games[_list->getSelected()].isSelected;
		updateGameList();
		_list->scrollTo(curretScrollPos);
	} else {
		Dialog::handleCommand(sender, cmd, data);
	}
}

void MassAddDialog::updateGameList() {
	// Update list to correctly display selected / unselected games
	Common::U32StringArray l;
	_list->setList(l);
	_list->clearSelectedList();

	for (const auto &game : _games) {
		Common::U32String displayString = game.isSelected ? Common::String("[x] ") + game.description : Common::String("[\u2000] ") + game.description;
		_list->append(displayString);
		_list->appendToSelectedList(game.isSelected);
	}
}

void MassAddDialog::handleTickle() {
	if (_scanStack.empty())
		return;	// We have finished scanning

	uint32 t = g_system->getMillis();

	// Perform a breadth-first scan of the filesystem.
	while (!_scanStack.empty() && (g_system->getMillis() - t) < kMaxScanTime) {
		Common::FSNode dir = _scanStack.pop();

		Common::FSList files;
		if (!dir.getChildren(files, Common::FSNode::kListAll)) {
			continue;
		}

		// Run the detector on the dir
		DetectionResults detectionResults = EngineMan.detectGames(files, (ADGF_WARNING | ADGF_UNSUPPORTED | ADGF_ADDON), true);

		if (detectionResults.foundUnknownGames()) {
			Common::U32String report = detectionResults.generateUnknownGameReport(false, 80);
			g_system->logMessage(LogMessageType::kInfo, report.encode().c_str());
		}

		// Just add all detected games / game variants. If we get more than one,
		// that either means the directory contains multiple games, or the detector
		// could not fully determine which game variant it was seeing. In either
		// case, let the user choose which entries he wants to keep.
		//
		// However, we only add games which are not already in the config file.
		DetectedGames candidates = detectionResults.listRecognizedGames();
		for (const auto &cand : candidates) {
			const DetectedGame &result = cand;

			Common::Path path = dir.getPath();
			path.removeTrailingSeparators();

			// Check for existing config entries for this path/engineid/gameid/lang/platform combination
			if (_pathToTargets.contains(path)) {
				Common::String resultPlatformCode = Common::getPlatformCode(result.platform);
				Common::String resultLanguageCode = Common::getLanguageCode(result.language);

				bool duplicate = false;
				const Common::StringArray &targets = _pathToTargets[path];
				for (const auto &target : targets) {
					// If the engineid, gameid, platform and language match -> skip it
					Common::ConfigManager::Domain *dom = ConfMan.getDomain(target);
					assert(dom);

					if ((!dom->contains("engineid") || (*dom)["engineid"] == result.engineId) &&
						(*dom)["gameid"] == result.gameId &&
					    dom->getValOrDefault("platform") == resultPlatformCode &&
						parseLanguage(dom->getValOrDefault("language")) == parseLanguage(resultLanguageCode)) {
						duplicate = true;
						break;
					}
				}
				if (duplicate) {
					_oldGamesCount++;
					continue;	// Skip duplicates
				}
			}
			_games.push_back(result);

			_list->append(result.description);
		}

		for (DetectedGame &game : _games) {
			game.isSelected = true;
		}

		updateGameList();

		// Recurse into all subdirs
		for (const auto &file : files) {
			if (file.isDirectory()) {
				_scanStack.push(file);

				_dirTotal++;
			}
		}

		_dirsScanned++;

#if defined(USE_TASKBAR)
		g_system->getTaskbarManager()->setProgressValue(_dirsScanned, _dirTotal);
		g_system->getTaskbarManager()->setCount(_games.size());
#endif
	}


	// Update the dialog
	Common::U32String buf;

	if (_scanStack.empty()) {
		// Enable the OK button
		_okButton->setEnabled(true);

		buf = _("Scan complete!");
		_dirProgressText->setLabel(buf);

		buf = Common::U32String::format(_("Discovered %d new games, ignored %d previously added games."), _games.size(), _oldGamesCount);
		_gameProgressText->setLabel(buf);

	} else {
		buf = Common::U32String::format(_("Scanned %d directories ..."), _dirsScanned);
		_dirProgressText->setLabel(buf);

		buf = Common::U32String::format(_("Discovered %d new games, ignored %d previously added games ..."), _games.size(), _oldGamesCount);
		_gameProgressText->setLabel(buf);
	}

	if (_games.size() > 0) {
		_list->scrollToEnd();
	}

	drawDialog(kDrawLayerForeground);
}


} // End of namespace GUI

#endif // DISABLE_MASS_ADD
