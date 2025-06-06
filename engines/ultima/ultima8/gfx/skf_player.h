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

#ifndef ULTIMA8_GFX_SKFPLAYER_H
#define ULTIMA8_GFX_SKFPLAYER_H

#include "ultima/shared/std/containers.h"
#include "ultima/ultima8/gfx/movie_player.h"
#include "ultima/ultima8/gfx/render_surface.h"
#include "ultima/ultima8/gfx/palette.h"

namespace Ultima {
namespace Ultima8 {

struct SKFEvent;
class RawArchive;
class RenderedText;
class Palette;

class SKFPlayer : public MoviePlayer  {
public:
	SKFPlayer(Common::SeekableReadStream *rs, int width, int height, bool introMusicHack = false);
	~SKFPlayer();

	void run();
	void paint(RenderSurface *surf, int lerp);

	void start();
	void stop();
	bool isPlaying() const {
		return _playing;
	}

private:

	void parseEventList(Common::ReadStream *rs);

	int _width, _height;
	RawArchive *_skf;
	Std::vector<SKFEvent *> _events;
	unsigned int _curFrame, _curObject;
	unsigned int _curAction;
	unsigned int _curEvent;
	bool _playing;
	unsigned int _lastUpdate;
	unsigned int _timer;
	unsigned int _frameRate;
	uint8 _fadeColour, _fadeLevel;
	RenderSurface *_buffer;
	Palette _palette;
	RenderedText *_subs;
	int _subtitleY;
	bool _introMusicHack;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
