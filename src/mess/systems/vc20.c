/***************************************************************************

	commodore vic20 home computer
	Peter Trauner
	(peter.trauner@jk.uni-linz.ac.at)

    documentation
     Marko.Makela@HUT.FI (vic6560)
     www.funet.fi

***************************************************************************/

/*
------------------------------------
vic20 commodore vic20 (ntsc version)
vc20  commodore vc20  (pal version)
------------------------------------

vic20 ntsc-version
 when screen is two wide right or low,
  or screen doesn't fitt in visible area
  or gameplay is too fast
 try the pal version

vc20  pal-version

a normal or good written program does not rely on the video system

vic20cr
 cost reduced
 only modified for cheaper production (newer rams, ...)
 (2 2kx8 rams instead of 8 1kx4 rams, ...)

State
-----

rasterline based video system
 should be enough for all vic20 games and programs
imperfect sound
timer system only 98% accurate
simple tape support
serial bus
 simple disk support
 no printer or other devices
no userport
 no rs232/v.24 interface
keyboard
gameport
 joystick
 paddles
 lightpen
expansion slot
 ram and rom cartridges
 no special expansion modules like ieee488 interface
Quickloader

for a more complete vic20 emulation take a look at the very good
vice emulator

Video
-----
NTSC:
Screen Size (normal TV (horicontal),4/3 ratio)
pixel ratio: about 7/5 !
so no standard Vesa Resolution is good
tweaked mode 256x256 acceptable
best define own display mode (when graphic driver supports this)
PAL:
pixel ratio about 13/10 !
good (but only part of screen filled) 1024x768
acceptable 800x600
better define own display mode (when graphic driver supports this)

XMESS:
use -scalewidth 3 -scaleheight 2 for acceptable display
better define own display mode

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

stop-restore in much cases prompt will appear
shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) load and start program from tape

Lightpen
--------
Paddle 3 x-axe
Paddle 4 y-axe

Tape
----
(DAC 1 volume in noise volume)
loading of wav, prg and prg files in zip archiv
commandline -cassette image
wav:
 8 or 16(not tested) bit, mono, 12500 Hz minimum
 has the same problems like an original tape drive (tone head must
 be adjusted to get working (no load error,...) wav-files)
zip:
 must be placed in current directory
 prg's are played in the order of the files in zip file

use LOAD or LOAD"" or LOAD"",1 for loading of normal programs
use LOAD"",1,1 for loading programs to their special address

several programs rely on more features
(loading other file types, writing, ...)

Discs
-----
only file load from drive 8 and 9 implemented
 loads file from rom directory (*.prg,*.p00)(must NOT be specified on commandline)
 or file from d64 image (here also directory command LOAD"$",8 supported)
LOAD"filename",8
or LOAD"filename",8,1 (for loading machine language programs at their address)
for loading
type RUN or the appropriate SYS call to start them

several programs rely on more features
(loading other file types, writing, ...)

some games rely on starting own programs in the floppy drive
(and therefor cpu level emulation is needed)

Roms
----
.bin .rom .a0 .20 .40 .60 .prg
files with boot-sign in it
  recogniced as roms

.20 files loaded at 0x2000
.40 files loaded at 0x4000
.60 files loaded at 0x6000
.a0 files loaded at 0xa000
.prg files loaded at address in its first two bytes
.bin .rom files loaded at 0x4000 when 0x4000 bytes long
else loaded at 0xa000

Quickloader
-----------
.prg files supported
loads program into memory and sets program end pointer
(works with most programs)
program ready to get started with RUN
currently loads first rom when you press quickload key (f8)

when problems start with -log and look into error.log file
 */


#include "driver.h"

#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"
#include "mess/machine/vc20.h"
#include "mess/machine/6522via.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/vc20tape.h"
#include "mess/vidhrdw/vic6560.h"

static struct MemoryReadAddress vc20_readmem[] =
{
	{0x0000, 0x03ff, MRA_RAM},
#if 0
	{0x0400, 0x0fff, MRA_RAM},		   /* ram, rom or nothing; I think read 0xff! */
#endif
	{0x1000, 0x1fff, MRA_RAM},
#if 0
	{0x2000, 0x3fff, MRA_RAM},		   /* ram, rom or nothing */
	{0x4000, 0x5fff, MRA_RAM},		   /* ram, rom or nothing */
	{0x6000, 0x7fff, MRA_RAM},		   /* ram, rom or nothing */
#endif
	{0x8000, 0x8fff, MRA_ROM},
	{0x9000, 0x900f, vic6560_port_r},
	{0x9010, 0x910f, MRA_NOP},
	{0x9110, 0x911f, via_0_r},
	{0x9120, 0x912f, via_1_r},
	{0x9130, 0x93ff, MRA_NOP},
	{0x9400, 0x97ff, MRA_RAM},		   /*color ram 4 bit */
	{0x9800, 0x9fff, MRA_NOP},
#if 0
	{0xa000, 0xbfff, MRA_RAM},		   /* or nothing */
#endif
	{0xc000, 0xffff, MRA_ROM},
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress vc20_writemem[] =
{
	{0x0000, 0x03ff, MWA_RAM, &vc20_memory},
	{0x1000, 0x1fff, MWA_RAM},
	{0x8000, 0x8fff, MWA_ROM},
	{0x9000, 0x900f, vic6560_port_w},
	{0x9010, 0x910f, MWA_NOP},
	{0x9110, 0x911f, via_0_w},
	{0x9120, 0x912f, via_1_w},
	{0x9130, 0x93ff, MWA_NOP},
	{0x9400, 0x97ff, vc20_write_9400, &vc20_memory_9400},
	{0x9800, 0x9fff, MWA_NOP},
	{0xc000, 0xffff, MWA_NOP},		   /* MWA_ROM }, but logfile */
	{-1}							   /* end of table */
};

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BITX(bit, IP_ACTIVE_LOW, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define DIPS_HELPER2(bit, name, keycode) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define DIPS_BOTH \
	PORT_START\
	PORT_BIT( 0x80, IP_ACTIVE_LOW,	IPT_JOYSTICK_RIGHT | IPF_8WAY )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW,	IPT_UNUSED )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_BUTTON1)\
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_JOYSTICK_LEFT | IPF_8WAY )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_JOYSTICK_DOWN | IPF_8WAY )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW,	IPT_JOYSTICK_UP | IPF_8WAY )\
	PORT_BIT( 0x03, IP_ACTIVE_LOW,	IPT_UNUSED )\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2, \
		   "Paddle 2 Button", KEYCODE_DEL, 0)\
	PORT_BIT ( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )\
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1, \
		   "Paddle 1 Button", KEYCODE_INSERT, 0)\
	PORT_BIT ( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )\
	PORT_START \
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_REVERSE,\
		     30,20,0,255,KEYCODE_HOME,KEYCODE_PGUP,0,0)\
	PORT_START \
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_PLAYER2|IPF_REVERSE,\
		     30,20,0,255,KEYCODE_END,KEYCODE_PGDN,0,0)\
	PORT_START \
	DIPS_HELPER( 0x80, "DEL INST",          KEYCODE_BACKSPACE)\
	DIPS_HELPER( 0x40, "Pound",             KEYCODE_MINUS)\
	DIPS_HELPER( 0x20, "+",                 KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x10, "9 )   RVS-ON",      KEYCODE_9)\
	DIPS_HELPER( 0x08, "7 '   BLU",         KEYCODE_7)\
	DIPS_HELPER( 0x04, "5 %   PUR",         KEYCODE_5)\
	DIPS_HELPER( 0x02, "3 #   RED",         KEYCODE_3)\
	DIPS_HELPER( 0x01, "1 !   BLK",         KEYCODE_1)\
	PORT_START \
	DIPS_HELPER( 0x80, "RETURN",            KEYCODE_ENTER)\
	DIPS_HELPER( 0x40, "*",                 KEYCODE_ASTERISK)\
	DIPS_HELPER( 0x20, "P",                 KEYCODE_P)\
	DIPS_HELPER( 0x10, "I",                 KEYCODE_I)\
	DIPS_HELPER( 0x08, "Y",                 KEYCODE_Y)\
	DIPS_HELPER( 0x04, "R",                 KEYCODE_R)\
	DIPS_HELPER( 0x02, "W",                 KEYCODE_W)\
	DIPS_HELPER( 0x01, "Arrow-Left",        KEYCODE_TILDE)\
	PORT_START \
	DIPS_HELPER( 0x80, "CRSR-RIGHT LEFT",   KEYCODE_6_PAD)\
	DIPS_HELPER( 0x40, "; ]",               KEYCODE_QUOTE)\
	DIPS_HELPER( 0x20, "L",                 KEYCODE_L)\
	DIPS_HELPER( 0x10, "J",                 KEYCODE_J)\
	DIPS_HELPER( 0x08, "G",                 KEYCODE_G)\
	DIPS_HELPER( 0x04, "D",                 KEYCODE_D)\
	DIPS_HELPER( 0x02, "A",                 KEYCODE_A)\
	DIPS_HELPER( 0x01, "CTRL",              KEYCODE_RCONTROL)\
	PORT_START \
	DIPS_HELPER( 0x80, "CRSR-DOWN UP",      KEYCODE_2_PAD)\
	DIPS_HELPER( 0x40, "/ ?",               KEYCODE_SLASH)\
	DIPS_HELPER( 0x20, ", <",               KEYCODE_COMMA)\
	DIPS_HELPER( 0x10, "N",                 KEYCODE_N)\
	DIPS_HELPER( 0x08, "V",                 KEYCODE_V)\
	DIPS_HELPER( 0x04, "X",                 KEYCODE_X)\
	DIPS_HELPER( 0x02, "Left-Shift",        KEYCODE_LSHIFT)\
	DIPS_HELPER( 0x01, "STOP RUN",          KEYCODE_TAB)\
	PORT_START \
	DIPS_HELPER( 0x80, "f1 f2",             KEYCODE_F1)\
	DIPS_HELPER( 0x40, "Right-Shift",       KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x20, ". >",               KEYCODE_STOP)\
	DIPS_HELPER( 0x10, "M",                 KEYCODE_M)\
	DIPS_HELPER( 0x08, "B",                 KEYCODE_B)\
	DIPS_HELPER( 0x04, "C",                 KEYCODE_C)\
	DIPS_HELPER( 0x02, "Z",                 KEYCODE_Z)\
	DIPS_HELPER( 0x01, "Space",             KEYCODE_SPACE)\
	PORT_START \
	DIPS_HELPER( 0x80, "f3 f4",             KEYCODE_F2)\
	DIPS_HELPER( 0x40, "=",                 KEYCODE_BACKSLASH)\
	DIPS_HELPER( 0x20, ": [",               KEYCODE_COLON)\
	DIPS_HELPER( 0x10, "K",                 KEYCODE_K)\
	DIPS_HELPER( 0x08, "H",                 KEYCODE_H)\
	DIPS_HELPER( 0x04, "F",                 KEYCODE_F)\
	DIPS_HELPER( 0x02, "S",                 KEYCODE_S)\
	DIPS_HELPER( 0x01, "CBM",               KEYCODE_RALT)\
	PORT_START \
	DIPS_HELPER( 0x80, "f5 f6",             KEYCODE_F3)\
	DIPS_HELPER( 0x40, "Arrow-Up Pi",       KEYCODE_CLOSEBRACE)\
	DIPS_HELPER( 0x20, "At",                KEYCODE_OPENBRACE)\
	DIPS_HELPER( 0x10, "O",                 KEYCODE_O)\
	DIPS_HELPER( 0x08, "U",                 KEYCODE_U)\
	DIPS_HELPER( 0x04, "T",                 KEYCODE_T)\
	DIPS_HELPER( 0x02, "E",                 KEYCODE_E)\
	DIPS_HELPER( 0x01, "Q",                 KEYCODE_Q)\
	PORT_START \
	DIPS_HELPER( 0x80, "f7 f8",             KEYCODE_F4)\
	DIPS_HELPER( 0x40, "HOME CLR",          KEYCODE_EQUALS)\
	DIPS_HELPER( 0x20, "-",                 KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x10, "0     RVS-OFF",     KEYCODE_0)\
	DIPS_HELPER( 0x08, "8 (   YEL",         KEYCODE_8)\
	DIPS_HELPER( 0x04, "6 &   GRN",         KEYCODE_6)\
	DIPS_HELPER( 0x02, "4 $   CYN",         KEYCODE_4)\
	DIPS_HELPER( 0x01, "2 \"   WHT",        KEYCODE_2)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPF_TOGGLE, \
		   "SHIFT-LOCK (switch)", KEYCODE_CAPSLOCK, IP_JOY_NONE )\
	DIPS_HELPER2( 0x40, "RESTORE",               KEYCODE_PRTSCR)\
	DIPS_HELPER2( 0x20, "Special CRSR Up",       KEYCODE_8_PAD)\
	DIPS_HELPER2( 0x10, "Special CRSR Left",     KEYCODE_4_PAD)\
	DIPS_HELPER2( 0x08, "Quickload",       KEYCODE_F8)\
	DIPS_HELPER2( 0x04, "Tape Drive Play",       KEYCODE_F5)\
	DIPS_HELPER2( 0x02, "Tape Drive Record",     KEYCODE_F6)\
	DIPS_HELPER2( 0x01, "Tape Drive Stop",       KEYCODE_F7)\
	PORT_START \
	PORT_DIPNAME ( 0x07, 0, "RAM Cartridge")\
	PORT_DIPSETTING(	0, "None" )\
	PORT_DIPSETTING(	1, "3k" )\
	PORT_DIPSETTING(	2, "8k" )\
	PORT_DIPSETTING(	3, "16k" )\
	PORT_DIPSETTING(	4, "32k" )\
	PORT_DIPSETTING(	5, "Custom" )\
	PORT_DIPNAME   ( 0x08, 0, " Ram at 0x0400 till 0x0fff")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x08, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x10, 0, " Ram at 0x2000 till 0x3fff")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x10, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x20, 0, " Ram at 0x4000 till 0x5fff")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x40, 0, " Ram at 0x6000 till 0x7fff")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x40, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x80, 0, " Ram at 0xa000 till 0xbfff")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )\
	PORT_START \
	PORT_DIPNAME   ( 0x80, 0x80, "Joystick")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x40, 0x40, "Paddles")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x40, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x20, 0x00, "Lightpen")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x10, 0x10, " Draw Pointer")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x10, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x08, 0x08, "Tape Drive/Device 1")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x08, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x04, 0x00, " Tape Sound")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x04, DEF_STR( Yes ) )\
	PORT_DIPNAME ( 0x02, 0x02, "Serial/Dev 8/VC1541 Floppy")\
	PORT_DIPSETTING(  0, DEF_STR( No ) )\
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )\
	PORT_DIPNAME ( 0x01, 0x01, "Serial/Dev 9/VC1541 Floppy")\
	PORT_DIPSETTING(  0, DEF_STR( No ) )\
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2, "Lightpen Signal", KEYCODE_LALT, 0)

INPUT_PORTS_START (vic20)
	 DIPS_BOTH
     PORT_START							   /* in 16 lightpen X */
     PORT_ANALOGX (0xff, 0, IPT_PADDLE | IPF_PLAYER3,
				   30, 2, 0, (VIC6560_MAME_XSIZE - 1),
				   KEYCODE_LEFT, KEYCODE_RIGHT,
				   JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	 PORT_START							   /* in 17 lightpen Y */
     PORT_ANALOGX (0xff, 0, IPT_PADDLE | IPF_PLAYER4,
				   30, 2, 0, (VIC6560_MAME_YSIZE - 1),
				   KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)

INPUT_PORTS_END

INPUT_PORTS_START (vc20)
	 DIPS_BOTH
     PORT_START							   /* in 16 lightpen X */
     PORT_ANALOGX (0xff, 0, IPT_PADDLE | IPF_PLAYER3,
				   30, 2, 0, (VIC6561_MAME_XSIZE - 1),
				   KEYCODE_LEFT, KEYCODE_RIGHT,
				   JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	 PORT_START							   /* in 17 lightpen Y */
     PORT_ANALOGX (0x1ff, 0, IPT_PADDLE | IPF_PLAYER4,
				   30, 2, 0, (VIC6561_MAME_YSIZE - 1),
				   KEYCODE_UP, KEYCODE_DOWN,
				   JOYCODE_1_UP, JOYCODE_1_DOWN)
INPUT_PORTS_END


/* Initialise the vc20 palette */
static void vc20_init_palette (unsigned char *sys_palette,
							   unsigned short *sys_colortable,
							   const unsigned char *color_prom)
{
	memcpy (sys_palette, vic6560_palette, sizeof (vic6560_palette));
/*  memcpy(sys_colortable,colortable,sizeof(colortable)); */
}

#if 0
     /* 901460-03 */
	 ROM_LOAD ("chargen.80", 0x8000, 0x1000, 0x83e032a6)
	 /* 901486-01 */
	 ROM_LOAD ("basic.80", 0xc000, 0x2000, 0xdb4c43c1)
	 /* 901486-06 ntsc */
	 ROM_LOAD ("kernal6.c0", 0xe000, 0x2000, 0xe5e7c174)
	 /* 901486-07 pal */
	 ROM_LOAD ("kernal7p.c0", 0xe000, 0x2000, 0x4be07cb4)

	 /* patched pal system for swedish/finish keyboard and chars */
	 ROM_LOAD ("char.80", 0x8000, 0x1000, 0xd808551d)
	 ROM_LOAD ("kernal7p.c0", 0xe000, 0x2000, 0xb2a60662)
#endif

ROM_START (vic20)
	 ROM_REGION (0x10000, REGION_CPU1)
	 ROM_LOAD ("chargen.80", 0x8000, 0x1000, 0x83e032a6)
	 ROM_LOAD ("basic.c0", 0xc000, 0x2000, 0xdb4c43c1)
	 ROM_LOAD ("kernal6.e0", 0xe000, 0x2000, 0xe5e7c174)
#ifdef VC1541
	 VC1540_ROM (REGION_CPU2)
#endif
ROM_END

ROM_START (vc20)
	 ROM_REGION (0x10000, REGION_CPU1)
	 ROM_LOAD ("chargen.80", 0x8000, 0x1000, 0x83e032a6)
	 ROM_LOAD ("basic.c0", 0xc000, 0x2000, 0xdb4c43c1)
	 ROM_LOAD ("kernal7p.e0", 0xe000, 0x2000, 0x4be07cb4)
#ifdef VC1541
	 VC1540_ROM (REGION_CPU2)
#endif
ROM_END

static struct MachineDriver machine_driver_vic20 =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			VIC6560_CLOCK,
			vc20_readmem, vc20_writemem,
			0, 0,
			vc20_frame_interrupt, 1,
			vic656x_raster_interrupt, VIC656X_HRETRACERATE,
		},
#ifdef VC1541
		VC1540_CPU,
#endif
	},
	VIC6560_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,
	vc20_init_machine,
	vc20_shutdown_machine,

  /* video hardware */
	(VIC6560_XSIZE + 7) & ~7,		   /* screen width */
	VIC6560_YSIZE,					   /* screen height */
	{VIC6560_MAME_XPOS, VIC6560_MAME_XPOS + VIC6560_MAME_XSIZE - 1,
	 VIC6560_MAME_YPOS, VIC6560_MAME_YPOS + VIC6560_MAME_YSIZE - 1},
	0,								   /* graphics decode info */
	sizeof (vic6560_palette) / sizeof (vic6560_palette[0]) / 3,
	0,
	vc20_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic6560_vh_start,
	vic6560_vh_stop,
	vic6560_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{SOUND_CUSTOM, &vic6560_sound_interface},
		{SOUND_DAC, &vc20tape_sound_interface}
	}
};

static struct MachineDriver machine_driver_vc20 =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			VIC6561_CLOCK,
			vc20_readmem, vc20_writemem,
			0, 0,
			vc20_frame_interrupt, 1,
			vic656x_raster_interrupt, VIC656X_HRETRACERATE,
		},
#ifdef VC1541
		VC1540_CPU,
#endif
	},
	VIC6561_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,
	vc20_init_machine,
	vc20_shutdown_machine,

  /* video hardware */
	(VIC6561_XSIZE + 7) & ~7,		   /* screen width */
	VIC6561_YSIZE,					   /* screen height */
	{VIC6561_MAME_XPOS, VIC6561_MAME_XPOS + VIC6561_MAME_XSIZE - 1,
	 VIC6561_MAME_YPOS, VIC6561_MAME_YPOS + VIC6561_MAME_YSIZE - 1},	/* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic6560_palette) / sizeof (vic6560_palette[0]) / 3,
	0,
	vc20_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic6560_vh_start,
	vic6560_vh_stop,
	vic6560_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{SOUND_CUSTOM, &vic6560_sound_interface},
		{SOUND_DAC, &vc20tape_sound_interface}
	}
};

static const struct IODevice io_vc20[] =
{
	IODEVICE_CBM_QUICK,
	{
		IO_CARTSLOT,				   /* type */
		2,							   /* normal 1 *//* count */
		NULL,						   /* file extensions */
		NULL,						   /* private */
		vc20_rom_id,				   /* id */
		vc20_rom_load,				   /* init */
		NULL,						   /* exit */
		NULL,						   /* info */
		NULL,						   /* open */
		NULL,						   /* close */
		NULL,						   /* status */
		NULL,						   /* seek */
		NULL,				/* tell */
        NULL,                          /* input */
		NULL,						   /* output */
		NULL,						   /* input_chunk */
		NULL						   /* output_chunk */
	},
	IODEVICE_VC20TAPE,
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

#define init_vc20   vc20_driver_init
#define init_vic20  vic20_driver_init
#define io_vic20    io_vc20

#ifdef PET_TEST_CODE
/*	   YEAR  NAME	 PARENT  MACHINE INPUT	 INIT	 COMPANY							 FULLNAME */
COMP (1981, vic20,	0,		vic20,	vic20,	vic20,	"Commodore Business Machines Co.",  "Commodore VIC20 (NTSC)")
COMP (1981, vc20,	vic20,	vc20,	vc20,	vc20,	"Commodore Business Machines Co.",  "Commodore VC20 (PAL)")
#else
COMPX(1981, vic20,	0,		vic20,	vic20,	vic20,	"Commodore Business Machines Co.", "Commodore VIC20 (NTSC)",GAME_IMPERFECT_SOUND)
COMPX(1981, vc20,	vic20,	vc20,	vc20,	vc20,	"Commodore Business Machines Co.", "Commodore VC20 (PAL)",  GAME_IMPERFECT_SOUND)
#endif
