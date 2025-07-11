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

#include "common/system.h"
#include "common/file.h"
#include "common/language.h"
#include "common/platform.h"
#include "common/tokenizer.h"
#include "common/translation.h"

#include "gui/gui-manager.h"
#include "gui/widgets/grid.h"

#include "gui/ThemeEval.h"

namespace GUI {

GridItemWidget::GridItemWidget(GridWidget *boss)
	: ContainerWidget(boss, 0, 0, 0, 0), CommandSender(boss) {

	setFlags(WIDGET_ENABLED | WIDGET_TRACK_MOUSE | WIDGET_CLEARBG);
	_activeEntry = nullptr;
	_grid = boss;
	_isHighlighted = false;
}

void GridItemWidget::setActiveEntry(GridItemInfo &entry) {
	_activeEntry = &entry;
}

void GridItemWidget::updateThumb() {
	const Graphics::ManagedSurface *gfx = _grid->filenameToSurface(_activeEntry->thumbPath);
	_thumbGfx.free();
	if (gfx) {
		// TODO: Use a reference instead of copying the surface
		_thumbGfx.copyFrom(*gfx);
		_thumbAlpha = _thumbGfx.detectAlpha();
	}
}

void GridItemWidget::update() {
	if (_activeEntry) {
		updateThumb();
		setTooltip(_activeEntry->description);
		markAsDirty();
	}
}

void GridItemWidget::move(int x, int y) {
	Widget::setPos(getRelX() + x, getRelY() + y);
}

void GridItemWidget::drawWidget() {
	if (_activeEntry->isHeader) {
		const int indicatorSize = MIN(MIN((int)_h, _grid->_gridXSpacing), (int)_w);
		g_gui.theme()->drawFoldIndicator(Common::Rect(_x, _y, _x + indicatorSize, _y + indicatorSize), _grid->groupExpanded(_activeEntry->entryID));
		g_gui.theme()->drawText(Common::Rect(_x + indicatorSize, _y, _x + _w, _y + _h), Common::U32String(_activeEntry->title),
								ThemeEngine::kStateEnabled, Graphics::kTextAlignLeft, ThemeEngine::kTextInversionFocus, _h, false);
		return;
	}
	int thumbHeight = _grid->getThumbnailHeight();
	int thumbWidth = _grid->getThumbnailWidth();
	Common::Array<Common::U32String> titleLines;
	g_gui.getFont().wordWrapText(_activeEntry->title, thumbWidth, titleLines);

	// FIXME/HACK: We reserve 1/3 of the space between two items to draw the
	//			selection border. This can break when the stroke width of
	//			the border is comparable to 1/3 of grid item spacing. Also,
	//			border shadow is not considered.
	const int kMarginX = _grid->_gridXSpacing / 3;
	const int kMarginY = _grid->_gridYSpacing / 3;

	if ((_isHighlighted) || (_grid->getSelected() == _activeEntry->entryID)) {
		Common::Rect r(_x - kMarginX, _y - kMarginY,
					   _x + _w + kMarginX, _y + _h + kMarginY);
		// Draw a highlighted BG on hover
		g_gui.theme()->drawWidgetBackground(r, ThemeEngine::kGridItemHighlight);
	} else {
		Common::Rect r(_x - 2 * kMarginX, _y - 2 * kMarginY,
					   _x + _w + 2 * kMarginX, _y + _h + 2 * kMarginY);
		// Draw a BG of the same color as grid area
		// when it is not highlighted to cover up
		// the highlight shadow
		// FIXME: Find a way to redraw the area around the widget
		//		 instead of just drawing a cover-up
		g_gui.theme()->drawWidgetBackground(r, ThemeEngine::kGridItemBackground);
	}

	// Draw Thumbnail Background
	g_gui.theme()->drawWidgetBackground(Common::Rect(_x, _y, _x + thumbWidth, _y + thumbHeight),
										ThemeEngine::kThumbnailBackground);

	// Draw Thumbnail
	if (_thumbGfx.empty()) {
		// Draw Title when thumbnail is missing
		int linesInThumb = MIN(thumbHeight / kLineHeight, (int)titleLines.size());
		Common::Rect r(_x, _y + (thumbHeight - linesInThumb * kLineHeight) / 2,
					   _x + thumbWidth, _y + (thumbHeight - linesInThumb * kLineHeight) / 2 + kLineHeight);
		for (int i = 0; i < linesInThumb; ++i) {
			g_gui.theme()->drawText(r, titleLines[i], ThemeEngine::kStateEnabled,
									Graphics::kTextAlignCenter, ThemeEngine::kTextInversionNone,
									0, true, ThemeEngine::kFontStyleNormal,
									ThemeEngine::kFontColorAlternate, false);
			r.translate(0, kLineHeight);
		}
	} else {
		g_gui.theme()->drawManagedSurface(Common::Point(_x + _grid->_thumbnailMargin, _y + _grid->_thumbnailMargin), _thumbGfx, _thumbAlpha);
	}

	Graphics::AlphaType alphaType;

	// Draw Platform Icon
	const Graphics::ManagedSurface *platGfx = _grid->platformToSurface(_activeEntry->platform, alphaType);
	if (platGfx) {
		Common::Point p(_x + thumbWidth - platGfx->w, _y + thumbHeight - platGfx->h);
		g_gui.theme()->drawManagedSurface(p, *platGfx, alphaType);
	}

	// Draw Flag
	const Graphics::ManagedSurface *flagGfx = _grid->languageToSurface(_activeEntry->language, alphaType);
	if (flagGfx) {
		// SVG and PNG can resize differently so it's better to use thumbWidth as reference to
		// ensure all flags are aligned
		Common::Point p(_x + thumbWidth - (thumbWidth / 5), _y + 5);
		g_gui.theme()->drawManagedSurface(p, *flagGfx, alphaType);
	}

	// Draw Demo Overlay
	const Graphics::ManagedSurface *demoGfx = _grid->demoToSurface(_activeEntry->extra, alphaType);
	if (demoGfx) {
		Common::Point p(_x, _y);
		g_gui.theme()->drawManagedSurface(p, *demoGfx, alphaType);
	}

	bool validEntry = _activeEntry->validEntry;

	// Darken thumbnail if path is unreachable
	if (!validEntry) {
		const Graphics::ManagedSurface *darkenGfx = _grid->disabledThumbnail();
		if (darkenGfx) {
			Common::Point p(_x, _y);
			g_gui.theme()->drawManagedSurface(p, *darkenGfx, Graphics::ALPHA_FULL);
		}
	}

	// Draw Title
	if (_grid->_isTitlesVisible) {
		// TODO: Currently title is fixed to two lines at all times, we may want
		//		flexibility to let the theme define the number of title lines.
		if (titleLines.size() > 2) {
			for (uint k = 0; (k < 3) && (titleLines[1].size() > 0); ++k)
				titleLines[1].deleteLastChar();
			titleLines[1] += Common::U32String("...");
		}
		// Display text in alternate color if the path is unreachable
		// Should be ok using kStateDisabled, but the list widget uses FontColorAlternate so let's stick to that
		GUI::ThemeEngine::FontColor color = validEntry ? GUI::ThemeEngine::kFontColorNormal : GUI::ThemeEngine::kFontColorAlternate;
		Common::Rect r(_x, _y + thumbHeight, _x + thumbWidth, _y + thumbHeight + kLineHeight);
		for (uint k = 0; k < MIN(2U, titleLines.size()); ++k) {
			g_gui.theme()->drawText(r, titleLines[k], GUI::ThemeEngine::kStateEnabled, Graphics::kTextAlignCenter, GUI::ThemeEngine::kTextInversionNone,
									0, true, ThemeEngine::kFontStyleBold, color);
			r.translate(0, kLineHeight);
		}
	}
}

void GridItemWidget::handleMouseWheel(int x, int y, int direction) {
	_grid->handleMouseWheel(x, y, direction);
	_isHighlighted = false;
}

void GridItemWidget::handleMouseEntered(int button) {
	if (!_isHighlighted) {
		_isHighlighted = true;
		markAsDirty();
	}
}

void GridItemWidget::handleMouseLeft(int button) {
	if (_isHighlighted) {
		_isHighlighted = false;
		markAsDirty();
	}
}

void GridItemWidget::handleMouseMoved(int x, int y, int button) {
	if (!_isHighlighted) {
		handleMouseEntered(button);
	}
}

void GridWidget::toggleGroup(int groupID) {
	_groupExpanded[groupID] = !_groupExpanded[groupID];
	sortGroups();
	// TODO: Replace reflowLayout with only the necessary sequence of steps
	reflowLayout();
}

void GridWidget::loadClosedGroups(const Common::U32String &groupName) {
	// Recalls what groups were closed from the config
	if (ConfMan.hasKey("group_" + groupName, ConfMan.kApplicationDomain)) {
		const Common::String &val = ConfMan.get("group_" + groupName, ConfMan.kApplicationDomain);
		Common::StringTokenizer hiddenGroups(val);

		for (Common::String tok = hiddenGroups.nextToken(); tok.size(); tok = hiddenGroups.nextToken()) {
			// See if the hidden group is in our group headers still, if so, hide it
			for (Common::U32StringArray::size_type i = 0; i < _groupHeaders.size(); ++i) {
				if (_groupHeaders[i] == tok || (tok == "unnamed" && _groupHeaders[i].size() == 0)) {
					_groupExpanded[i] = false;
					break;
				}
			}
		}
		sortGroups();
		// TODO: Replace reflowLayout with only the necessary sequence of steps
		// reflowLayout();
	}
}

void GridWidget::saveClosedGroups(const Common::U32String &groupName) {
	// Save the hidden groups to the config
	Common::String hiddenGroups;
	for (Common::U32StringArray::size_type i = 0; i < _groupHeaders.size(); ++i) {
		if (!_groupExpanded[i]) {
			if (_groupHeaders[i].size()) {
				hiddenGroups += _groupHeaders[i];
			} else {
				hiddenGroups += "unnamed";
			}
			hiddenGroups += ' ';
		}
	}
	ConfMan.set("group_" + groupName, hiddenGroups, ConfMan.kApplicationDomain);
	ConfMan.flushToDisk();
}

void GridItemWidget::handleMouseDown(int x, int y, int button, int clickCount) {
	if (_activeEntry->isHeader) {
		_grid->_selectedEntry = nullptr;
		_grid->toggleGroup(_activeEntry->entryID);
	} else if (_isHighlighted && isVisible()) {
		_grid->_selectedEntry = _activeEntry;
		sendCommand(kItemClicked, _activeEntry->entryID);
	}
}

#pragma mark -

GridItemTray::GridItemTray(GuiObject *boss, int x, int y, int w, int h, int entryID, GridWidget *grid)
	: Dialog(x, y, w, h), CommandSender(boss) {

	_entryID = entryID;
	_boss = boss;
	_grid = grid;

	// TODO: Currently, the tray has a hardcoded layout. Theme file may
	//		provide a different layout of buttons.
	int buttonWidth = w / 3;
	int buttonHeight = h / 3;
	int buttonSpacingX = buttonWidth / 3;
	int buttonSpacingY = buttonHeight / 3;
	int trayPaddingX = buttonSpacingX;
	int trayPaddingY = buttonSpacingY;

	_playButton = new PicButtonWidget(this, trayPaddingX, trayPaddingY,
									  2 * buttonWidth + buttonSpacingX, buttonHeight,
									  _("Play"), kPlayButtonCmd);
	_loadButton = new PicButtonWidget(this, trayPaddingX, trayPaddingY + buttonHeight + buttonSpacingY,
									  buttonWidth, buttonHeight,
									  _("Saves"), kLoadButtonCmd);
	_editButton = new PicButtonWidget(this, trayPaddingX + buttonWidth + buttonSpacingX, trayPaddingY + buttonHeight + buttonSpacingY,
									  buttonWidth, buttonHeight,
									  _("Edit"), kEditButtonCmd);
}

void GridItemTray::reflowLayout() {
	_playButton->setGfxFromTheme("button_play.bmp", 0, false);
	_loadButton->setGfxFromTheme("button_load.bmp", 0, false);
	_editButton->setGfxFromTheme("button_options.bmp", 0, false);
}

void GridItemTray::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kPlayButtonCmd:
		close();
		sendCommand(kPlayButtonCmd, _entryID);
		break;
	case kLoadButtonCmd:
		close();
		sendCommand(kLoadButtonCmd, _entryID);
		break;
	case kEditButtonCmd:
		close();
		sendCommand(kEditButtonCmd, _entryID);
		break;
	case kCloseCmd:
		close();
		break;
	default:
		Dialog::handleCommand(sender, cmd, 0);
		break;
	}
}

void GridItemTray::handleMouseDown(int x, int y, int button, int clickCount) {
	Dialog::handleMouseDown(x, y, button, clickCount);
	if ((x < 0 || x > _w) || (y > _h || y < -(_grid->_gridItemHeight))) {
		// Close on clicking outside
		close();
	}
}

void GridItemTray::handleMouseUp(int x, int y, int button, int clickCount) {
	Dialog::handleMouseUp(x, y, button, clickCount);
	if (y < 0 && clickCount >= 2) {
		// Run on double clicking thumbnail
		close();
		sendCommand(kItemDoubleClickedCmd, _entryID);
	}
}

void GridItemTray::handleMouseWheel(int x, int y, int direction) {
	close();
}

void GridItemTray::handleMouseMoved(int x, int y, int button) {
	Dialog::handleMouseMoved(x, y, button);
	if ((x < 0 || x > _w) || (y > _h || y < -(_grid->_gridItemHeight))) {
		// Close on going outside
		close();
	}
}

#pragma mark -

// Load an image file by String name, provide additional render dimensions for SVG images.
// TODO: Add BMP support, and add scaling of non-vector images.
Graphics::ManagedSurface *loadSurfaceFromFile(const Common::String &name, int renderWidth = 0, int renderHeight = 0) {
	Common::Path path(name);
	Graphics::ManagedSurface *surf = nullptr;
	if (name.hasSuffix(".png")) {
#ifdef USE_PNG
		const Graphics::Surface *srcSurface = nullptr;
		Image::PNGDecoder decoder;
		g_gui.lockIconsSet();
		if (g_gui.getIconsSet().hasFile(path)) {
			Common::SeekableReadStream *stream = g_gui.getIconsSet().createReadStreamForMember(path);
			if (!decoder.loadStream(*stream)) {
				g_gui.unlockIconsSet();
				warning("Error decoding PNG");
				return surf;
			}

			srcSurface = decoder.getSurface();
			delete stream;
			if (!srcSurface) {
				warning("Failed to load surface : %s", name.c_str());
			} else if (srcSurface->format.bytesPerPixel != 1) {
				surf = new Graphics::ManagedSurface();
				surf->copyFrom(*srcSurface);
			}
		} else {
			debug(5, "GridWidget: Cannot read file '%s'", name.c_str());
		}
		g_gui.unlockIconsSet();
#else
		error("No PNG support compiled");
#endif
	} else if (name.hasSuffix(".svg")) {
		g_gui.lockIconsSet();
		if (g_gui.getIconsSet().hasFile(path)) {
			Common::SeekableReadStream *stream = g_gui.getIconsSet().createReadStreamForMember(path);
			surf = new Graphics::SVGBitmap(stream, renderWidth, renderHeight);
			delete stream;
		} else {
			debug(5, "GridWidget: Cannot read file '%s'", name.c_str());
		}
		g_gui.unlockIconsSet();
	}
	return surf;
}

#pragma mark -

GridWidget::GridWidget(GuiObject *boss, const Common::String &name)
	: ContainerWidget(boss, name), CommandSender(boss) {

	_thumbnailHeight = 0;
	_thumbnailWidth = 0;
	_flagIconHeight = 0;
	_flagIconWidth = 0;
	_platformIconHeight = 0;
	_platformIconWidth = 0;
	_extraIconHeight = 0;
	_extraIconWidth = 0;
	_disabledIconOverlay = nullptr;

	_minGridXSpacing = 0;
	_minGridYSpacing = 0;
	_isTitlesVisible = 0;
	_scrollBarWidth = 0;
	_thumbnailMargin = 0;

	_scrollWindowPaddingX = 0;
	_scrollWindowPaddingY = 0;

	_scrollBar = new ScrollBarWidget(this, _w - _scrollBarWidth, _y, _scrollBarWidth, _y + _h);
	_scrollBar->setTarget(this);
	_scrollPos = 0;
	_scrollSpeed = 1;
	_firstVisibleItem = 0;
	_lastVisibleItem = 0;
	_rows = 0;
	_itemsPerRow = 0;

	_innerHeight = 0;
	_innerWidth = 0;
	_scrollWindowHeight = 0;
	_scrollWindowWidth = 0;
	_gridYSpacing = 0;
	_gridXSpacing = 0;
	_gridHeaderHeight = kLineHeight;
	_gridHeaderWidth = 0;
	_gridItemHeight = 0;
	_gridItemWidth = 0;
	_trayHeight = kLineHeight * 3;

	_selectedEntry = nullptr;
	_isGridInvalid = true;
}

GridWidget::~GridWidget() {
	unloadSurfaces(_platformIcons);
	unloadSurfaces(_languageIcons);
	unloadSurfaces(_extraIcons);
	unloadSurfaces(_loadedSurfaces);
	delete _disabledIconOverlay;
	_gridItems.clear();
	_dataEntryList.clear();
	_headerEntryList.clear();
	_sortedEntryList.clear();
	_visibleEntryList.clear();
	_platformIconsAlpha.clear();
	_languageIconsAlpha.clear();
	_extraIconsAlpha.clear();
}

template<typename T>
void GridWidget::unloadSurfaces(Common::HashMap<T, const Graphics::ManagedSurface *> &surfaces) {
	for (typename Common::HashMap<T, const Graphics::ManagedSurface *>::iterator i = surfaces.begin(); i != surfaces.end(); ++i) {
		delete i->_value;
	}
	surfaces.clear();
}

const Graphics::ManagedSurface *GridWidget::filenameToSurface(const Common::String &name) {
	if (name.empty())
		return nullptr;
	return _loadedSurfaces[name];
}

const Graphics::ManagedSurface *GridWidget::languageToSurface(Common::Language languageCode, Graphics::AlphaType &alphaType) {
	if (languageCode == Common::UNK_LANG)
		return nullptr;
	alphaType = _languageIconsAlpha[languageCode];
	return _languageIcons[languageCode];
}

const Graphics::ManagedSurface *GridWidget::platformToSurface(Common::Platform platformCode, Graphics::AlphaType &alphaType) {
	if (platformCode == Common::kPlatformUnknown)
		return nullptr;
	alphaType = _platformIconsAlpha[platformCode];
	return _platformIcons[platformCode];
}

const Graphics::ManagedSurface *GridWidget::demoToSurface(const Common::String &extraString, Graphics::AlphaType &alphaType) {
	if (! extraString.contains("Demo") )
		return nullptr;
	alphaType = _extraIconsAlpha[0];
	return _extraIcons[0];
}

const Graphics::ManagedSurface *GridWidget::disabledThumbnail() {
	return _disabledIconOverlay;
}

void GridWidget::setEntryList(Common::Array<GridItemInfo> *list) {
	_dataEntryList.clear();
	_headerEntryList.clear();
	_sortedEntryList.clear();
	_visibleEntryList.clear();
	_isGridInvalid = true;
	_selectedEntry = nullptr;

	for (Common::Array<GridItemInfo>::iterator entryIter = list->begin(); entryIter != list->end(); ++entryIter) {
		_dataEntryList.push_back(*entryIter);
	}
	// TODO: Remove this below, add drawWidget(), that should do the drawing
	if (!_gridItems.empty()) {
		reflowLayout();
	}
}

void GridWidget::setAttributeValues(const Common::Array<Common::U32String> &attrs) {
	assert(attrs.size() == _dataEntryList.size());
	for (uint i = 0; i < _dataEntryList.size(); ++i) {
		_dataEntryList[i].attribute = attrs[i];
	}
}

void GridWidget::setMetadataNames(const Common::StringMap &metadata) {
	_metadataNames = metadata;
}

void GridWidget::groupEntries() {
	_groupExpanded.clear();
	_groupHeaders.clear();
	_groupValueIndex.clear();
	_itemsInGroup.clear();

	for (uint i = 0; i < _dataEntryList.size(); ++i) {
		Common::U32String attrVal = _dataEntryList[i].attribute;
		if (!_groupValueIndex.contains(attrVal)) {
			int newGroupID = _groupValueIndex.size();
			_groupValueIndex.setVal(attrVal, newGroupID);
			_groupHeaders.push_back(attrVal);
			_groupExpanded.push_back(true);
		}
		int groupID = _groupValueIndex.getVal(attrVal);

		_itemsInGroup[groupID].push_back(i);
	}

	sortGroups();
}

void GridWidget::sortGroups() {
	uint oldHeight = _innerHeight;
	_sortedEntryList.clear();
	_headerEntryList.clear();

	if (_filter.empty()) {
		// No filter -> display everything with group headers
		Common::sort(_groupHeaders.begin(), _groupHeaders.end());

		// Avoid reallocation during iteration: that would invalidate our _sortedEntryList items
		_headerEntryList.reserve(_groupHeaders.size());

		for (uint i = 0; i != _groupHeaders.size(); ++i) {
			Common::U32String header = _groupHeaders[i];
			Common::U32String displayedHeader;
			if (_metadataNames.contains(header)) {
				displayedHeader = _metadataNames[header];
			} else {
				displayedHeader = header;
			}
			uint groupID = _groupValueIndex[header];

			// Keep the header in a buffer to be used later
			_headerEntryList.push_back(GridItemInfo(_groupHeaderPrefix + displayedHeader + _groupHeaderSuffix, groupID));
			_sortedEntryList.push_back(&_headerEntryList.back());

			if (_groupExpanded[groupID]) {
				for (int *k = _itemsInGroup[groupID].begin(); k != _itemsInGroup[groupID].end(); ++k) {
					_sortedEntryList.push_back(&_dataEntryList[*k]);
				}
			}
		}
	} else {
		// With filter don't display any group header
		// Restrict the list to everything which contains all words in _filter
		// as substrings, ignoring case.

		Common::U32StringTokenizer tok(_filter);
		Common::U32String tmp;
		int n = 0;

		_sortedEntryList.clear();

		for (GridItemInfo *i = _dataEntryList.begin(); i != _dataEntryList.end(); ++i, ++n) {
			tmp = i->title;
			tmp.toLowercase();
			bool matches = true;
			tok.reset();
			while (!tok.empty()) {
				if (!tmp.contains(tok.nextToken())) {
					matches = false;
					break;
				}
			}

			if (matches) {
				_sortedEntryList.push_back(i);
			}
		}
	}

	calcEntrySizes();
	calcInnerHeight();
	markGridAsInvalid();

	scrollBarRecalc();

	if (calcVisibleEntries()) {
		reloadThumbnails();
	}

	assignEntriesToItems();
	// FIXME: Temporary solution to clear/display the background ofthe scrollbar when list
	// grows too small or large during group toggle. We shouldn't have to redraw the top dialog,
	// but not doing so the background of scrollbar isn't cleared.
	if ((((uint)_scrollBar->_entriesPerPage < oldHeight) && (_scrollBar->_entriesPerPage > _innerHeight)) ||
		(((uint)_scrollBar->_entriesPerPage > oldHeight) && (_scrollBar->_entriesPerPage < _innerHeight))) {
		g_gui.scheduleTopDialogRedraw();
	} else {
		markAsDirty();
	}
}

// Perform a binary search to find the last element before position yPos in arr.
int lastItemBeforeY(const Common::Array<GridItemInfo *> &arr, int yPos) {
	// Binary search to find the last element whose y value is less
	// than _scrollPos, i.e., the last item of the topmost visible row.
	int start = 0;
	int end = (int)arr.size() - 1;
	int mid;
	int ans = -1;
	while (start <= end) {
		mid = start + (end - start) / 2;
		if (arr[mid]->y >= yPos) {
			end = mid - 1;
		} else {
			ans = mid;
			start = mid + 1;
		}
	}
	return ans;
}

bool GridWidget::calcVisibleEntries() {
	bool needsReload = false;

	int nFirstVisibleItem = 0, nLastVisibleItem = 0;
	int temp = lastItemBeforeY(_sortedEntryList, _scrollPos);
	nFirstVisibleItem = temp;
	// We want the leftmost item from the topmost visible row, so we traverse backwards
	while ((nFirstVisibleItem >= 0) &&
		   (_sortedEntryList[nFirstVisibleItem]->y == _sortedEntryList[temp]->y)) {
			nFirstVisibleItem--;
	}
	nFirstVisibleItem++;
	nFirstVisibleItem = (nFirstVisibleItem < 0) ? 0 : nFirstVisibleItem;

	nLastVisibleItem = lastItemBeforeY(_sortedEntryList, _scrollPos + _scrollWindowHeight);
	nLastVisibleItem = (nLastVisibleItem < 0) ? 0 : nLastVisibleItem;

	if ((nFirstVisibleItem != _firstVisibleItem) || (nLastVisibleItem != _lastVisibleItem) || (_isGridInvalid)) {
		needsReload = true;
		_isGridInvalid = false;
		_lastVisibleItem = nLastVisibleItem;
		_firstVisibleItem = nFirstVisibleItem;

		int toRender = MIN(_lastVisibleItem + 1, (int)_sortedEntryList.size());

		_visibleEntryList.clear();
		for (int ind = _firstVisibleItem; ind < toRender; ++ind) {
			_visibleEntryList.push_back(_sortedEntryList[ind]);
		}
	}
	return needsReload;
}

void GridWidget::setTitlesVisible(bool vis) {
	_isTitlesVisible = vis;
}

void GridWidget::setGroupHeaderFormat(const Common::U32String &prefix, const Common::U32String &suffix) {
	_groupHeaderPrefix = prefix;
	_groupHeaderSuffix = suffix;
}

void GridWidget::reloadThumbnails() {
	const int thumbnailWidth = MAX(_thumbnailWidth - 2 * _thumbnailMargin, 0);
	const int thumbnailHeight = MAX(_thumbnailHeight - 2 * _thumbnailMargin, 0);
	for (Common::Array<GridItemInfo *>::iterator iter = _visibleEntryList.begin(); iter != _visibleEntryList.end(); ++iter) {
		GridItemInfo *entry = *iter;
		if (entry->thumbPath.empty())
			continue;

		if (!_loadedSurfaces.contains(entry->thumbPath)) {
			_loadedSurfaces[entry->thumbPath] = nullptr;
			Common::String path = Common::String::format("icons/%s-%s.png", entry->engineid.c_str(), entry->gameid.c_str());
			Graphics::ManagedSurface *surf = loadSurfaceFromFile(path);
			if (!surf) {
				path = Common::String::format("icons/%s.png", entry->engineid.c_str());
				if (!_loadedSurfaces.contains(path)) {
					surf = loadSurfaceFromFile(path);
				} else {
					const Graphics::ManagedSurface *scSurf = _loadedSurfaces[path];
					// TODO: Use SharedPtr instead of duplicating the surface
					Graphics::ManagedSurface *thSurf = new Graphics::ManagedSurface();
					thSurf->copyFrom(*scSurf);
					_loadedSurfaces[entry->thumbPath] = thSurf;
				}
			}

			if (surf) {
				const Graphics::ManagedSurface *scSurf(scaleGfx(surf, thumbnailWidth, thumbnailHeight, true));
				_loadedSurfaces[entry->thumbPath] = scSurf;

				if (path != entry->thumbPath) {
					// TODO: Use SharedPtr instead of duplicating the surface
					Graphics::ManagedSurface *thSurf = new Graphics::ManagedSurface();
					thSurf->copyFrom(*scSurf);
					_loadedSurfaces[path] = thSurf;
				}

				if (surf != scSurf) {
					surf->free();
					delete surf;
				}
			}
		}
	}
}

void GridWidget::loadFlagIcons() {
	const Common::LanguageDescription *l = Common::g_languages;
	for (; l->code; ++l) {
		Common::String path = Common::String::format("icons/flags/%s.svg", l->code);
		Graphics::ManagedSurface *gfx = loadSurfaceFromFile(path, _flagIconWidth, _flagIconHeight);
		if (gfx) {
			_languageIcons[l->id] = gfx;
			_languageIconsAlpha[l->id] = gfx->detectAlpha();
			continue;
		} // if no .svg flag is available, search for a .png
		path = Common::String::format("icons/flags/%s.png", l->code);
		gfx = loadSurfaceFromFile(path);
		if (gfx) {
			const Graphics::ManagedSurface *scGfx = scaleGfx(gfx, _flagIconWidth, _flagIconHeight, true);
			_languageIcons[l->id] = scGfx;
			_languageIconsAlpha[l->id] = gfx->detectAlpha();
			if (gfx != scGfx) {
				gfx->free();
				delete gfx;
			}
		} else {
			_languageIcons[l->id] = nullptr; // nothing found, set to nullptr
		}
	}
}

void GridWidget::loadPlatformIcons() {
	const Common::PlatformDescription *l = Common::g_platforms;
	for (; l->code; ++l) {
		Common::String path = Common::String::format("icons/platforms/%s.png", l->code);
		Graphics::ManagedSurface *gfx = loadSurfaceFromFile(path);
		if (gfx) {
			const Graphics::ManagedSurface *scGfx = scaleGfx(gfx, _platformIconWidth, _platformIconHeight, true);
			_platformIcons[l->id] = scGfx;
			_platformIconsAlpha[l->id] = scGfx->detectAlpha();
			if (gfx != scGfx) {
				gfx->free();
				delete gfx;
			}
		} else {
			_platformIcons[l->id] = nullptr;
		}
	}
}

void GridWidget::loadExtraIcons() {  // for now only the demo icon is available
	Graphics::ManagedSurface *gfx = loadSurfaceFromFile("icons/extra/demo.svg", _extraIconWidth, _extraIconHeight);
	if (gfx) {
		_extraIcons[0] = gfx;
		_extraIconsAlpha[0] = gfx->detectAlpha();
		return;
	} // if no .svg file is available, search for a .png
	gfx = loadSurfaceFromFile("icons/extra/demo.png");
	if (gfx) {
		const Graphics::ManagedSurface *scGfx = scaleGfx(gfx, _extraIconWidth, _extraIconHeight, true);
		_extraIcons[0] = scGfx;
		_extraIconsAlpha[0] = scGfx->detectAlpha();
		if (gfx != scGfx) {
			gfx->free();
			delete gfx;
		}
	} else {
		_extraIcons[0] = nullptr;
	}
}

void GridWidget::destroyItems() {
	for (Common::Array<GridItemWidget *>::iterator i = _gridItems.begin(), end = _gridItems.end(); i != end; ++i) {
		removeWidget((*i));
		delete (*i);
	}

	_gridItems.clear();
}

void GridWidget::move(int x, int y) {
	for (Common::Array<GridItemWidget *>::iterator i = _gridItems.begin(); i != _gridItems.end(); ++i) {
		(*i)->move(x, y);
	}
}

// Scroll to entry id. Optional parameter to decide if the entry should be forced to be on the top, or merely
// scrolled into view.
void GridWidget::scrollToEntry(int id, bool forceToTop) {
	int newScrollPos = _scrollPos;
	for (uint i = 0; i < _sortedEntryList.size(); ++i) {
		if ((!_sortedEntryList[i]->isHeader) && (_sortedEntryList[i]->entryID == id)) {
			if (forceToTop) {
				newScrollPos = _sortedEntryList[i]->y + _scrollWindowPaddingY + _gridYSpacing;
			} else {
				if (_sortedEntryList[i]->y < _scrollPos) {
					// Item is above the visible view
					newScrollPos = _sortedEntryList[i]->y - _scrollWindowPaddingY - _gridYSpacing;
				} else if (_sortedEntryList[i]->y > _scrollPos + _scrollWindowHeight - _gridItemHeight - _trayHeight) {
					// Item is below the visible view
					newScrollPos = _sortedEntryList[i]->y - _scrollWindowHeight + _gridItemHeight + _trayHeight;
				} else {
					// Item already in view, do nothing
					newScrollPos = _scrollPos;
				}
			}
			break;
		}
	}
	handleCommand(this, kSetPositionCmd, newScrollPos);
}

void GridWidget::updateGrid() {
	for (Common::Array<GridItemWidget *>::iterator i = _gridItems.begin(); i != _gridItems.end(); ++i) {
		(*i)->update();
	}
}

void GridWidget::assignEntriesToItems() {
	// Assign entries from _visibleEntries to each GridItem in _gridItems

	// In case we have less ContainerWidgets than the number of visible entries
	if (_visibleEntryList.size() > _gridItems.size()) {
		for (uint l = _gridItems.size(); l < _visibleEntryList.size(); ++l) {
			GridItemWidget *newItem = new GridItemWidget(this);
			newItem->setVisible(false);
			newItem->markAsDirty();
			_gridItems.push_back(newItem);
		}
	}

	for (uint k = 0; k < _gridItems.size(); ++k) {
		GridItemWidget *item = _gridItems[k];
		if (k >= _visibleEntryList.size()) {
			// If we run out of visible entries to display.
			// e.g., scrolled to the very bottom, we make items invisible,
			// and move them out of view to keep them from registering mouse events.
			item->setPos(_scrollWindowWidth, _scrollWindowHeight);
			item->setVisible(false);
		} else {
			// Assign entry and update
			item->setVisible(true);
			GridItemInfo *entry = _visibleEntryList[k];
			item->setActiveEntry(*entry);
			item->setPos(entry->x, entry->y - _scrollPos);
			item->setSize(entry->w, entry->h);
			item->update();
		}
	}
}

int GridWidget::getItemPos(int item) {
	int pos = 0;

	for (uint i = 0; i < _sortedEntryList.size(); i++) {
		if (_sortedEntryList[i]->entryID == item) {
			return pos;
		} else if (!_sortedEntryList[i]->isHeader) {
			pos++;
		}
	}

	return -1;
}

int GridWidget::getNewSel(int index) {
	if (_sortedEntryList.size() == 0) {
		return -1;
	}

	// Find the index-th item in the grid
	for (uint i = 0; i < _sortedEntryList.size(); i++) {
		if (index == 0 && _sortedEntryList[i]->isHeader == 0) {
			return _sortedEntryList[i]->entryID;
		} else if (_sortedEntryList[i]->isHeader == 0) {
			index--;
		}
	}

	if (index == 0) {
		return _sortedEntryList[_sortedEntryList.size() - 1]->entryID;
	} else {
		return -1;
	}
}

void GridWidget::handleMouseWheel(int x, int y, int direction) {
	_scrollBar->handleMouseWheel(x, y, direction);
	_scrollPos = _scrollBar->_currentPos;
}

void GridWidget::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	// Work in progress
	switch (cmd) {
	case kSetPositionCmd:
		if (_scrollPos != (int)data) {
			_scrollPos = data;

			if (calcVisibleEntries()) {
				reloadThumbnails();
			}

			assignEntriesToItems();
			scrollBarRecalc();
			markAsDirty();

			((GUI::Dialog *)_boss)->setFocusWidget(this);
		}
		break;
	case kItemClicked:
		scrollToEntry(data, false);
		// Redraw the grid, before we open the tray dialog
		draw();
		openTrayAtSelected();
		// fall through
	default:
		sendCommand(cmd, 0);
		break;
	}
}

void GridWidget::calcInnerHeight() {
	int row = 0;
	int col = 0;
	int32 lastRowHeight = 0;
	int32 x = _scrollWindowPaddingX + _gridXSpacing, y = _scrollWindowPaddingY;

	for (int k = 0; k < (int)_sortedEntryList.size(); ++k) {
		if (_sortedEntryList[k]->isHeader) {
			while (col != 0) {
				if (++col >= _itemsPerRow) {
					col = 0;
					++row;
					y += lastRowHeight;
					lastRowHeight = 0;
				}
			}
			x = _scrollWindowPaddingX;
			_sortedEntryList[k]->x = x;
			_sortedEntryList[k]->y = y;
			x = _scrollWindowPaddingX + _gridXSpacing;
			++row;
			y += _sortedEntryList[k]->h + _gridYSpacing;
			lastRowHeight = 0;
		} else {
			_sortedEntryList[k]->x = x;
			_sortedEntryList[k]->y = y;
			lastRowHeight = MAX(lastRowHeight, _sortedEntryList[k]->h + _gridYSpacing);
			if (++col >= _itemsPerRow) {
				++row;
				y += lastRowHeight;
				lastRowHeight = 0;
				col = 0;
				x = _scrollWindowPaddingX + _gridXSpacing;
			} else {
				x += _sortedEntryList[k]->w + _gridXSpacing;
			}
		}
	}

	_rows = row;

	_innerHeight = y + _gridItemHeight + _scrollWindowPaddingY + _trayHeight;
	_innerWidth = 2 * _scrollWindowPaddingX + (_itemsPerRow * (_gridItemWidth + _gridXSpacing) - _gridXSpacing);
}

void GridWidget::calcEntrySizes() {
	_gridHeaderHeight = kLineHeight;
	_gridHeaderWidth = _scrollWindowWidth - _scrollBarWidth - 2 * _scrollWindowPaddingX;

	for (uint i = 0; i != _sortedEntryList.size(); ++i) {
		GridItemInfo *entry = _sortedEntryList[i];
		if (entry->isHeader) {
			entry->h = _gridHeaderHeight;
			entry->w = _gridHeaderWidth;
		} else {
			int titleRows;
			if (_isTitlesVisible) {
				Common::Array<Common::U32String> titleLines;
				g_gui.getFont().wordWrapText(entry->title, _gridItemWidth, titleLines);
				titleRows = MIN(2U, titleLines.size());
			} else {
				titleRows = 0;
			}
			entry->h = _thumbnailHeight + titleRows * kLineHeight;
			entry->w = _gridItemWidth;
		}
	}
}

void GridWidget::reflowLayout() {
	Widget::reflowLayout();
	destroyItems();

	// Recompute thumbnail size
	int oldThumbnailHeight = _thumbnailHeight;
	int oldThumbnailWidth = _thumbnailWidth;
	int oldThumbnailMargin = _thumbnailMargin;

	_scrollWindowHeight = _h;
	_scrollWindowWidth = _w;

	_itemsPerRow = ConfMan.getInt("grid_items_per_row");

	_minGridXSpacing = int(g_gui.xmlEval()->getVar("Globals.Grid.XSpacing") * g_gui.getScaleFactor() + .5f);
	_minGridYSpacing = int(g_gui.xmlEval()->getVar("Globals.Grid.YSpacing") * g_gui.getScaleFactor() + .5f);
	_isTitlesVisible = g_gui.xmlEval()->getVar("Globals.Grid.ShowTitles");
	_scrollBarWidth = g_gui.xmlEval()->getVar("Globals.Scrollbar.Width", 0);
	_thumbnailMargin = g_gui.xmlEval()->getVar("Globals.Grid.ThumbnailMargin", 0);

	_scrollWindowPaddingX = _minGridXSpacing;
	_scrollWindowPaddingY = _minGridYSpacing;
	_gridYSpacing = _minGridYSpacing;

	int availableWidth = _scrollWindowWidth - (2 * _scrollWindowPaddingX) - _scrollBarWidth;
	_thumbnailWidth = availableWidth / _itemsPerRow - _minGridXSpacing;
	const int minimumthumbnailWidth = int(36 * g_gui.getScaleFactor() + .5f);
	if (_thumbnailWidth < minimumthumbnailWidth) {
		_thumbnailWidth = minimumthumbnailWidth;
		_itemsPerRow = MAX((availableWidth / (_thumbnailWidth + _minGridXSpacing)), 1);
	}

	_thumbnailHeight = _thumbnailWidth;
	_flagIconWidth = _thumbnailWidth / 4;
	_flagIconHeight = _flagIconWidth / 2;
	_platformIconHeight = _platformIconWidth = _thumbnailWidth / 6;
	_extraIconWidth = _thumbnailWidth;
	_extraIconHeight = _thumbnailHeight;

	if ((oldThumbnailHeight != _thumbnailHeight) ||
		(oldThumbnailWidth != _thumbnailWidth) ||
		(oldThumbnailMargin != _thumbnailMargin)) {
		unloadSurfaces(_extraIcons);
		unloadSurfaces(_platformIcons);
		unloadSurfaces(_languageIcons);
		unloadSurfaces(_loadedSurfaces);
		_platformIconsAlpha.clear();
		_languageIconsAlpha.clear();
		_extraIconsAlpha.clear();
		delete _disabledIconOverlay;
		reloadThumbnails();
		loadFlagIcons();
		loadPlatformIcons();
		loadExtraIcons();

		Graphics::ManagedSurface *gfx = new Graphics::ManagedSurface(_thumbnailWidth, _thumbnailHeight, g_system->getOverlayFormat());
		uint32 disabledThumbnailColor = gfx->format.ARGBToColor(153, 0, 0, 0);  // 60% opacity black
		gfx->fillRect(Common::Rect(0, 0, _thumbnailWidth, _thumbnailHeight), disabledThumbnailColor);
		_disabledIconOverlay = gfx;
	}

	_trayHeight = kLineHeight * 3;
	_gridItemHeight = _thumbnailHeight + (2 * kLineHeight * _isTitlesVisible);
	_gridItemWidth = _thumbnailWidth;

	_gridXSpacing = MAX(((_scrollWindowWidth - _scrollBarWidth - (2 * _scrollWindowPaddingX)) - (_itemsPerRow * _gridItemWidth)) / (_itemsPerRow + 1), _minGridXSpacing);

	calcEntrySizes();
	calcInnerHeight();

	_scrollBar->resize(_scrollWindowWidth - _scrollBarWidth, 0, _scrollBarWidth, _scrollWindowHeight, false);

	if (calcVisibleEntries()) {
		reloadThumbnails();
	}

	assignEntriesToItems();
	if (_selectedEntry) {
		scrollToEntry(_selectedEntry->entryID, false);
	}
	scrollBarRecalc();
	markAsDirty();
}

void GridWidget::openTrayAtSelected() {
	if (_selectedEntry) {
		GridItemTray *tray = new GridItemTray(this, _x + _selectedEntry->x - _gridXSpacing / 3, _y + _selectedEntry->y + _selectedEntry->h - _scrollPos,
								_gridItemWidth + 2 * (_gridXSpacing / 3), _trayHeight, _selectedEntry->entryID, this);
		tray->runModal();
		delete tray;
	}
}

void GridWidget::scrollBarRecalc() {
	_scrollBar->_numEntries = _innerHeight;
	_scrollBar->_entriesPerPage = _scrollWindowHeight - 2 * _scrollWindowPaddingY;
	_scrollBar->_currentPos = _scrollPos;
	_scrollBar->_singleStep = kLineHeight;

	_scrollBar->checkBounds(_scrollBar->_currentPos);
	_scrollPos = _scrollBar->_currentPos;
	_scrollBar->recalc();
}

void GridWidget::setFilter(const Common::U32String &filter) {
	Common::U32String filt = filter;
	filt.toLowercase();

	if (_filter == filt) // Filter was not changed
		return;

	_filter = filt;

	// Reset the scrollbar and deselect everything if filter has changed
	_scrollPos = 0;
	_selectedEntry = nullptr;

	sortGroups();
}

void GridWidget::setSelected(int id) {
	for (uint i = 0; i < _sortedEntryList.size(); ++i) {
		if ((!_sortedEntryList[i]->isHeader) && (_sortedEntryList[i]->entryID == id)) {
			_selectedEntry = _sortedEntryList[i];
			scrollToEntry(id, false);
			break;
		}
	}
}

} // End of namespace GUI
