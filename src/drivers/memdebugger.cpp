/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* memdebugger.cpp:
**  Copyright (C) 2007-2016 Mednafen Team
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

#include "main.h"
#include "memdebugger.h"
#include "debugger.h"
#include "prompt.h"
#include <mednafen/FileStream.h>

#include <ctype.h>
#include <trio/trio.h>
#include <errno.h>

bool MemDebugger::ICV_Init(const char *newcode)
{
 if((size_t)ict != (size_t)-1)
 {
  iconv_close(ict);
  ict = (iconv_t)-1;
 }

 if((size_t)ict_to_utf8 != (size_t)-1)
 {
  iconv_close(ict_to_utf8);
  ict_to_utf8 = (iconv_t)-1;
 }

 if((size_t)ict_utf16_to_game != (size_t)-1)
 {
  iconv_close(ict_utf16_to_game);
  ict_utf16_to_game = (iconv_t)-1;
 }

 ict = iconv_open(newcode, "UTF-8");
 if((size_t)ict == (size_t)-1)
 {
  error_string = trio_aprintf("iconv_open() error: %m");
  error_time = SDL_GetTicks();
  return(0);
 }

 ict_to_utf8 = iconv_open("UTF-8", newcode);
 if((size_t)ict_to_utf8 == (size_t)-1)
 {
  error_string = trio_aprintf("iconv_open() error: %m");
  error_time = SDL_GetTicks();
  return(0);
 }

 ict_utf16_to_game = iconv_open(newcode, "UTF-16");
 if((size_t)ict_utf16_to_game == (size_t)-1)
 {
  error_string = trio_aprintf("iconv_open() error: %m");
  error_time = SDL_GetTicks();
  return(0);
 }

 GameCode = std::string(newcode);
 return(1);
}

class MemDebuggerPrompt : public HappyPrompt
{
        public:

        MemDebuggerPrompt(MemDebugger* memdbg_in, const std::string &ptext, const std::string &zestring) : HappyPrompt(ptext, zestring), memdbg(memdbg_in)
        {

        }
        ~MemDebuggerPrompt()
        {

        }

        private:

        void TheEnd(const std::string &pstring);

	MemDebugger* memdbg;
};

void MemDebuggerPrompt::TheEnd(const std::string &pstring)
{
	memdbg->PromptFinish(pstring);
}


bool MemDebugger::DoBSSearch(uint32 byte_count, uint8 *thebytes)
{
	 const uint64 zemod = SizeCache[CurASpace];
         const uint32 start_a = ASpacePos[CurASpace] % zemod;
         uint32 a = start_a;
	 bool found = FALSE;
	 uint8 *bbuffer = (uint8 *)calloc(1, byte_count);

	 
         do
         {
          ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), a, byte_count, bbuffer);
          if(!memcmp(bbuffer, thebytes, byte_count))
          {
           ASpacePos[CurASpace] = a;
           found = TRUE;
           break;
          }
          a = (a + 1) % zemod;
         } while(a != start_a);
	 

	 free(bbuffer);
	 return(found);
}

bool MemDebugger::DoRSearch(uint32 byte_count, uint8 *the_bytes)
{
	 const uint64 zemod = SizeCache[CurASpace];
         const uint32 start_a = (ASpacePos[CurASpace] - 1) % zemod;
         uint32 a = start_a;
	 bool found = FALSE;
	 uint8 *bbuffer = (uint8 *)calloc(1, byte_count);

	 
         do
         {
          ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), a, byte_count + 1, bbuffer);
          bool match = TRUE;

          for(uint32 i = 1; i <= byte_count; i++)
          {
           if(((bbuffer[i] - bbuffer[i - 1]) & 0xFF) != the_bytes[i - 1])
           {
            match = FALSE;
            break;
           }
          }
          if(match)
          {
           found = TRUE;
           ASpacePos[CurASpace] = (a + 1) % zemod;
           break;
          }
          a = (a + 1) % zemod;
         } while(a != start_a);
	 
	 free(bbuffer);

	 return(found);
}

uint8 *MemDebugger::TextToBS(const char *text, size_t *TheCount)
{
          size_t byte_count;
          uint8 *thebytes = NULL;
	  size_t text_len = strlen(text);
          size_t nib_count;

          thebytes = (uint8 *)calloc(1, text_len / 2);

          nib_count = 0;
          for(size_t x = 0; x < text_len; x++)
          {
           int c = tolower(text[x]);
           if(c >= '0' && c <= '9')
            c -= '0';
           else if(c >= 'a' && c <= 'f')
           {
            c = c - 'a' + 0xa;
           }
           else if(c == ' ')
           {
            continue;
           }
           else
           {
            error_string = trio_aprintf("Invalid character '%c' in bytestring.", c);
            error_time = SDL_GetTicks();
            InPrompt = None;
	    free(thebytes);
            return(NULL);
           }
           thebytes[nib_count >> 1] |= c << ((nib_count & 1) ? 0 : 4);
           nib_count++;
          }

          if(nib_count & 1)
          {
           error_string = trio_aprintf(_("Invalid number of characters in bytestring."));
           error_time = SDL_GetTicks();
           InPrompt = None;
	   free(thebytes);
           return(NULL);;
          }

          byte_count = nib_count >> 1;
	  *TheCount = byte_count;
	  return(thebytes);
}

void MemDebugger::PromptFinish(const std::string &pstring)
{
	 if(error_string)
	 {
	  free(error_string);
	  error_string = NULL;
	 }
         if(InPrompt == Goto || InPrompt == GotoDD)
         {
	  unsigned long long NewAddie;

	  if(trio_sscanf(pstring.c_str(), "%llx", &NewAddie) == 1)
	  {
	   ASpacePos[CurASpace] = NewAddie;
	   LowNib = FALSE;
	   if(InPrompt == GotoDD)
	    GoGoPowerDD[CurASpace] = NewAddie;
	  }
         }
	 else if(InPrompt == SetCharset)
	 {
	  if(ICV_Init(pstring.c_str()))
	  {
	   
	   MDFNI_SetSetting(std::string(CurGame->shortname) + "." + std::string("debugger.memcharenc"), pstring);
	   
	  }
	 }
         else if(InPrompt == DumpMem)
         {
          uint32 A1, A2, tmpsize;
          char fname[256];
	  bool acceptable = FALSE;


          if(trio_sscanf(pstring.c_str(), "%08x %08x %255[^\r\n]", &A1, &A2, fname) == 3)
	   acceptable = TRUE;
          else if(trio_sscanf(pstring.c_str(), "%08x +%08x %255[^\r\n]", &A1, &tmpsize, fname) == 3)
	  {
	   acceptable = TRUE;
	   A2 = A1 + tmpsize - 1;
	  }

	  if(acceptable)
          {
           
	   try
	   {
	    FileStream fp(fname, FileStream::MODE_WRITE);
            uint8 write_buffer[256];
            uint64 a = A1;

	    //printf("%08x %08x\n", A1, A2);

	    while(a <= A2)
	    {
             size_t to_write;
             to_write = A2 - a + 1;
             if(to_write > 256) to_write = 256;

	     ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), a, to_write, write_buffer);

	     fp.write(write_buffer, to_write);
	     a += to_write;
	    }
           }
	   catch(std::exception &e)
	   {
            error_string = trio_aprintf("%s", e.what());
            error_time = SDL_GetTicks();
	   }
           
          }
	  else
	  {
	   error_string = trio_aprintf(_("Invalid memory dump specification."));
	   error_time = SDL_GetTicks();
	  }
         }
         else if(InPrompt == LoadMem)
         {
          uint32 A1, A2, tmpsize;
          char fname[256];
          bool acceptable = FALSE;

          if(trio_sscanf(pstring.c_str(), "%08x %08x %255[^\r\n]", &A1, &A2, fname) == 3)
           acceptable = TRUE;
          else if(trio_sscanf(pstring.c_str(), "%08x +%08x %255[^\r\n]", &A1, &tmpsize, fname) == 3)
          {
           acceptable = TRUE;
           A2 = A1 + tmpsize - 1;
          }

          if(acceptable)
          {
	   try
	   {
            FileStream fp(fname, FileStream::MODE_READ);
	    uint8 read_buffer[256];
	    uint64 a = A1;

	    while(a <= A2)
	    {
	     size_t to_read; 
	     size_t read_len;

	     to_read = A2 - a + 1;
	     if(to_read > 256) to_read = 256;

	     read_len = fp.read(read_buffer, to_read, false);

	     if(read_len > 0)
              ASpace->PutAddressSpaceBytes(ASpace->name.c_str(), a, read_len, 1, TRUE, read_buffer);

             a += read_len;

	     if(read_len != to_read)
	     {
	      error_string = trio_aprintf(_("Warning: unexpected EOF(short by %08llx byte(s))"), (unsigned long long)(A2 - a + 1));
	      error_time = SDL_GetTicks();
	      break;
	     }
	    }
           }
	   catch(std::exception &e)
	   {
            error_string = trio_aprintf("%s", e.what());
            error_time = SDL_GetTicks();
	   }
          }
          else
          {
           error_string = trio_aprintf(_("Invalid memory load specification."));
           error_time = SDL_GetTicks();
          }
         }
	 else if(InPrompt == TextSearch)
	 {
          uint8 *thebytes;
	  uint32 bcount;

          TS_String = pstring;

          char *inbuf, *outbuf;
          char *utf8_string;
          size_t ibl, obl, obl_start;
          size_t result;

          utf8_string = strdup(pstring.c_str());

          ibl = strlen(utf8_string);
          obl_start = obl = (ibl + 1) * 8; // Hehe, ugly maximum estimation!
          thebytes = (uint8 *)calloc(1, obl_start);

          inbuf = utf8_string;
          outbuf = (char*)thebytes;

	  result = iconv(ict, (ICONV_CONST char **)&inbuf, &ibl, &outbuf, &obl);

          if(result == (size_t)-1)
          {
           error_string = trio_aprintf(_("iconv() error: %m"));
           error_time = SDL_GetTicks();
           InPrompt = None;
	   free(utf8_string);
	   free(thebytes);
           return;
          }
	  bcount = obl_start - obl;
          free(utf8_string);

	  if(!DoBSSearch(bcount, thebytes))
	  {
	   error_string = trio_aprintf(_("String not found."));
	   error_time = SDL_GetTicks();
	   InPrompt = None;
	  }
	  free(thebytes);
	 }
	 else if(InPrompt == ByteStringSearch)
	 {
	  size_t byte_count;
	  uint8 *the_bytes;
	  BSS_String = pstring;

	  if(!(the_bytes = TextToBS(pstring.c_str(), &byte_count)))
	   return;

	  if(!DoBSSearch(byte_count, the_bytes))
          {
	   free(the_bytes);
           error_string = trio_aprintf(_("Bytestring \"%s\" not found."), pstring.c_str());
           error_time = SDL_GetTicks();
           InPrompt = None;
           return;
          }
	  free(the_bytes);
	 }
	 else if(InPrompt == RelSearch)
	 {
          size_t byte_count;
          uint8 *the_bytes;
	  RS_String = pstring;

          if(!(the_bytes = TextToBS(pstring.c_str(), &byte_count)))
           return;

	  if(!DoRSearch(byte_count, the_bytes))
	  {
	   free(the_bytes);
           error_string = trio_aprintf(_("Bytestring \"%s\" not found."), pstring.c_str());
           error_time = SDL_GetTicks();
           InPrompt = None;
           return;
	  }
	  free(the_bytes);
	 }
         InPrompt = None;
}

// Call this function from the game thread.
void MemDebugger::SetActive(bool newia)
{
 if(CurGame->Debugger)
 {
  IsActive = newia;
  if(!newia)
  {
   InEditMode = FALSE;
   LowNib = FALSE;
  }
 }
}

//
//
//
INLINE int32 MemDebugger::DrawWaveform(MDFN_Surface* surface, const int32 base_y, const uint32 hcenterw)
{
 const uint32 sample_color = surface->MakeColor(0x00,0xA0,0x00,0xFF);
 const int32 wf_size = SizeCache[CurASpace];
 uint8 waveform[wf_size];
 const int32 pcm_max = (1 << ASpace->WaveBits) - 1;
 int32 xo, yo;
 int32 area_w, area_h;

 xo = 0;
 yo = base_y;

 area_w = 2 + wf_size * 2 + 2;
 area_h = 2 + (pcm_max + 1) + 2;

 xo += (hcenterw - area_w) / 2;

 MDFN_DrawRect(surface, xo, yo, area_w, area_h, surface->MakeColor(0xA0,0xA0,0xA0,0xFF));
 MDFN_DrawFillRect(surface, xo + 1, yo + 1, area_w - 2, area_h - 2, surface->MakeColor(0x30,0x30,0x30,0xFF), surface->MakeColor(0,0,0,0xFF));
 xo += 2;
 yo += 2;

 //
 //
 //
 uint32* pixels = surface->pixels + xo + (yo * surface->pitchinpix);
 ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), 0, wf_size, waveform);

 for(int i = 0; MDFN_LIKELY(i < wf_size); i++)
 {
  int32 delta;
  int32 current;
  int32 previous;

  current = waveform[i];
  previous = waveform[(i + wf_size - 1) % wf_size];

  delta = current - previous;

  for(int y = previous; y != current; y += delta / abs(delta))
   pixels[i * 2 + 0 + ((pcm_max - y)) * surface->pitchinpix] = sample_color;

  pixels[i * 2 + 0 + ((pcm_max - current)) * surface->pitchinpix] = sample_color;
  pixels[i * 2 + 1 + ((pcm_max - current)) * surface->pitchinpix] = sample_color;
 }

 return base_y + area_h;
}

INLINE void MemDebugger::DrawAtCursorInfo(MDFN_Surface* surface, const int32 base_y, const uint32 hcenterw)
{
 static const int32 line_spacing = 12;
 static const unsigned fontid = MDFN_FONT_6x12;
 char cpstr[64];
 uint8 zebytes[4];
 uint32 tmpval;
 uint32 cpplen;
 uint32 cplen;
 int32 x = 0;
 int32 y = base_y;
 const uint64 asz = SizeCache[CurASpace];
 const int curpos_fw = (std::max<int>(12, 63 - MDFN_lzcount64(round_up_pow2(asz))) + 3) / 4;
 const uint32 curpos = ASpacePos[CurASpace] % asz;

 ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), curpos, 4, zebytes);

 x += 8;
 y += 5;
 trio_snprintf(cpstr, sizeof(cpstr), "<0x%0*X / 0x%llX>", curpos_fw, curpos, (unsigned long long)asz);
 cpplen = DrawText(surface, x, y, "Position: ", surface->MakeColor(0xa0, 0xa0, 0xFF, 0xFF), fontid);
 cplen = DrawText(surface, x + cpplen, y, cpstr, surface->MakeColor(0xFF, 0xFF, 0xFF, 0xFF), fontid);

 //
 //
 //
 if(GoGoPowerDD[CurASpace])
 {
  char ggddstr[64];

  trio_snprintf(ggddstr, sizeof(ggddstr), "(0x%0*X+0x%0*X)", curpos_fw, (unsigned)GoGoPowerDD[CurASpace], curpos_fw, (unsigned)(curpos - GoGoPowerDD[CurASpace]));
  DrawText(surface, x + cpplen + cplen + 8, y, ggddstr, surface->MakeColor(0xFF, 0x80, 0x80, 0xFF), fontid);
 }
 x += 8;
 y += line_spacing;

 tmpval = zebytes[0];
 trio_snprintf(cpstr, sizeof(cpstr), "      0x%02x(%10u, %11d)", tmpval, (uint8)tmpval, (int8)tmpval);
 cpplen = DrawText(surface, x, y, "   1-byte: ", surface->MakeColor(0xA0, 0xA0, 0xFF, 0xFF), fontid);
 DrawText(surface, x + cpplen, y, cpstr, surface->MakeColor(0xEF, 0xEF, 0xEF, 0xFF), fontid);
 y += line_spacing;

 tmpval = zebytes[0] | (zebytes[1] << 8);
 trio_snprintf(cpstr, sizeof(cpstr), "    0x%04x(%10u, %11d)", tmpval, (uint16)tmpval, (int16)tmpval);
 cpplen = DrawText(surface, x, y, "LE 2-byte: ", surface->MakeColor(0xA0, 0xA0, 0xFF, 0xFF), fontid);
 DrawText(surface, x + cpplen, y, cpstr, surface->MakeColor(0xEF, 0xEF, 0xEF, 0xFF), fontid);
 y += line_spacing;

 tmpval = zebytes[0] | (zebytes[1] << 8) | (zebytes[2] << 16) | (zebytes[3] << 24);
 trio_snprintf(cpstr, sizeof(cpstr), "0x%08x(%10u, %11d)", tmpval, (uint32)tmpval, (int32)tmpval);
 cpplen = DrawText(surface, x, y, "LE 4-byte: ", surface->MakeColor(0xA0, 0xA0, 0xFF, 0xFF), fontid);
 DrawText(surface, x + cpplen, y, cpstr, surface->MakeColor(0xEF, 0xEF, 0xEF, 0xFF), fontid);
 y += line_spacing;

 tmpval = zebytes[1] | (zebytes[0] << 8);
 trio_snprintf(cpstr, sizeof(cpstr), "    0x%04x(%10u, %11d)", tmpval, (uint16)tmpval, (int16)tmpval);
 cpplen = DrawText(surface, x, y, "BE 2-byte: ", surface->MakeColor(0xA0, 0xA0, 0xFF, 0xFF), fontid);
 DrawText(surface, x + cpplen, y, cpstr, surface->MakeColor(0xEF, 0xEF, 0xEF, 0xFF), fontid);
 y += line_spacing;

 tmpval = zebytes[3] | (zebytes[2] << 8) | (zebytes[1] << 16) | (zebytes[0] << 24);
 trio_snprintf(cpstr, sizeof(cpstr), "0x%08x(%10u, %11d)", tmpval, (uint32)tmpval, (int32)tmpval);
 cpplen = DrawText(surface, x, y, "BE 4-byte: ", surface->MakeColor(0xA0, 0xA0, 0xFF, 0xFF), fontid);
 DrawText(surface, x + cpplen, y, cpstr, surface->MakeColor(0xEF, 0xEF, 0xEF, 0xFF), fontid);
 y += line_spacing;

 trio_snprintf(cpstr, sizeof(cpstr), "%s text: ", GameCode.c_str());
 cpplen = DrawText(surface, x, y, cpstr, surface->MakeColor(0xA0, 0xA0, 0xFF, 0xFF), fontid);
 {
  char rawbuf[64];
  char textbuf[256];

  ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), curpos, 64, (uint8*)rawbuf);

  size_t ibl, obl, obl_start;
  char *inbuf, *outbuf;
  ibl = 64;
  obl_start = obl = 255; // Hehe, ugly maximum estimation!
  inbuf = rawbuf;
  outbuf = textbuf;

  iconv(ict_to_utf8, (ICONV_CONST char **)&inbuf, &ibl, &outbuf, &obl);
  textbuf[obl_start - obl] = 0;

  DrawText(surface, x + cpplen, y - 2, textbuf, surface->MakeColor(0xEF, 0xEF, 0xEF, 0xFF), MDFN_FONT_9x18_18x18);
 }
}

static const unsigned addr_font = MDFN_FONT_6x12; //5x7;
static const int32 addr_left_padding = 1;
static const int32 addr_right_padding = 3;

static const int32 byte_vspacing = 12;

static const unsigned byte_hex_font = MDFN_FONT_6x12;
static const int32 byte_hex_font_width = 6;
static const int32 byte_hex_spacing = 3 + (byte_hex_font_width * 2);
static const int32 byte_hex_group_pad = 4; //3;
static const int32 byte_hex_right_padding = 1;

static const unsigned byte_char_font = MDFN_FONT_5x7;
static const int32 byte_char_y_adjust = (byte_vspacing - 4) / 2;
static const int32 byte_char_font_width = 5;
static const int32 byte_char_spacing = byte_char_font_width;

static const int32 byte_bpr = 16;	// Bytes per row.
static const int32 byte_minrows = 4;
static const int32 byte_maxrows = 16;


// Call this function from the game thread
void MemDebugger::Draw(MDFN_Surface *surface, const MDFN_Rect *rect, const MDFN_Rect *screen_rect)
{
 if(!IsActive)
  return;

 const MDFN_PixelFormat pf_cache = surface->format;
 int32 text_y = 0;
 const uint64 zemod = SizeCache[CurASpace];

 DrawText(surface, 0, text_y, ASpace->long_name, pf_cache.MakeColor(0x20, 0xFF, 0x20, 0xFF), MDFN_FONT_6x12, rect->w);
 text_y += 12;

 if(ASpace->IsWave && SizeCache[CurASpace] <= 128 && ASpace->WaveBits <= 6)
 {
  text_y += 4;
  text_y += DrawWaveform(surface, text_y, rect->w);
 }

 uint32 A;
 uint32 Ameow; // A meow for a cat

 if(ASpacePos[CurASpace] < (byte_bpr * byte_maxrows / 2))
  A = (SizeCache[CurASpace] - (byte_bpr * byte_maxrows / 2) + ASpacePos[CurASpace]) % SizeCache[CurASpace];
 else
  A = ASpacePos[CurASpace] - (byte_bpr * byte_maxrows / 2);

 Ameow = A - (A % byte_bpr);

 int numrows = zemod / byte_bpr;

 if(numrows > byte_maxrows)
  numrows = byte_maxrows;
 else if(numrows < byte_minrows)
  numrows = byte_minrows;

 for(int y = 0; y < numrows; y++)
 {
  uint8 byte_buffer[byte_bpr];
  char abuf[32];

  Ameow %= zemod;

  ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), Ameow, byte_bpr, byte_buffer);

  trio_snprintf(abuf, 32, "%0*X:", (std::max<int>(12, 63 - MDFN_lzcount64(round_up_pow2(zemod))) + 3) / 4, Ameow);

  uint32 alen = addr_left_padding;
  uint32 addr_color = pf_cache.MakeColor(0xA0, 0xA0, 0xFF, 0xFF);

  if(Ameow == (ASpacePos[CurASpace] & ~0xF))
   addr_color = pf_cache.MakeColor(0xB0, 0xC0, 0xFF, 0xFF);

  alen += DrawText(surface, alen, text_y, abuf, addr_color, addr_font);
  alen += addr_right_padding;

  for(int x = 0; x < byte_bpr; x++)
  {
   uint32 bcolor = pf_cache.MakeColor(0xEF, 0xEF, 0xEF, 0xEF);
   uint32 acolor = pf_cache.MakeColor(0xA0, 0xB0, 0xA0, 0xFF);

   char quickbuf[16];
   uint32 test_match_pos;
   char ascii_str[2];

   ascii_str[1] = 0;
   ascii_str[0] = byte_buffer[x];

   if((uint8)ascii_str[0] < 0x20 || (uint8)ascii_str[0] >= 128)
    ascii_str[0] = '.';

   trio_snprintf(quickbuf, 16, "%02X", byte_buffer[x]);

   test_match_pos = ASpacePos[CurASpace] % zemod;

   if(Ameow == test_match_pos)
   {
    if(InEditMode)
    {
     if(SDL_GetTicks() & 0x80)
     {
      int pix_offset = alen;

      if(InTextArea)
      {
       pix_offset += byte_bpr * byte_hex_spacing + byte_hex_right_padding + x * byte_char_spacing + ((byte_bpr - 1) / 4) * byte_hex_group_pad;
       DrawText(surface, pix_offset, text_y + byte_char_y_adjust, "▉", pf_cache.MakeColor(0xFF, 0xFF, 0xFF, 0xFF), byte_char_font); 
      }
      else
      {
       pix_offset += (LowNib ? byte_hex_font_width : 0) + x * byte_hex_spacing + (x / 4) * byte_hex_group_pad;
       DrawText(surface, pix_offset, text_y, "▉", pf_cache.MakeColor(0xFF, 0xFF, 0xFF, 0xFF), byte_hex_font);
      }

     }
    }

    if(InTextArea)
    {
     acolor = pf_cache.MakeColor(0xFF, 0x00, 0x00, 0xFF);
     bcolor = pf_cache.MakeColor(0xFF, 0x80, 0x80, 0xFF);
    }
    else
    {
     acolor = pf_cache.MakeColor(0xFF, 0x80, 0x80, 0xFF);
     bcolor = pf_cache.MakeColor(0xFF, 0x00, 0x00, 0xFF);
    }
   }

   // hex display
   DrawText(surface, alen + x * byte_hex_spacing + ((x / 4) * byte_hex_group_pad), text_y, quickbuf, bcolor, byte_hex_font);

   // ASCII display
   DrawText(surface, alen + byte_bpr * byte_hex_spacing + byte_hex_right_padding + x * byte_char_spacing + ((byte_bpr - 1) / 4) * byte_hex_group_pad, text_y + byte_char_y_adjust, ascii_str, acolor, byte_char_font);
   Ameow++;
  }
  text_y += byte_vspacing;
 }

 DrawAtCursorInfo(surface, 10 + byte_maxrows * byte_vspacing, rect->w);
 
 if(InPrompt)
  myprompt->Draw(surface, rect);
 else if(myprompt)
 {
  delete myprompt;
  myprompt = NULL;
 }

 if(error_string)
 {
  if(SDL_GetTicks() >= (error_time + 4000))
  {
   free(error_string);
   error_string = NULL;
  }
  else
  {
   MDFN_DrawFillRect(surface, 0, (rect->h - 13 - 2), rect->w, 13 + 2, pf_cache.MakeColor(0xFF, 0xFF, 0xFF, 0xFF), pf_cache.MakeColor(0, 0, 0, 0xFF));
   DrawText(surface, 0, (rect->h - 13 - 1), error_string, pf_cache.MakeColor(0xFF, 0x00, 0x00, 0xFF), MDFN_FONT_6x13_12x13, rect->w);
  }
 }
}

void MemDebugger::ChangePos(int64 delta)
{
 int64 prevpos = ASpacePos[CurASpace];
 int64 newpos;

 newpos = prevpos + delta;

 while(newpos < 0)
  newpos += SizeCache[CurASpace];

 newpos %= SizeCache[CurASpace];
 ASpacePos[CurASpace] = newpos;

 LowNib = FALSE;
}

// Call this from the game thread
int MemDebugger::Event(const SDL_Event *event)
{
 if(!InPrompt && myprompt)
 {
  delete myprompt;
  myprompt = NULL;
 }
 unsigned int keysym = event->key.keysym.sym;

 switch(event->type)
 {
  case SDL_KEYDOWN:
	if(event->key.keysym.mod & KMOD_ALT)
	 break;

        if(InPrompt)
        {
         myprompt->Event(event);
        }
	else if(InEditMode && InTextArea && keysym != SDLK_TAB && keysym != SDLK_INSERT && keysym != SDLK_UP && keysym != SDLK_DOWN && keysym != SDLK_LEFT
	 && keysym != SDLK_RIGHT && (event->key.keysym.unicode >= 0x20))
	{
	 uint8 to_write[16];
	 int to_write_len;

	 size_t ibl, obl, obl_start;
	 char *inbuf, *outbuf;

	 ibl = 2;
	 obl_start = obl = 16;

	 inbuf = (char *)&event->key.keysym.unicode;
	 outbuf = (char*)to_write;

	 size_t result = iconv(ict_utf16_to_game, (ICONV_CONST char **)&inbuf, &ibl, &outbuf, &obl);
	 if(result != (size_t)-1)
	 {
          to_write_len = obl_start - obl;

	  
	  ASpace->PutAddressSpaceBytes(ASpace->name.c_str(), ASpacePos[CurASpace], to_write_len, 1, TRUE, to_write);
	  

	  LowNib = 0;
	  ChangePos(to_write_len);
	 }
	}
	else if(InEditMode && ((event->key.keysym.sym >= SDLK_0 && event->key.keysym.sym <= SDLK_9)	|| 
	   (event->key.keysym.sym >= SDLK_a && event->key.keysym.sym <= SDLK_f)))
	{
         uint8 tc = 0;
         uint8 meowbyte = 0;

         if(event->key.keysym.sym >= SDLK_0 && event->key.keysym.sym <= SDLK_9)
          tc = 0x0 + event->key.keysym.sym - SDLK_0;
         else if(event->key.keysym.sym >= SDLK_a && event->key.keysym.sym <= SDLK_f)
          tc = 0xA + event->key.keysym.sym - SDLK_a;

	 
         ASpace->GetAddressSpaceBytes(ASpace->name.c_str(), ASpacePos[CurASpace], 1, &meowbyte);
         meowbyte &= 0xF << ((LowNib) * 4);
         meowbyte |= tc << ((!LowNib) * 4);
         ASpace->PutAddressSpaceBytes(ASpace->name.c_str(), ASpacePos[CurASpace], 1, 1, TRUE, &meowbyte);
	 

         LowNib = !LowNib;
         if(!LowNib)
	  ChangePos(1);
        }
	else switch(event->key.keysym.sym)
	{
	 default: break;

         case SDLK_MINUS: Debugger_GT_ModOpacity(-8);
                          break;
         case SDLK_EQUALS: Debugger_GT_ModOpacity(8);
                           break;

	 case SDLK_BACKSPACE:
		if(InEditMode && !InTextArea)
		{
	         LowNib = !LowNib;
        	 if(LowNib)
		 {
		  ChangePos(-1);
		  LowNib = true;
		 }
		}
		break;

	 case SDLK_SPACE:
		if(InEditMode && !InTextArea)
		{
	         LowNib = !LowNib;
        	 if(!LowNib)
		  ChangePos(1);
		}
		break;

	 case SDLK_TAB:
		InTextArea = !InTextArea;
		LowNib = FALSE;
		break;

	 case SDLK_d:
                InPrompt = DumpMem;
                myprompt = new MemDebuggerPrompt(this, "Dump Memory(start end filename)", "");
		break;
	 case SDLK_l:
                InPrompt = LoadMem;
                myprompt = new MemDebuggerPrompt(this, "Load Memory(start end filename)", "");
                break;
	 case SDLK_s:
	        if(SizeCache[CurASpace] > (1 << 24))
                {
                 error_string = trio_aprintf(_("Address space is too large to search!"));
                 error_time = SDL_GetTicks();
                }
		else
		{
		 InPrompt = ByteStringSearch;
		 myprompt = new MemDebuggerPrompt(this, "Byte String Search", BSS_String);
		}
		break;
	 case SDLK_r:
		if(SizeCache[CurASpace] > (1 << 24))
                {
                 error_string = trio_aprintf(_("Address space is too large to search!"));
                 error_time = SDL_GetTicks();
                }
                else
                {
		 InPrompt = RelSearch;
		 myprompt = new MemDebuggerPrompt(this, "Byte String Relative/Delta Search", RS_String);
		}
		break;

	 case SDLK_c:
		InPrompt = SetCharset;
		myprompt = new MemDebuggerPrompt(this, "Charset", GameCode);
		break;

         case SDLK_t:
                if(ASpace->TotalBits > 24)
                {
                 error_string = trio_aprintf(_("Address space is too large to search!"));
                 error_time = SDL_GetTicks();
                }
                else
                {
                 InPrompt = TextSearch;
                 myprompt = new MemDebuggerPrompt(this, "Text Search", TS_String);
                }
                break;

	 case SDLK_RETURN:
	 case SDLK_g:
	        if(event->key.keysym.mod & KMOD_SHIFT)
		{
                 InPrompt = GotoDD;
                 myprompt = new MemDebuggerPrompt(this, "Goto Address(DD)", "");
		}
		else
		{
		 InPrompt = Goto;
		 myprompt = new MemDebuggerPrompt(this, "Goto Address", "");
		}
		break;

	 case SDLK_INSERT:
		InEditMode = !InEditMode;
		LowNib = FALSE;
		break;

	 case SDLK_END: ASpacePos[CurASpace] = (SizeCache[CurASpace] - (byte_bpr * byte_maxrows / 2)) % SizeCache[CurASpace]; 
			LowNib = FALSE;
			break;

	 case SDLK_HOME: ASpacePos[CurASpace] = 0;
			 LowNib = FALSE;
			 break;


         case SDLK_PAGEUP: ChangePos(-byte_bpr * byte_maxrows); break;
	 case SDLK_PAGEDOWN: ChangePos(byte_bpr * byte_maxrows); break;
	 case SDLK_UP: ChangePos(-byte_bpr); break;
	 case SDLK_DOWN: ChangePos(byte_bpr); break;
	 case SDLK_LEFT: ChangePos(-1); break;
	 case SDLK_RIGHT: ChangePos(1); break;

	 case SDLK_COMMA: 
			if(CurASpace)
			 CurASpace--;
			else
			 CurASpace = AddressSpaces->size() - 1;

			ASpace = &(*AddressSpaces)[CurASpace];

			LowNib = FALSE;
			break;
	 case SDLK_PERIOD:
			CurASpace = (CurASpace + 1) % AddressSpaces->size();
			ASpace = &(*AddressSpaces)[CurASpace];

			LowNib = FALSE;
			break;
	}
	break;
 }
 return(1);
}


// Called after a game is loaded.
MemDebugger::MemDebugger() : AddressSpaces(NULL), ASpace(NULL), IsActive(false), CurASpace(0),
			     LowNib(false), InEditMode(false), InTextArea(false), error_string(NULL), error_time(0),
			     ict((iconv_t)-1), ict_to_utf8((iconv_t)-1), ict_utf16_to_game((iconv_t)-1), InPrompt(None), myprompt(NULL)			     
{
 if(CurGame->Debugger)
 {
  AddressSpaces = CurGame->Debugger->AddressSpaces;

  CurASpace = 0;
  ASpace = &(*AddressSpaces)[CurASpace];

  size_t num = AddressSpaces->size();

  ASpacePos.resize(num);
  SizeCache.resize(num);
  GoGoPowerDD.resize(num);

  for(size_t i = 0; i < num; i++)
  {
   uint64 tmpsize;

   tmpsize = (*AddressSpaces)[i].NP2Size;

   if(!tmpsize)
    tmpsize = (uint64)1 << (*AddressSpaces)[i].TotalBits;

   SizeCache[i] = tmpsize;
  }

  ICV_Init( MDFN_GetSettingS(std::string(CurGame->shortname) + "." + std::string("debugger.memcharenc")).c_str() );
 }
}

MemDebugger::~MemDebugger()
{
 if(ict != (iconv_t)-1)
 {
  iconv_close(ict);
  ict = (iconv_t)-1;
 }

 if(ict_to_utf8 != (iconv_t)-1)
 {
  iconv_close(ict_to_utf8);
  ict_to_utf8 = (iconv_t)-1;
 }

 if(ict_utf16_to_game != (iconv_t)-1)
 {
  iconv_close(ict_utf16_to_game);
  ict_utf16_to_game = (iconv_t)-1;
 }

 if(error_string)
 {
  free(error_string);
  error_string = NULL;
 }
}
