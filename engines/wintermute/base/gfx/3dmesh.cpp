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

#include "common/file.h"

#include "engines/wintermute/base/gfx/3dloader_3ds.h"
#include "engines/wintermute/base/gfx/3dmesh.h"

namespace Wintermute {

Mesh3DS::Mesh3DS() : _vertexData(nullptr), _vertexCount(0), _indexData(nullptr), _indexCount(0) {
}

Mesh3DS::~Mesh3DS() {
	delete[] _vertexData;
	delete[] _indexData;
}

bool Mesh3DS::loadFrom3DS(Common::MemoryReadStream &fileStream) {
	uint32 wholeChunkSize = fileStream.readUint32LE();
	int32 end = fileStream.pos() + wholeChunkSize - 6;

	while (fileStream.pos() < end) {
		uint16 chunkId = fileStream.readUint16LE();
		uint32 chunkSize = fileStream.readUint32LE();

		switch (chunkId) {
		case VERTICES:
			_vertexCount = fileStream.readUint16LE();
			_vertexData = new GeometryVertex[_vertexCount]();

			for (int i = 0; i < _vertexCount; ++i) {
				_vertexData[i].x = fileStream.readFloatLE();
				_vertexData[i].z = fileStream.readFloatLE();
				_vertexData[i].y = fileStream.readFloatLE();
			}
			break;

		case FACES: {
			uint16 faceCount = fileStream.readUint16LE();
			_indexCount = 3 * faceCount;
			_indexData = new uint16[_indexCount];

			for (int i = 0; i < faceCount; ++i) {
				_indexData[i * 3 + 0] = fileStream.readUint16LE();
				_indexData[i * 3 + 2] = fileStream.readUint16LE();
				_indexData[i * 3 + 1] = fileStream.readUint16LE();
				fileStream.readUint16LE(); // not used
			}
			break;
		}
		case FACES_MATERIAL:
		case MAPPING_COORDS:
		case LOCAL_COORDS:
		case SMOOTHING_GROUPS:
		default:
			fileStream.seek(chunkSize - 6, SEEK_CUR);
			break;
		}
	}

	return true;
}

void Mesh3DS::computeNormals() {
	DXVector3 *normals = new DXVector3[_vertexCount];
	for (int i = 0; i < _vertexCount; ++i) {
		normals[i]._x = 0.0f;
		normals[i]._y = 0.0f;
		normals[i]._z = 0.0f;
	}

	for (int i = 0; i < faceCount(); ++i) {
		uint16 a = _indexData[3 * i + 0];
		uint16 b = _indexData[3 * i + 1];
		uint16 c = _indexData[3 * i + 2];

		DXVector3 v1(getVertexPosition(a));
		DXVector3 v2(getVertexPosition(b));
		DXVector3 v3(getVertexPosition(c));

		DXVector3 edge1 = v2 - v1;
		DXVector3 edge2 = v3 - v2;
		DXVector3 normal;
		DXVec3Cross(&normal, &edge1, &edge2);
		DXVec3Normalize(&normal, &normal);

		normals[a] += normal;
		normals[b] += normal;
		normals[c] += normal;
	}

	// Assign the newly computed normals back to the vertices
	for (int i = 0; i < faceCount(); ++i) {
		for (int j = 0; j < 3; j++) {
			DXVector3 normal;
			DXVec3Normalize(&normal, &normals[_indexData[3 * i + j]]);
			//_vertexData[_indexData[3 * i + j]].nx = normal._x;
			//_vertexData[_indexData[3 * i + j]].ny = normal._y;
			//_vertexData[_indexData[3 * i + j]].nz = normal._z;
		}
	}

	delete[] normals;
}

void Mesh3DS::dumpVertexCoordinates(const char *filename) {
	Common::DumpFile dump;
	dump.open(filename);

	for (uint16 *index = _indexData; index < _indexData + _indexCount; ++index) {
		float x = _vertexData[*index].x;
		float y = _vertexData[*index].y;
		float z = _vertexData[*index].z;

		dump.writeString(Common::String::format("%u ", *index));
		dump.writeString(Common::String::format("%g ", x));
		dump.writeString(Common::String::format("%g ", y));
		dump.writeString(Common::String::format("%g\n", z));
	}
}

int Mesh3DS::faceCount() {
	// .3ds files have only triangles anyways
	return _indexCount / 3;
}

uint16 *Mesh3DS::getFace(int index) {
	return _indexData + 3 * index;
}

float *Mesh3DS::getVertexPosition(int index) {
	return reinterpret_cast<float *>(&((_vertexData + index)->x));
}

int Wintermute::Mesh3DS::vertexCount() {
	return _vertexCount;
}

} // namespace Wintermute
