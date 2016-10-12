/******************************************************************************/
/* Mednafen Sega Saturn Emulation Module                                      */
/******************************************************************************/
/* db.cpp:
**  Copyright (C) 2016 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <mednafen/mednafen.h>
#include <mednafen/FileStream.h>

#include "ss.h"
#include "smpc.h"
#include "cart.h"
#include "db.h"

namespace MDFN_IEN_SS
{

bool DB_LookupRegionDB(const uint8* fd_id, unsigned* const region)
{
 static const struct
 {
  uint8 id[16];
  unsigned area;
 } regiondb[] =
 {
  { { 0xc6, 0x06, 0xa8, 0xa8, 0x13, 0xa1, 0x4e, 0xa9, 0x6b, 0x8a, 0x07, 0xd6, 0x26, 0x61, 0x0e, 0x76, }, SMPC_AREA_KR }, // Fighting Vipers (Korea)
  { { 0xa8, 0x28, 0x1d, 0xb2, 0x27, 0x30, 0xcf, 0xaf, 0x08, 0x09, 0x1b, 0x3b, 0xc4, 0xbe, 0x85, 0x52, }, SMPC_AREA_KR }, // Myst (Korea)
  //{ { 0x50, 0x65, 0x88, 0xeb, 0x98, 0x0f, 0xd8, 0x10, 0x53, 0x34, 0x6c, 0x3c, 0x92, 0x9f, 0x19, 0x11, }, SMPC_AREA_KR }, // NiGHTS into Dreams... (Korea)
  { { 0x10, 0x8f, 0xe1, 0xaf, 0x55, 0x5a, 0x95, 0x42, 0x04, 0x85, 0x7e, 0x98, 0x8c, 0x53, 0x6a, 0x31, }, SMPC_AREA_EU_PAL }, // Preview Sega Saturn Vol. 1 (Europe)
  { { 0xed, 0x4c, 0x0b, 0x87, 0x35, 0x37, 0x86, 0x76, 0xa0, 0xf6, 0x32, 0xc6, 0xa4, 0xc3, 0x99, 0x88, }, SMPC_AREA_EU_PAL }, // Primal Rage (Europe) (En,Fr,De,Es,It,Pt)
  { { 0x15, 0xfc, 0x3a, 0x82, 0x16, 0xa9, 0x85, 0xa5, 0xa8, 0xad, 0x30, 0xaf, 0x9a, 0xff, 0x03, 0xa9, }, SMPC_AREA_JP }, // Race Drivin' (Japan) (2M)
  { { 0xe1, 0xdd, 0xfd, 0xa1, 0x8b, 0x47, 0x02, 0x21, 0x36, 0x1e, 0x5a, 0xae, 0x20, 0xc0, 0x59, 0x9f, }, SMPC_AREA_CSA_NTSC }, // Riven - A Sequencia de Myst (Brazil) (Disc 1)
  { { 0xbf, 0x5f, 0xf8, 0x5f, 0xf2, 0x0c, 0x35, 0xf6, 0xc9, 0x8d, 0x03, 0xbc, 0x34, 0xd9, 0xda, 0x7f, }, SMPC_AREA_CSA_NTSC }, // Riven - A Sequencia de Myst (Brazil) (Disc 2)
  { { 0x98, 0xb6, 0x6e, 0x09, 0xe6, 0xdc, 0x30, 0xe6, 0x55, 0xdb, 0x85, 0x01, 0x33, 0x0c, 0x0b, 0x9c, }, SMPC_AREA_CSA_NTSC }, // Riven - A Sequencia de Myst (Brazil) (Disc 3)
  { { 0xa2, 0x34, 0xb0, 0xb9, 0xaa, 0x47, 0x74, 0x1f, 0xd4, 0x1e, 0x35, 0xda, 0x3d, 0xe7, 0x4d, 0xe3, }, SMPC_AREA_CSA_NTSC }, // Riven - A Sequencia de Myst (Brazil) (Disc 4)
  { { 0xf7, 0xe9, 0x23, 0x0a, 0x9e, 0x92, 0xf1, 0x93, 0x16, 0x43, 0xf8, 0x6c, 0xe8, 0x21, 0x50, 0x66, }, SMPC_AREA_JP }, // Sega International Victory Goal (Japan) (5M)
  { { 0xf7, 0x00, 0xb9, 0x37, 0xef, 0x01, 0xb1, 0x8d, 0x99, 0xc2, 0xc2, 0x60, 0xe7, 0x55, 0x01, 0xd9, }, SMPC_AREA_KR }, // Sega Rally Championship (Korea)
  { { 0x69, 0xf1, 0x61, 0x9d, 0x39, 0xac, 0x9e, 0xbf, 0x52, 0xb5, 0x37, 0x59, 0x0c, 0x77, 0x33, 0xa0, }, SMPC_AREA_KR }, // Virtua Fighter Kids (Korea)
  { { 0x64, 0x75, 0x25, 0x0c, 0xa1, 0x9b, 0x6c, 0x5e, 0x4e, 0xa0, 0x6d, 0x69, 0xd9, 0x0f, 0x32, 0xca, }, SMPC_AREA_EU_PAL }, // Virtua Racing (Europe)
  { { 0x0d, 0xe3, 0xfa, 0xfb, 0x2b, 0xb9, 0x6d, 0x79, 0xe0, 0x3a, 0xb7, 0x6d, 0xcc, 0xbf, 0xb0, 0x2c, }, SMPC_AREA_JP }, // Virtua Racing (Japan)
  { { 0x6b, 0x29, 0x33, 0xfc, 0xdd, 0xad, 0x8e, 0x0d, 0x95, 0x81, 0xa6, 0xee, 0xfd, 0x90, 0x4b, 0x43, }, SMPC_AREA_EU_PAL }, // Winter Heat (Europe) (Demo)
  //{ { 0x19, 0x84, 0x06, 0xa4, 0x92, 0xe5, 0xec, 0xff, 0x1b, 0x53, 0x00, 0x77, 0x0f, 0x6c, 0xde, 0x3f, }, SMPC_AREA_KR }, // Worldwide Soccer - Sega International Victory Goal Edition (Korea)
  { { 0x73, 0x91, 0x4b, 0xe1, 0xad, 0x4d, 0xaf, 0x69, 0xc3, 0xeb, 0xb8, 0x43, 0xee, 0x3e, 0xb5, 0x09, }, SMPC_AREA_EU_PAL }, // WWF WrestleMania - The Arcade Game (Europe) (Demo)
 };

 for(auto& re : regiondb)
 {
  if(!memcmp(re.id, fd_id, 16))
  {
   *region = re.area;
   return true;
  }
 }

 return false;
}

bool DB_LookupCartDB(const char* sgid, int* const cart_type)
{
// printf("  { \"%s\", CART_EXTRAM_1M },\n", sgid);
// printf("  { \"%s\", CART_EXTRAM_4M },\n", sgid);
// printf("  { \"%s\", CART_NONE },\n", sgid);

 static const struct
 {
  const char* sgid;
  int cart_type;
 } cartdb[] =
 {
#if 0
  { "T-19708G", CART_NONE },	// Pia Carrot e Youkoso
  { "T-32901G", CART_NONE },	// Silhouette Mirage
  { "MK-81086", CART_NONE },	// Tomb Raider (Europe)
  { "T-7910H", CART_NONE },	// Tomb Raider (USA)
  { "T-6010G", CART_NONE },	// Tomb Raiders
#endif
  //
  //
  //
  // Modem TODO:
  { "MK-81218", CART_NONE },	// Daytona USA CCE Net Link Edition
  { "MK-81071", CART_NONE },	// Duke Nukem 3D
  { "T-319-01H", CART_NONE },	// PlanetWeb Browser
  { "T-14302G", CART_NONE },	// Saturn Bomberman (Japan)
  { "MK-81070", CART_NONE },	// Saturn Bomberman

  //
  //
  //
  { "MK-81088", CART_KOF95 },	// The King of Fighters '95 (Europe)
  { "T-3101G", CART_KOF95 },	// The King of Fighters '95
  { "T-13308G", CART_ULTRAMAN },// Ultraman: Hikari no Kyojin Densetsu
  //
  //
  //
  { "T-1521G", CART_EXTRAM_1M },	// Astra Superstars
  { "T-9904G", CART_EXTRAM_1M },	// Cotton 2
  { "T-1217G", CART_EXTRAM_1M },	// Cyberbots
  { "GS-9107", CART_EXTRAM_1M },	// Fighter's History Dynamite
  { "T-20109G", CART_EXTRAM_1M },	// Friends
  { "T-14411G", CART_EXTRAM_1M },	// Groove on Fight
  { "T-7032H-50", CART_EXTRAM_1M },	// Marvel Super Heroes (Europe)
  { "T-1215G", CART_EXTRAM_1M },	// Marvel Super Heroes (Japan)
  { "T-3111G", CART_EXTRAM_1M },	// Metal Slug
  { "T-22205G", CART_EXTRAM_1M },	// Noel 3
  { "T-22206G", CART_EXTRAM_1M },	// Noel 3 (TODO: Test)
  { "T-20114G", CART_EXTRAM_1M },	// Pia Carrot e Youkoso!! 2 (TODO: Test)
  { "T-20121M", CART_EXTRAM_1M },	// Pia Carrot e Youkoso!! 2 (TODO: Test)
  { "T-3105G", CART_EXTRAM_1M },	// Real Bout Garou Densetsu
  { "T-3119G", CART_EXTRAM_1M },	// Real Bout Garou Densetsu Special
  { "T-3116G", CART_EXTRAM_1M },	// Samurai Spirits - Amakusa Kourin
  { "T-3104G", CART_EXTRAM_1M },	// Samurai Spirits - Zankurou Musouken
  { "T-16509G", CART_EXTRAM_1M },	// Super Real Mahjong P7 (TODO: Test)
  { "T-16510G", CART_EXTRAM_1M },	// Super Real Mahjong P7 (TODO: Test)
  { "T-3108G", CART_EXTRAM_1M },	// The King of Fighters '96
  { "T-3121G", CART_EXTRAM_1M },	// The King of Fighters '97
  { "T-1515G", CART_EXTRAM_1M },	// Waku Waku 7
  //
  //
  //
  { "T-1245G", CART_EXTRAM_4M },	// D&D Collection
  { "T-1248G", CART_EXTRAM_4M },	// Final Fight Revenge
  { "T-1238G", CART_EXTRAM_4M },	// Marvel Super Heroes vs. Street Fighter
  { "T-1230G", CART_EXTRAM_4M },	// Pocket Fighter
  { "T-1246G", CART_EXTRAM_4M },	// Street Fighter Zero 3
  { "T-1229G", CART_EXTRAM_4M },	// Vampire Savior
  { "T-1226G", CART_EXTRAM_4M },	// X-Men vs. Street Fighter
 };

 for(auto& ca : cartdb)
 {
  if(!strcmp(ca.sgid, sgid))
  {
   *cart_type = ca.cart_type;
   return true;
  }
 }
 return false;
}

}
