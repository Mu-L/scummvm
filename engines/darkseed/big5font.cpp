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

#include "darkseed/darkseed.h"
#include "darkseed/big5font.h"

#include "graphics/fonts/dosfont.h"

namespace Darkseed {

Big5Font::Big5Font() {
	Common::File fontData;
	if (!fontData.open("big5font_game.dat")) {
		error("Error: failed to open big5font_game.dat");
	}
	_big5.loadPrefixedRaw(fontData, 15);
	fontData.close();
}

int Big5Font::getFontHeight() const {
	return 15;
}

int Big5Font::getMaxCharWidth() const {
	return 17;
}

int Big5Font::getCharWidth(uint32 chr) const {
	if (_big5.hasGlyphForBig5Char(chr)) {
		return getMaxCharWidth();
	}
	return 9;
}

void Big5Font::drawChar(Graphics::Surface *dst, uint32 chr, int x, int y, uint32 color) const {
	Common::Point charPos = {(int16)x, (int16)y};
	if (_big5.drawBig5Char(g_engine->_screen->surfacePtr(), chr, charPos, 0xf)) {
		charPos.x++;
		_big5.drawBig5Char(g_engine->_screen->surfacePtr(), chr, charPos, 0xc);
		charPos.x += Graphics::Big5Font::kChineseTraditionalWidth + 1;
	} else if (chr < 128) {
		drawBiosFontGlyph(chr, x, y, 0xf);
		drawBiosFontGlyph(chr, x+1, y, 0xc);
	}
}

void Big5Font::drawBiosFontGlyph(uint8 chr, int x, int y, uint8 color) const {
	byte *ptr = (byte *)g_engine->_screen->getBasePtr(x, y);
	int srcPixel = chr * 8;
	for (int sy = 0; sy < 8; sy++) {
		for (int sx = 0; sx < 8; sx++) {
			if (Graphics::DosFont::fontData_PCBIOS[srcPixel] & 1 << (7 - sx)) {
				*ptr = color;
				ptr[g_engine->_screen->pitch] = color;
			}
			ptr++;
		}
		srcPixel++;
		ptr -= 8;
		ptr += (g_engine->_screen->pitch * 2);
	}
}

} // namespace Darkseed
