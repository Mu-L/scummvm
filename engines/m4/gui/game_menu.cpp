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

#include "m4/core/imath.h"
#include "m4/graphics/gr_sprite.h"
#include "m4/gui/game_menu.h"
#include "m4/gui/gui_vmng.h"
#include "m4/m4.h"

namespace M4 {
namespace GUI {

void SaveLoadMenuBase::initializeSlotTables(void) {
	const SaveStateList saves = g_engine->listSaves();

	// First reset all the slots to empty
	for (int i = 0; i < MAX_SLOTS; ++i) {
		Common::strcpy_s(_GM(slotTitles)[i], 80, "<empty>");
		_GM(slotInUse)[i] = false;
	}

	for (const auto &save : saves) {
		if (save.getSaveSlot() != 0) {
			Common::String desc = save.getDescription();
			Common::strcpy_s(_GM(slotTitles)[save.getSaveSlot() - 1], 80, desc.c_str());
			_GM(slotInUse)[save.getSaveSlot() - 1] = true;
		}
	}
}

Sprite *SaveLoadMenuBase::menu_CreateThumbnail(int32 *spriteSize) {
	Sprite *thumbNailSprite;
	GrBuff *thumbNail;
	Buffer *scrnBuff, *intrBuff, *destBuff, RLE8Buff;
	uint8 *srcPtr, *srcPtr2, *srcPtr3, *srcRowPtr, *destPtr;
	ScreenContext *gameScreen;
	int32 i, status;
	int32 currRow, beginRow, endRow;

	// Create a Sprite for the thumbNail
	if ((thumbNailSprite = (Sprite *)mem_alloc(sizeof(Sprite), "sprite")) == nullptr) {
		return nullptr;
	}

	thumbNail = new GrBuff((MAX_VIDEO_X + 1) / 3, (MAX_VIDEO_Y + 1) / 3);
	if (!thumbNail) {
		return nullptr;
	}

	destBuff = thumbNail->get_buffer();
	if (!destBuff) {
		return nullptr;
	}

	gameScreen = vmng_screen_find(_G(gameDrawBuff), &status);
	if ((!gameScreen) || (status != SCRN_ACTIVE)) {
		return nullptr;
	}

	scrnBuff = _G(gameDrawBuff)->get_buffer();
	if (!scrnBuff) {
		return nullptr;
	}

	// Grab the interface buffer
	intrBuff = _G(gameInterfaceBuff)->get_buffer();

	if (gameScreen->y1 > 0) {
		// Paint the top of the thumbnail black
		beginRow = gameScreen->y1;
		memset(destBuff->data, 21, (beginRow / 3) * destBuff->stride);
		srcRowPtr = (uint8 *)(scrnBuff->data + (-gameScreen->x1));
		destPtr = (uint8 *)(destBuff->data + ((beginRow / 3) * destBuff->stride));
	} else {
		srcRowPtr = (uint8 *)(scrnBuff->data + ((-gameScreen->y1) * scrnBuff->stride) + (-gameScreen->x1));
		beginRow = 0;
		destPtr = destBuff->data;
	}
	endRow = imath_min(MAX_VIDEO_Y, gameScreen->y2);

	for (currRow = beginRow; currRow <= endRow; currRow += 3) {

		// Set the src pointers
		srcPtr = srcRowPtr;
		srcPtr2 = srcRowPtr + scrnBuff->stride;
		srcPtr3 = srcRowPtr + (scrnBuff->stride << 1);

		for (i = 0; i < (MAX_VIDEO_X + 1) / 3; i++) {

			// Calculate the average - make sure not to extend past the end of the buffer
			if (endRow - currRow < 1) {
				*destPtr = (uint8)((uint32)((*srcPtr + *(srcPtr + 1) + *(srcPtr + 2)) / 3));
			} else if (endRow - currRow < 2) {
				*destPtr = (uint8)((uint32)((*srcPtr + *(srcPtr + 1) + *(srcPtr + 2) +
					*srcPtr2 + *(srcPtr2 + 1) + *(srcPtr2 + 2)) / 6));
			} else {
				*destPtr = (uint8)((uint32)((*srcPtr + *(srcPtr + 1) + *(srcPtr + 2) +
					*srcPtr2 + *(srcPtr2 + 1) + *(srcPtr2 + 2) +
					*srcPtr3 + *(srcPtr3 + 1) + *(srcPtr3 + 2)) / 9));
			}

			if (*destPtr == 0) {
				*destPtr = 21;
			}

			// Increment the pointers
			srcPtr += 3;
			srcPtr2 += 3;
			srcPtr3 += 3;
			destPtr++;
		}

		// Update the row pointer
		srcRowPtr += scrnBuff->stride * 3;
	}

	// Reset the currRow
	beginRow = currRow;

	// Paint the interface section of the thumbnail
	if (currRow < MAX_VIDEO_Y) {
		// If the interface is visible, grab it
		if (intrBuff) {
			srcRowPtr = intrBuff->data;
			endRow = imath_min(MAX_VIDEO_Y, beginRow + intrBuff->h - 1);
			for (currRow = beginRow; currRow <= endRow; currRow += 3) {
				// Set the src pointers
				srcPtr = srcRowPtr;
				srcPtr2 = srcRowPtr + intrBuff->stride;
				srcPtr3 = srcRowPtr + (intrBuff->stride << 1);

				for (i = 0; i < (MAX_VIDEO_X + 1) / 3; i++) {
					// If the pix is outside of the inventory objects in the interface, set to black
					// If ((srcPtr - srcRowPtr < 180) || (srcPtr - srcRowPtr > 575)) {
					if (true) {			// for now make everything in the interface black
						*destPtr = 21;
					}

					// Else calculate the average - make sure not to extend past the end of the buffer
					else {
						if (endRow - currRow < 1) {
							*destPtr = (uint8)((uint32)((*srcPtr + *(srcPtr + 1) + *(srcPtr + 2)) / 3));
						} else if (endRow - currRow < 2) {
							*destPtr = (uint8)((uint32)((*srcPtr + *(srcPtr + 1) + *(srcPtr + 2) +
								*srcPtr2 + *(srcPtr2 + 1) + *(srcPtr2 + 2)) / 6));
						} else {
							*destPtr = (uint8)((uint32)((*srcPtr + *(srcPtr + 1) + *(srcPtr + 2) +
								*srcPtr2 + *(srcPtr2 + 1) + *(srcPtr2 + 2) +
								*srcPtr3 + *(srcPtr3 + 1) + *(srcPtr3 + 2)) / 9));
						}
						if (*destPtr == 0) {
							*destPtr = 21;
						}
					}

					// Increment the pointers
					srcPtr += 3;
					srcPtr2 += 3;
					srcPtr3 += 3;
					destPtr++;
				}

				// Update the row pointer
				srcRowPtr += intrBuff->stride * 3;
			}
		} else {
			// Else paint the bottom of the thumbnail black
			destPtr = (uint8 *)(destBuff->data + ((currRow / 3) * destBuff->stride));
			memset(destPtr, 21, (destBuff->h - (currRow / 3)) * destBuff->stride);
		}
	}

	// Reset the currRow
	beginRow = currRow;

	if (currRow < MAX_VIDEO_Y) {
		// Paint the bottom of the thumbnail black
		destPtr = (uint8 *)(destBuff->data + ((currRow / 3) * destBuff->stride));
		memset(destPtr, 21, (destBuff->h - (currRow / 3)) * destBuff->stride);
	}

	// Compress the thumbNail data into the RLE8Buff
	if ((*spriteSize = (int32)gr_sprite_RLE8_encode(destBuff, &RLE8Buff)) <= 0) {
		return nullptr;
	}

	// Fill in the Sprite structure
	thumbNailSprite->w = destBuff->w;
	thumbNailSprite->h = destBuff->h;
	thumbNailSprite->encoding = RLE8;
	thumbNailSprite->data = nullptr;
	if ((thumbNailSprite->sourceHandle = NewHandle(*spriteSize, "thumbNail source")) == nullptr) {
		return nullptr;
	}
	thumbNailSprite->sourceOffset = 0;

	// Now copy the RLE8Buff into the thumbNail source handle
	HLock(thumbNailSprite->sourceHandle);
	thumbNailSprite->data = (uint8 *)(*(thumbNailSprite->sourceHandle));
	memcpy(thumbNailSprite->data, RLE8Buff.data, *spriteSize);
	HUnLock(thumbNailSprite->sourceHandle);

	// Release all buffers
	_G(gameDrawBuff)->release();
	if (intrBuff) {
		_G(gameInterfaceBuff)->release();
	}
	thumbNail->release();

	// Free up both the thumbNail and the RLE8Buff
	delete thumbNail;
	mem_free((void *)RLE8Buff.data);

	return thumbNailSprite;
}

bool SaveLoadMenuBase::loadThumbnail(int32 slotNum) {
	Sprite *&thumbNailSprite = _GM(thumbNails)[slotNum];
	return g_engine->loadSaveThumbnail(slotNum + 1, thumbNailSprite);
}

void SaveLoadMenuBase::unloadThumbnail(int32 slotNum) {
	if (_GM(thumbNails)[slotNum]->sourceHandle) {
		HUnLock(_GM(thumbNails)[slotNum]->sourceHandle);
		DisposeHandle(_GM(thumbNails)[slotNum]->sourceHandle);
		_GM(thumbNails)[slotNum]->sourceHandle = nullptr;
	}
}

void SaveLoadMenuBase::updateThumbnails(int32 firstSlot, guiMenu *myMenu) {
	int32 i, startIndex, endIndex;

	// Make sure there is something to update
	if (firstSlot == _GM(thumbIndex)) {
		return;
	}

	// Ensure firstSlot is in a valid range
	firstSlot = imath_max(imath_min(firstSlot, 89), 0);

	if (firstSlot > _GM(thumbIndex)) {
		// Dump Out all thumbnails in slots which don't overlap
		startIndex = _GM(thumbIndex);
		endIndex = imath_min(_GM(thumbIndex) + 9, firstSlot - 1);
		for (i = startIndex; i <= endIndex; i++) {
			unloadThumbnail(i);
		}

		// Load in all thumbnails missing thumbnails
		startIndex = imath_max(_GM(thumbIndex) + 10, firstSlot);
		endIndex = imath_min(firstSlot + 9, 98);
		for (i = startIndex; i <= endIndex; i++) {
			if (_GM(slotInUse)[i]) {
				if (!loadThumbnail(i)) {
					_GM(slotInUse)[i] = false;
					menuItemButton::disableButton(nullptr, 1001 + i - firstSlot, myMenu);
					guiMenu::itemRefresh(nullptr, 1001 + i - firstSlot, myMenu);
				}
			}
		}
	} else {
		// Else firstSlot < _GM(thumbIndex)
		// Dump Out all thumbnails in slots which don't overlap
		startIndex = imath_max(firstSlot + 10, _GM(thumbIndex));
		endIndex = imath_min(_GM(thumbIndex) + 9, 98);
		for (i = startIndex; i <= endIndex; i++) {
			unloadThumbnail(i);
		}

		// Load in all thumbnails missing thumbnails
		startIndex = firstSlot;
		endIndex = imath_min(firstSlot + 9, _GM(thumbIndex) - 1);
		for (i = startIndex; i <= endIndex; i++) {
			if (_GM(slotInUse)[i]) {
				if (!loadThumbnail(i)) {
					_GM(slotInUse)[i] = false;
					menuItemButton::disableButton(nullptr, 1001 + i - firstSlot, myMenu);
					guiMenu::itemRefresh(nullptr, 1001 + i - firstSlot, myMenu);
				}
			}
		}
	}

	// Set the var
	_GM(thumbIndex) = firstSlot;
}

void SaveLoadMenuBase::setFirstSlot(int32 firstSlot, guiMenu *myMenu) {
	menuItemButton *myButton;
	int32 i;

	if (!myMenu) {
		return;
	}

	// Ensure firstSlot is in a valid range
	firstSlot = imath_max(imath_min(firstSlot, 89), 0);

	// Change the prompt and special tag of each of the slot buttons
	for (i = 0; i < MAX_SLOTS_SHOWN; i++) {
		myButton = (menuItemButton *)guiMenu::getItem(i + 1001, myMenu);

		myButton->prompt = _GM(slotTitles)[firstSlot + i];
		if (_GM(currMenuIsSave) || _GM(slotInUse)[firstSlot + i]) {
			myButton->itemFlags = menuItemButton::BTN_STATE_NORM;
		} else {
			myButton->itemFlags = menuItemButton::BTN_STATE_GREY;
		}

		myButton->specialTag = firstSlot + i + 1;
		guiMenu::itemRefresh(myButton, i + 1001, myMenu);
	}
}

} // namespace GUI
} // namespace M4
