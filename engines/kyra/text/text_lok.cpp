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

#include "kyra/text/text.h"
#include "kyra/engine/kyra_lok.h"
#include "kyra/graphics/animator_lok.h"
#include "kyra/engine/sprites.h"
#include "kyra/engine/timer.h"

#include "common/system.h"

namespace Kyra {

void KyraEngine_LoK::waitForChatToFinish(int vocFile, int chatDuration, const char *chatStr, uint8 charNum, const bool printText) {
	bool hasUpdatedNPCs = false;
	bool runLoop = true;
	uint8 currPage;

	uint32 timeToEnd = strlen(chatStr) * 8 * _tickLength + _system->getMillis();
	bool textOnly = textEnabled() && vocFile == - 1;

	if (textOnly && chatDuration != -1) {
		switch (_configTextspeed) {
		case 0:
			chatDuration *= 2;
			break;
		case 2:
			chatDuration /= 4;
			break;
		case 3:
			chatDuration = -1;
			break;
		default:
			break;
		}
	}

	if (chatDuration != -1)
		chatDuration *= _tickLength;

	if (vocFile != -1)
		snd_playVoiceFile(vocFile);

	_timer->disable(14);
	_timer->disable(18);
	_timer->disable(19);

	uint32 timeAtStart = _system->getMillis();
	uint32 loopStart;
	while (runLoop) {
		loopStart = _system->getMillis();
		if (_currentCharacter->sceneId == 210)
			if (seq_playEnd())
				break;

		if (_system->getMillis() > timeToEnd && !hasUpdatedNPCs) {
			hasUpdatedNPCs = true;
			_timer->disable(15);
			_currHeadShape = 4;
			_animator->animRefreshNPC(0);
			_animator->animRefreshNPC(_talkingCharNum);

			if (_charSayUnk2 != -1) {
				_animator->sprites()[_charSayUnk2].active = 0;
				_sprites->_anims[_charSayUnk2].play = false;
				_charSayUnk2 = -1;
			}
		}

		_timer->update();
		_sprites->updateSceneAnims();
		_animator->restoreAllObjectBackgrounds();
		_animator->preserveAnyChangedBackgrounds();
		_animator->prepDrawAllObjects();

		if (printText) {
			currPage = _screen->_curPage;
			_screen->_curPage = 2;
			_text->printCharacterText(chatStr, charNum, _characterList[charNum].x1);
			_screen->_curPage = currPage;
		}

		_animator->copyChangedObjectsForward(0);
		updateTextFade();

		if (((chatDuration < (int)(_system->getMillis() - timeAtStart)) && chatDuration != -1 && printText && textOnly) || (!textOnly && !snd_voiceIsPlaying()))
			break;

		uint32 nextTime = loopStart + _tickLength;

		while (_system->getMillis() < nextTime) {
			updateInput();

			if (skipFlag()) {
				runLoop = false;
				break;
			}

			if (nextTime - _system->getMillis() >= 10) {
				_system->delayMillis(10);
				_screen->updateBackendScreen(true);
			}
		}
	}

	if (skipFlag()) {
		resetSkipFlag();
		snd_stopVoice();
	}

	_timer->enable(14);
	_timer->enable(15);
	_timer->enable(18);
	_timer->enable(19);
}

void KyraEngine_LoK::endCharacterChat(int8 charNum, int16 convoInitialized) {
	_talkHeadAnimCharNum = -1;

	if (charNum > 4 && charNum < 11) {
		_animator->sprites()[_disabledTalkAnimObject].active = 1;
		_sprites->_anims[_disabledTalkAnimObject].play = true;

		_animator->sprites()[_enabledTalkAnimObject].active = 0;
		_sprites->_anims[_enabledTalkAnimObject].play = false;
	}

	if (convoInitialized != 0) {
		_talkingCharNum = -1;
		if (_currentCharacter->currentAnimFrame != 88)
			_currentCharacter->currentAnimFrame = 7;
		_animator->animRefreshNPC(0);
		_animator->updateAllObjectShapes();
	}
}

void KyraEngine_LoK::restoreChatPartnerAnimFrame(int8 charNum) {
	_talkingCharNum = -1;

	if (charNum > 0 && charNum < 5) {
		_characterList[charNum].currentAnimFrame = _currentChatPartnerBackupFrame;
		_animator->animRefreshNPC(charNum);
	}

	if (_currentCharacter->currentAnimFrame != 88)
		_currentCharacter->currentAnimFrame = 7;

	_animator->animRefreshNPC(0);
	_animator->updateAllObjectShapes();
}

void KyraEngine_LoK::backupChatPartnerAnimFrame(int8 charNum) {
	_talkingCharNum = 0;

	if (charNum < 5 && charNum > 0)
		_currentChatPartnerBackupFrame = _characterList[charNum].currentAnimFrame;

	if (_currentCharacter->currentAnimFrame != 88) {
		_currentCharacter->currentAnimFrame = 16;
		if (_scaleMode != 0)
			_currentCharacter->currentAnimFrame = 7;
	}

	_animator->animRefreshNPC(0);
	_animator->updateAllObjectShapes();
}

int8 KyraEngine_LoK::getChatPartnerNum() {
	uint8 sceneTable[] = {0x2, 0x5, 0x2D, 0x7, 0x1B, 0x8, 0x22, 0x9, 0x30, 0x0A};
	int pos = 0;
	int partner = -1;

	for (int i = 1; i < 6; i++) {
		if (_currentCharacter->sceneId == sceneTable[pos]) {
			partner = sceneTable[pos + 1];
			break;
		}
		pos += 2;
	}

	for (int i = 1; i < 5; i++) {
		if (_characterList[i].sceneId == _currentCharacter->sceneId) {
			partner = i;
			break;
		}
	}
	return partner;
}

int KyraEngine_LoK::initCharacterChat(int8 charNum) {
	int returnValue = 0;

	if (_talkingCharNum == -1) {
		returnValue = 1;
		_talkingCharNum = 0;

		if (_currentCharacter->currentAnimFrame != 88) {
			_currentCharacter->currentAnimFrame = 16;
			if (_scaleMode != 0)
				_currentCharacter->currentAnimFrame = 7;
		}

		_animator->animRefreshNPC(0);
		_animator->updateAllObjectShapes();
	}

	_charSayUnk2 = -1;
	_animator->flagAllObjectsForBkgdChange();
	_animator->restoreAllObjectBackgrounds();

	if (charNum > 4 && charNum < 11) {
		const uint8 animDisableTable[] = { 3, 1, 1, 5, 0, 6 };
		const uint8 animEnableTable[] = { 4, 2, 5, 6, 1, 7 };

		_disabledTalkAnimObject = animDisableTable[charNum - 5];
		_enabledTalkAnimObject = animEnableTable[charNum - 5];

		_animator->sprites()[_disabledTalkAnimObject].active = 0;
		_sprites->_anims[_disabledTalkAnimObject].play = false;

		_animator->sprites()[_enabledTalkAnimObject].active = 1;
		_sprites->_anims[_enabledTalkAnimObject].play = true;

		_charSayUnk2 = _enabledTalkAnimObject;
	}

	_animator->flagAllObjectsForRefresh();
	_animator->flagAllObjectsForBkgdChange();
	_animator->preserveAnyChangedBackgrounds();
	_talkHeadAnimCharNum = charNum;

	return returnValue;
}

void KyraEngine_LoK::characterSays(int vocFile, const char *chatStr, int16 charNum, int16 chatDuration) {
	uint8 startAnimFrames[] =  { 0x10, 0x32, 0x56, 0x0, 0x0, 0x0 };

	uint16 chatTicks;
	int16 convoInitialized;
	int8 chatPartnerNum;

	if (_currentCharacter->sceneId == 210)
		return;

	snd_voiceWaitForFinish(true);

	convoInitialized = initCharacterChat(charNum);
	chatPartnerNum = getChatPartnerNum();

	if (chatPartnerNum >= 0 && chatPartnerNum < 5)
		backupChatPartnerAnimFrame(chatPartnerNum);

	if (charNum < 5) {
		if (_flags.isTalkie || _flags.platform == Common::kPlatformFMTowns || _flags.platform == Common::kPlatformPC98 || _animator->_brandonScaleX == 0x100 || !_scaleMode) {
			_characterList[charNum].currentAnimFrame = startAnimFrames[charNum];
			_talkHeadAnimCharNum = charNum;
			_talkingCharNum = charNum;
		}
		_animator->animRefreshNPC(charNum);
	}

	char *processedString = _text->preprocessString(chatStr);
	int lineNum = _text->buildMessageSubstrings(processedString);
	if (_flags.lang == Common::ZH_TWN)
		lineNum = (strlen(chatStr) + 31) >> 5;

	int16 yPos = _characterList[charNum].y1;
	yPos -= ((_scaleTable[yPos] * _characterList[charNum].height) >> 8);
	yPos -= 8;
	yPos -= (lineNum * _screen->getFontHeight() + (lineNum - 1) * _screen->_lineSpacing);

	_text->_talkMessageY = (_flags.lang == Common::ZH_TWN) ? CLIP<int>(yPos, 10, 80) : CLIP<int>(yPos, 11, 100);
	_text->_talkMessageH = lineNum * _screen->getFontHeight() + (lineNum - 1) * _screen->_lineSpacing + _text->_langExtraSpacing;

	const bool printText = textEnabled();

	if (printText) {
		_animator->restoreAllObjectBackgrounds();

		_screen->copyRegion(8, _text->_talkMessageY, 8, 136, 304, _text->_talkMessageH, 2, 2);

		_text->printCharacterText(processedString, charNum, _characterList[charNum].x1);
	}

	// This happens right at the beginning, when talking to the tree and can be seen in DOSBox, too.
	// It will make the sentence stay basically forever. We just set it to
	// the value from the other versions (probably some typo from the translators).
	if (_flags.lang == Common::KO_KOR && chatDuration == -20)
		chatDuration = -2;

	if (chatDuration == -2)
		chatTicks = strlen(processedString) * 9;
	else
		chatTicks = (uint16)chatDuration;

	if (!speechEnabled())
		vocFile = -1;
	waitForChatToFinish(vocFile, (chatDuration == -1) ? -1 : chatTicks, chatStr, charNum, printText);

	if (printText) {
		_animator->restoreAllObjectBackgrounds();

		_screen->copyRegion(8, 136, 8, _text->_talkMessageY, 304, _text->_talkMessageH, 2, 2);
		_animator->preserveAllBackgrounds();
		_animator->prepDrawAllObjects();

		_screen->copyRegion(8, _text->_talkMessageY, 8, _text->_talkMessageY, 304, _text->_talkMessageH, 2, 0);
		_animator->flagAllObjectsForRefresh();
		_animator->copyChangedObjectsForward(0);
	}

	if (chatPartnerNum != -1 && chatPartnerNum < 5)
		restoreChatPartnerAnimFrame(chatPartnerNum);

	endCharacterChat(charNum, convoInitialized);
}

void KyraEngine_LoK::drawSentenceCommand(const char *sentence, int color) {
	int boxY1 = 143;
	int boxY2 = 152;
	int col2 = _flags.platform == Common::kPlatformAmiga ? 19 : 12;

	if (_flags.lang == Common::ZH_TWN || _flags.lang == Common::KO_KOR) {
		boxY1 = 140;
		boxY2 = _flags.lang == Common::KO_KOR ? 155 : 153;
		col2 = 0;
	}

	_screen->fillRect(8, boxY1, 311, boxY2, _flags.platform == Common::kPlatformAmiga ? 19 : 12);

	if (_flags.platform == Common::kPlatformAmiga) {
		if (color != 19) {
			_currSentenceColor[0] = 0x3F;
			_currSentenceColor[1] = 0x3F;
			_currSentenceColor[2] = 0x3F;

			_screen->setInterfacePalette(_screen->getPalette(1),
			                             _currSentenceColor[0], _currSentenceColor[1], _currSentenceColor[2]);
		}
	} else if (_startSentencePalIndex != color || _fadeText != false) {
		_currSentenceColor[0] = _screen->getPalette(0)[765] = _screen->getPalette(0)[color * 3 + 0];
		_currSentenceColor[1] = _screen->getPalette(0)[766] = _screen->getPalette(0)[color * 3 + 1];
		_currSentenceColor[2] = _screen->getPalette(0)[767] = _screen->getPalette(0)[color * 3 + 2];

		_screen->setScreenPalette(_screen->getPalette(0));
		_startSentencePalIndex = color;
	}

	if (_flags.lang != Common::HE_ISR) {
		_text->printText(sentence, 8, boxY1, 0xFF, col2, 0);
	} else {
		_screen->_charSpacing = -2;
		_text->printText(sentence, 311 - _screen->getTextWidth(sentence), boxY1, 0xFF, col2, 0);
		_screen->_charSpacing = 0;
	}
	setTextFadeTimerCountdown(15);
	_fadeText = false;
}

void KyraEngine_LoK::updateSentenceCommand(const char *str1, const char *str2, int color) {
	char sentenceCommand[500];
	if (_flags.lang == Common::ZH_TWN) {
		Common::strlcpy(sentenceCommand, str2 ? str2 : str1, sizeof(sentenceCommand));
		if (str2)
			Common::strlcat(sentenceCommand, str1, sizeof(sentenceCommand));
	} else if (_flags.lang == Common::HE_ISR) {
		if (str2)
			Common::strlcpy(sentenceCommand, str2, sizeof(sentenceCommand));
		Common::strlcat(sentenceCommand, str1, sizeof(sentenceCommand));
	} else {
		Common::strlcpy(sentenceCommand, str1, sizeof(sentenceCommand));
		if (str2)
			Common::strlcat(sentenceCommand, str2, sizeof(sentenceCommand));
	}

	drawSentenceCommand(sentenceCommand, color);
	_screen->updateScreen();
}

void KyraEngine_LoK::updateTextFade() {
	if (!_fadeText)
		return;

	bool finished = false;
	for (int i = 0; i < 3; i++) {
		if (_currSentenceColor[i] > 4)
			_currSentenceColor[i] -= 4;
		else if (_currSentenceColor[i]) {
			_currSentenceColor[i] = 0;
			finished = true;
		}
	}

	if (_flags.platform == Common::kPlatformAmiga) {
		_screen->setInterfacePalette(_screen->getPalette(1),
		                             _currSentenceColor[0], _currSentenceColor[1], _currSentenceColor[2]);
	} else {
		_screen->getPalette(0)[765] = _currSentenceColor[0];
		_screen->getPalette(0)[766] = _currSentenceColor[1];
		_screen->getPalette(0)[767] = _currSentenceColor[2];
		_screen->setScreenPalette(_screen->getPalette(0));
	}

	if (finished) {
		_fadeText = false;
		_startSentencePalIndex = -1;
	}
}

} // End of namespace Kyra
