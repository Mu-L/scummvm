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

#include "drascula/drascula.h"
#include "graphics/surface.h"

#include "common/stream.h"
#include "common/textconsole.h"
#include "common/text-to-speech.h"

namespace Drascula {

void DrasculaEngine::allocMemory() {
	// FIXME: decodeOffset writes beyond 64000, so this
	// buffer has been initialized to 64256 bytes (like
	// the original did with the MiVideoSSN buffer)
	screenSurface = (byte *)malloc(64256);
	assert(screenSurface);
	frontSurface = (byte *)malloc(64000);
	assert(frontSurface);
	backSurface = (byte *)malloc(64000);
	assert(backSurface);
	bgSurface = (byte *)malloc(64000);
	assert(bgSurface);
	drawSurface2 = (byte *)malloc(64000);
	assert(drawSurface2);
	drawSurface3 = (byte *)malloc(64000);
	assert(drawSurface3);
	tableSurface = (byte *)malloc(64000);
	assert(tableSurface);
	extraSurface = (byte *)malloc(64000);
	assert(extraSurface);
	crosshairCursor = (byte *)malloc(OBJWIDTH * OBJHEIGHT);
	assert(crosshairCursor);
	mouseCursor = (byte *)malloc(OBJWIDTH * OBJHEIGHT);
	assert(mouseCursor);
	cursorSurface = (byte *)malloc(64000);
}

void DrasculaEngine::freeMemory() {
	free(screenSurface);
	free(bgSurface);
	free(backSurface);
	free(drawSurface2);
	free(tableSurface);
	free(drawSurface3);
	free(extraSurface);
	free(frontSurface);
	free(crosshairCursor);
	free(mouseCursor);
	free(cursorSurface);
}

void DrasculaEngine::moveCursor() {
	copyBackground();

	updateRefresh_pre();
	moveCharacters();
	updateRefresh();

	if (!strcmp(textName, _textmisc[3]) && _hasName) {
		if (_color != kColorRed && !_menuScreen)
			color_abc(kColorRed);
	} else if (!_menuScreen && _color != kColorLightGreen)
		color_abc(kColorLightGreen);
	if (_hasName && !_menuScreen) {
		sayText(textName, Common::TextToSpeechManager::INTERRUPT);

		centerText(textName, _mouseX, _mouseY);
	} else if (!_menuBar && !_menuScreen)
		_previousSaid.clear();
	
	if (_menuScreen)
		showMenu();
	else if (_menuBar)
		clearMenu();
}

void DrasculaEngine::loadPic(const char *NamePcc, byte *targetSurface, int colorCount) {
	debug(5, "loadPic(%s)", NamePcc);

	uint dataSize = 0;
	byte *pcxData;

	Common::SeekableReadStream *stream = _archives.open(NamePcc);
	if (!stream)
		error("missing game data %s %c", NamePcc, 7);

	dataSize = stream->size() - 128 - (256 * 3);
	pcxData = (byte *)malloc(dataSize);

	stream->seek(128, SEEK_SET);
	stream->read(pcxData, dataSize);

	decodeRLE(pcxData, targetSurface);
	free(pcxData);

	for (int i = 0; i < 256; i++) {
		cPal[i * 3 + 0] = stream->readByte();
		cPal[i * 3 + 1] = stream->readByte();
		cPal[i * 3 + 2] = stream->readByte();
	}

	delete stream;

	setRGB((byte *)cPal, colorCount);
}

void DrasculaEngine::showFrame(Common::SeekableReadStream *stream, bool firstFrame) {
	int dataSize = stream->readSint32LE();
	byte *pcxData = (byte *)malloc(dataSize);
	stream->read(pcxData, dataSize);

	for (int i = 0; i < 256; i++) {
		cPal[i * 3 + 0] = stream->readByte();
		cPal[i * 3 + 1] = stream->readByte();
		cPal[i * 3 + 2] = stream->readByte();
	}

	byte *prevFrame = (byte *)malloc(64000);
	Graphics::Surface *screenSurf = _system->lockScreen();
	byte *screenBuffer = (byte *)screenSurf->getPixels();
	uint16 screenPitch = screenSurf->pitch;
	for (int y = 0; y < 200; y++) {
		memcpy(prevFrame+y*320, screenBuffer+y*screenPitch, 320);
	}

	decodeRLE(pcxData, screenBuffer, screenPitch);
	free(pcxData);

	if (!firstFrame)
		mixVideo(screenBuffer, prevFrame, screenPitch);

	_system->unlockScreen();
	_system->updateScreen();

	if (firstFrame)
		setPalette(cPal);

	free(prevFrame);
}

void DrasculaEngine::copyBackground(int xorg, int yorg, int xdes, int ydes, int width, int height, byte *src, byte *dest) {
	debug(5, "DrasculaEngine::copyBackground(xorg:%d, yorg:%d, xdes:%d, ydes:%d width:%d height:%d, src, dest)", xorg, yorg, xdes, ydes, width,height);
	dest += xdes + ydes * 320;
	src += xorg + yorg * 320;
	/* Unoptimized code
	for (int x = 0; x < height; x++) {
		memcpy(dest + 320 * x, src + 320 * x, width);
	} */

	// A bit more optimized code, thanks to Fingolfin
	// Uses 2 less registers and performs 2 less multiplications
	int x = height;
	while (x--) {
		memcpy(dest, src, width);
		dest += 320;
		src += 320;
	}
}

void DrasculaEngine::copyRect(int xorg, int yorg, int xdes, int ydes, int width,
								   int height, byte *src, byte *dest) {
	int y, x;

	//
	if (ydes < 0) {
		yorg += -ydes;
		height += ydes;
		ydes = 0;
	}
	if (xdes < 0) {
		xorg += -xdes;
		width += xdes;
		xdes = 0;
	}
	if ((xdes + width) > 319)
		width -= (xdes + width) - 320;
	if ((ydes + height) > 199)
		height -= (ydes + height) - 200;
	//

	dest += xdes + ydes * 320;
	src += xorg + yorg * 320;

	assert(xorg >= 0);
	assert(yorg >= 0);
	assert(xorg + width <= 320);
	assert(yorg + height <= 200);

	int ptr = 0;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if (src[ptr] != 255)
				dest[ptr] = src[ptr];
			ptr++;
		}
		ptr += 320 - width;
	}

}

void DrasculaEngine::updateScreen(int xorg, int yorg, int xdes, int ydes, int width, int height, byte *buffer) {
	_system->copyRectToScreen(buffer + xorg + yorg * 320, 320, xdes, ydes, width, height);
	_system->updateScreen();
}

void DrasculaEngine::print_abc(const char *said, int screenX, int screenY) {
	int letterY = 0, letterX = 0, i;
	uint len = strlen(said);
	byte c;

	byte *srcSurface = tableSurface;
	if (_lang == kSpanish && currentChapter == 6)
		srcSurface = extraSurface;

	for (uint h = 0; h < len; h++) {
		c = toupper(said[h]);

		for (i = 0; i < _charMapSize; i++) {
			if (c == _charMap[i].inChar) {
				letterX = _charMap[i].mappedChar;

				switch (_charMap[i].charType) {
				case 0:		// letters
				default:
					letterY = (_lang == kSpanish) ? 149 : 158;
					break;
				case 1:		// signs
					letterY = (_lang == kSpanish) ? 160 : 169;
					break;
				case 2:		// accented
					letterY = 180;
					break;
				}	// switch
				break;
			}	// if
		}	// for

		copyRect(letterX, letterY, screenX, screenY,
				 CHAR_WIDTH, CHAR_HEIGHT, srcSurface, screenSurface);

		screenX = screenX + CHAR_WIDTH;
		if (screenX > 317) {
			screenX = 0;
			screenY = screenY + CHAR_HEIGHT + 2;
		}
	}	// for
}

int DrasculaEngine::print_abc_opc(const char *said, int screenY, int game) {
	int signY, letterY, letterX = 0;
	uint len = strlen(said);

	int screenX = 1;
	int lines = 1;

	for (uint h = 0; h < len; h++) {
		int wordLength;

		// Look ahead to the end of the word.
		wordLength = 0;
		int pos = h;
		while (said[pos] && said[pos] != ' ') {
			wordLength++;
			pos++;
		}

		if (screenX + wordLength * CHAR_WIDTH_OPC > 317) {
			screenX = 0;
			screenY += (CHAR_HEIGHT + 2);
			lines++;
		}

		if (game == 1) {
			letterY = 6;
			signY = 15;
		} else if (game == 3) {
			letterY = 56;
			signY = 65;
		} else {
			letterY = 31;
			signY = 40;
		}

		byte c = toupper(said[h]);

		// WORKAROUND: Even original did not process it correctly
		// Fixes apostrophe rendering
		if (_lang != kSpanish)
			if (c == '\'')
				c = (byte)'\244';

		for (int i = 0; i < _charMapSize; i++) {
			if (c == _charMap[i].inChar) {
				// Convert the mapped char of the normal font to the
				// mapped char of the dialogue font

				int multiplier = (_charMap[i].mappedChar - 6) / 9;

				letterX = multiplier * 7 + 10;

				if (_charMap[i].charType > 0)
					letterY = signY;
				break;
			}	// if
		}	// for

		copyRect(letterX, letterY, screenX, screenY,
				 CHAR_WIDTH_OPC, CHAR_HEIGHT_OPC, backSurface, screenSurface);

		screenX = screenX + CHAR_WIDTH_OPC;
	}

	return lines;
}

bool DrasculaEngine::textFitsCentered(char *text, int x) {
	int textLen = strlen(text);
	int halfLen = (textLen / 2) * CHAR_WIDTH;

	//if (x > 160)
	//	x = 315 - x;
	//return (halfLen <= x);

	// The commented out code above is what the original engine is doing. Instead of testing the
	// upper bound if x is greater than 160 it takes the complement to 315 and test only the lower
	// bounds.
	// Also note that since it does an integer division to compute the half length of the string,
	// in the case where the string has an odd number of characters there is one more character to
	// the right than to the left. If the string center is beyond 160, this is taken care of by
	// taking the complement to 315 instead of 320. But if the string center is close to the screen
	// center, but not greater than 160, this can lead to the string being accepted despite having
	// one character beyond the right edge of the screen.
	// In ScummVM we therefore also test the right edge, which leads to differences
	// with the original engine, but for the better.
	if (x > 160)
		return (315 - x - halfLen >= 0);
	return (x - halfLen >= 0 && x + halfLen + (textLen % 2) * CHAR_WIDTH <= 320);
}

void DrasculaEngine::centerText(const char *message, int textX, int textY) {
	char msg[200];
	Common::strlcpy(msg, message, 200);

	// We make sure to have a width of at least 120 pixels by clipping the center.
	// In theory since the screen width is 320 I would expect something like this:
	// x = CLIP<int>(x, 60, 260);
	// return (x - halfLen >= 0 && x + halfLen <= 319);

	// The engines does things differently though. It tries to clips text at 315 instead of 319.
	// See also the comment in textFitsCentered().

	textX = CLIP<int>(textX, 60, 255);

	// If the message fits on screen as-is, just print it here
	if (textFitsCentered(msg, textX)) {
		int x = textX - (strlen(msg) / 2) * CHAR_WIDTH - 1;
		// The original starts to draw (nbLines + 2) lines above textY, except if there is a single line
		// in which case it starts drawing at (nbLines + 3) above textY.
		// Also clip to the screen height although the original does not do it.
		int y = textY - 4 * CHAR_HEIGHT;
		y = CLIP<int>(y, 0, 200 - CHAR_HEIGHT);
		print_abc(msg, x, y);
		return;
	}

	// If it's a one-word message it can't be broken up. It's probably a
	// mouse-over text, so try just sliding it to the side a bit to make it
	// fit. This happens with the word "TOTENKOPF" in the very first room
	// with the German translation.
	if (!strchr(msg, ' ')) {
		int len = strlen(msg);
		int x = CLIP<int>(textX - (len / 2) * CHAR_WIDTH - 1, 0, 319 - len * CHAR_WIDTH);
		int y = textY - 4 * CHAR_HEIGHT;
		y = CLIP<int>(y, 0, 200 - CHAR_HEIGHT);
		print_abc(msg, x, y);
		return;
	}

	// Message doesn't fit on screen, split it
	char messageLines[15][41]; // screenWidth/charWidth = 320/8 = 40. Thus lines can have up to 41 characters with the null terminator (despite the original allocating only 40 characters here).
	int curLine = 0;
	char messageCurLine[50];
	char tmpMessageCurLine[50];
	*messageCurLine = 0;
	*tmpMessageCurLine = 0;
	// Get a word from the message
	char* curWord = strtok(msg, " ");
	while (curWord != nullptr) {
		// Check if the word and the current line fit on screen
		if (tmpMessageCurLine[0] != '\0')
			Common::strlcat(tmpMessageCurLine, " ", 50);
		Common::strlcat(tmpMessageCurLine, curWord, 50);
		if (textFitsCentered(tmpMessageCurLine, textX)) {
			// Line fits, so add the word to the current message line
			Common::strcpy_s(messageCurLine, tmpMessageCurLine);
		} else {
			// Line does't fit. Store the current line and start a new line.
			Common::strlcpy(messageLines[curLine++], messageCurLine, 41);
			Common::strlcpy(messageCurLine, curWord, 50);
			Common::strlcpy(tmpMessageCurLine, curWord, 50);
		}

		// Get next word
		curWord = strtok(nullptr, " ");
		if (curWord == nullptr) {
			// The original has an interesting bug that if we split the text on several lines
			// a space is added at the end (which impacts the alignment, and may even cause the line
			// to become too long).
			Common::strlcat(messageCurLine, " ", 50);
			if (!textFitsCentered(messageCurLine, textX)) {
				messageCurLine[strlen(messageCurLine) - 1] = '\0';
				Common::strlcpy(messageLines[curLine++], messageCurLine, 41);
				Common::strcpy_s(messageLines[curLine++], " ");
			} else
				Common::strlcpy(messageLines[curLine++], messageCurLine, 41);
		}
	}

	// The original starts to draw (nbLines + 2) lines above textY.
	// Also clip to the screen height although the original does not do it.
	int y = textY - (curLine + 2) * CHAR_HEIGHT;
	y = CLIP<int>(y, 0, 200 - curLine * (CHAR_HEIGHT + 2) + 2);
	for (int line = 0 ; line < curLine ; ++line, y += CHAR_HEIGHT + 2) {
		int textHalfLen = (strlen(messageLines[line]) / 2) * CHAR_WIDTH;
		print_abc(messageLines[line], textX - textHalfLen - 1, y);
	}
}

void DrasculaEngine::screenSaver() {
	int xr, yr;
	byte *copia, *ghost;
	float coeff = 0, coeff2 = 0;
	int count = 0;
	int count2 = 0;
	int tempLine[320];
	int tempRow[200];

	hideCursor();

	clearRoom();

	loadPic("sv.alg", bgSurface, HALF_PAL);

	// inicio_ghost();
	copia = (byte *)malloc(64000);
	ghost = (byte *)malloc(65536);

	// carga_ghost();
	Common::SeekableReadStream *stream = _archives.open("ghost.drv");
	if (!stream)
		error("Cannot open file ghost.drv");

	stream->read(ghost, 65536);
	delete stream;

	updateEvents();
	xr = _mouseX;
	yr = _mouseY;

	while (!shouldQuit()) {
		// efecto(bgSurface);

		memcpy(copia, bgSurface, 64000);
		coeff += 0.1f;
		coeff2 = coeff;

		if (++count > 319)
			count = 0;

		for (int i = 0; i < 320; i++) {
			tempLine[i] = (int)(sin(coeff2) * 16);
			coeff2 += 0.02f;
			tempLine[i] = checkWrapY(tempLine[i]);
		}

		coeff2 = coeff;
		for (int i = 0; i < 200; i++) {
			tempRow[i] = (int)(sin(coeff2) * 16);
			coeff2 += 0.02f;
			tempRow[i] = checkWrapX(tempRow[i]);
		}

		if (++count2 > 199)
			count2 = 0;

		int x1_, y1_, off1, off2;

		Graphics::Surface *screenSurf = _system->lockScreen();
		byte *screenBuffer = (byte *)screenSurf->getPixels();
		uint16 screenPitch = screenSurf->pitch;
		for (int i = 0; i < 200; i++) {
			for (int j = 0; j < 320; j++) {
				x1_ = j + tempRow[i];
				x1_ = checkWrapX(x1_);

				y1_ = i + count2;
				y1_ = checkWrapY(y1_);

				off1 = 320 * y1_ + x1_;

				x1_ = j + count;
				x1_ = checkWrapX(x1_);

				y1_ = i + tempLine[j];
				y1_ = checkWrapY(y1_);
				off2 = 320 * y1_ + x1_;

				screenBuffer[screenPitch * i + j] = ghost[bgSurface[off2] + (copia[off1] << 8)];
			}
		}

		_system->unlockScreen();
		_system->updateScreen();

		_system->delayMillis(20);

		// end of efecto()

		updateEvents();
		if (_rightMouseButton == 1 || _leftMouseButton == 1)
			break;
		if (_mouseX != xr)
			break;
		if (_mouseY != yr)
			break;
	}
	// fin_ghost();
	free(copia);
	free(ghost);

	loadPic(_roomNumber, bgSurface, HALF_PAL);
	showCursor();
}

void DrasculaEngine::playFLI(const char *filefli, int vel) {
	// Open file
	globalSpeed = 1000 / vel;
	FrameSSN = 0;
	Common::SeekableReadStream *stream = _archives.open(filefli);

	if (!stream) {
		warning("playFLI: Failed to load file '%s'", filefli);
		return;
	}
	LastFrame = _system->getMillis();

	while (playFrameSSN(stream) && (!term_int) && !shouldQuit()) {
		if (getScan() == Common::KEYCODE_ESCAPE)
			term_int = 1;
	}

	delete stream;
}

int DrasculaEngine::playFrameSSN(Common::SeekableReadStream *stream) {
	int Exit = 0;
	uint32 length;
	byte *BufferSSN;

	byte CHUNK = stream->readByte();

	switch (CHUNK) {
	case kFrameSetPal: {
		byte dacSSN[768];
		stream->read(dacSSN, 768);
		setPalette(dacSSN);
		break;
		}
	case kFrameEmptyFrame:
		waitFrameSSN();
		break;
	case kFrameInit: {
		byte CMP = stream->readByte();
		length = stream->readUint32LE();
		if (CMP == kFrameCmpRle) {
			BufferSSN = (byte *)malloc(length);
			stream->read(BufferSSN, length);
			decodeRLE(BufferSSN, screenSurface);
			free(BufferSSN);
			waitFrameSSN();

			if (FrameSSN) {
				Graphics::Surface *screenSurf = _system->lockScreen();
				byte *screenBuffer = (byte *)screenSurf->getPixels();
				uint16 screenPitch = screenSurf->pitch;
				mixVideo(screenBuffer, screenSurface, screenPitch);
				_system->unlockScreen();
			} else
				_system->copyRectToScreen(screenSurface, 320, 0, 0, 320, 200);

			_system->updateScreen();
			FrameSSN++;
		} else {
			if (CMP == kFrameCmpOff) {
				BufferSSN = (byte *)malloc(length);
				stream->read(BufferSSN, length);
				decodeOffset(BufferSSN, screenSurface, length);
				free(BufferSSN);
				waitFrameSSN();
				if (FrameSSN) {
					Graphics::Surface *screenSurf = _system->lockScreen();
					byte *screenBuffer = (byte *)screenSurf->getPixels();
					uint16 screenPitch = screenSurf->pitch;
					mixVideo(screenBuffer, screenSurface, screenPitch);
					_system->unlockScreen();
				} else
					_system->copyRectToScreen(screenSurface, 320, 0, 0, 320, 200);

				_system->updateScreen();
				FrameSSN++;
			}
		}
		break;
		}
	case kFrameEndAnim:
		Exit = 1;
		break;
	default:
		Exit = 1;
		break;
	}

	return (!Exit);
}

void DrasculaEngine::decodeOffset(byte *BufferOFF, byte *MiVideoOFF, int length) {
	int x = 0;
	int size;
	int offset;

	memset(screenSurface, 0, 64000);
	while (x < length) {
		offset = BufferOFF[x] + BufferOFF[x + 1] * 256;
		// FIXME: this writes beyond 64000, so the buffer has been initialized
		// to 64256 bytes (like the original did)
		size = BufferOFF[x + 2];
		memcpy(MiVideoOFF + offset, &BufferOFF[x + 3], size);
		x += 3 + size;
	}
}

  void DrasculaEngine::decodeRLE(byte* srcPtr, byte* dstPtr, uint16 pitch) {
	bool stopProcessing = false;
	byte pixel;
	uint repeat;
	int curByte = 0, curLine = 0;
	pitch -= 320;

	while (!stopProcessing) {
		pixel = *srcPtr++;
		repeat = 1;
		if ((pixel & 192) == 192) {
			repeat = (pixel & 63);
			pixel = *srcPtr++;
		}
		for (uint j = 0; j < repeat; j++) {
			*dstPtr++ = pixel;
			if (++curByte >= 320) {
				curByte = 0;
				dstPtr += pitch;
				if (++curLine >= 200) {
					stopProcessing = true;
					break;
				}
			}
		}
	}
}

void DrasculaEngine::mixVideo(byte *OldScreen, byte *NewScreen, uint16 oldPitch) {
	for (int y = 0; y < 200; y++) {
		for (int x = 0; x < 320; x++)
			OldScreen[x] ^= NewScreen[x];
		OldScreen += oldPitch;
		NewScreen += 320;
	}
}

void DrasculaEngine::waitFrameSSN() {
	uint32 now;
	while ((now = _system->getMillis()) - LastFrame < ((uint32) globalSpeed))
		_system->delayMillis(globalSpeed - (now - LastFrame));
	LastFrame = LastFrame + globalSpeed;
}

bool DrasculaEngine::animate(const char *animationFile, int FPS) {
	int NFrames;
	int cnt = 2;

	Common::SeekableReadStream *stream = _archives.open(animationFile);

	if (!stream) {
		warning("Animation file %s not found", animationFile);
		return true;
	}

	NFrames = stream->readSint32LE();
	showFrame(stream, true);
	_system->delayMillis(1000 / FPS);
	while (cnt < NFrames) {
		showFrame(stream);
		_system->delayMillis(1000 / FPS);
		cnt++;
		byte key = getScan();
		if (key == Common::KEYCODE_ESCAPE)
			term_int = 1;
		if (key != 0)
			break;
	}
	delete stream;

	return ((term_int == 1) || (getScan() == Common::KEYCODE_ESCAPE) || shouldQuit());
}

} // End of namespace Drascula
