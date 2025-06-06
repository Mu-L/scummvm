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

#include "common/config-manager.h"
#include "common/rect.h"
#include "common/textconsole.h"

#include "graphics/surface.h"
#include "graphics/tinygl/tinygl.h"

#include "math/vector2d.h"
#include "math/glmath.h"

#include "engines/myst3/gfx.h"
#include "engines/myst3/gfx_tinygl.h"
#include "engines/myst3/gfx_tinygl_texture.h"

namespace Myst3 {

Renderer *CreateGfxTinyGL(OSystem *system) {
	return new TinyGLRenderer(system);
}

TinyGLRenderer::TinyGLRenderer(OSystem *system) :
		Renderer(system) {
}

TinyGLRenderer::~TinyGLRenderer() {
	TinyGL::destroyContext();
}

Texture *TinyGLRenderer::createTexture2D(const Graphics::Surface *surface) {
	return new TinyGLTexture2D(surface);
}

Texture *TinyGLRenderer::createTexture3D(const Graphics::Surface *surface) {
	return new TinyGLTexture3D(surface);
}

void TinyGLRenderer::init() {
	debug("Initializing Software 3D Renderer");

	computeScreenViewport();

	TinyGL::createContext(kOriginalWidth, kOriginalHeight, g_system->getScreenFormat(), 512, false, ConfMan.getBool("dirtyrects"));

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();

	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglDisable(TGL_LIGHTING);
	tglEnable(TGL_TEXTURE_2D);
	tglEnable(TGL_DEPTH_TEST);
}

void TinyGLRenderer::clear() {
	tglClearColor(0.f, 0.f, 0.f, 1.f); // Solid black
	tglClear(TGL_COLOR_BUFFER_BIT | TGL_DEPTH_BUFFER_BIT);
	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void TinyGLRenderer::selectTargetWindow(Window *window, bool is3D, bool scaled) {
	if (!window) {
		// No window found ...
		if (scaled) {
			// ... in scaled mode draw in the original game screen area
			_viewport = viewport();
		} else {
			// ... otherwise, draw on the whole screen
			_viewport = Common::Rect(_system->getWidth(), _system->getHeight());
		}
	} else {
		// Found a window, draw inside it
		_viewport = window->getPosition();
	}
	tglViewport(_viewport.left, _system->getHeight() - _viewport.top - _viewport.height(), _viewport.width(), _viewport.height());

	if (is3D) {
		tglMatrixMode(TGL_PROJECTION);
		tglLoadMatrixf(_projectionMatrix.getData());

		tglMatrixMode(TGL_MODELVIEW);
		tglLoadMatrixf(_modelViewMatrix.getData());
	} else {
		tglMatrixMode(TGL_PROJECTION);
		tglLoadIdentity();

		if (!window) {
			if (scaled) {
				tglOrthof(0, kOriginalWidth, kOriginalHeight, 0, -1, 1);
			} else {
				tglOrthof(0, _system->getWidth(), _system->getHeight(), 0, -1, 1);
			}
		} else {
			if (scaled) {
				Common::Rect originalRect = window->getOriginalPosition();
				tglOrthof(0, originalRect.width(), originalRect.height(), 0, -1, 1);
			} else {
				Common::Rect vp = window->getPosition();
				tglOrthof(0, vp.width(), vp.height(), 0, -1, 1);
			}
		}

		tglMatrixMode(TGL_MODELVIEW);
		tglLoadIdentity();
	}
}

void TinyGLRenderer::drawRect2D(const Common::Rect &rect, uint8 a, uint8 r, uint8 g, uint8 b) {
	tglDisable(TGL_TEXTURE_2D);
	tglColor4ub(r, g, b, a);

	if (a != 255) {
		tglEnable(TGL_BLEND);
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	}

	tglBegin(TGL_TRIANGLE_STRIP);
		tglVertex3f(rect.left, rect.bottom, 0.0f);
		tglVertex3f(rect.right, rect.bottom, 0.0f);
		tglVertex3f(rect.left, rect.top, 0.0f);
		tglVertex3f(rect.right, rect.top, 0.0f);
	tglEnd();

	tglDisable(TGL_BLEND);
}

void TinyGLRenderer::drawTexturedRect2D(const Common::Rect &screenRect, const Common::Rect &textureRect,
	                                Texture *texture, float transparency, bool additiveBlending) {
	TinyGLTexture2D *glTexture = static_cast<TinyGLTexture2D *>(texture);

	const float sLeft = screenRect.left;
	const float sTop = screenRect.top;
	const float sWidth = screenRect.width();
	const float sHeight = screenRect.height();

	if (transparency >= 0.0) {
		if (additiveBlending) {
			tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE);
		} else {
			tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
		}
		tglEnable(TGL_BLEND);
	} else {
		transparency = 1.0;
	}

	tglEnable(TGL_TEXTURE_2D);
	tglDepthMask(TGL_FALSE);

	// HACK: tglBlit is not affected by the viewport, so we offset the draw coordinates here
	TinyGL::BlitTransform transform(sLeft + _viewport.left, sTop + _viewport.top);
	transform.sourceRectangle(textureRect.left, textureRect.top, sWidth, sHeight);
	transform.tint(transparency);
	tglBlit(glTexture->getBlitTexture(), transform);

	tglDisable(TGL_BLEND);
	tglDepthMask(TGL_TRUE);
}

void TinyGLRenderer::draw2DText(const Common::String &text, const Common::Point &position) {
	TinyGLTexture2D *glFont = static_cast<TinyGLTexture2D *>(_font);

	// The font only has uppercase letters
	Common::String textToDraw = text;
	textToDraw.toUppercase();

	tglEnable(TGL_BLEND);
	tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);

	tglEnable(TGL_TEXTURE_2D);
	tglDepthMask(TGL_FALSE);

	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	int x = position.x;
	int y = position.y;

	for (uint i = 0; i < textToDraw.size(); i++) {
		Common::Rect textureRect = getFontCharacterRect(textToDraw[i]);
		int w = textureRect.width();
		int h = textureRect.height();

		TinyGL::BlitTransform transform(x, y);
		transform.sourceRectangle(textureRect.left, textureRect.top, w, h);
		transform.flip(true, false);
		tglBlit(glFont->getBlitTexture(), transform);

		x += textureRect.width() - 3;
	}

	tglDisable(TGL_TEXTURE_2D);
	tglDisable(TGL_BLEND);
	tglDepthMask(TGL_TRUE);
}

void TinyGLRenderer::drawFace(uint face, Texture *texture) {
	TinyGLTexture3D *glTexture = static_cast<TinyGLTexture3D *>(texture);

	tglBindTexture(TGL_TEXTURE_2D, glTexture->id);
	tglBegin(TGL_TRIANGLE_STRIP);
	for (uint i = 0; i < 4; i++) {
		tglTexCoord2f(cubeVertices[5 * (4 * face + i) + 0], cubeVertices[5 * (4 * face + i) + 1]);
		tglVertex3f(cubeVertices[5 * (4 * face + i) + 2], cubeVertices[5 * (4 * face + i) + 3], cubeVertices[5 * (4 * face + i) + 4]);
	}
	tglEnd();
}

void TinyGLRenderer::drawCube(Texture **textures) {
	tglEnable(TGL_TEXTURE_2D);
	tglDepthMask(TGL_FALSE);

	for (uint i = 0; i < 6; i++) {
		drawFace(i, textures[i]);
	}

	tglDepthMask(TGL_TRUE);
}

void TinyGLRenderer::drawTexturedRect3D(const Math::Vector3d &topLeft, const Math::Vector3d &bottomLeft,
	                                const Math::Vector3d &topRight, const Math::Vector3d &bottomRight, Texture *texture) {
	TinyGLTexture3D *glTexture = static_cast<TinyGLTexture3D *>(texture);

	tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	tglEnable(TGL_BLEND);
	tglDepthMask(TGL_FALSE);

	tglBindTexture(TGL_TEXTURE_2D, glTexture->id);

	tglBegin(TGL_TRIANGLE_STRIP);
		tglTexCoord2f(0, 0);
		tglVertex3f(-topLeft.x(), topLeft.y(), topLeft.z());

		tglTexCoord2f(0, 1);
		tglVertex3f(-bottomLeft.x(), bottomLeft.y(), bottomLeft.z());

		tglTexCoord2f(1, 0);
		tglVertex3f(-topRight.x(), topRight.y(), topRight.z());

		tglTexCoord2f(1, 1);
		tglVertex3f(-bottomRight.x(), bottomRight.y(), bottomRight.z());
	tglEnd();

	tglDisable(TGL_BLEND);
	tglDepthMask(TGL_TRUE);
}

Graphics::Surface *TinyGLRenderer::getScreenshot() {
	return TinyGL::copyFromFrameBuffer(Texture::getRGBAPixelFormat());
}

void TinyGLRenderer::flipBuffer() {
	Common::List<Common::Rect> dirtyAreas;
	TinyGL::presentBuffer(dirtyAreas);

	Graphics::Surface glBuffer;
	TinyGL::getSurfaceRef(glBuffer);

	if (!dirtyAreas.empty()) {
		for (Common::List<Common::Rect>::iterator itRect = dirtyAreas.begin(); itRect != dirtyAreas.end(); ++itRect) {
			g_system->copyRectToScreen(glBuffer.getBasePtr((*itRect).left, (*itRect).top), glBuffer.pitch,
			                           (*itRect).left, (*itRect).top, (*itRect).width(), (*itRect).height());
		}
	}
}

} // End of namespace Myst3
