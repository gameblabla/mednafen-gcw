/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* cart.h:
**  Copyright (C) 2015-2016 Mednafen Team
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

#ifndef __MDFN_SNES_FAUST_CART_H
#define __MDFN_SNES_FAUST_CART_H

namespace MDFN_IEN_SNES_FAUST
{
 void CART_Init(Stream* fp, uint8 id[16]);
 void CART_Kill(void);
 void CART_Reset(bool powering_up);
 void CART_StateAction(StateMem* sm, const unsigned load, const bool data_only);

 bool CART_LoadNV(void);
 void CART_SaveNV(void);
}

#endif
