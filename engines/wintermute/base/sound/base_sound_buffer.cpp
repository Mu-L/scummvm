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

/*
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/base/base_game.h"
#include "engines/wintermute/base/sound/base_sound_manager.h"
#include "engines/wintermute/base/sound/base_sound_buffer.h"
#include "engines/wintermute/base/base_file_manager.h"
#include "engines/wintermute/wintermute.h"
#include "audio/audiostream.h"
#include "audio/mixer.h"
#ifdef USE_VORBIS
#include "audio/decoders/vorbis.h"
#endif
#include "audio/decoders/wave.h"
#include "audio/decoders/raw.h"
#include "common/system.h"
#include "common/substream.h"

namespace Wintermute {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_NONSTREAMED_FILE_SIZE 1024*1024

//////////////////////////////////////////////////////////////////////////
BaseSoundBuffer::BaseSoundBuffer(BaseGame *inGame) : BaseClass(inGame) {
	_stream = nullptr;
	_handle = nullptr;
//	_sync = nullptr;

	_streamed = false;
	_filename = "";
	_privateVolume = 255;
	_volume = 255;
	_pan = 0;

	_looping = false;
	_loopStart = 0;
	_startPos = 0;

	_type = Audio::Mixer::kSFXSoundType;

	_freezePaused = false;
}


//////////////////////////////////////////////////////////////////////////
BaseSoundBuffer::~BaseSoundBuffer() {
	stop();

	if (_handle) {
		g_system->getMixer()->stopHandle(*_handle);
		delete _handle;
		_handle = nullptr;
	}
	delete _stream;
	_stream = nullptr;
}


//////////////////////////////////////////////////////////////////////////
void BaseSoundBuffer::setStreaming(bool streamed, uint32 numBlocks, uint32 blockSize) {
	_streamed = streamed;
}


//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::loadFromFile(const Common::String &filename, bool forceReload) {
	debugC(kWintermuteDebugAudio, "BSoundBuffer::LoadFromFile(%s,%d)", filename.c_str(), forceReload);

	// Load a file, but avoid having the File-manager handle the disposal of it.
	Common::SeekableReadStream *file = BaseFileManager::getEngineInstance()->openFile(filename, true, false);
	if (!file) {
		_gameRef->LOG(0, "Error opening sound file '%s'", filename.c_str());
		return STATUS_FAILED;
	}
	Common::String strFilename(filename);
	strFilename.toLowercase();
	if (strFilename.hasSuffix(".ogg")) {
#ifdef USE_VORBIS
		_stream = Audio::makeVorbisStream(file, DisposeAfterUse::YES);
#else
		error("BSoundBuffer::LoadFromFile - Ogg Vorbis not supported by this version of ScummVM (please report as this shouldn't trigger)");
#endif
	} else if (strFilename.hasSuffix(".wav")) {
		int waveSize, waveRate;
		byte waveFlags;
		uint16 waveType;

		if (Audio::loadWAVFromStream(*file, waveSize, waveRate, waveFlags, &waveType)) {
			if (waveType == 1) {
				// We need to wrap the file in a substream to make sure the size is right.
				file = new Common::SeekableSubReadStream(file, file->pos(), waveSize + file->pos(), DisposeAfterUse::YES);
				_stream = Audio::makeRawStream(file, waveRate, waveFlags, DisposeAfterUse::YES);
			} else {
				error("BSoundBuffer::LoadFromFile - WAVE not supported yet for %s with type %d", filename.c_str(), waveType);
			}
		}
	} else {
		error("BSoundBuffer::LoadFromFile - Unknown filetype for %s", filename.c_str());
	}
	if (!_stream) {
		return STATUS_FAILED;
	}
	_filename = filename;

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::play(bool looping, uint32 startSample) {
	if (_handle) {
		g_system->getMixer()->stopHandle(*_handle);
		delete _handle;
		_handle = nullptr;
	}
	// Store the loop-value for save-games.
	setLooping(looping);
	if (_stream) {
		_stream->seek(startSample);
		_handle = new Audio::SoundHandle;
		if (_looping) {
			if (_loopStart != 0) {
				Audio::AudioStream *loopStream = new Audio::SubLoopingAudioStream(_stream, 0, Audio::Timestamp(_loopStart, _stream->getRate()), _stream->getLength(), DisposeAfterUse::NO);
				g_system->getMixer()->playStream(_type, _handle, loopStream, -1, _volume, _pan, DisposeAfterUse::YES);
			} else {
				Audio::AudioStream *loopStream = new Audio::LoopingAudioStream(_stream, 0, DisposeAfterUse::NO);
				g_system->getMixer()->playStream(_type, _handle, loopStream, -1, _volume, _pan, DisposeAfterUse::YES);
			}
		} else {
			g_system->getMixer()->playStream(_type, _handle, _stream, -1, _volume, _pan, DisposeAfterUse::NO);
		}
	}

	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
void BaseSoundBuffer::setLooping(bool looping) {
	if (isPlaying()) {
		// This warning is here, to see if this is ever the case.
		warning("BSoundBuffer::SetLooping(%d) - won't change a playing sound", looping); // TODO
	}
	_looping = looping;
}

//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::resume() {
	// If the sound was paused while active:
	if (_stream && _handle) {
		g_system->getMixer()->pauseHandle(*_handle, false);
	} else if (_stream) { // Otherwise we come from a savegame, and thus have no handle
		play(_looping, _startPos);
	} else {
		warning("BaseSoundBuffer::resume - Called without a handle or a stream");
	}
	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::stop() {
	if (_stream && _handle) {
		g_system->getMixer()->stopHandle(*_handle);
	}
	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::pause() {
	if (_stream && _handle) {
		g_system->getMixer()->pauseHandle(*_handle, true);
	}
	return STATUS_OK;

}

//////////////////////////////////////////////////////////////////////////
uint32 BaseSoundBuffer::getLength() {
	if (_stream) {
		uint32 len = _stream->getLength().msecs();
		return len * 1000;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
void BaseSoundBuffer::setType(Audio::Mixer::SoundType type) {
	_type = type;
}

//////////////////////////////////////////////////////////////////////////
void BaseSoundBuffer::updateVolume() {
	setVolume(_privateVolume);
}

//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::setVolume(int volume) {
	_volume = volume * _gameRef->_soundMgr->getMasterVolume() / 255;
	if (_stream && _handle) {
		byte vol = (byte)(_volume);
		g_system->getMixer()->setChannelVolume(*_handle, vol);
	}
	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::setPrivateVolume(int volume) {
	_privateVolume = volume;
	return setVolume(_privateVolume);
}


//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::isPlaying() {
	if (_stream && _handle) {
		return _freezePaused || g_system->getMixer()->isSoundHandleActive(*_handle);
	} else {
		return false;
	}
}


//////////////////////////////////////////////////////////////////////////
uint32 BaseSoundBuffer::getPosition() {
	if (_stream && _handle) {
		uint32 pos = g_system->getMixer()->getSoundElapsedTime(*_handle);
		return pos;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::setPosition(uint32 pos) {
	if (_stream) {
		_stream->seek(pos);
	} else {
		if (isPlaying()) {
			warning("BaseSoundBuffer::SetPosition - not implemented for playing sounds yet.");
		}
	}
	_startPos = pos;
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::setLoopStart(uint32 pos) {
	_loopStart = pos;
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::setPan(float pan) {
	pan = MAX(pan, -1.0f);
	pan = MIN(pan, 1.0f);
	_pan = (int8)(pan * 127);
	if (_handle) {
		g_system->getMixer()->setChannelBalance(*_handle, _pan);
	}
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool BaseSoundBuffer::applyFX(TSFXType type, float param1, float param2, float param3, float param4) {
	// This function was stubbed out in WME Lite.
	switch (type) {
	case SFX_ECHO:
		warning("BaseSoundBuffer::ApplyFX(SFX_ECHO, %f, %f, %f, %f)  - not implemented yet", param1, param2, param3, param4);
		break;

	case SFX_REVERB:
		warning("BaseSoundBuffer::ApplyFX(SFX_REVERB, %f, %f, %f, %f)  - not implemented yet", param1, param2, param3, param4);
		break;

	default:
		break;
	}
	return STATUS_OK;
}

int32 BaseSoundBuffer::getPrivateVolume() const {
	return _privateVolume;
}

bool BaseSoundBuffer::isLooping() const {
	return _looping;
}

bool BaseSoundBuffer::isFreezePaused() const {
	return _freezePaused;
}

void BaseSoundBuffer::setFreezePaused(bool freezePaused) {
	_freezePaused = freezePaused;
}

Audio::Mixer::SoundType BaseSoundBuffer::getType() const {
	return _type;
}

} // End of namespace Wintermute
