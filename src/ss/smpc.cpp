/******************************************************************************/
/* Mednafen Sega Saturn Emulation Module                                      */
/******************************************************************************/
/* smpc.cpp - SMPC Emulation
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

// TODO: CD On/Off

#include "ss.h"
#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/Stream.h>
#include <mednafen/cdrom/CDUtility.h>
using namespace CDUtility;

#include "smpc.h"
#include "sound.h"
#include "vdp1.h"
#include "vdp2.h"
#include "cdb.h"
#include "scu.h"

#include "input/gamepad.h"
#include "input/3dpad.h"
#include "input/mouse.h"

#include <time.h>

namespace MDFN_IEN_SS
{
#include "sh7095.h"

enum
{
 CLOCK_DIVISOR_26M = 65,
 CLOCK_DIVISOR_28M = 61
};

enum
{
 CMD_MSHON = 0x00,
 CMD_SSHON = 0x02,
 CMD_SSHOFF = 0x03,

 CMD_SNDON = 0x06,
 CMD_SNDOFF = 0x07,

 CMD_CDON = 0x08,
 CMD_CDOFF = 0x09,

 // A, B, C do something...

 CMD_SYSRES = 0x0D,

 CMD_CKCHG352 = 0x0E,
 CMD_CKCHG320 = 0x0F,

 CMD_INTBACK = 0x10,
 CMD_SETTIME = 0x16,
 CMD_SETSMEM = 0x17,

 CMD_NMIREQ = 0x18,
 CMD_RESENAB = 0x19,
 CMD_RESDISA = 0x1A
};

static uint8 AreaCode;
static int32 MasterClock;

static struct
{
 uint64 ClockAccum;

 bool Valid;

 union
 {
  uint8 raw[7];
  struct
  {
   uint8 year[2];		// BCD; [0] = xx00, [1] = 00xx
   uint8 wday_mon;	// 0x0-0x6(upper; 6=Saturday), 0x1-0xC(lower)
   uint8 mday;		// BCD; 0x01-0x31
   uint8 hour;		// BCD; 0x00-0x23
   uint8 minute;		// BCD; 0x00-0x59
   uint8 second;		// BCD; 0x00-0x59
  };
 };
} RTC;

static uint8 SaveMem[4];

static uint8 IREG[7];
static uint8 OREG[0x20];
static uint8 SR;
static bool SF;

enum
{
 PMODE_15BYTE = 0,
 PMODE_255BYTE = 1,
 PMODE_ILL = 2,
 PMODE_0BYTE = 3
};

enum
{
 SR_RESB = 0x10,
 SR_NPE = 0x20,
 SR_PDL = 0x40,
};

static bool ResetNMIEnable;

static bool ResetButtonPhysStatus;
static int32 ResetButtonCount;
static bool ResetPending;
static int32 PendingCommand;
static int32 PendingClockDivisor;
static int32 CurrentClockDivisor;

static bool PendingVB;

static int32 SubPhase;
static int64 ClockCounter;
static uint32 SMPC_ClockRatio;

static bool SoundCPUOn;
static bool SlaveSH2On;
static bool CDOn;

static uint8 BusBuffer;
//
//
static struct
{
 int64 TimeCounter;
 int32 StartTime;
 int32 OptWaitUntilTime;
 int32 OptEatTime;

 int32 OptReadTime;

 uint8 Mode[2];
 bool TimeOptEn;
 bool Remaining;
 bool NextContBit;

 uint8 CurPort;
 uint8 ID1;
 uint8 ID2;

 uint8 CommMode;

 uint8 OWP;

 uint8 work[8];
 //
 //
 uint8 ReadCounter;
 uint8 ReadBuffer[255]; //16];
 uint8 WriteCounter;
} JRS;
//
//
static uint8 DataOut[2][2];
static uint8 DataDir[2][2];
static bool DirectModeEn[2];
static bool ExLatchEn[2];

static uint8 IOBusState[2];
static IODevice* IOPorts[2];

static struct
{
 IODevice none;
 IODevice_Gamepad gamepad;
 IODevice_3DPad threedpad;
 IODevice_Mouse mouse;
} PossibleDevices[12];

static IODevice* VirtualPorts[12];
static uint8* VirtualPortsDPtr[12];
static uint8* MiscInputPtr;

IODevice::IODevice() { }
IODevice::~IODevice() { }
void IODevice::Power(void) { }
void IODevice::UpdateInput(const uint8* data) { }
void IODevice::StateAction(StateMem* sm, const unsigned load, const bool data_only, const char* sname_prefix) { }
uint8 IODevice::UpdateBus(const uint8 smpc_out, const uint8 smpc_out_asserted) { return smpc_out; }

//
//

static bool vb;
static sscpu_timestamp_t lastts;

static void UpdateIOBus(unsigned port)
{
 IOBusState[port] = IOPorts[port]->UpdateBus((DataOut[port][DirectModeEn[port]] | ~DataDir[port][DirectModeEn[port]]) & 0x7F, DataDir[port][DirectModeEn[port]]);
 assert(!(IOBusState[port] & 0x80));
}

static void MapPorts(void)
{
 for(unsigned port = 0; port < 2; port++)
  IOPorts[port] = VirtualPorts[port];
}

void SMPC_SetInput(unsigned port, const char* type, uint8* ptr)
{
 if(port == 12) 
 {
  MiscInputPtr = ptr;
  return;
 }

 assert(port < 12);

 IODevice* nd = &PossibleDevices[port].none;

 if(!strcmp(type, "gamepad"))
  nd = &PossibleDevices[port].gamepad;
 else if(!strcmp(type, "3dpad"))
  nd = &PossibleDevices[port].threedpad;
 else if(!strcmp(type, "mouse"))
  nd = &PossibleDevices[port].mouse;

 if(nd != VirtualPorts[port])
 {
  VirtualPorts[port] = nd;
  VirtualPorts[port]->Power();
 }

 VirtualPortsDPtr[port] = ptr;

 MapPorts();
}

#if 0
static void RTC_Reset(void)
{


}
#endif

void SMPC_LoadNV(Stream* s)
{
 RTC.Valid = s->get_u8();
 s->read(RTC.raw, sizeof(RTC.raw));
 s->read(SaveMem, sizeof(SaveMem));
}

void SMPC_SaveNV(Stream* s)
{
 s->put_u8(RTC.Valid);
 s->write(RTC.raw, sizeof(RTC.raw));
 s->write(SaveMem, sizeof(SaveMem));
}

void SMPC_SetRTC(const struct tm* ht, const uint8 lang)
{
 if(!ht)
 {
  RTC.Valid = false;
  RTC.year[0] = 0x19;
  RTC.year[1] = 0x93;
  RTC.wday_mon = 0x5C;
  RTC.mday = 0x31;
  RTC.hour = 0x23;
  RTC.minute = 0x59;
  RTC.second = 0x59;

  for(unsigned i = 0; i < 4; i++)
   SaveMem[i] = 0x00;
 }
 else
 {
  int year_adj = ht->tm_year;
  //if(year_adj >= 100)
  // year_adj = 100 + ((year_adj - 100) % 28);

  RTC.Valid = true; //false;
  RTC.year[0] = U8_to_BCD(19 + year_adj / 100);
  RTC.year[1] = U8_to_BCD(year_adj % 100);
  RTC.wday_mon = (std::min<unsigned>(6, ht->tm_wday) << 4) | ((std::min<unsigned>(11, ht->tm_mon) + 1) << 0);
  RTC.mday = U8_to_BCD(std::min<unsigned>(31, ht->tm_mday));
  RTC.hour = U8_to_BCD(std::min<unsigned>(23, ht->tm_hour));
  RTC.minute = U8_to_BCD(std::min<unsigned>(59, ht->tm_min));
  RTC.second = U8_to_BCD(std::min<unsigned>(59, ht->tm_sec));

  //if((SaveMem[3] & 0x0F) <= 0x05 || (SaveMem[3] & 0x0F) == 0xF)
  SaveMem[3] = (SaveMem[3] & 0xF0) | lang;
 }
}

void SMPC_Init(const uint8 area_code_arg, const int32 master_clock_arg)
{
 AreaCode = area_code_arg;
 MasterClock = master_clock_arg;

 ResetPending = false;
 vb = false;
 lastts = 0;

 for(unsigned i = 0; i < 12; i++)
  SMPC_SetInput(i, "none", NULL);

 SMPC_SetRTC(NULL, 0);
}

bool SMPC_IsSlaveOn(void)
{
 return SlaveSH2On;
}

static void SlaveOn(void)
{
 SlaveSH2On = true;
 CPU[1].AdjustTS(SH7095_mem_timestamp, true);
 CPU[1].Reset(true);
 SS_SetEventNT(&events[SS_EVENT_SH2_S_DMA], SH7095_mem_timestamp + 1);
}

static void SlaveOff(void)
{
 SlaveSH2On = false;
 CPU[1].Reset(true);
 CPU[1].AdjustTS(0x7FFFFFFF, true);
 SS_SetEventNT(&events[SS_EVENT_SH2_S_DMA], SS_EVENT_DISABLED_TS);
}

static void TurnSoundCPUOn(void)
{
 SOUND_Reset68K();
 SoundCPUOn = true;
 SOUND_Set68KActive(true);
}

static void TurnSoundCPUOff(void)
{
 SOUND_Reset68K();
 SoundCPUOn = false;
 SOUND_Set68KActive(false);
}

void SMPC_Reset(bool powering_up)
{
 SlaveOff();
 TurnSoundCPUOff();
 CDOn = true; // ? false;

 ResetButtonCount = 0;
 ResetNMIEnable = false;	// or only on powering_up?

 CPU[0].SetNMI(true);

 memset(IREG, 0, sizeof(IREG));
 memset(OREG, 0, sizeof(OREG));
 PendingCommand = -1;
 SF = 0;

 BusBuffer = 0x00;

 for(unsigned port = 0; port < 2; port++)
 {
  for(unsigned sel = 0; sel < 2; sel++)
  {
   DataOut[port][sel] = 0;
   DataDir[port][sel] = 0;
  }
  DirectModeEn[port] = false;
  ExLatchEn[port] = false;
  UpdateIOBus(port);
 }

 ResetPending = false;

 PendingClockDivisor = 0;
 CurrentClockDivisor = CLOCK_DIVISOR_26M;

 SubPhase = 0;
 PendingVB = false;
 ClockCounter = 0;
 //
 memset(&JRS, 0, sizeof(JRS));
}

int32 SMPC_StartFrame(EmulateSpecStruct* espec)
{
 if(ResetPending)
  SS_Reset(false);

 if(PendingClockDivisor > 0)
 {
  CurrentClockDivisor = PendingClockDivisor;
  PendingClockDivisor = 0;
 }

 if(!SlaveSH2On)
  CPU[1].AdjustTS(0x7FFFFFFF, true);

 SMPC_ClockRatio = (1ULL << 32) * 4000000 * CurrentClockDivisor / MasterClock;
 SOUND_SetClockRatio((1ULL << 32) * 11289600 * CurrentClockDivisor / MasterClock);
 CDB_SetClockRatio((1ULL << 32) * 11289600 * CurrentClockDivisor / MasterClock);

 return CurrentClockDivisor;
}

void SMPC_UpdateInput(void)
{
 ResetButtonPhysStatus = (bool)(*MiscInputPtr & 0x1);
 for(unsigned vp = 0; vp < 12; vp++)
 {
  VirtualPorts[vp]->UpdateInput(VirtualPortsDPtr[vp]);
 }
}


void SMPC_Write(const sscpu_timestamp_t timestamp, uint8 A, uint8 V)
{
 BusBuffer = V;
 A &= 0x3F;

 SS_DBGTI(SS_DBG_SMPC_REGW, "[SMPC] Write to 0x%02x:0x%02x", A, V);

 //
 // Call VDP2::Update() to prevent out-of-temporal-order calls to SMPC_Update() from here and the event system.
 //
 SS_SetEventNT(&events[SS_EVENT_VDP2], VDP2::Update(timestamp));	// TODO: conditionalize so we don't consume so much CPU time if a game writes continuously to SMPC ports
 sscpu_timestamp_t nt = SMPC_Update(timestamp);
 switch(A)
 {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x04:
  case 0x05:
  case 0x06:
	if(MDFN_UNLIKELY(PendingCommand >= 0))
	{
	 SS_DBGTI(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] Input register %u port written with 0x%02x while command 0x%02x is executing.", A, V, PendingCommand);
	}

	IREG[A] = V;
	break;

  case 0x0F:
	if(MDFN_UNLIKELY(PendingCommand >= 0))
	{
	 SS_DBGTI(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] Command port written with 0x%02x while command 0x%02x is still executing.", V, PendingCommand);
	}

	PendingCommand = V;
	break;

  case 0x31:
	if(MDFN_UNLIKELY(SF))
	{
	 SS_DBGTI(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] SF port written while SF is 1.");
	}

	SF = true;
	break;

  //
  //
  //
  case 0x3A:
	DataOut[0][1] = V & 0x7F;
	UpdateIOBus(0);
	break;

  case 0x3B:
	DataOut[1][1] = V & 0x7F;
	UpdateIOBus(1);
	break;

  case 0x3C:
	DataDir[0][1] = V & 0x7F;
	UpdateIOBus(0);
	break;

  case 0x3D:
	DataDir[1][1] = V & 0x7F;
	UpdateIOBus(1);
	break;

  case 0x3E:
	DirectModeEn[0] = (bool)(V & 0x1);
	UpdateIOBus(0);

	DirectModeEn[1] = (bool)(V & 0x2);
	UpdateIOBus(1);
	break;

  case 0x3F:
	ExLatchEn[0] = (bool)(V & 0x1);
	ExLatchEn[1] = (bool)(V & 0x2);
	break;

  default:
	SS_DBG(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] Unknown write of 0x%02x to 0x%02x\n", V, A);
	break;

 }

 if(PendingCommand >= 0)
  nt = timestamp + 1;

 SS_SetEventNT(&events[SS_EVENT_SMPC], nt);
}

uint8 SMPC_Read(const sscpu_timestamp_t timestamp, uint8 A)
{
 uint8 ret = BusBuffer;

 A &= 0x3F;

 switch(A)
 {
  default:
	SS_DBG(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] Unknown read from 0x%02x\n", A);
	break;

  case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
  case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
  case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
  case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F:
	if(MDFN_UNLIKELY(PendingCommand >= 0))
	{
	 //SS_DBG(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] Output register %u port read while command 0x%02x is executing.\n", A - 0x10, PendingCommand);
	}

	ret = (OREG - 0x10)[A];
	break;

  case 0x30:
	if(MDFN_UNLIKELY(PendingCommand >= 0))
	{
	 //SS_DBG(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] SR port read while command 0x%02x is executing.\n", PendingCommand);
	}

	ret = SR;
	break;
 
  case 0x31:
	ret &= ~0x01;
	ret |= SF;
	break;

  case 0x3A:
	ret = (ret & 0x80) | IOBusState[0];
	break;

  case 0x3B:
	ret = (ret & 0x80) | IOBusState[1];
	break;

 }

 return ret;
}

void SMPC_ResetTS(void)
{
 lastts = 0;
}

#define SMPC_WAIT_UNTIL_COND(cond)  {					\
			    case __COUNTER__:				\
			    ClockCounter = 0; /* before if(), not after, otherwise the variable will overflow eventually. */	\
			    if(!(cond))					\
			    {						\
			     SubPhase = __COUNTER__ - SubPhaseBias - 1;	\
			     return timestamp + 1000;			\
			    }						\
			   }

#define SMPC_WAIT_UNTIL_COND_TIMEOUT(cond, n)							\
		{										\
		 ClockCounter -= (int64)(n) << 32;						\
		 case __COUNTER__:								\
		 if(!(cond) && ClockCounter < 0)						\
		 {										\
		  SubPhase = __COUNTER__ - SubPhaseBias - 1;					\
		  return timestamp + (-ClockCounter + SMPC_ClockRatio - 1) / SMPC_ClockRatio;	\
		 }										\
		 ClockCounter = 0;								\
		}

#define SMPC_EAT_CLOCKS(n)									\
		{										\
		 ClockCounter -= (int64)(n) << 32;						\
		 case __COUNTER__:								\
		 if(ClockCounter < 0)								\
		 {										\
		  SubPhase = __COUNTER__ - SubPhaseBias - 1;					\
		  return timestamp + (-ClockCounter + SMPC_ClockRatio - 1) / SMPC_ClockRatio;	\
		 }										\
		 /*printf("%f\n", (double)ClockCounter / (1LL << 32));*/			\
		}										\


static unsigned RTC_BCDInc(uint8 v)
{
 unsigned tmp = v & 0xF;

 tmp++;

 if(tmp >= 0xA)
  tmp += 0x06;

 tmp += v & 0xF0;

 if(tmp >= 0xA0)
  tmp += 0x60;

 return tmp;
}

static void RTC_IncTime(void)
{
 // Seconds
 if(RTC.second == 0x59)
 {
  RTC.second = 0x00;

  // Minutes
  if(RTC.minute == 0x59)
  {
   RTC.minute = 0x00;

   // Hours
   if(RTC.hour == 0x23)
   {
    RTC.hour = 0x00;

    // Day of week
    if(RTC.wday_mon >= 0x60)
     RTC.wday_mon &= 0x0F;
    else
     RTC.wday_mon += 0x10;

    //					
    static const uint8 mdtab[0x10] = { 
    //         Jan,  Feb,  Mar,  Apr,   May, June, July,  Aug, Sept, Oct,  Nov,  Dec
	0x10, 0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31, 0xC1, 0xF5, 0xFF
    };
    const uint8 day_compare = mdtab[RTC.wday_mon & 0x0F] + ((RTC.wday_mon & 0x0F) == 0x02 && ((RTC.year[1] & 0x1F) < 0x1A) && !((RTC.year[1] + ((RTC.year[1] & 0x10) >> 3)) & 0x3));

    // Day of month
    if(RTC.mday >= day_compare)
    {
     RTC.mday = 0x01;

     // Month of year
     if((RTC.wday_mon & 0x0F) == 0x0C)
     {
      RTC.wday_mon &= 0xF0;
      RTC.wday_mon |= 0x01;

      // Year
      unsigned tmp = RTC_BCDInc(RTC.year[1]);
      RTC.year[1] = tmp;

      if(tmp >= 0x100)
       RTC.year[0] = RTC_BCDInc(RTC.year[0]);
     }
     else
      RTC.wday_mon++;
    }
    else
     RTC.mday = RTC_BCDInc(RTC.mday);
   }
   else
    RTC.hour = RTC_BCDInc(RTC.hour);
  }
  else
   RTC.minute = RTC_BCDInc(RTC.minute);
 }
 else
  RTC.second = RTC_BCDInc(RTC.second);
}

enum { SubPhaseBias = __COUNTER__ + 1 };
sscpu_timestamp_t SMPC_Update(sscpu_timestamp_t timestamp)
{
 int64 clocks;

 if(MDFN_UNLIKELY(timestamp < lastts))
 {
  SS_DBG(SS_DBG_WARNING | SS_DBG_SMPC, "[SMPC] [BUG] timestamp(%d) < lastts(%d)\n", timestamp, lastts);
  clocks = 0;
 }
 else
 {
  clocks = (int64)(timestamp - lastts) * SMPC_ClockRatio;
  lastts = timestamp;
 }

 ClockCounter += clocks;
 RTC.ClockAccum += clocks;
 JRS.TimeCounter += clocks;

 switch(SubPhase + SubPhaseBias)
 {
  for(;;)
  {
   default:
   case __COUNTER__:

   SMPC_WAIT_UNTIL_COND(PendingCommand >= 0 || PendingVB);

   if(PendingVB)
   {
    PendingVB = false;

    if(JRS.OptReadTime)
     JRS.OptWaitUntilTime = std::max<int32>(0, (JRS.TimeCounter >> 32) - JRS.OptReadTime - 5000);
    else
     JRS.OptWaitUntilTime = 0;
    JRS.TimeCounter = 0;
    SMPC_EAT_CLOCKS(234);

    SR &= ~SR_RESB;
    if(ResetButtonPhysStatus)	// FIXME: Semantics may not be right in regards to CMD_RESENAB timing.
    {
     SR |= SR_RESB;
     if(ResetButtonCount >= 0)
     {
      ResetButtonCount++;
 
      if(ResetButtonCount >= 3)
      {
       ResetButtonCount = 3;

       if(ResetNMIEnable)
       {
        CPU[0].SetNMI(false);
        CPU[0].SetNMI(true);

        ResetButtonCount = -1;
       }
      }
     }
    }
    else
     ResetButtonCount = 0;

    //
    // Do RTC increment here
    // 
    while(MDFN_UNLIKELY(RTC.ClockAccum >= (4000000ULL << 32)))
    {
     RTC_IncTime();
     RTC.ClockAccum -= (4000000ULL << 32);
    }

    continue;
   }


   SMPC_EAT_CLOCKS(92);
   if(PendingCommand < 0x20)
   {
    OREG[0x1F] = PendingCommand;

    SS_DBGTI(SS_DBG_SMPC, "[SMPC] Command 0x%02x --- 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", PendingCommand, IREG[0], IREG[1], IREG[2], IREG[3], IREG[4], IREG[5], IREG[6]);

    if(PendingCommand == CMD_MSHON)
    {

    }
    else if(PendingCommand == CMD_SSHON)
    {
     if(!SlaveSH2On)
      SlaveOn();
    }
    else if(PendingCommand == CMD_SSHOFF)
    {
     if(SlaveSH2On)
      SlaveOff();
    }
    else if(PendingCommand == CMD_SNDON)
    {
     if(!SoundCPUOn)
      TurnSoundCPUOn();
    }
    else if(PendingCommand == CMD_SNDOFF)
    {
     if(SoundCPUOn)
      TurnSoundCPUOff();
    }
    else if(PendingCommand == CMD_CDON)
    {
     CDOn = true;
    }
    else if(PendingCommand == CMD_CDOFF)
    {
     CDOn = false;
    }
    else if(PendingCommand == CMD_SYSRES)
    {
     ResetPending = true;
     SMPC_WAIT_UNTIL_COND(!ResetPending);

     // TODO/FIXME(unreachable currently?):
    }
    else if(PendingCommand == CMD_CKCHG352 || PendingCommand == CMD_CKCHG320)
    {
     // Devour some time

     if(SlaveSH2On)
      SlaveOff();
 
     if(SoundCPUOn)
      TurnSoundCPUOff();

     SOUND_Reset(false);
     VDP1::Reset(false);
     VDP2::Reset(false);
     SCU_Reset(false);

     // Change clock
     PendingClockDivisor = (PendingCommand == CMD_CKCHG352) ? CLOCK_DIVISOR_28M : CLOCK_DIVISOR_26M;

     // Wait for a few vblanks
     SMPC_WAIT_UNTIL_COND(!vb);
     SMPC_WAIT_UNTIL_COND(vb);
     SMPC_WAIT_UNTIL_COND(!vb);
     SMPC_WAIT_UNTIL_COND(vb);
     SMPC_WAIT_UNTIL_COND(!vb);
     SMPC_WAIT_UNTIL_COND(vb);


     //
     SMPC_WAIT_UNTIL_COND(!PendingClockDivisor);

     // Send NMI to master SH-2
     CPU[0].SetNMI(false);
     CPU[0].SetNMI(true);
    }
    else if(PendingCommand == CMD_INTBACK)
    {
     //SS_DBGTI(SS_DBG_SMPC, "[SMPC] INTBACK IREG0=0x%02x, IREG1=0x%02x, IREG2=0x%02x, %d", IREG[0], IREG[1], IREG[2], vb);

     SR &= ~SR_NPE;
     if(IREG[0] & 0xF)
     {
      SMPC_EAT_CLOCKS(952);

      OREG[0] = (RTC.Valid << 7) | (!ResetNMIEnable << 6);

      for(unsigned i = 0; i < 7; i++)
       OREG[1 + i] = RTC.raw[i];

      OREG[0x8] = 0; // TODO FIXME: Cartridge code?
      OREG[0x9] = AreaCode;
      OREG[0xA] = 0x24 | 
		 ((CurrentClockDivisor == CLOCK_DIVISOR_28M) << 6) |
		 (SlaveSH2On << 4) | 
		 (true << 3) | 	// TODO?: Master NMI
		 (true << 1) |	// TODO?: sysres
		 (SoundCPUOn << 0);	// sndres

      OREG[0xB] = (CDOn << 6) | (1 << 1);	// cdres, TODO?: bit1

      for(unsigned i = 0; i < 4; i++)
       OREG[0xC + i] = SaveMem[i];

      if(IREG[1] & 0x8)
       SR |= SR_NPE;

      SR &= ~0x80;
      SR |= 0x0F;

      SCU_SetInt(SCU_INT_SMPC, true);
      SCU_SetInt(SCU_INT_SMPC, false);
     }

     // Wait for !vb, wait until (IREG[0] & 0x80), time-optimization wait.

     if(IREG[1] & 0x8)
     {
      #define JR_WAIT(cond)	{ SMPC_WAIT_UNTIL_COND((cond) || PendingVB); if(PendingVB) { SS_DBGTI(SS_DBG_SMPC, "[SMPC] abortjr wait"); goto AbortJR; } }
      #define JR_EAT(n)		{ SMPC_EAT_CLOCKS(n); if(PendingVB) { SS_DBGTI(SS_DBG_SMPC, "[SMPC] abortjr eat"); goto AbortJR; } }
      #define JR_WRNYB(val)	\
	{			\
	 /*if(!JRS.OWP) { JR_WAIT((bool)(IREG[0] & 0x80) == JRS.NextContBit); JRS.NextContBit = !JRS.NextContBit; }*/	\
	 OREG[(JRS.OWP >> 1)] &= 0x0F << ((JRS.OWP & 1) << 2);								\
	 OREG[(JRS.OWP >> 1)] |= ((val) & 0xF) << (((JRS.OWP & 1) ^ 1) << 2);						\
	 JRS.OWP = (JRS.OWP + 1) & 0x3F;										\
	}

      #define JR_BS	IOBusState[JRS.CurPort]

      #define JR_TH_TR(th, tr)											\
	{													\
	 DataDir[JRS.CurPort][0] = ((th >= 0) << 6) | ((tr >= 0) << 5);						\
	 DataOut[JRS.CurPort][0] = (DataOut[JRS.CurPort][0] & 0x1F) | (((th) > 0) << 6) | (((tr) > 0) << 5);	\
	 UpdateIOBus(JRS.CurPort);										\
	}

      JR_WAIT(!vb);
      JRS.NextContBit = true;
      if(SR & SR_NPE)
      {
       JR_WAIT((bool)(IREG[0] & 0x80) == JRS.NextContBit || (IREG[0] & 0x40));
       if(IREG[0] & 0x40)
       {
        SS_DBGTI(SS_DBG_SMPC, "[SMPC] Break");
        goto AbortJR;
       }
       JRS.NextContBit = !JRS.NextContBit;
      }

      JRS.TimeOptEn = !(IREG[1] & 0x2);
      JRS.Mode[0] = (IREG[1] >> 4) & 0x3;
      JRS.Mode[1] = (IREG[1] >> 6) & 0x3;

      JRS.OptReadTime = 0;
      JRS.OptEatTime = std::max<int32>(0, (JRS.OptWaitUntilTime - (JRS.TimeCounter >> 32)));
      JRS.OptWaitUntilTime = 0;

      if(JRS.TimeOptEn)
      {
       SMPC_WAIT_UNTIL_COND_TIMEOUT(PendingVB, JRS.OptEatTime);
       if(PendingVB)
       {
	SS_DBGTI(SS_DBG_SMPC, "[SMPC] abortjr timeopt");
	goto AbortJR;
       }
       SS_SetEventNT(&events[SS_EVENT_MIDSYNC], timestamp + 1);
      }

      JRS.StartTime = JRS.TimeCounter >> 32;
      JR_EAT(120);
      JRS.OWP = 0;
      for(JRS.CurPort = 0; JRS.CurPort < 2; JRS.CurPort++)
      {
       JR_EAT(380);

	// TODO: Check read size mode.

       JRS.ID1 = 0;
       JR_TH_TR(1, 1);
       JR_EAT(50);
       JRS.work[0] = JR_BS;
       JRS.ID1 |= ((((JRS.work[0] >> 3) | (JRS.work[0] >> 2)) & 1) << 3) | ((((JRS.work[0] >> 1) | (JRS.work[0] >> 0)) & 1) << 2);

       JR_TH_TR(0, 1);
       JR_EAT(50);
       JRS.work[1] = JR_BS;
       JRS.ID1 |= ((((JRS.work[1] >> 3) | (JRS.work[1] >> 2)) & 1) << 1) | ((((JRS.work[1] >> 1) | (JRS.work[1] >> 0)) & 1) << 0);

       //printf("%02x, %02x\n", JRS.work[0], JRS.work[1]);

       if(JRS.ID1 == 0xB)
       {
	// Saturn digital pad.
	JR_TH_TR(1, 0)
	JR_EAT(50);
	JRS.work[2] = JR_BS;

	JR_TH_TR(0, 0)
	JR_EAT(50);
	JRS.work[3] = JR_BS;

	JR_EAT(30);

	JR_WRNYB(0xF);	// Multitap ID
	JR_EAT(21);

	JR_WRNYB(0x1);	// Number of connected devices behind multitap
	JR_EAT(21);

	JR_WRNYB(0x0);	// Peripheral ID-2.
	JR_EAT(21);

	JR_WRNYB(0x2);	// Data size.
	JR_EAT(21);

	JR_WRNYB(JRS.work[1] & 0xF);
	JR_EAT(21);

	JR_WRNYB(JRS.work[2] & 0xF);
	JR_EAT(21);

	JR_WRNYB(JRS.work[3] & 0xF);
	JR_EAT(21);

	JR_WRNYB((JRS.work[0] & 0xF) | 0x7);
	JR_EAT(21);

	//JR_EAT();

	//
	//
	//
       }
       else if(JRS.ID1 == 0x3 || JRS.ID1 == 0x5)
       {
	JR_TH_TR(0, 0)
	JR_EAT(50);
	JR_WAIT(!(JR_BS & 0x10));
	JRS.ID2 = ((JR_BS & 0xF) << 4);

	JR_TH_TR(0, 1)
	JR_EAT(50);
	JR_WAIT(JR_BS & 0x10);
	JRS.ID2 |= ((JR_BS & 0xF) << 0);

	if(JRS.ID1 == 0x3)
	 JRS.ID2 = 0xE3;

	JRS.ReadCounter = 0;
	while(JRS.ReadCounter < (JRS.ID2 & 0xF))
	{
	 JR_TH_TR(0, 0)
	 JR_EAT(50);
	 JR_WAIT(!(JR_BS & 0x10));
	 JRS.ReadBuffer[JRS.ReadCounter] = ((JR_BS & 0xF) << 4);

	 JR_TH_TR(0, 1)
	 JR_EAT(50);
	 JR_WAIT(JR_BS & 0x10);
	 JRS.ReadBuffer[JRS.ReadCounter] |= ((JR_BS & 0xF) << 0);
	 JRS.ReadCounter++;
	}

	JR_WRNYB(0xF);
	JR_EAT(21);

	JR_WRNYB(0x1);
	JR_EAT(21);

	JR_WRNYB(JRS.ID2 >> 4);
	JR_EAT(21);

	JR_WRNYB(JRS.ID2 >> 0);
	JR_EAT(21);

	JRS.WriteCounter = 0;
	while(JRS.WriteCounter < JRS.ReadCounter)
	{
	 JR_WRNYB(JRS.ReadBuffer[JRS.WriteCounter] >> 4);
 	 JR_EAT(21);

	 JR_WRNYB(JRS.ReadBuffer[JRS.WriteCounter] >> 0);
 	 JR_EAT(21);

         JRS.WriteCounter++;
	}
	

	// Saturn analog joystick, keyboard, multitap
        // OREG[0x0] = 0xF1;	// Upper nybble, multitap ID.  Lower nybble, number of connected devices behind multitap.
        // OREG[0x1] = 0x02;	// Upper nybble, peripheral ID 2.  Lower nybble, data size.
       }
       else
       {
	JR_WRNYB(0xF);
	JR_WRNYB(0x0);

	JR_WRNYB(0x0);
	JR_WRNYB(0x0);
       }
       JR_EAT(26);
       JR_TH_TR(-1, -1);
      }

      SR &= ~SR_NPE;
      SR &= ~0xF;
      SR |= JRS.Mode[0] << 0;
      SR |= JRS.Mode[1] << 2;
      SR |= SR_PDL;
      SR |= 0x80;
      SCU_SetInt(SCU_INT_SMPC, true);
      SCU_SetInt(SCU_INT_SMPC, false);

      if(JRS.TimeOptEn)
       JRS.OptReadTime = std::max<int32>(0, (JRS.TimeCounter >> 32) - JRS.StartTime);
     }
     AbortJR:;
     // TODO: Set TH TR to inputs on abort.
    }
    else if(PendingCommand == CMD_SETTIME)	// Warning: Execute RTC setting atomically(all values or none) in regards to emulator exit/power toggle.
    {
     SMPC_EAT_CLOCKS(380);

     RTC.ClockAccum = 0;	// settime resets sub-second count.
     RTC.Valid = true;

     for(unsigned i = 0; i < 7; i++)
      RTC.raw[i] = IREG[i];
    }
    else if(PendingCommand == CMD_SETSMEM)	// Warning: Execute save mem setting(all values or none) atomically in regards to emulator exit/power toggle.
    {
     SMPC_EAT_CLOCKS(234);

     for(unsigned i = 0; i < 4; i++)
      SaveMem[i] = IREG[i];
    }
    else if(PendingCommand == CMD_NMIREQ)
    {
     CPU[0].SetNMI(false);
     CPU[0].SetNMI(true);
    }
    else if(PendingCommand == CMD_RESENAB)
    {
     ResetNMIEnable = true;
    }
    else if(PendingCommand == CMD_RESDISA)
    {
     ResetNMIEnable = false;
    }
   }

   PendingCommand = -1;
   SF = false;
   continue;
  }
 }
}

void SMPC_SetVB(sscpu_timestamp_t event_timestamp, bool vb_status)
{
 if(vb ^ vb_status)
 {
  if(vb_status)	// Going into vblank
   PendingVB = true;

  SS_SetEventNT(&events[SS_EVENT_SMPC], event_timestamp + 1);
 }

 vb = vb_status;
}

static const std::vector<InputDeviceInfoStruct> InputDeviceInfoSSPort =
{
 // None
 {
  "none",
  "none",
  NULL,
  IDII_Empty
 },

 // Digital Gamepad
 {
  "gamepad",
  "Digital Gamepad",
  "Standard Saturn digital gamepad.",
  IODevice_Gamepad_IDII,
 },

 // 3D Gamepad
 {
  "3dpad",
  "3D Control Pad",
  "3D Control Pad",
  IODevice_3DPad_IDII,
 },

 // Mouse
 {
  "mouse",
  "Mouse",
  "Mouse",
  IODevice_Mouse_IDII,
 },
};

static IDIISG IDII_Builtin =
{
 { "reset", "Reset", -1, IDIT_RESET_BUTTON },
 { "smpc_reset", "SMPC Reset", -1, IDIT_BUTTON },
};

static const std::vector<InputDeviceInfoStruct> InputDeviceInfoBuiltin =
{
 {
  "builtin",
  "builtin",
  NULL,
  IDII_Builtin
 }
};

const std::vector<InputPortInfoStruct> SMPC_PortInfo =
{
 { "port1", "Virtual Port 1", InputDeviceInfoSSPort, "gamepad" },
 { "port2", "Virtual Port 2", InputDeviceInfoSSPort, "gamepad" },
 { "port3", "Virtual Port 3", InputDeviceInfoSSPort, "gamepad" },
 { "port4", "Virtual Port 4", InputDeviceInfoSSPort, "gamepad" },
 { "port5", "Virtual Port 5", InputDeviceInfoSSPort, "gamepad" },
 { "port6", "Virtual Port 6", InputDeviceInfoSSPort, "gamepad" },
 { "port7", "Virtual Port 7", InputDeviceInfoSSPort, "gamepad" },
 { "port8", "Virtual Port 8", InputDeviceInfoSSPort, "gamepad" },
 { "port9", "Virtual Port 9", InputDeviceInfoSSPort, "gamepad" },
 { "port10", "Virtual Port 10", InputDeviceInfoSSPort, "gamepad" },
 { "port11", "Virtual Port 11", InputDeviceInfoSSPort, "gamepad" },
 { "port12", "Virtual Port 12", InputDeviceInfoSSPort, "gamepad" },

 { "builtin", "Builtin", InputDeviceInfoBuiltin, "builtin" },
};


}
