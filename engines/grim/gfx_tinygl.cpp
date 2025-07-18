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
#include "common/endian.h"
#include "common/system.h"

#include "graphics/surface.h"

#include "math/glmath.h"

#include "engines/grim/actor.h"
#include "engines/grim/colormap.h"
#include "engines/grim/material.h"
#include "engines/grim/font.h"
#include "engines/grim/gfx_tinygl.h"
#include "engines/grim/grim.h"
#include "engines/grim/bitmap.h"
#include "engines/grim/primitives.h"
#include "engines/grim/model.h"
#include "engines/grim/sprite.h"
#include "engines/grim/set.h"
#include "engines/grim/emi/modelemi.h"

namespace Grim {

GfxBase *CreateGfxTinyGL() {
	return new GfxTinyGL();
}

GfxTinyGL::GfxTinyGL() :
		_alpha(1.f),
		_currentActor(nullptr), _smushImage(nullptr),
		_storedDisplay(nullptr) {
	type = Graphics::RendererType::kRendererTypeTinyGL;
	// TGL_LEQUAL as tglDepthFunc ensures that subsequent drawing attempts for
	// the same triangles are not ignored by the depth test.
	// That's necessary for EMI where some models have multiple faces which
	// refer to the same vertices. The first face is usually using the
	// color map and the following are using textures.
	_depthFunc = (g_grim->getGameType() == GType_MONKEY4) ? TGL_LEQUAL : TGL_LESS;
	for (int i = 0; i < 96; i++) {
		_emergFont[i] = nullptr;
	}
}

GfxTinyGL::~GfxTinyGL() {
	_storedDisplay->free();
	delete _storedDisplay;
	releaseMovieFrame();
	for (unsigned int i = 0; i < _numSpecialtyTextures; i++) {
		destroyTexture(&_specialtyTextures[i]);
	}
	for (int i = 0; i < 96; i++) {
		tglDeleteBlitImage(_emergFont[i]);
	}
	TinyGL::destroyContext();
}

void GfxTinyGL::setupScreen(int screenW, int screenH) {
	_screenWidth = screenW;
	_screenHeight = screenH;
	_scaleW = _screenWidth / (float)_gameWidth;
	_scaleH = _screenHeight / (float)_gameHeight;

	g_system->showMouse(false);

	_pixelFormat = g_system->getScreenFormat();
	debug(2, "INFO: TinyGL front buffer pixel format: %s", _pixelFormat.toString().c_str());
	TinyGL::createContext(screenW, screenH, _pixelFormat, 256, true, ConfMan.getBool("dirtyrects"));

	_storedDisplay = new Graphics::Surface;
	_storedDisplay->create(_gameWidth, _gameHeight, _pixelFormat);

	_currentShadowArray = nullptr;
	tglViewport(0, 0, _screenWidth, _screenHeight);

	TGLfloat ambientSource[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	tglLightModelfv(TGL_LIGHT_MODEL_AMBIENT, ambientSource);
	TGLfloat diffuseReflectance[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	tglMaterialfv(TGL_FRONT, TGL_DIFFUSE, diffuseReflectance);
	
	tglClearStencil(0xff);

	if (g_grim->getGameType() == GType_GRIM) {
		tglPolygonOffset(-6.0, -6.0);
	}
}

const char *GfxTinyGL::getVideoDeviceName() {
	return "Software Renderer";
}

void GfxTinyGL::setupCameraFrustum(float fov, float nclip, float fclip) {
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();

	float right = nclip * tan(fov / 2 * ((float)M_PI / 180));
	tglFrustumf(-right, right, -right * 0.75f, right * 0.75f, nclip, fclip);

	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
}

void GfxTinyGL::positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest, float roll) {
	Math::Vector3d up_vec(0, 0, 1);

	tglRotatef(roll, 0, 0, -1);

	if (pos.x() == interest.x() && pos.y() == interest.y())
		up_vec = Math::Vector3d(0, 1, 0);

	Math::Matrix4 lookMatrix = Math::makeLookAtMatrix(pos, interest, up_vec);
	tglMultMatrixf(lookMatrix.getData());
	tglTranslatef(-pos.x(), -pos.y(), -pos.z());
}

void GfxTinyGL::positionCamera(const Math::Vector3d &pos, const Math::Matrix4 &rot) {
	tglScalef(1.0f, 1.0f, -1.0f);
	_currentPos = pos;
	_currentRot = rot;
}

Math::Matrix4 GfxTinyGL::getModelView() {
	Math::Matrix4 modelView;

	if (g_grim->getGameType() == GType_MONKEY4) {
		tglMatrixMode(TGL_MODELVIEW);
		tglPushMatrix();

		tglMultMatrixf(_currentRot.getData());
		tglTranslatef(-_currentPos.x(), -_currentPos.y(), -_currentPos.z());

		tglGetFloatv(TGL_MODELVIEW_MATRIX, modelView.getData());

		tglPopMatrix();
	} else {
		tglGetFloatv(TGL_MODELVIEW_MATRIX, modelView.getData());
	}

	modelView.transpose();
	return modelView;
}

Math::Matrix4 GfxTinyGL::getProjection() {
	Math::Matrix4 projection;
	tglGetFloatv(TGL_PROJECTION_MATRIX, projection.getData());
	projection.transpose();
	return projection;
}

void GfxTinyGL::clearScreen() {
	tglClear(TGL_COLOR_BUFFER_BIT | TGL_DEPTH_BUFFER_BIT);
}

void GfxTinyGL::clearDepthBuffer() {
	tglClear(TGL_DEPTH_BUFFER_BIT);
}

void GfxTinyGL::flipBuffer(bool opportunistic) {
	if (opportunistic) {
		g_system->updateScreen();
		return;
	}

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

	g_system->updateScreen();
}

bool GfxTinyGL::isHardwareAccelerated() {
	return false;
}

bool GfxTinyGL::supportsShaders() {
	return false;
}

static void shadowProjection(const Math::Vector3d &light, const Math::Vector3d &plane, const Math::Vector3d &normal, bool dontNegate) {
	// Based on GPL shadow projection example by
	// (c) 2002-2003 Phaetos <phaetos@gaffga.de>
	float d, c;
	float mat[16];
	float nx, ny, nz, lx, ly, lz, px, py, pz;

	nx = normal.x();
	ny = normal.y();
	nz = normal.z();
	// for some unknown for me reason normal need negation
	if (!dontNegate) {
		nx = -nx;
		ny = -ny;
		nz = -nz;
	}
	lx = light.x();
	ly = light.y();
	lz = light.z();
	px = plane.x();
	py = plane.y();
	pz = plane.z();

	d = nx * lx + ny * ly + nz * lz;
	c = px * nx + py * ny + pz * nz - d;

	mat[0] = lx * nx + c;
	mat[4] = ny * lx;
	mat[8] = nz * lx;
	mat[12] = -lx * c - lx * d;

	mat[1] = nx * ly;
	mat[5] = ly * ny + c;
	mat[9] = nz * ly;
	mat[13] = -ly * c - ly * d;

	mat[2] = nx * lz;
	mat[6] = ny * lz;
	mat[10] = lz * nz + c;
	mat[14] = -lz * c - lz * d;

	mat[3] = nx;
	mat[7] = ny;
	mat[11] = nz;
	mat[15] = -d;

	tglMultMatrixf(mat);
}

void GfxTinyGL::getScreenBoundingBox(const Mesh *model, int *x1, int *y1, int *x2, int *y2) {
	if (_currentShadowArray) {
		*x1 = -1;
		*y1 = -1;
		*x2 = -1;
		*y2 = -1;
		return;
	}

	TGLfloat top = 1000;
	TGLfloat right = -1000;
	TGLfloat left = 1000;
	TGLfloat bottom = -1000;

	for (int i = 0; i < model->_numFaces; i++) {
		Math::Vector3d obj;
		float *pVertices;

		for (int j = 0; j < model->_faces[i].getNumVertices(); j++) {
			TGLfloat modelView[16], projection[16];
			TGLint viewPort[4];

			tglGetFloatv(TGL_MODELVIEW_MATRIX, modelView);
			tglGetFloatv(TGL_PROJECTION_MATRIX, projection);
			tglGetIntegerv(TGL_VIEWPORT, viewPort);

			pVertices = model->_vertices + 3 * model->_faces[i].getVertex(j);

			obj.set(*(pVertices), *(pVertices + 1), *(pVertices + 2));

			Math::Vector3d win;
			Math::gluMathProject<TGLfloat, TGLint>(obj, modelView, projection, viewPort, win);

			if (win.x() > right)
				right = win.x();
			if (win.x() < left)
				left = win.x();
			if (win.y() < top)
				top = win.y();
			if (win.y() > bottom)
				bottom = win.y();
		}
	}

	float t = bottom;
	bottom = _gameHeight - top;
	top = _gameHeight - t;

	if (left < 0)
		left = 0;
	if (right >= _gameWidth)
		right = _gameWidth - 1;
	if (top < 0)
		top = 0;
	if (bottom >= _gameHeight)
		bottom = _gameHeight - 1;

	if (top >= _gameHeight || left >= _gameWidth || bottom < 0 || right < 0) {
		*x1 = -1;
		*y1 = -1;
		*x2 = -1;
		*y2 = -1;
		return;
	}

	*x1 = (int)left;
	*y1 = (int)top;
	*x2 = (int)right;
	*y2 = (int)bottom;
}

void GfxTinyGL::getScreenBoundingBox(const EMIModel *model, int *x1, int *y1, int *x2, int *y2) {
	if (_currentShadowArray) {
		*x1 = -1;
		*y1 = -1;
		*x2 = -1;
		*y2 = -1;
		return;
	}

	TGLfloat top = 1000;
	TGLfloat right = -1000;
	TGLfloat left = 1000;
	TGLfloat bottom = -1000;

	TGLfloat modelView[16], projection[16];
	TGLint viewPort[4];

	tglGetFloatv(TGL_MODELVIEW_MATRIX, modelView);
	tglGetFloatv(TGL_PROJECTION_MATRIX, projection);
	tglGetIntegerv(TGL_VIEWPORT, viewPort);

	for (uint i = 0; i < model->_numFaces; i++) {
		uint16 *indices = (uint16 *)model->_faces[i]._indexes;

		for (uint j = 0; j < model->_faces[i]._faceLength * 3; j++) {
			uint16 index = indices[j];
			Math::Vector3d obj = model->_drawVertices[index];
			Math::Vector3d win;
			Math::gluMathProject<TGLfloat, TGLint>(obj, modelView, projection, viewPort, win);

			if (win.x() > right)
				right = win.x();
			if (win.x() < left)
				left = win.x();
			if (win.y() < top)
				top = win.y();
			if (win.y() > bottom)
				bottom = win.y();
		}
	}

	float t = bottom;
	bottom = _gameHeight - top;
	top = _gameHeight - t;

	if (left < 0)
		left = 0;
	if (right >= _gameWidth)
		right = _gameWidth - 1;
	if (top < 0)
		top = 0;
	if (bottom >= _gameHeight)
		bottom = _gameHeight - 1;

	if (top >= _gameHeight || left >= _gameWidth || bottom < 0 || right < 0) {
		*x1 = -1;
		*y1 = -1;
		*x2 = -1;
		*y2 = -1;
		return;
	}

	*x1 = (int)left;
	*y1 = (int)(_gameHeight - bottom);
	*x2 = (int)right;
	*y2 = (int)(_gameHeight - top);
}

void GfxTinyGL::getActorScreenBBox(const Actor *actor, Common::Point &p1, Common::Point &p2) {
	// Get the actor's bounding box information (describes a 3D box)
	Math::Vector3d bboxPos, bboxSize;
	actor->getBBoxInfo(bboxPos, bboxSize);

	// Translate the bounding box to the actor's position
	Math::Matrix4 m = actor->getFinalMatrix();
	bboxPos = bboxPos + actor->getWorldPos();

	// Set up the camera coordinate system
	tglMatrixMode(TGL_MODELVIEW);
	tglPushMatrix();

	// Apply the view transform.
	Math::Matrix4 worldRot = _currentRot;
	tglMultMatrixf(worldRot.getData());
	tglTranslatef(-_currentPos.x(), -_currentPos.y(), -_currentPos.z());

	// Get the current OpenGL state
	TGLfloat modelView[16], projection[16];
	TGLint viewPort[4];
	tglGetFloatv(TGL_MODELVIEW_MATRIX, modelView);
	tglGetFloatv(TGL_PROJECTION_MATRIX, projection);
	tglGetIntegerv(TGL_VIEWPORT, viewPort);

	// Set values outside of the screen range
	p1.x = 1000;
	p1.y = 1000;
	p2.x = -1000;
	p2.y = -1000;

	// Project all of the points in the 3D bounding box
	Math::Vector3d p, projected;
	for (int x = 0; x < 2; x++) {
		for (int y = 0; y < 2; y++) {
			for (int z = 0; z < 2; z++) {
				Math::Vector3d added(bboxSize.x() * 0.5f * (x * 2 - 1), bboxSize.y() * 0.5f * (y * 2 - 1), bboxSize.z() * 0.5f * (z * 2 - 1));
				m.transform(&added, false);
				p = bboxPos + added;
				Math::gluMathProject<TGLfloat, TGLint>(p, modelView, projection, viewPort, projected);

				// Find the points
				if (projected.x() < p1.x)
					p1.x = projected.x();
				if (projected.y() < p1.y)
					p1.y = projected.y();
				if (projected.x() > p2.x)
					p2.x = projected.x();
				if (projected.y() > p2.y)
					p2.y = projected.y();
			}
		}
	}

	// Swap the p1/p2 y coorindates
	int16 tmp = p1.y;
	p1.y = 480 - p2.y;
	p2.y = 480 - tmp;

	// Restore the state
	tglPopMatrix();
}

void GfxTinyGL::startActorDraw(const Actor *actor) {
	_currentActor = actor;
	tglEnable(TGL_TEXTURE_2D);
	tglEnable(TGL_LIGHTING);
	tglMatrixMode(TGL_PROJECTION);
	tglPushMatrix();
	tglMatrixMode(TGL_MODELVIEW);
	tglPushMatrix();

	if (g_grim->getGameType() == GType_MONKEY4 && !actor->isInOverworld()) {
		// Apply the view transform.
		tglMultMatrixf(_currentRot.getData());
		tglTranslatef(-_currentPos.x(), -_currentPos.y(), -_currentPos.z());
	}

	if (_currentShadowArray) {
		Sector *shadowSector = _currentShadowArray->planeList.front().sector;
		tglDepthMask(TGL_FALSE);
		tglEnable(TGL_POLYGON_OFFSET_FILL);
		tglDisable(TGL_LIGHTING);
		tglDisable(TGL_TEXTURE_2D);
		if (g_grim->getGameType() == GType_GRIM) {
			tglColor4ub(_shadowColorR, _shadowColorG, _shadowColorB, 255);
		} else {
			tglColor4ub(_currentShadowArray->color.getRed(), _currentShadowArray->color.getGreen(), _currentShadowArray->color.getBlue(), 255);
		}
		//tglColor4f(0.0f, 1.0f, 0.0f, 1.0f); // debug draw color
		shadowProjection(_currentShadowArray->pos, shadowSector->getVertices()[0], shadowSector->getNormal(), _currentShadowArray->dontNegate);
	}

	const float alpha = actor->getEffectiveAlpha();
	if (alpha < 1.f) {
		_alpha = alpha;
		tglEnable(TGL_BLEND);
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	}

	if (g_grim->getGameType() == GType_MONKEY4) {
		tglEnable(TGL_CULL_FACE);
		tglFrontFace(TGL_CW);

		if (actor->isInOverworld()) {
			const Math::Vector3d &pos = actor->getWorldPos();
			const Math::Quaternion &quat = actor->getRotationQuat();
			// At distance 3.2, a 6.4x4.8 actor fills the screen.
			tglMatrixMode(TGL_PROJECTION);
			tglLoadIdentity();
			float right = 1;
			float top = right * 0.75;
			float div = 6.0f;
			tglFrustumf(-right / div, right / div, -top / div, top / div, 1.0f / div, 3276.8f);
			tglMatrixMode(TGL_MODELVIEW);
			tglLoadIdentity();
			tglScalef(1.0, 1.0, -1.0);
			tglTranslatef(pos.x(), pos.y(), pos.z());
			tglMultMatrixf(quat.toMatrix().getData());
		} else {
			Math::Matrix4 m = actor->getFinalMatrix();
			m.transpose();
			tglMultMatrixf(m.getData());
		}
	} else {
		// Grim
		Math::Vector3d pos = actor->getWorldPos();
		const Math::Quaternion &quat = actor->getRotationQuat();
		const float &scale = actor->getScale();

		tglTranslatef(pos.x(), pos.y(), pos.z());
		tglScalef(scale, scale, scale);
		tglMultMatrixf(quat.toMatrix().getData());
	}
}

void GfxTinyGL::finishActorDraw() {
	tglMatrixMode(TGL_MODELVIEW);
	tglPopMatrix();
	tglMatrixMode(TGL_PROJECTION);
	tglPopMatrix();
	tglMatrixMode(TGL_MODELVIEW);

	tglDisable(TGL_TEXTURE_2D);
	if (_alpha < 1.f) {
		tglDisable(TGL_BLEND);
		_alpha = 1.f;
	}

	if (_currentShadowArray) {
		tglEnable(TGL_LIGHTING);
		tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		tglDisable(TGL_POLYGON_OFFSET_FILL);
	}

	if (g_grim->getGameType() == GType_MONKEY4) {
		tglDisable(TGL_CULL_FACE);
	}

	_currentActor = nullptr;
}

void GfxTinyGL::drawShadowPlanes() {
/*	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	_currentShadowArray->planeList.begin();
	for (SectorListType::iterator i = _currentShadowArray->planeList.begin(); i != _currentShadowArray->planeList.end(); i++) {
		Sector *shadowSector = i->sector;
		tglBegin(TGL_POLYGON);
		for (int k = 0; k < shadowSector->getNumVertices(); k++) {
			tglVertex3f(shadowSector->getVertices()[k].x(), shadowSector->getVertices()[k].y(), shadowSector->getVertices()[k].z());
		}
		tglEnd();
	}*/

	tglPushMatrix();

	if (g_grim->getGameType() == GType_MONKEY4) {
		// Apply the view transform.
		tglMultMatrixf(_currentRot.getData());
		tglTranslatef(-_currentPos.x(), -_currentPos.y(), -_currentPos.z());
	}

	tglColorMask(TGL_FALSE, TGL_FALSE, TGL_FALSE, TGL_FALSE);
	tglDepthMask(TGL_FALSE);

	tglClear(TGL_STENCIL_BUFFER_BIT);
	tglEnable(TGL_STENCIL_TEST);
	tglStencilFunc(TGL_ALWAYS, 1, 0xff);
	tglStencilOp(TGL_REPLACE, TGL_REPLACE, TGL_REPLACE);

	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_TEXTURE_2D);
	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	for (SectorListType::iterator i = _currentShadowArray->planeList.begin(); i != _currentShadowArray->planeList.end(); ++i) {
		Sector *shadowSector = i->sector;
		tglBegin(TGL_POLYGON);
		for (int k = 0; k < shadowSector->getNumVertices(); k++) {
			tglVertex3f(shadowSector->getVertices()[k].x(), shadowSector->getVertices()[k].y(), shadowSector->getVertices()[k].z());
		}
		tglEnd();
	}
	tglColorMask(TGL_TRUE, TGL_TRUE, TGL_TRUE, TGL_TRUE);

	tglStencilFunc(TGL_EQUAL, 1, 0xff);
	tglStencilOp(TGL_KEEP, TGL_KEEP, TGL_KEEP);

	tglPopMatrix();
}

void GfxTinyGL::setShadow(Shadow *shadow) {
	_currentShadowArray = shadow;
}

void GfxTinyGL::setShadowMode() {
	GfxBase::setShadowMode();
}

void GfxTinyGL::clearShadowMode() {
	GfxBase::clearShadowMode();

	tglDisable(TGL_STENCIL_TEST);
	tglDepthMask(TGL_TRUE);
}

void GfxTinyGL::setShadowColor(byte r, byte g, byte b) {
	_shadowColorR = r;
	_shadowColorG = g;
	_shadowColorB = b;
}

void GfxTinyGL::getShadowColor(byte *r, byte *g, byte *b) {
	*r = _shadowColorR;
	*g = _shadowColorG;
	*b = _shadowColorB;
}

void GfxTinyGL::set3DMode() {
	tglMatrixMode(TGL_MODELVIEW);
	tglEnable(TGL_DEPTH_TEST);
	tglDepthFunc(_depthFunc);
}

void GfxTinyGL::drawEMIModelFace(const EMIModel *model, const EMIMeshFace *face) {
	uint16 *indices = (uint16 *)face->_indexes;

	tglEnable(TGL_DEPTH_TEST);
	tglDisable(TGL_ALPHA_TEST);
	tglDisable(TGL_LIGHTING);
	if (!_currentShadowArray && face->_hasTexture)
		tglEnable(TGL_TEXTURE_2D);
	else
		tglDisable(TGL_TEXTURE_2D);
	if (face->_flags & EMIMeshFace::kAlphaBlend || face->_flags & EMIMeshFace::kUnknownBlend || _currentActor->hasLocalAlpha() || _alpha < 1.0f)
		tglEnable(TGL_BLEND);

	tglBegin(TGL_TRIANGLES);
	float alpha = _alpha;
	if (model->_meshAlphaMode == Actor::AlphaReplace) {
		alpha *= model->_meshAlpha;
	}
	Math::Vector3d noLighting(1.f, 1.f, 1.f);
	for (uint j = 0; j < face->_faceLength * 3; j++) {
		uint16 index = indices[j];

		if (!_currentShadowArray) {
			if (face->_hasTexture) {
				tglTexCoord2f(model->_texVerts[index].getX(), model->_texVerts[index].getY());
			}
			Math::Vector3d lighting = (face->_flags & EMIMeshFace::kNoLighting) ? noLighting : model->_lighting[index];
			byte r = (byte)(model->_colorMap[index].r * lighting.x());
			byte g = (byte)(model->_colorMap[index].g * lighting.y());
			byte b = (byte)(model->_colorMap[index].b * lighting.z());
			byte a = (int)(alpha * (model->_meshAlphaMode == Actor::AlphaReplace ? model->_colorMap[index].a * _currentActor->getLocalAlpha(index) : 255.f));
			tglColor4ub(r, g, b, a);
		}

		Math::Vector3d normal = model->_normals[index];
		Math::Vector3d vertex = model->_drawVertices[index];

		tglNormal3fv(normal.getData());
		tglVertex3fv(vertex.getData());
	}
	tglEnd();

	if (!_currentShadowArray) {
		tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	tglEnable(TGL_TEXTURE_2D);
	tglEnable(TGL_DEPTH_TEST);
	tglEnable(TGL_ALPHA_TEST);
	tglEnable(TGL_LIGHTING);
	tglDisable(TGL_BLEND);

	if (!_currentShadowArray)
		tglDepthMask(TGL_TRUE);
}

void GfxTinyGL::drawModelFace(const Mesh *mesh, const MeshFace *face) {
	// Support transparency in actor objects, such as the message tube
	// in Manny's Office
	float *vertices = mesh->_vertices;
	float *vertNormals = mesh->_vertNormals;
	float *textureVerts = mesh->_textureVerts;
	tglAlphaFunc(TGL_GREATER, 0.5);
	tglEnable(TGL_ALPHA_TEST);
	tglNormal3fv(const_cast<float *>(face->getNormal().getData()));
	tglBegin(TGL_POLYGON);
	for (int i = 0; i < face->getNumVertices(); i++) {
		tglNormal3fv(vertNormals + 3 * face->getVertex(i));

		if (face->hasTexture())
			tglTexCoord2fv(textureVerts + 2 * face->getTextureVertex(i));

		tglVertex3fv(vertices + 3 * face->getVertex(i));
	}
	tglEnd();
	// Done with transparency-capable objects
	tglDisable(TGL_ALPHA_TEST);
}

void GfxTinyGL::drawSprite(const Sprite *sprite) {
	tglMatrixMode(TGL_TEXTURE);
	tglLoadIdentity();
	tglMatrixMode(TGL_MODELVIEW);
	tglPushMatrix();

	if (g_grim->getGameType() == GType_MONKEY4) {
		TGLfloat modelview[16];
		tglGetFloatv(TGL_MODELVIEW_MATRIX, modelview);
		Math::Matrix4 act;
		act.buildAroundZ(_currentActor->getYaw());
		act.transpose();
		act(3, 0) = modelview[12];
		act(3, 1) = modelview[13];
		act(3, 2) = modelview[14];
		tglLoadMatrixf(act.getData());
		tglTranslatef(sprite->_pos.x(), sprite->_pos.y(), -sprite->_pos.z());
	} else {
		tglTranslatef(sprite->_pos.x(), sprite->_pos.y(), sprite->_pos.z());
		TGLfloat modelview[16];
		tglGetFloatv(TGL_MODELVIEW_MATRIX, modelview);

		// We want screen-aligned sprites so reset the rotation part of the matrix.
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if (i == j) {
					modelview[i * 4 + j] = 1.0f;
				} else {
					modelview[i * 4 + j] = 0.0f;
				}
			}
		}
		tglLoadMatrixf(modelview);
	}

	if (sprite->_flags1 & Sprite::BlendAdditive) {
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE);
	} else {
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	}

	tglDisable(TGL_LIGHTING);

	if (g_grim->getGameType() == GType_GRIM) {
		tglEnable(TGL_ALPHA_TEST);
		tglAlphaFunc(TGL_GEQUAL, 0.5f);
	} else if (sprite->_flags2 & Sprite::AlphaTest) {
		tglEnable(TGL_ALPHA_TEST);
		tglAlphaFunc(TGL_GEQUAL, 0.1f);
	} else {
		tglDisable(TGL_ALPHA_TEST);
	}

	if (sprite->_flags2 & Sprite::DepthTest) {
		tglEnable(TGL_DEPTH_TEST);
	} else {
		tglDisable(TGL_DEPTH_TEST);
	}

	if (g_grim->getGameType() == GType_MONKEY4) {
		tglDepthMask(TGL_TRUE);

		float halfWidth = sprite->_width / 2;
		float halfHeight = sprite->_height / 2;
		float vertexX[] = { -1.0f, 1.0f, 1.0f, -1.0f };
		float vertexY[] = { 1.0f, 1.0f, -1.0f, -1.0f };

		tglBegin(TGL_POLYGON);
		for (int i = 0; i < 4; ++i) {
			float r = sprite->_red[i] / 255.0f;
			float g = sprite->_green[i] / 255.0f;
			float b = sprite->_blue[i] / 255.0f;
			float a = sprite->_alpha[i] * _alpha / 255.0f;

			tglColor4f(r, g, b, a);
			tglTexCoord2f(sprite->_texCoordX[i], sprite->_texCoordY[i]);
			tglVertex3f(vertexX[i] * halfWidth, vertexY[i] * halfHeight, 0.0f);
		}
		tglEnd();
		tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	} else {
		// In Grim, the bottom edge of the sprite is at y=0 and
		// the texture is flipped along the X-axis.
		float halfWidth = sprite->_width / 2;
		float height = sprite->_height;

		tglBegin(TGL_POLYGON);
		tglTexCoord2f(0.0f, 1.0f);
		tglVertex3f(+halfWidth, 0.0f, 0.0f);
		tglTexCoord2f(0.0f, 0.0f);
		tglVertex3f(+halfWidth, +height, 0.0f);
		tglTexCoord2f(1.0f, 0.0f);
		tglVertex3f(-halfWidth, +height, 0.0f);
		tglTexCoord2f(1.0f, 1.0f);
		tglVertex3f(-halfWidth, 0.0f, 0.0f);
		tglEnd();
	}

	tglEnable(TGL_LIGHTING);
	tglDisable(TGL_ALPHA_TEST);
	tglDepthMask(TGL_TRUE);
	tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	tglDisable(TGL_BLEND);
	tglEnable(TGL_DEPTH_TEST);

	tglPopMatrix();
}

void GfxTinyGL::translateViewpointStart() {
	tglMatrixMode(TGL_MODELVIEW);
	tglPushMatrix();
}

void GfxTinyGL::translateViewpoint(const Math::Vector3d &vec) {
	tglTranslatef(vec.x(), vec.y(), vec.z());
}

void GfxTinyGL::rotateViewpoint(const Math::Angle &angle, const Math::Vector3d &axis) {
	tglRotatef(angle.getDegrees(), axis.x(), axis.y(), axis.z());
}

void GfxTinyGL::rotateViewpoint(const Math::Matrix4 &rot) {
	tglMultMatrixf(rot.getData());
}

void GfxTinyGL::translateViewpointFinish() {
	tglMatrixMode(TGL_MODELVIEW);
	tglPopMatrix();
}

void GfxTinyGL::enableLights() {
	tglEnable(TGL_LIGHTING);
}

void GfxTinyGL::disableLights() {
	tglDisable(TGL_LIGHTING);
}

void GfxTinyGL::setupLight(Light *light, int lightId) {
	tglEnable(TGL_LIGHTING);
	float lightColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float lightDir[] = { 0.0f, 0.0f, -1.0f };
	float cutoff = 180.0f;
	float spot_exp = 0.0f;
	float q_attenuation = 1.0f;

	float intensity = light->_scaledintensity;
	lightColor[0] = (float)light->_color.getRed() * intensity;
	lightColor[1] = (float)light->_color.getGreen() * intensity;
	lightColor[2] = (float)light->_color.getBlue() * intensity;

	if (light->_type == Light::Omni) {
		lightPos[0] = light->_pos.x();
		lightPos[1] = light->_pos.y();
		lightPos[2] = light->_pos.z();
	} else if (light->_type ==  Light::Direct) {
		lightPos[0] = -light->_dir.x();
		lightPos[1] = -light->_dir.y();
		lightPos[2] = -light->_dir.z();
		lightPos[3] = 0;
	} else if (light->_type ==  Light::Spot) {
		lightPos[0] = light->_pos.x();
		lightPos[1] = light->_pos.y();
		lightPos[2] = light->_pos.z();
		lightDir[0] = light->_dir.x();
		lightDir[1] = light->_dir.y();
		lightDir[2] = light->_dir.z();
		spot_exp = 2.0f;
		cutoff = light->_penumbraangle;
		q_attenuation = 0.0f;
	}

	tglDisable(TGL_LIGHT0 + lightId);
	tglLightfv(TGL_LIGHT0 + lightId, TGL_DIFFUSE, lightColor);
	tglLightfv(TGL_LIGHT0 + lightId, TGL_POSITION, lightPos);
	tglLightfv(TGL_LIGHT0 + lightId, TGL_SPOT_DIRECTION, lightDir);
	tglLightf(TGL_LIGHT0 + lightId, TGL_SPOT_EXPONENT, spot_exp);
	tglLightf(TGL_LIGHT0 + lightId, TGL_SPOT_CUTOFF, cutoff);
	tglLightf(TGL_LIGHT0 + lightId, TGL_QUADRATIC_ATTENUATION, q_attenuation);
	tglEnable(TGL_LIGHT0 + lightId);
}

void GfxTinyGL::turnOffLight(int lightId) {
	tglDisable(TGL_LIGHT0 + lightId);
}

void GfxTinyGL::createBitmap(BitmapData *bitmap) {
	TinyGL::BlitImage **imgs = new TinyGL::BlitImage*[bitmap->_numImages];
	bitmap->_texIds = (void *)imgs;

	if (bitmap->_format != 1) {
		for (int pic = 0; pic < bitmap->_numImages; pic++) {
			Graphics::Surface buffer;
			buffer.create(bitmap->_width, bitmap->_height, Graphics::PixelFormat::createFormatRGBA32());
			uint32 *buf = (uint32 *)buffer.getPixels();
			const uint16 *bufPtr = (const uint16 *)(bitmap->getImageData(pic).getPixels());
			for (int i = 0; i < (bitmap->_width * bitmap->_height); i++) {
				uint16 val = bufPtr[i];
				// fix the value if it is incorrectly set to the bitmap transparency color
				if (val == 0xf81f) {
					val = 0;
				}
				buf[i] = ((uint32)val) * 0x10000 / 100 / (0x10000 - val) << 14;
			}
			bitmap->_data[pic].free();
			bitmap->_data[pic] = buffer;
			imgs[pic] = tglGenBlitImage();
			tglUploadBlitImage(imgs[pic], bitmap->_data[pic], 0, false, true);
		}
	} else {
		for (int i = 0; i < bitmap->_numImages; ++i) {
			imgs[i] = tglGenBlitImage();
			const Graphics::Surface &imageBuffer = bitmap->getImageData(i);
			tglUploadBlitImage(imgs[i], imageBuffer, imageBuffer.format.ARGBToColor(0, 255, 0, 255), true);
		}
	}
}

void GfxTinyGL::drawBitmap(const Bitmap *bitmap, int x, int y, uint32 layer) {
	// PS2 EMI uses a TGA for it's splash-screen, avoid using the following
	// code for drawing that (as it has no tiles).
	if (g_grim->getGameType() == GType_MONKEY4 && bitmap->_data && bitmap->_data->_texc) {
		tglEnable(TGL_BLEND);
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
		tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		BitmapData *data = bitmap->_data;
		float *texc = data->_texc;

		TinyGL::BlitImage **b = (TinyGL::BlitImage **)bitmap->getTexIds();

		assert(layer < data->_numLayers);
		uint32 offset = data->_layers[layer]._offset;
		for (uint32 i = offset; i < offset + data->_layers[layer]._numImages; ++i) {
			const BitmapData::Vert &v = data->_verts[i];
			uint32 texId = v._texid;
			uint32 ntex = data->_verts[i]._pos * 4;
			uint32 numRects = data->_verts[i]._verts / 4;
			while (numRects-- > 0) {
				// TODO: better way to fix this:
				// adding '+ 1' fixing broken lines at edges of bitmaps
				// example: EMI ship scene
				int dx1 = (((texc[ntex + 0] + 1) * _screenWidth) / 2) + 1;
				int dy1 = (((1 - texc[ntex + 1]) * _screenHeight) / 2) + 1;
				int dx2 = (((texc[ntex + 8] + 1) * _screenWidth) / 2) + 1;
				int dy2 = (((1 - texc[ntex + 9]) * _screenHeight) / 2) + 1;
				int srcX = texc[ntex + 2] * bitmap->getWidth();
				int srcY = texc[ntex + 3] * bitmap->getHeight();

				TinyGL::BlitTransform transform(x + dx1, y + dy1);
				transform.sourceRectangle(srcX, srcY, dx2 - dx1, dy2 - dy1);
				transform.tint(1.0f, 1.0f - _dimLevel, 1.0f - _dimLevel, 1.0f  - _dimLevel);
				tglBlit(b[texId], transform);
				ntex += 16;
			}
		}

		tglDisable(TGL_BLEND);
		return;
	}

	int format = bitmap->getFormat();
	if ((format == 1 && !_renderBitmaps) || (format == 5 && !_renderZBitmaps)) {
		return;
	}

	assert(bitmap->getActiveImage() > 0);
	const int num = bitmap->getActiveImage() - 1;

	TinyGL::BlitImage **b = (TinyGL::BlitImage **)bitmap->getTexIds();

	if (bitmap->getFormat() == 1) {
		if (bitmap->getHasTransparency()) {
			tglEnable(TGL_BLEND);
			tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
		}
		tglBlit(b[num], x, y);
		if (bitmap->getHasTransparency()) {
			tglDisable(TGL_BLEND);
		}
	} else {
		tglBlitZBuffer(b[num], x, y);
	}
}

void GfxTinyGL::destroyBitmap(BitmapData *bitmap) {
	TinyGL::BlitImage **imgs = (TinyGL::BlitImage **)bitmap->_texIds;
	for (int pic = 0; pic < bitmap->_numImages; pic++) {
		tglDeleteBlitImage(imgs[pic]);
	}
	delete[] imgs;
}

void GfxTinyGL::createFont(Font *font) {
}

void GfxTinyGL::destroyFont(Font *font) {
}

struct TextObjectData {
	TinyGL::BlitImage *image;
	int width, height, x, y;
};

void GfxTinyGL::createTextObject(TextObject *text) {
	int numLines = text->getNumLines();
	const Common::String *lines = text->getLines();
	const Font *font = text->getFont();
	const Color &fgColor = text->getFGColor();
	TextObjectData *userData = new TextObjectData[numLines];
	text->setUserData(userData);
	uint32 kKitmapColorkey = _pixelFormat.RGBToColor(0, 255, 0);
	const uint32 blackColor = _pixelFormat.RGBToColor(0, 0, 0);
	const uint32 color = _pixelFormat.RGBToColor(fgColor.getRed(), fgColor.getGreen(), fgColor.getBlue());
	while (color == kKitmapColorkey || blackColor == kKitmapColorkey) {
		kKitmapColorkey += 1;
	}
	for (int j = 0; j < numLines; j++) {
		const Common::String &currentLine = lines[j];
		Graphics::Surface buf;

		font->render(buf, currentLine, _pixelFormat, blackColor, color, kKitmapColorkey);

		userData[j].width = buf.w;
		userData[j].height = buf.h;
		userData[j].image = tglGenBlitImage();
		tglUploadBlitImage(userData[j].image, buf, kKitmapColorkey, true);
		userData[j].x = text->getLineX(j);
		userData[j].y = text->getLineY(j);

		if (g_grim->getGameType() == GType_MONKEY4) {
			userData[j].y -= font->getBaseOffsetY();
			if (userData[j].y < 0)
				userData[j].y = 0;
		}

		buf.free();
	}
}

void GfxTinyGL::drawTextObject(const TextObject *text) {
	const TextObjectData *userData = (const TextObjectData *)text->getUserData();
	if (userData) {
		tglEnable(TGL_BLEND);
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
		int numLines = text->getNumLines();
		for (int i = 0; i < numLines; ++i) {
			tglBlit(userData[i].image, userData[i].x, userData[i].y);
		}
		tglDisable(TGL_BLEND);
	}
}

void GfxTinyGL::destroyTextObject(TextObject *text) {
	const TextObjectData *userData = (const TextObjectData *)text->getUserData();
	if (userData) {
		int numLines = text->getNumLines();
		for (int i = 0; i < numLines; ++i) {
			tglDeleteBlitImage(userData[i].image);
		}
		delete[] userData;
	}
}

void GfxTinyGL::createTexture(Texture *texture, const uint8 *data, const CMap *cmap, bool clamp) {
	texture->_texture = new TGLuint[1];
	tglGenTextures(1, (TGLuint *)texture->_texture);

	TGLuint *textures = (TGLuint *)texture->_texture;
	tglBindTexture(TGL_TEXTURE_2D, textures[0]);

	// TinyGL doesn't have issues with dark lines in EMI intro so doesn't need TGL_CLAMP_TO_EDGE
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_S, TGL_REPEAT);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_T, TGL_REPEAT);

	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MAG_FILTER, TGL_LINEAR);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MIN_FILTER, TGL_LINEAR);

	if (cmap != nullptr) { // EMI doesn't have colour-maps
		uint8 *texdata = new uint8[texture->_width * texture->_height * 4];
		uint8 *texdatapos = texdata;

		for (int y = 0; y < texture->_height; y++) {
			for (int x = 0; x < texture->_width; x++) {
				uint8 col = *data;
				if (col == 0) {
					memset(texdatapos, 0, 4); // transparent
					if (!texture->_hasAlpha) {
						texdatapos[3] = 0xff; // fully opaque
					}
				} else {
					memcpy(texdatapos, cmap->_colors + 3 * (col), 3);
					texdatapos[3] = 0xff; // fully opaque
				}
				texdatapos += 4;
				data++;
			}
		}

		tglTexImage2D(TGL_TEXTURE_2D, 0, TGL_RGBA, texture->_width, texture->_height, 0, TGL_RGBA, TGL_UNSIGNED_BYTE, texdata);
		delete[] texdata;
	} else {
		TGLint format = (texture->_bpp == 4) ? TGL_RGBA : TGL_RGB;

		tglTexImage2D(TGL_TEXTURE_2D, 0, format, texture->_width, texture->_height, 0, format, TGL_UNSIGNED_BYTE, data);
	}
}

void GfxTinyGL::selectTexture(const Texture *texture) {
	TGLuint *textures = (TGLuint *)texture->_texture;
	tglBindTexture(TGL_TEXTURE_2D, textures[0]);

	if (texture->_hasAlpha && g_grim->getGameType() == GType_MONKEY4) {
		tglEnable(TGL_BLEND);
	}

	// Grim has inverted tex-coords, EMI doesn't
	if (g_grim->getGameType() != GType_MONKEY4) {
		tglMatrixMode(TGL_TEXTURE);
		tglLoadIdentity();
		tglScalef(1.0f / texture->_width, 1.0f / texture->_height, 1);
	}
}

void GfxTinyGL::destroyTexture(Texture *texture) {
	TGLuint *textures = (TGLuint *)texture->_texture;
	if (textures) {
		tglDeleteTextures(1, textures);
		delete[] textures;
	}
}

const Graphics::PixelFormat GfxTinyGL::getMovieFormat() const {
	return g_system->getScreenFormat();
}

void GfxTinyGL::prepareMovieFrame(Graphics::Surface *frame) {
	if (_smushImage == nullptr)
		_smushImage = tglGenBlitImage();
	tglUploadBlitImage(_smushImage, *frame, 0, false);
}

void GfxTinyGL::drawMovieFrame(int offsetX, int offsetY) {
	tglBlitFast(_smushImage, offsetX, offsetY);
}

void GfxTinyGL::releaseMovieFrame() {
	tglDeleteBlitImage(_smushImage);
}

void GfxTinyGL::loadEmergFont() {
	Graphics::Surface characterSurface;
	Graphics::PixelFormat textureFormat = Graphics::PixelFormat::createFormatRGBA32();
	characterSurface.create(8, 13, textureFormat);
	uint32 color = textureFormat.ARGBToColor(255, 255, 255, 255);
	uint32 colorTransparent = textureFormat.ARGBToColor(0, 255, 255, 255);
	for (int i = 0; i < 96; i++) {
		_emergFont[i] = tglGenBlitImage();
		const uint8 *ptr = BitmapFont::emerFont[i];
		for (int py = 0; py < 13; py++) {
				int line = ptr[12 - py];
				for (int px = 0; px < 8; px++) {
					int pixel = line & 0x80;
					line <<= 1;
					*(uint32 *)characterSurface.getBasePtr(px, py) = pixel ? color : colorTransparent;
				}
		}
		tglUploadBlitImage(_emergFont[i], characterSurface, 0, false);
	}
	characterSurface.free();
}

void GfxTinyGL::drawEmergString(int x, int y, const char *text, const Color &fgColor) {
	int length = strlen(text);

	for (int l = 0; l < length; l++) {
		int c = text[l];
		assert(c >= 32 && c <= 127);
		TinyGL::BlitTransform transform(x, y);
		transform.tint(1.0f, fgColor.getRed() / 255.0f, fgColor.getGreen() / 255.0f, fgColor.getBlue() / 255.0f);
		tglBlit(_emergFont[c - 32], transform);
		x += 10;
	}
}

Bitmap *GfxTinyGL::getScreenshot(int w, int h, bool useStored) {
	Bitmap *bmp;
	if (useStored) {
		bmp = createScreenshotBitmap(_storedDisplay, w, h, true);
	} else {
		Graphics::Surface *src = TinyGL::copyFromFrameBuffer(_pixelFormat);
		bmp = createScreenshotBitmap(src, w, h, true);
		src->free();
		delete src;
	}
	return bmp;
}

void GfxTinyGL::createSpecialtyTextureFromScreen(uint id, uint8 *data, int x, int y, int width, int height) {
	readPixels(x, y, width, height, data);
	createSpecialtyTexture(id, data, width, height);
}

void GfxTinyGL::storeDisplay() {
	TinyGL::presentBuffer();
	_storedDisplay->free();
	delete _storedDisplay;
	_storedDisplay = TinyGL::copyFromFrameBuffer(_pixelFormat);
}

void GfxTinyGL::copyStoredToDisplay() {
	Bitmap *bitmap = getScreenshot(_gameWidth, _gameHeight, true);
	drawBitmap(bitmap, 0, 0, 0);
	delete bitmap;
}

void GfxTinyGL::dimScreen() {
	dimRegion(0, 0, _gameWidth, _gameHeight, 0.2f);
}

void GfxTinyGL::dimRegion(int x, int y, int w, int h, float level) {
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrthof(0, _gameWidth, _gameHeight, 0, 0, 1);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_FALSE);
	tglEnable(TGL_BLEND);
	tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);

	tglColor4f(0, 0, 0, 1 - level);

	tglBegin(TGL_QUADS);
	tglVertex2f(x, y);
	tglVertex2f(x + w, y);
	tglVertex2f(x + w, y + h);
	tglVertex2f(x, y + h);
	tglEnd();

	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	tglDisable(TGL_BLEND);
	tglDepthMask(TGL_TRUE);
	tglEnable(TGL_DEPTH_TEST);
	tglEnable(TGL_LIGHTING);
}

void GfxTinyGL::irisAroundRegion(int x1, int y1, int x2, int y2) {
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrthof(0, _gameWidth, _gameHeight, 0, 0, 1);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglDisable(TGL_DEPTH_TEST);
	tglDisable(TGL_TEXTURE_2D);
	tglDisable(TGL_BLEND);
	tglDisable(TGL_LIGHTING);
	tglDepthMask(TGL_FALSE);

	tglColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	// Explicitly cast to avoid problems with C++11
	float fx1 = x1;
	float fx2 = x2;
	float fy1 = y1;
	float fy2 = y2;
	float width = _screenWidth;
	float height = _screenHeight;
	float points[20] = {
		0.0f, 0.0f,
		0.0f, fy1,
		width, 0.0f,
		fx2, fy1,
		width, height,
		fx2, fy2,
		0.0f, height,
		fx1, fy2,
		0.0f, fy1,
		fx1, fy1
	};

	tglEnableClientState(TGL_VERTEX_ARRAY);
	tglVertexPointer(2, TGL_FLOAT, 0, points);
	tglDrawArrays(TGL_TRIANGLE_STRIP, 0, 10);
	tglDisableClientState(TGL_VERTEX_ARRAY);

	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	tglEnable(TGL_DEPTH_TEST);
	tglEnable(TGL_LIGHTING);
	tglDepthMask(TGL_TRUE);
}

void GfxTinyGL::drawRectangle(const PrimitiveObject *primitive) {
	float x1 = primitive->getP1().x * _scaleW;
	float y1 = primitive->getP1().y * _scaleH;
	float x2 = primitive->getP2().x * _scaleW;
	float y2 = primitive->getP2().y * _scaleH;
	const Color color(primitive->getColor());

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrthof(0, _screenWidth, _screenHeight, 0, 0, 1);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_FALSE);

	tglColor4ub(color.getRed(), color.getGreen(), color.getBlue(), 255);

	if (primitive->isFilled()) {
		tglBegin(TGL_QUADS);
		tglVertex2f(x1, y1);
		tglVertex2f(x2 + 1, y1);
		tglVertex2f(x2 + 1, y2 + 1);
		tglVertex2f(x1, y2 + 1);
		tglEnd();
	} else {
		//tglLineWidth(_scaleW); // Not implemented in TinyGL
		tglBegin(TGL_LINE_LOOP);
		tglVertex2f(x1, y1);
		tglVertex2f(x2 + 1, y1);
		tglVertex2f(x2 + 1, y2 + 1);
		tglVertex2f(x1, y2 + 1);
		tglEnd();
	}

	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	tglDepthMask(TGL_TRUE);
	tglEnable(TGL_DEPTH_TEST);
	tglEnable(TGL_LIGHTING);
}

void GfxTinyGL::drawLine(const PrimitiveObject *primitive) {
	float x1 = primitive->getP1().x * _scaleW;
	float y1 = primitive->getP1().y * _scaleH;
	float x2 = primitive->getP2().x * _scaleW;
	float y2 = primitive->getP2().y * _scaleH;

	const Color &color = primitive->getColor();

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrthof(0, _screenWidth, _screenHeight, 0, 0, 1);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_FALSE);

	tglColor4ub(color.getRed(), color.getGreen(), color.getBlue(), 255);

	//tglLineWidth(_scaleW); // Not implemented in TinyGL

	tglBegin(TGL_LINES);
	tglVertex2f(x1, y1);
	tglVertex2f(x2, y2);
	tglEnd();

	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	tglDepthMask(TGL_TRUE);
	tglEnable(TGL_DEPTH_TEST);
	tglEnable(TGL_LIGHTING);
}

void GfxTinyGL::drawDimPlane() {
	if (_dimLevel == 0.0f) return;

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrthof(0, 1, 1, 0, 0, 1);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglDisable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_FALSE);

	tglDisable(TGL_LIGHTING);
	tglEnable(TGL_BLEND);
	tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);

	tglColor4f(0.0f, 0.0f, 0.0f, _dimLevel);

	tglBegin(TGL_QUADS);
	tglVertex2f(-1, -1);
	tglVertex2f(1.0, -1);
	tglVertex2f(1.0, 1.0);
	tglVertex2f(-1, 1.0);
	tglEnd();

	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	tglDisable(TGL_BLEND);
	tglDepthMask(TGL_TRUE);
	tglEnable(TGL_DEPTH_TEST);
	tglEnable(TGL_LIGHTING);
}

void GfxTinyGL::drawPolygon(const PrimitiveObject *primitive) {
	float x1 = primitive->getP1().x * _scaleW;
	float y1 = primitive->getP1().y * _scaleH;
	float x2 = primitive->getP2().x * _scaleW;
	float y2 = primitive->getP2().y * _scaleH;
	float x3 = primitive->getP3().x * _scaleW;
	float y3 = primitive->getP3().y * _scaleH;
	float x4 = primitive->getP4().x * _scaleW;
	float y4 = primitive->getP4().y * _scaleH;

	const Color &color = primitive->getColor();

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrthof(0, _screenWidth, _screenHeight, 0, 0, 1);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_FALSE);

	tglColor4ub(color.getRed(), color.getGreen(), color.getBlue(), 255);

	tglBegin(TGL_LINES);
	tglVertex2f(x1, y1);
	tglVertex2f(x2 + 1, y2 + 1);
	tglVertex2f(x3, y3 + 1);
	tglVertex2f(x4 + 1, y4);
	tglEnd();

	tglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	tglDepthMask(TGL_TRUE);
	tglEnable(TGL_DEPTH_TEST);
	tglEnable(TGL_LIGHTING);
}

void GfxTinyGL::readPixels(int x, int y, int width, int height, uint8 *buffer) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < _screenWidth);
	assert(y < _screenHeight);

	Graphics::Surface glBuffer;
	TinyGL::getSurfaceRef(glBuffer);
	uint8 r, g, b;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			if ((j + x) >= _screenWidth || (i + y) >= _screenHeight) {
				buffer[0] = buffer[1] = buffer[2] = 0;
			} else {
				uint32 pixel = glBuffer.getPixel(j, i);
				glBuffer.format.colorToRGB(pixel, r, g, b);
				buffer[0] = r;
				buffer[1] = g;
				buffer[2] = b;
			}
			buffer[3] = 255;
			buffer += 4;
		}
	}
}

void GfxTinyGL::setBlendMode(bool additive) {
	if (additive) {
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE);
	} else {
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	}
}

} // end of namespace Grim
