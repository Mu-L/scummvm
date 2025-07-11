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

#include "common/scummsys.h"

#if defined(USE_CLOUD) && defined(USE_LIBCURL)
#include "backends/cloud/cloudmanager.h"
#endif
#include "common/file.h"
#include "common/system.h"

#if !defined(DISABLE_DEFAULT_SAVEFILEMANAGER)

#include "backends/saves/default/default-saves.h"

#include "common/savefile.h"
#include "common/util.h"
#include "common/fs.h"
#include "common/archive.h"
#include "common/config-manager.h"
#include "common/compression/deflate.h"

#include <errno.h>	// for removeSavefile()

#if defined(USE_CLOUD) && defined(USE_LIBCURL)
const char *const DefaultSaveFileManager::TIMESTAMPS_FILENAME = "timestamps";
#endif

DefaultSaveFileManager::DefaultSaveFileManager() {
}

DefaultSaveFileManager::DefaultSaveFileManager(const Common::Path &defaultSavepath) {
	ConfMan.registerDefault("savepath", defaultSavepath);
}


void DefaultSaveFileManager::checkPath(const Common::FSNode &dir) {
	clearError();
	if (!dir.exists()) {
		if (!dir.createDirectory()) {
			setError(Common::kPathDoesNotExist, Common::String::format("Failed to create directory '%s'", dir.getPath().toString(Common::Path::kNativeSeparator).c_str()));
		}
	} else if (!dir.isDirectory()) {
		setError(Common::kPathNotDirectory, Common::String::format("The savepath '%s' is not a directory", dir.getPath().toString(Common::Path::kNativeSeparator).c_str()));
	}
}

void DefaultSaveFileManager::updateSavefilesList(Common::StringArray &lockedFiles) {
	//make it refresh the cache next time it lists the saves
	_cachedDirectory = "";

	//remember the locked files list because some of these files don't exist yet
	_lockedFiles = lockedFiles;
}

Common::StringArray DefaultSaveFileManager::listSavefiles(const Common::String &pattern) {
	// Assure the savefile name cache is up-to-date.
	assureCached(getSavePath());
	if (getError().getCode() != Common::kNoError)
		return Common::StringArray();

	Common::HashMap<Common::String, bool> locked;
	for (const auto &lockedFile : _lockedFiles) {
		locked[lockedFile] = true;
	}

	Common::StringArray results;
	for (const auto &file : _saveFileCache) {
		if (!locked.contains(file._key) && file._key.matchString(pattern, true)) {
			results.push_back(file._key);
		}
	}
	return results;
}

Common::InSaveFile *DefaultSaveFileManager::openRawFile(const Common::String &filename) {
	// Assure the savefile name cache is up-to-date.
	assureCached(getSavePath());
	if (getError().getCode() != Common::kNoError)
		return nullptr;

	SaveFileCache::const_iterator file = _saveFileCache.find(filename);
	if (file == _saveFileCache.end()) {
		return nullptr;
	} else {
		// Open the file for loading.
		Common::SeekableReadStream *sf = file->_value.createReadStream();
		return sf;
	}
}

Common::InSaveFile *DefaultSaveFileManager::openForLoading(const Common::String &filename) {
	// Assure the savefile name cache is up-to-date.
	assureCached(getSavePath());
	if (getError().getCode() != Common::kNoError)
		return nullptr;

	for (const auto &lockedFile : _lockedFiles) {
		if (filename == lockedFile) {
			setError(Common::kReadingFailed, Common::String::format("Savefile '%s' is locked and cannot be loaded", filename.c_str()));
			return nullptr; // file is locked, no loading available
		}
	}

	SaveFileCache::const_iterator file = _saveFileCache.find(filename);
	if (file == _saveFileCache.end()) {
		setError(Common::kPathDoesNotExist, Common::String::format("Savefile '%s' does not exist", filename.c_str()));
		return nullptr;
	} else {
		// Open the file for loading.
		Common::SeekableReadStream *sf = file->_value.createReadStream();
		return Common::wrapCompressedReadStream(sf);
	}
}

Common::OutSaveFile *DefaultSaveFileManager::openForSaving(const Common::String &filename, bool compress) {
	// Assure the savefile name cache is up-to-date.
	const Common::Path savePathName = getSavePath();
	assureCached(savePathName);
	if (getError().getCode() != Common::kNoError)
		return nullptr;

	for (const auto &lockedFile : _lockedFiles) {
		if (filename == lockedFile) {
			return nullptr; // file is locked, no saving available
		}
	}

#if defined(USE_CLOUD) && defined(USE_LIBCURL)
	// Update file's timestamp
	Common::HashMap<Common::String, uint32> timestamps = loadTimestamps();
	timestamps[filename] = INVALID_TIMESTAMP;
	saveTimestamps(timestamps);
#endif

	// Obtain node.
	SaveFileCache::const_iterator file = _saveFileCache.find(filename);
	Common::FSNode fileNode;

	// If the file did not exist before, we add it to the cache.
	if (file == _saveFileCache.end()) {
		const Common::FSNode savePath(savePathName);
		fileNode = savePath.getChild(filename);
	} else {
		fileNode = file->_value;
	}

	// Open the file for saving.
	Common::SeekableWriteStream *const sf = fileNode.createWriteStream();
	if (!sf)
		return nullptr;
	Common::OutSaveFile *const result = new Common::OutSaveFile(compress ? Common::wrapCompressedWriteStream(sf) : sf);

	// Add file to cache now that it exists.
	_saveFileCache[filename] = Common::FSNode(fileNode.getPath());

	return result;
}

bool DefaultSaveFileManager::removeSavefile(const Common::String &filename) {
	// Assure the savefile name cache is up-to-date.
	assureCached(getSavePath());
	if (getError().getCode() != Common::kNoError)
		return false;

#if defined(USE_CLOUD) && defined(USE_LIBCURL)
	// Update file's timestamp
	Common::HashMap<Common::String, uint32> timestamps = loadTimestamps();
	Common::HashMap<Common::String, uint32>::iterator it = timestamps.find(filename);
	if (it != timestamps.end()) {
		timestamps.erase(it);
		saveTimestamps(timestamps);
	}
#endif

	// Obtain node if exists.
	SaveFileCache::const_iterator file = _saveFileCache.find(filename);
	if (file == _saveFileCache.end()) {
		return false;
	} else {
		const Common::FSNode fileNode = file->_value;
		// Remove from cache, this invalidates the 'file' iterator.
		_saveFileCache.erase(file);
		file = _saveFileCache.end();

		Common::ErrorCode result = removeFile(fileNode);
		if (result == Common::kNoError)
			return true;
		Common::Error error(result);
		setError(error, "Failed to remove savefile '" + fileNode.getName() + "': " + error.getDesc());
		return false;
	}
}

Common::ErrorCode DefaultSaveFileManager::removeFile(const Common::FSNode &fileNode) {
	Common::String filepath(fileNode.getPath().toString(Common::Path::kNativeSeparator));
	if (remove(filepath.c_str()) == 0)
		return Common::kNoError;
	if (errno == EACCES)
		return Common::kWritePermissionDenied;
	if (errno == ENOENT)
		return Common::kPathDoesNotExist;
	return Common::kUnknownError;
}

bool DefaultSaveFileManager::exists(const Common::String &filename) {
	// Assure the savefile name cache is up-to-date.
	assureCached(getSavePath());
	if (getError().getCode() != Common::kNoError)
		return false;

	for (const auto &lockedFile : _lockedFiles) {
		if (filename == lockedFile)
			return true;
	}

	return _saveFileCache.contains(filename);
}

Common::Path DefaultSaveFileManager::getSavePath() const {

	Common::Path dir;

	// Try to use game specific savepath from config
	dir = ConfMan.getPath("savepath");

	// Work around a bug (#1689) in the original 0.6.1 release of
	// ScummVM, which would insert a bad savepath value into config files.
	if (dir == "None") {
		ConfMan.removeKey("savepath", ConfMan.getActiveDomainName());
		ConfMan.flushToDisk();
		dir = ConfMan.getPath("savepath");
	}

	return dir;
}

void DefaultSaveFileManager::assureCached(const Common::Path &savePathName) {
	// Check that path exists and is usable.
	checkPath(Common::FSNode(savePathName));

#if defined(USE_CLOUD) && defined(USE_LIBCURL)
	Common::Array<Common::String> files = CloudMan.getSyncingFiles(); //returns empty array if not syncing
	if (!files.empty()) updateSavefilesList(files); //makes this cache invalid
	else _lockedFiles = files;
#endif

	if (_cachedDirectory == savePathName) {
		return;
	}

	_saveFileCache.clear();
	_cachedDirectory.clear();

	if (getError().getCode() != Common::kNoError) {
		warning("DefaultSaveFileManager::assureCached: Can not cache path '%s': '%s'", savePathName.toString(Common::Path::kNativeSeparator).c_str(), getErrorDesc().c_str());
		return;
	}

	// FSNode can cache its members, thus create it after checkPath to reflect
	// actual file system state.
	const Common::FSNode savePath(savePathName);

	Common::FSList children;
	if (!savePath.getChildren(children, Common::FSNode::kListFilesOnly)) {
		return;
	}

	// Build the savefile name cache.
	for (const auto &file : children) {
		if (_saveFileCache.contains(file.getName())) {
			warning("DefaultSaveFileManager::assureCached: Name clash when building cache, ignoring file '%s'", file.getName().c_str());
		} else {
			_saveFileCache[file.getName()] = file;
		}
	}

	// Only now store that we cached 'savePathName' to indicate we successfully
	// cached the directory.
	_cachedDirectory = savePathName;
}

#if defined(USE_CLOUD) && defined(USE_LIBCURL)

Common::HashMap<Common::String, uint32> DefaultSaveFileManager::loadTimestamps() {
	Common::HashMap<Common::String, uint32> timestamps;

	//refresh the files list
	Common::Array<Common::String> files;
	g_system->getSavefileManager()->updateSavefilesList(files);

	//start with listing all the files in saves/ directory and setting invalid timestamp to them
	Common::StringArray localFiles = g_system->getSavefileManager()->listSavefiles("*");
	for (uint32 i = 0; i < localFiles.size(); ++i)
		timestamps[localFiles[i]] = INVALID_TIMESTAMP;

	//now actually load timestamps from file
	Common::InSaveFile *file = g_system->getSavefileManager()->openRawFile(TIMESTAMPS_FILENAME);
	if (!file) {
		warning("DefaultSaveFileManager: failed to open '%s' file to load timestamps", TIMESTAMPS_FILENAME);
		return timestamps;
	}

	while (!file->eos()) {
		//read filename into buffer (reading until the first ' ')
		Common::String buffer;
		while (true) {
			byte b = file->readByte();
			if (file->eos()) break;
			if (b == ' ') break;
			buffer += (char)b;
		}

		//read timestamp info buffer (reading until ' ' or some line ending char)
		Common::String filename = buffer;
		while (true) {
			bool lineEnded = false;
			buffer = "";
			while (true) {
				byte b = file->readByte();
				if (file->eos()) break;
				if (b == ' ' || b == '\n' || b == '\r') {
					lineEnded = (b == '\n');
					break;
				}
				buffer += (char)b;
			}

			if (buffer == "" && file->eos()) break;
			if (!lineEnded) filename += " " + buffer;
			else break;
		}

		//parse timestamp
		uint32 timestamp = buffer.asUint64();
		if (buffer == "" || timestamp == 0) break;
		if (timestamps.contains(filename))
			timestamps[filename] = timestamp;
	}

	delete file;
	return timestamps;
}

void DefaultSaveFileManager::saveTimestamps(Common::HashMap<Common::String, uint32> &timestamps) {
	Common::DumpFile f;
	Common::Path filename = concatWithSavesPath(TIMESTAMPS_FILENAME);
	if (!f.open(filename, true)) {
		warning("DefaultSaveFileManager: failed to open '%s' file to save timestamps", filename.toString(Common::Path::kNativeSeparator).c_str());
		return;
	}

	for (auto &timestamp : timestamps) {
		uint32 v = timestamp._value;
		if (v < 1) v = 1; // 0 timestamp is treated as EOF up there, so we should never save zeros

		Common::String data = timestamp._key + Common::String::format(" %u\n", v);
		if (f.write(data.c_str(), data.size()) != data.size()) {
			warning("DefaultSaveFileManager: failed to write timestamps data into '%s'", filename.toString(Common::Path::kNativeSeparator).c_str());
			return;
		}
	}

	f.flush();
	f.finalize();
	f.close();
}

#endif // ifdef USE_LIBCURL

Common::Path DefaultSaveFileManager::concatWithSavesPath(Common::String name) {
	DefaultSaveFileManager *manager = dynamic_cast<DefaultSaveFileManager *>(g_system->getSavefileManager());
	Common::Path path = (manager ? manager->getSavePath() : ConfMan.getPath("savepath"));
	path.joinInPlace(name);
	return path;
}

#endif // !defined(DISABLE_DEFAULT_SAVEFILEMANAGER)
