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

namespace Voyeur {

static const VoyeurGameDescription gameDescriptions[] = {
	{
		// Voyeur DOS English
		{
			"voyeur",
			nullptr,
			AD_ENTRY1s("bvoy.blt", "12e9e10654171501cf8be3a7aa7198e1", 13036269),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO2(GUIO_NOMIDI, GAMEOPTION_COPY_PROTECTION)
		}
	},

	{
		// Voyeur DOS English
		{
			"voyeur",
			"Demo",
			AD_ENTRY1s("bvoy.blt", "abd10f241b845ebc2a76d27a43770709", 13015827),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_DEMO,
			GUIO2(GUIO_NOMIDI, GAMEOPTION_COPY_PROTECTION)
		}
	},

	{// Voyeur German Fan Translation - By Bakhtosh
	 {
		 "voyeur",
		 "German Fan Made Version",
		 AD_ENTRY1s("bvoy.blt", "af281a228ddcf7daa18a3c8b591bace9", 15134654),
		 Common::DE_DEU,
		 Common::kPlatformDOS,
		 ADGF_NO_FLAGS,
		 GUIO2(GUIO_NOMIDI, GAMEOPTION_COPY_PROTECTION)}
	},

	{AD_TABLE_END_MARKER}};

} // End of namespace Voyeur
