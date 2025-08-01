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

#include "engines/wintermute/ad/ad_game.h"
#include "engines/wintermute/ad/ad_item.h"
#include "engines/wintermute/ad/ad_object.h"
#include "engines/wintermute/ad/ad_inventory.h"
#include "engines/wintermute/ad/ad_layer.h"
#include "engines/wintermute/ad/ad_scene.h"
#include "engines/wintermute/ad/ad_scene_node.h"
#include "engines/wintermute/ad/ad_sentence.h"
#include "engines/wintermute/ad/ad_waypoint_group.h"
#include "engines/wintermute/base/base_game.h"
#include "engines/wintermute/base/base_frame.h"
#include "engines/wintermute/base/base_sprite.h"
#include "engines/wintermute/base/base_sub_frame.h"
#include "engines/wintermute/base/base_surface_storage.h"
#include "engines/wintermute/base/font/base_font.h"
#include "engines/wintermute/base/font/base_font_storage.h"
#include "engines/wintermute/base/gfx/base_renderer.h"
#include "engines/wintermute/base/particles/part_emitter.h"
#include "engines/wintermute/base/scriptables/script_engine.h"
#include "engines/wintermute/base/scriptables/script.h"
#include "engines/wintermute/base/scriptables/script_stack.h"
#include "engines/wintermute/base/scriptables/script_value.h"
#include "engines/wintermute/base/sound/base_sound.h"

#include "common/str.h"
#include "common/util.h"

namespace Wintermute {

IMPLEMENT_PERSISTENT(AdObject, false)

//////////////////////////////////////////////////////////////////////////
AdObject::AdObject(BaseGame *inGame) : BaseObject(inGame) {
	_type = OBJECT_NONE;
	_state = _nextState = STATE_NONE;

	_active = true;
	_drawn = false;

	_currentSprite = nullptr;
	_animSprite = nullptr;
	_tempSprite2 = nullptr;

	_font = nullptr;

	_sentence = nullptr;

	_forcedTalkAnimName = nullptr;
	_forcedTalkAnimUsed = false;

	_blockRegion = nullptr;
	_wptGroup = nullptr;

	_currentBlockRegion = nullptr;
	_currentWptGroup = nullptr;

	_ignoreItems = false;
	_sceneIndependent = false;

	_stickRegion = nullptr;

	_subtitlesModRelative = true;
	_subtitlesModX = 0;
	_subtitlesModY = 0;
	_subtitlesWidth = 0;
	_subtitlesModXCenter = true;

	_inventory = nullptr;

	for (int i = 0; i < MAX_NUM_REGIONS; i++) {
		_currentRegions[i] = nullptr;
	}

	_partEmitter = nullptr;
	_partFollowParent = false;
	_partOffsetX = _partOffsetY = 0;

	_registerAlias = this;
}


//////////////////////////////////////////////////////////////////////////
AdObject::~AdObject() {
	_currentSprite = nullptr; // reference only, don't delete
	delete _animSprite;
	_animSprite = nullptr;
	delete _sentence;
	_sentence = nullptr;
	delete[] _forcedTalkAnimName;
	_forcedTalkAnimName = nullptr;

	delete _blockRegion;
	_blockRegion = nullptr;
	delete _wptGroup;
	_wptGroup = nullptr;

	delete _currentBlockRegion;
	_currentBlockRegion = nullptr;
	delete _currentWptGroup;
	_currentWptGroup = nullptr;

	_tempSprite2 = nullptr; // reference only
	_stickRegion = nullptr;

	if (_font) {
		_gameRef->_fontStorage->removeFont(_font);
	}

	if (_inventory) {
		((AdGame *)_gameRef)->unregisterInventory(_inventory);
		_inventory = nullptr;
	}

	if (_partEmitter) {
		_gameRef->unregisterObject(_partEmitter);
	}


	for (uint32 i = 0; i < _attachmentsPre.getSize(); i++) {
		_gameRef->unregisterObject(_attachmentsPre[i]);
	}
	_attachmentsPre.removeAll();

	for (uint32 i = 0; i < _attachmentsPost.getSize(); i++) {
		_gameRef->unregisterObject(_attachmentsPost[i]);
	}
	_attachmentsPost.removeAll();
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::playAnim(const char *filename) {
	delete _animSprite;
	_animSprite = new BaseSprite(_gameRef, this);
	if (!_animSprite) {
		_gameRef->LOG(0, "AdObject::PlayAnim: error creating temp sprite (object:\"%s\" sprite:\"%s\")", getName(), filename);
		return STATUS_FAILED;
	}
	bool res = _animSprite->loadFile(filename);
	if (DID_FAIL(res)) {
		_gameRef->LOG(res, "AdObject::PlayAnim: error loading temp sprite (object:\"%s\" sprite:\"%s\")", getName(), filename);
		delete _animSprite;
		_animSprite = nullptr;
		return res;
	}
	_state = STATE_PLAYING_ANIM;

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::display() {
	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::update() {
	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
// high level scripting interface
//////////////////////////////////////////////////////////////////////////
bool AdObject::scCallMethod(ScScript *script, ScStack *stack, ScStack *thisStack, const char *name) {

	//////////////////////////////////////////////////////////////////////////
	// PlayAnim / PlayAnimAsync
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "PlayAnim") == 0 || strcmp(name, "PlayAnimAsync") == 0) {
		stack->correctParams(1);
		if (DID_FAIL(playAnim(stack->pop()->getString()))) {
			stack->pushBool(false);
		} else {
			if (strcmp(name, "PlayAnimAsync") != 0) {
				script->waitFor(this);
			}
			stack->pushBool(true);
		}
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Reset
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Reset") == 0) {
		stack->correctParams(0);
		reset();
		stack->pushNULL();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// IsTalking
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IsTalking") == 0) {
		stack->correctParams(0);
		stack->pushBool(_state == STATE_TALKING);
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// StopTalk / StopTalking
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "StopTalk") == 0 || strcmp(name, "StopTalking") == 0) {
		stack->correctParams(0);
		if (_sentence) {
			_sentence->finish();
		}
		if (_state == STATE_TALKING) {
			_state = _nextState;
			_nextState = STATE_READY;
			stack->pushBool(true);
		} else {
			stack->pushBool(false);
		}
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ForceTalkAnim
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ForceTalkAnim") == 0) {
		stack->correctParams(1);
		const char *animName = stack->pop()->getString();
		delete[] _forcedTalkAnimName;
		size_t animNameSize = strlen(animName) + 1;
		_forcedTalkAnimName = new char[animNameSize];
		Common::strcpy_s(_forcedTalkAnimName, animNameSize, animName);
		_forcedTalkAnimUsed = false;
		stack->pushBool(true);
		return STATUS_OK;
	}


	//////////////////////////////////////////////////////////////////////////
	// Talk / TalkAsync
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Talk") == 0 || strcmp(name, "TalkAsync") == 0) {
		stack->correctParams(5);

		const char *text = stack->pop()->getString();
		ScValue *soundVal = stack->pop();
		int duration  = stack->pop()->getInt();
		ScValue *valStances = stack->pop();

		const char *stances = valStances->isNULL() ? nullptr : valStances->getString();

		int align = 0;
		ScValue *val = stack->pop();
		if (val->isNULL()) {
			align = TAL_CENTER;
		} else {
			align = val->getInt();
		}

		align = MIN(MAX(0, align), NUM_TEXT_ALIGN - 1);

		const char *sound = soundVal->isNULL() ? nullptr : soundVal->getString();

		talk(text, sound, duration, stances, (TTextAlign)align);
		if (strcmp(name, "TalkAsync") != 0) {
			script->waitForExclusive(this);
		}

		stack->pushNULL();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// StickToRegion
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "StickToRegion") == 0) {
		stack->correctParams(1);

		AdLayer *main = ((AdGame *)_gameRef)->_scene->_mainLayer;
		bool regFound = false;

		uint32 i;
		ScValue *val = stack->pop();
		if (val->isNULL() || !main) {
			_stickRegion = nullptr;
			regFound = true;
		} else if (val->isString()) {
			const char *regionName = val->getString();
			for (i = 0; i < main->_nodes.getSize(); i++) {
				if (main->_nodes[i]->_type == OBJECT_REGION && main->_nodes[i]->_region->getName() && scumm_stricmp(main->_nodes[i]->_region->getName(), regionName) == 0) {
					_stickRegion = main->_nodes[i]->_region;
					regFound = true;
					break;
				}
			}
		} else if (val->isNative()) {
			BaseScriptable *obj = val->getNative();

			for (i = 0; i < main->_nodes.getSize(); i++) {
				if (main->_nodes[i]->_type == OBJECT_REGION && main->_nodes[i]->_region == obj) {
					_stickRegion = main->_nodes[i]->_region;
					regFound = true;
					break;
				}
			}

		}

		if (!regFound) {
			_stickRegion = nullptr;
		}
		stack->pushBool(regFound);
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetFont
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetFont") == 0) {
		stack->correctParams(1);
		ScValue *val = stack->pop();

		if (val->isNULL()) {
			setFont(nullptr);
		} else {
			setFont(val->getString());
		}

		stack->pushNULL();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetFont
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetFont") == 0) {
		stack->correctParams(0);
		if (_font && _font->getFilename()) {
			stack->pushString(_font->getFilename());
		} else {
			stack->pushNULL();
		}
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// TakeItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "TakeItem") == 0) {
		stack->correctParams(2);

		if (!_inventory) {
			_inventory = new AdInventory(_gameRef);
			((AdGame *)_gameRef)->registerInventory(_inventory);
		}

		ScValue *val = stack->pop();
		if (!val->isNULL()) {
			const char *itemName = val->getString();
			val = stack->pop();
			const char *insertAfter = val->isNULL() ? nullptr : val->getString();
			if (DID_FAIL(_inventory->insertItem(itemName, insertAfter))) {
				script->runtimeError("Cannot add item '%s' to inventory", itemName);
			} else {
				// hide associated entities
				((AdGame *)_gameRef)->_scene->handleItemAssociations(itemName, false);
			}

		} else {
			script->runtimeError("TakeItem: item name expected");
		}

		stack->pushNULL();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DropItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DropItem") == 0) {
		stack->correctParams(1);

		if (!_inventory) {
			_inventory = new AdInventory(_gameRef);
			((AdGame *)_gameRef)->registerInventory(_inventory);
		}

		ScValue *val = stack->pop();
		if (!val->isNULL()) {
			if (DID_FAIL(_inventory->removeItem(val->getString()))) {
				script->runtimeError("Cannot remove item '%s' from inventory", val->getString());
			} else {
				// show associated entities
				((AdGame *)_gameRef)->_scene->handleItemAssociations(val->getString(), true);
			}
		} else {
			script->runtimeError("DropItem: item name expected");
		}

		stack->pushNULL();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetItem") == 0) {
		stack->correctParams(1);

		if (!_inventory) {
			_inventory = new AdInventory(_gameRef);
			((AdGame *)_gameRef)->registerInventory(_inventory);
		}

		ScValue *val = stack->pop();
		if (val->_type == VAL_STRING) {
			AdItem *item = ((AdGame *)_gameRef)->getItemByName(val->getString());
			if (item) {
				stack->pushNative(item, true);
			} else {
				stack->pushNULL();
			}
		} else if (val->isNULL() || val->getInt() < 0 || val->getInt() >= (int32)_inventory->_takenItems.getSize()) {
			stack->pushNULL();
		} else {
			stack->pushNative(_inventory->_takenItems[val->getInt()], true);
		}

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// HasItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "HasItem") == 0) {
		stack->correctParams(1);

		if (!_inventory) {
			_inventory = new AdInventory(_gameRef);
			((AdGame *)_gameRef)->registerInventory(_inventory);
		}

		ScValue *val = stack->pop();
		if (!val->isNULL()) {
			for (uint32 i = 0; i < _inventory->_takenItems.getSize(); i++) {
				if (val->getNative() == _inventory->_takenItems[i]) {
					stack->pushBool(true);
					return STATUS_OK;
				} else if (scumm_stricmp(val->getString(), _inventory->_takenItems[i]->getName()) == 0) {
					stack->pushBool(true);
					return STATUS_OK;
				}
			}
		} else {
			script->runtimeError("HasItem: item name expected");
		}

		stack->pushBool(false);
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// CreateParticleEmitter
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CreateParticleEmitter") == 0) {
		stack->correctParams(3);
		bool followParent = stack->pop()->getBool();
		int offsetX = stack->pop()->getInt();
		int offsetY = stack->pop()->getInt();

		PartEmitter *emitter = createParticleEmitter(followParent, offsetX, offsetY);
		if (emitter) {
			stack->pushNative(_partEmitter, true);
		} else {
			stack->pushNULL();
		}

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DeleteParticleEmitter
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DeleteParticleEmitter") == 0) {
		stack->correctParams(0);
		if (_partEmitter) {
			_gameRef->unregisterObject(_partEmitter);
			_partEmitter = nullptr;
		}
		stack->pushNULL();

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AddAttachment
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AddAttachment") == 0) {
		stack->correctParams(4);
		const char *filename = stack->pop()->getString();
		bool preDisplay = stack->pop()->getBool(true);
		int offsetX = stack->pop()->getInt();
		int offsetY = stack->pop()->getInt();

		bool res;
		AdEntity *ent = new AdEntity(_gameRef);
		if (DID_FAIL(res = ent->loadFile(filename))) {
			delete ent;
			script->runtimeError("AddAttachment() failed loading entity '%s'", filename);
			stack->pushBool(false);
		} else {
			_gameRef->registerObject(ent);

			ent->_posX = offsetX;
			ent->_posY = offsetY;
			ent->_active = true;

			if (preDisplay) {
				_attachmentsPre.add(ent);
			} else {
				_attachmentsPost.add(ent);
			}

			stack->pushBool(true);
		}

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RemoveAttachment
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RemoveAttachment") == 0) {
		stack->correctParams(1);
		ScValue *val = stack->pop();
		bool found = false;
		if (val->isNative()) {
			BaseScriptable *obj = val->getNative();
			for (uint32 i = 0; i < _attachmentsPre.getSize(); i++) {
				if (_attachmentsPre[i] == obj) {
					found = true;
					_gameRef->unregisterObject(_attachmentsPre[i]);
					_attachmentsPre.removeAt(i);
					i--;
				}
			}
			for (uint32 i = 0; i < _attachmentsPost.getSize(); i++) {
				if (_attachmentsPost[i] == obj) {
					found = true;
					_gameRef->unregisterObject(_attachmentsPost[i]);
					_attachmentsPost.removeAt(i);
					i--;
				}
			}
		} else {
			const char *attachmentName = val->getString();
			for (int32 i = 0; i < (int32)_attachmentsPre.getSize(); i++) {
				if (_attachmentsPre[i]->getName() && scumm_stricmp(_attachmentsPre[i]->getName(), attachmentName) == 0) {
					found = true;
					_gameRef->unregisterObject(_attachmentsPre[i]);
					_attachmentsPre.removeAt(i);
					i--;
				}
			}
			for (int32 i = 0; i < (int32)_attachmentsPost.getSize(); i++) {
				if (_attachmentsPost[i]->getName() && scumm_stricmp(_attachmentsPost[i]->getName(), attachmentName) == 0) {
					found = true;
					_gameRef->unregisterObject(_attachmentsPost[i]);
					_attachmentsPost.removeAt(i);
					i--;
				}
			}
		}
		stack->pushBool(found);

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetAttachment
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetAttachment") == 0) {
		stack->correctParams(1);
		ScValue *val = stack->pop();

		AdObject *ret = nullptr;
		if (val->isInt()) {
			int index = val->getInt();
			int currIndex = 0;
			for (uint32 i = 0; i < _attachmentsPre.getSize(); i++) {
				if (currIndex == index) {
					ret = _attachmentsPre[i];
				}
				currIndex++;
			}
			for (uint32 i = 0; i < _attachmentsPost.getSize(); i++) {
				if (currIndex == index) {
					ret = _attachmentsPost[i];
				}
				currIndex++;
			}
		} else {
			const char *attachmentName = val->getString();
			for (uint32 i = 0; i < _attachmentsPre.getSize(); i++) {
				if (_attachmentsPre[i]->getName() && scumm_stricmp(_attachmentsPre[i]->getName(), attachmentName) == 0) {
					ret = _attachmentsPre[i];
					break;
				}
			}
			if (!ret) {
				for (uint32 i = 0; i < _attachmentsPost.getSize(); i++) {
					if (_attachmentsPost[i]->getName() && scumm_stricmp(_attachmentsPost[i]->getName(), attachmentName) == 0) {
						ret = _attachmentsPre[i];
						break;
					}
				}
			}
		}

		if (ret != nullptr) {
			stack->pushNative(ret, true);
		} else {
			stack->pushNULL();
		}

		return STATUS_OK;
	} else {
		return BaseObject::scCallMethod(script, stack, thisStack, name);
	}
}


//////////////////////////////////////////////////////////////////////////
ScValue *AdObject::scGetProperty(const Common::String &name) {
	_scValue->setNULL();

	//////////////////////////////////////////////////////////////////////////
	// Type
	//////////////////////////////////////////////////////////////////////////
	if (name == "Type") {
		_scValue->setString("object");
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Active
	//////////////////////////////////////////////////////////////////////////
	else if (name == "Active") {
		_scValue->setBool(_active);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// IgnoreItems
	//////////////////////////////////////////////////////////////////////////
	else if (name == "IgnoreItems") {
		_scValue->setBool(_ignoreItems);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SceneIndependent
	//////////////////////////////////////////////////////////////////////////
	else if (name == "SceneIndependent") {
		_scValue->setBool(_sceneIndependent);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesWidth
	//////////////////////////////////////////////////////////////////////////
	else if (name == "SubtitlesWidth") {
		_scValue->setInt(_subtitlesWidth);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosRelative
	//////////////////////////////////////////////////////////////////////////
	else if (name == "SubtitlesPosRelative") {
		_scValue->setBool(_subtitlesModRelative);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosX
	//////////////////////////////////////////////////////////////////////////
	else if (name == "SubtitlesPosX") {
		_scValue->setInt(_subtitlesModX);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosY
	//////////////////////////////////////////////////////////////////////////
	else if (name == "SubtitlesPosY") {
		_scValue->setInt(_subtitlesModY);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosXCenter
	//////////////////////////////////////////////////////////////////////////
	else if (name == "SubtitlesPosXCenter") {
		_scValue->setBool(_subtitlesModXCenter);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// NumItems (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (name == "NumItems") {
		_scValue->setInt(getInventory()->_takenItems.getSize());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// ParticleEmitter (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (name == "ParticleEmitter") {
		if (_partEmitter) {
			_scValue->setNative(_partEmitter, true);
		} else {
			_scValue->setNULL();
		}

		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// NumAttachments (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (name == "NumAttachments") {
		_scValue->setInt(_attachmentsPre.getSize() + _attachmentsPost.getSize());
		return _scValue;
	} else {
		return BaseObject::scGetProperty(name);
	}
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::scSetProperty(const char *name, ScValue *value) {

	//////////////////////////////////////////////////////////////////////////
	// Active
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Active") == 0) {
		_active = value->getBool();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// IgnoreItems
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IgnoreItems") == 0) {
		_ignoreItems = value->getBool();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SceneIndependent
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SceneIndependent") == 0) {
		_sceneIndependent = value->getBool();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesWidth
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesWidth") == 0) {
		_subtitlesWidth = value->getInt();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosRelative
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosRelative") == 0) {
		_subtitlesModRelative = value->getBool();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosX
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosX") == 0) {
		_subtitlesModX = value->getInt();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosY
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosY") == 0) {
		_subtitlesModY = value->getInt();
		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosXCenter
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosXCenter") == 0) {
		_subtitlesModXCenter = value->getBool();
		return STATUS_OK;
	} else {
		return BaseObject::scSetProperty(name, value);
	}
}


//////////////////////////////////////////////////////////////////////////
const char *AdObject::scToString() {
	return "[ad object]";
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::setFont(const char *filename) {
	if (_font) {
		_gameRef->_fontStorage->removeFont(_font);
	}
	if (filename) {
		_font = _gameRef->_fontStorage->addFont(filename);
		return _font == nullptr ? STATUS_FAILED : STATUS_OK;
	} else {
		_font = nullptr;
		return STATUS_OK;
	}
}


//////////////////////////////////////////////////////////////////////////
int32 AdObject::getHeight() {
	if (!_currentSprite || (int32)_currentSprite->_frames.getSize() <= _currentSprite->_currentFrame) {
		return 0;
	} else {
		BaseFrame *frame = _currentSprite->_frames[_currentSprite->_currentFrame];
		int32 ret = 0;
		for (uint32 i = 0; i < frame->_subframes.getSize(); i++) {
			ret = MAX(ret, frame->_subframes[i]->_hotspotY);
		}

		if (_zoomable) {
			float zoom = ((AdGame *)_gameRef)->_scene->getZoomAt(_posX, _posY);
			ret = (int32)(ret * zoom / 100);
		}
		return ret;
	}
}

TObjectType AdObject::getType() const {
	return _type;
}

//////////////////////////////////////////////////////////////////////////
void AdObject::talk(const char *text, const char *sound, uint32 duration, const char *stances, TTextAlign Align) {
	if (!_sentence) {
		_sentence = new AdSentence(_gameRef);
	}
	if (!_sentence) {
		return;
	}

	if (_forcedTalkAnimName && _forcedTalkAnimUsed) {
		delete[] _forcedTalkAnimName;
		_forcedTalkAnimName = nullptr;
		_forcedTalkAnimUsed = false;
	}

	delete(_sentence->_sound);
	_sentence->_sound = nullptr;

	_sentence->setText(text);
	_gameRef->expandStringByStringTable(&_sentence->_text);
	_sentence->setStances(stances);
	_sentence->_duration = duration;
	_sentence->_align = Align;
	_sentence->_startTime = _gameRef->getTimer()->getTime();
	_sentence->_currentStance = -1;
	_sentence->_font = _font == nullptr ? _gameRef->getSystemFont() : _font;
	_sentence->_freezable = _freezable;

	// try to locate speech file automatically
	bool deleteSound = false;
	if (!sound) {
		char *key = _gameRef->getKeyFromStringTable(text);
		if (key) {
			sound = ((AdGame *)_gameRef)->findSpeechFile(key);
			delete[] key;

			if (sound) {
				deleteSound = true;
			}
		}
	}

	// load sound and set duration appropriately
	if (sound) {
		BaseSound *snd = new BaseSound(_gameRef);
		if (snd && DID_SUCCEED(snd->setSound(sound, Audio::Mixer::kSpeechSoundType, true))) {
			_sentence->setSound(snd);
			if (_sentence->_duration <= 0) {
				uint32 length = snd->getLength();
				if (length != 0) {
					_sentence->_duration = length;
				}
			}
		} else {
			delete snd;
		}
	}

	// set duration by text length
	if (_sentence->_duration <= 0) {
		_sentence->_duration = MAX<int32>((size_t)1000, _gameRef->_subtitlesSpeed * strlen(_sentence->_text));
	}


	int32 x, y, width, height;

	x = _posX;
	y = _posY;

	if (!_sceneIndependent && _subtitlesModRelative) {
		x -= ((AdGame *)_gameRef)->_scene->getOffsetLeft();
		y -= ((AdGame *)_gameRef)->_scene->getOffsetTop();
	}


	if (_subtitlesWidth > 0) {
		width = _subtitlesWidth;
	} else {
		if ((x < _gameRef->_renderer->getWidth() / 4 || x > _gameRef->_renderer->getWidth() * 0.75)) {
			width = MAX(_gameRef->_renderer->getWidth() / 4, MIN(x * 2, (_gameRef->_renderer->getWidth() - x) * 2));
		} else {
			width = _gameRef->_renderer->getWidth() / 2;
		}
	}

	height = _sentence->_font->getTextHeight((byte *)_sentence->_text, width);

	y = y - height - getHeight() - 5;
	if (_subtitlesModRelative) {
		x += _subtitlesModX;
		y += _subtitlesModY;
	} else {
		x = _subtitlesModX;
		y = _subtitlesModY;
	}
	if (_subtitlesModXCenter) {
		x = x - width / 2;
	}


	x = MIN(MAX<int32>(0, x), _gameRef->_renderer->getWidth() - width);
	y = MIN(MAX<int32>(0, y), _gameRef->_renderer->getHeight() - height);

	_sentence->_width = width;


	_sentence->_pos.x = x;
	_sentence->_pos.y = y;


	if (_subtitlesModRelative) {
		_sentence->_pos.x += ((AdGame *)_gameRef)->_scene->getOffsetLeft();
		_sentence->_pos.y += ((AdGame *)_gameRef)->_scene->getOffsetTop();
	}

	_sentence->_fixedPos = !_subtitlesModRelative;


	_sentence->setupTalkFile(sound);

	_state = STATE_TALKING;

	if (deleteSound) {
		delete[] sound;
	}
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::reset() {
	if (_state == STATE_PLAYING_ANIM && _animSprite != nullptr) {
		delete _animSprite;
		_animSprite = nullptr;
	} else if (_state == STATE_TALKING && _sentence) {
		_sentence->finish();
	}

	_state = _nextState = STATE_READY;

	_gameRef->_scEngine->resetObject(this);

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::persist(BasePersistenceManager *persistMgr) {
	BaseObject::persist(persistMgr);

	persistMgr->transferBool(TMEMBER(_active));
	persistMgr->transferPtr(TMEMBER_PTR(_blockRegion));
	persistMgr->transferPtr(TMEMBER_PTR(_currentBlockRegion));
	persistMgr->transferPtr(TMEMBER_PTR(_currentWptGroup));
	persistMgr->transferPtr(TMEMBER_PTR(_currentSprite));
	persistMgr->transferBool(TMEMBER(_drawn));
	persistMgr->transferPtr(TMEMBER_PTR(_font));
	persistMgr->transferBool(TMEMBER(_ignoreItems));
	persistMgr->transferSint32(TMEMBER_INT(_nextState));
	persistMgr->transferPtr(TMEMBER_PTR(_sentence));
	persistMgr->transferSint32(TMEMBER_INT(_state));
	persistMgr->transferPtr(TMEMBER_PTR(_animSprite));
	persistMgr->transferBool(TMEMBER(_sceneIndependent));
	persistMgr->transferCharPtr(TMEMBER(_forcedTalkAnimName));
	persistMgr->transferBool(TMEMBER(_forcedTalkAnimUsed));
	persistMgr->transferPtr(TMEMBER_PTR(_tempSprite2));
	persistMgr->transferSint32(TMEMBER_INT(_type));
	persistMgr->transferPtr(TMEMBER_PTR(_wptGroup));
	persistMgr->transferPtr(TMEMBER_PTR(_stickRegion));
	persistMgr->transferBool(TMEMBER(_subtitlesModRelative));
	persistMgr->transferSint32(TMEMBER(_subtitlesModX));
	persistMgr->transferSint32(TMEMBER(_subtitlesModY));
	persistMgr->transferBool(TMEMBER(_subtitlesModXCenter));
	persistMgr->transferSint32(TMEMBER(_subtitlesWidth));
	persistMgr->transferPtr(TMEMBER_PTR(_inventory));
	persistMgr->transferPtr(TMEMBER_PTR(_partEmitter));

	for (int i = 0; i < MAX_NUM_REGIONS; i++) {
		persistMgr->transferPtr(TMEMBER_PTR(_currentRegions[i]));
	}

	_attachmentsPre.persist(persistMgr);
	_attachmentsPost.persist(persistMgr);
	persistMgr->transferPtr(TMEMBER_PTR(_registerAlias));

	persistMgr->transferBool(TMEMBER(_partFollowParent));
	persistMgr->transferSint32(TMEMBER(_partOffsetX));
	persistMgr->transferSint32(TMEMBER(_partOffsetY));

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::updateSounds() {
	if (_sentence && _sentence->_sound) {
		updateOneSound(_sentence->_sound);
	}

	return BaseObject::updateSounds();
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::resetSoundPan() {
	if (_sentence && _sentence->_sound) {
		_sentence->_sound->setPan(0.0f);
	}
	return BaseObject::resetSoundPan();
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::getExtendedFlag(const char *flagName) {
	if (!flagName) {
		return false;
	} else if (strcmp(flagName, "usable") == 0) {
		return true;
	} else {
		return BaseObject::getExtendedFlag(flagName);
	}
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::saveAsText(BaseDynamicBuffer *buffer, int indent) {
	if (_blockRegion) {
		_blockRegion->saveAsText(buffer, indent + 2, "BLOCKED_REGION");
	}
	if (_wptGroup) {
		_wptGroup->saveAsText(buffer, indent + 2);
	}

	BaseClass::saveAsText(buffer, indent + 2);

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::updateBlockRegion() {
	AdGame *adGame = (AdGame *)_gameRef;
	if (adGame->_scene) {
		if (_blockRegion && _currentBlockRegion) {
			_currentBlockRegion->mimic(_blockRegion, _zoomable ? adGame->_scene->getScaleAt(_posY) : 100.0f, _posX, _posY);
		}

		if (_wptGroup && _currentWptGroup) {
			_currentWptGroup->mimic(_wptGroup, _zoomable ? adGame->_scene->getScaleAt(_posY) : 100.0f, _posX, _posY);
		}
	}
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
AdInventory *AdObject::getInventory() {
	if (!_inventory) {
		_inventory = new AdInventory(_gameRef);
		((AdGame *)_gameRef)->registerInventory(_inventory);
	}
	return _inventory;
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::afterMove() {
	AdRegion *newRegions[MAX_NUM_REGIONS];

	((AdGame *)_gameRef)->_scene->getRegionsAt(_posX, _posY, newRegions, MAX_NUM_REGIONS);
	for (int i = 0; i < MAX_NUM_REGIONS; i++) {
		if (!newRegions[i]) {
			break;
		}
		bool regFound = false;
		for (int j = 0; j < MAX_NUM_REGIONS; j++) {
			if (_currentRegions[j] == newRegions[i]) {
				_currentRegions[j] = nullptr;
				regFound = true;
				break;
			}
		}
		if (!regFound) {
			newRegions[i]->applyEvent("ActorEntry");
		}
	}

	for (int i = 0; i < MAX_NUM_REGIONS; i++) {
		if (_currentRegions[i] && _gameRef->validObject(_currentRegions[i])) {
			_currentRegions[i]->applyEvent("ActorLeave");
		}
		_currentRegions[i] = newRegions[i];
	}

	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool AdObject::invalidateCurrRegions() {
	for (int i = 0; i < MAX_NUM_REGIONS; i++) {
		_currentRegions[i] = nullptr;
	}
	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool AdObject::getScale(float *scaleX, float *scaleY) {
	if (_zoomable) {
		if (_scaleX >= 0 || _scaleY >= 0) {
			*scaleX = _scaleX < 0 ? 100 : _scaleX;
			*scaleY = _scaleY < 0 ? 100 : _scaleY;
		} else if (_scale >= 0) {
			*scaleX = *scaleY = _scale;
		} else {
			*scaleX = *scaleY = ((AdGame *)_gameRef)->_scene->getZoomAt(_posX, _posY) + _relativeScale;
		}
	} else {
		*scaleX = *scaleY = 100;
	}
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool AdObject::updateSpriteAttachments() {
	for (uint32 i = 0; i < _attachmentsPre.getSize(); i++) {
		_attachmentsPre[i]->update();
	}
	for (uint32 i = 0; i < _attachmentsPost.getSize(); i++) {
		_attachmentsPost[i]->update();
	}
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool AdObject::displaySpriteAttachments(bool preDisplay) {
	if (preDisplay) {
		for (uint32 i = 0; i < _attachmentsPre.getSize(); i++) {
			displaySpriteAttachment(_attachmentsPre[i]);
		}
	} else {
		for (uint32 i = 0; i < _attachmentsPost.getSize(); i++) {
			displaySpriteAttachment(_attachmentsPost[i]);
		}
	}
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool AdObject::displaySpriteAttachment(AdObject *attachment) {
	if (!attachment->_active) {
		return STATUS_OK;
	}

	float scaleX, scaleY;
	getScale(&scaleX, &scaleY);

	int origX = attachment->_posX;
	int origY = attachment->_posY;

	// inherit position from owner
	attachment->_posX = (int32)(this->_posX + attachment->_posX * scaleX / 100.0f);
	attachment->_posY = (int32)(this->_posY + attachment->_posY * scaleY / 100.0f);

	// inherit other props
	attachment->_alphaColor = this->_alphaColor;
	attachment->_blendMode = this->_blendMode;

	attachment->_scale = this->_scale;
	attachment->_relativeScale = this->_relativeScale;
	attachment->_scaleX = this->_scaleX;
	attachment->_scaleY = this->_scaleY;

	attachment->_rotate = this->_rotate;
	attachment->_relativeRotate = this->_relativeRotate;
	attachment->_rotateValid = this->_rotateValid;

	attachment->_registerAlias = this;
	attachment->_registrable = this->_registrable;

	bool ret = attachment->display();

	attachment->_posX = origX;
	attachment->_posY = origY;

	return ret;
}

//////////////////////////////////////////////////////////////////////////
PartEmitter *AdObject::createParticleEmitter(bool followParent, int offsetX, int offsetY) {
	_partFollowParent = followParent;
	_partOffsetX = offsetX;
	_partOffsetY = offsetY;

	if (!_partEmitter) {
		_partEmitter = new PartEmitter(_gameRef, this);
		if (_partEmitter) {
			_gameRef->registerObject(_partEmitter);
		}
	}
	updatePartEmitter();
	return _partEmitter;
}

//////////////////////////////////////////////////////////////////////////
bool AdObject::updatePartEmitter() {
	if (!_partEmitter) {
		return STATUS_FAILED;
	}

	if (_partFollowParent) {
		float scaleX, scaleY;
		getScale(&scaleX, &scaleY);

		_partEmitter->_posX = (int32)(_posX + (scaleX / 100.0f) * _partOffsetX);
		_partEmitter->_posY = (int32)(_posY + (scaleY / 100.0f) * _partOffsetY);
	}
	return _partEmitter->update();
}

} // End of namespace Wintermute
