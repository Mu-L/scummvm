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

#ifndef DIRECTOR_LINGO_XLIBS_INSTOBJ_H
#define DIRECTOR_LINGO_XLIBS_INSTOBJ_H

namespace Director {

class InstObjXObject : public Object<InstObjXObject> {
public:
	InstObjXObject(ObjectType objType);
};

namespace InstObjXObj {

extern const char *const xlibName;
extern const XlibFileDesc fileNames[];

void open(ObjectType type, const Common::Path &path);
void close(ObjectType type);

void m_new(int nargs);
void m_dispose(int nargs);
void m_name(int nargs);
void m_status(int nargs);
void m_error(int nargs);
void m_lastError(int nargs);
void m_getWinDir(int nargs);
void m_getSysDir(int nargs);
void m_getWinVer(int nargs);
void m_getProcInfo(int nargs);
void m_getDriveType(int nargs);
void m_getFreeSpace(int nargs);
void m_makeDir(int nargs);
void m_dirExists(int nargs);
void m_copyFile(int nargs);
void m_deleteFile(int nargs);
void m_fileExists(int nargs);
void m_addPMGroup(int nargs);
void m_addPMItem(int nargs);
void m_readProfile(int nargs);
void m_writeProfile(int nargs);

} // End of namespace InstObjXObj

} // End of namespace Director

#endif
