/*
World Cup 90 bootleg driver
---------------------------

Ernesto Corvi
(ernesto@imagina.com)

CPU #1 : Handles background & foreground tiles, controllers, dipswitches.
CPU #2 : Handles sprites and palette
CPU #3 : Audio. The audio chip is a YM2203. I need help with this!.

Memory Layout:

CPU #1
0000-8000 ROM
8000-9000 RAM
a000-a800 Color Ram for background #1 tiles
a800-b000 Video Ram for background #1 tiles
c000-c800 Color Ram for background #2 tiles
c800-c000 Video Ram for background #2 tiles
e000-e800 Color Ram for foreground tiles
e800-f000 Video Ram for foreground tiles
f800-fc00 Common Ram with CPU #2
fd00-fd00 Stick 1, Coin 1 & Start 1 input port
fd02-fd02 Stick 2, Coin 2 & Start 2 input port
fd06-fc06 Dip Switch A
fd08-fc08 Dip Switch B

CPU #2
0000-c000 ROM
c000-d000 RAM
d000-d800 RAM Sprite Ram
e000-e800 RAM Palette Ram
f800-fc00 Common Ram with CPU #1

CPU #3
0000-0xc000 ROM
???????????

Notes:
-----
The bootleg video hardware is quite different from the original machine.
I could not figure out the encoding of the scrolling for the new
video hardware. The memory positions, in case anyone wants to try, are
the following ( CPU #1 memory addresses ):
fd06: scroll bg #1 X coordinate
fd04: scroll bg #1 Y coordinate
fd08: scroll bg #2 X coordinate
fd0a: scroll bg #2 Y coordinate
fd0e: ????

What i used instead, was the local copy kept in RAM. These values
are the ones the original machine uses. This will differ when trying
to use some of this code to write a driver for a similar tecmo bootleg.

Sprites are also very different. Theres a code snippet in the ROM
that converts the original sprites to the new format, wich only allows
16x16 sprites. That snippet also does some ( nasty ) clipping.

Colors are accurate. The graphics ROMs have been modified severely
and encoded in a different way from the original machine. Even if
sometimes it seems colors are not entirely correct, this is only due
to the crappy artwork of the person that did the bootleg.

Dip switches are not complete and they dont seem to differ from
the original machine.

Last but not least, the set of ROMs i have for Euro League seem to have
the sprites corrupted. The game seems to be exactly the same as the
World Cup 90 bootleg.
*/

#include "driver.h"
#include "vidhrdw/generic.h"

#define TEST_DIPS false /* enable to test unmapped dip switches */

extern unsigned char *wc90b_shared, *wc90b_palette;

extern unsigned char *wc90b_tile_colorram, *wc90b_tile_videoram;
extern unsigned char *wc90b_tile_colorram2, *wc90b_tile_videoram2;
extern unsigned char *wc90b_scroll1xlo, *wc90b_scroll1xhi;
extern unsigned char *wc90b_scroll2xlo, *wc90b_scroll2xhi;
extern unsigned char *wc90b_scroll1ylo, *wc90b_scroll1yhi;
extern unsigned char *wc90b_scroll2ylo, *wc90b_scroll2yhi;

extern int wc90b_tile_videoram_size;
extern int wc90b_tile_videoram_size2;

int wc90b_vh_start( void );
void wc90b_vh_stop ( void );
int wc90b_tile_videoram_r ( int offset );
void wc90b_tile_videoram_w( int offset, int v );
int wc90b_tile_colorram_r ( int offset );
void wc90b_tile_colorram_w( int offset, int v );
int wc90b_tile_videoram2_r ( int offset );
void wc90b_tile_videoram2_w( int offset, int v );
int wc90b_tile_colorram2_r ( int offset );
void wc90b_tile_colorram2_w( int offset, int v );
int wc90b_shared_r ( int offset );
void wc90b_shared_w( int offset, int v );
int wc90b_palette_r ( int offset );
void wc90b_palette_w( int offset, int v );
void wc90b_vh_screenrefresh(struct osd_bitmap *bitmap);


static void wc90b_bankswitch_w(int offset,int data) {
	int bankaddress;

	bankaddress = 0x10000 + ((data & 0xf8) << 8);
	cpu_setbank(1,&RAM[bankaddress]);
}

static void wc90b_bankswitch1_w(int offset,int data) {
	int bankaddress;

	bankaddress = 0x10000 + ((data & 0xf8) << 8);
	cpu_setbank(2,&RAM[bankaddress]);
}

static void wc90b_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(2,/*Z80_NMI_INT*/-1000);
}

static struct MemoryReadAddress wc90b_readmem1[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_RAM }, /* Main RAM */
	{ 0xa000, 0xa7ff, wc90b_tile_colorram_r }, /* bg 1 color ram */
	{ 0xa800, 0xafff, wc90b_tile_videoram_r }, /* bg 1 tile ram */
	{ 0xc000, 0xc7ff, wc90b_tile_colorram2_r }, /* bg 2 color ram */
	{ 0xc800, 0xcfff, wc90b_tile_videoram2_r }, /* bg 2 tile ram */
	{ 0xe000, 0xe7ff, colorram_r }, /* fg color ram */
	{ 0xe800, 0xefff, videoram_r }, /* fg tile ram */
	{ 0xf000, 0xf7ff, MRA_BANK1 },
	{ 0xf800, 0xfbff, wc90b_shared_r },
	{ 0xfd00, 0xfd00, input_port_0_r }, /* Stick 1, Coin 1 & Start 1 */
	{ 0xfd02, 0xfd02, input_port_1_r }, /* Stick 2, Coin 2 & Start 2 */
	{ 0xfd06, 0xfd06, input_port_2_r }, /* DIP Switch A */
	{ 0xfd08, 0xfd08, input_port_3_r }, /* DIP Switch B */
	{ 0xfd00, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress wc90b_readmem2[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc1ff, MRA_RAM },
	{ 0xc200, 0xe1ff, MRA_RAM },
	{ 0xe000, 0xe7ff, wc90b_palette_r },
	{ 0xf000, 0xf7ff, MRA_BANK2 },
	{ 0xf800, 0xfbff, wc90b_shared_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress wc90b_writemem1[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8075, MWA_RAM },
	{ 0x8076, 0x8076, MWA_RAM, &wc90b_scroll1xlo },
	{ 0x8077, 0x8077, MWA_RAM, &wc90b_scroll1xhi },
	{ 0x8078, 0x8078, MWA_RAM, &wc90b_scroll1ylo },
	{ 0x8079, 0x8079, MWA_RAM, &wc90b_scroll1yhi },
	{ 0x807a, 0x807a, MWA_RAM, &wc90b_scroll2xlo },
	{ 0x807b, 0x807b, MWA_RAM, &wc90b_scroll2xhi },
	{ 0x807c, 0x807c, MWA_RAM, &wc90b_scroll2ylo },
	{ 0x807d, 0x807d, MWA_RAM, &wc90b_scroll2yhi },
	{ 0x807e, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa7ff, wc90b_tile_colorram_w, &wc90b_tile_colorram },
	{ 0xa800, 0xafff, wc90b_tile_videoram_w, &wc90b_tile_videoram, &wc90b_tile_videoram_size },
	{ 0xc000, 0xc7ff, wc90b_tile_colorram2_w, &wc90b_tile_colorram2 },
	{ 0xc800, 0xcfff, wc90b_tile_videoram2_w, &wc90b_tile_videoram2, &wc90b_tile_videoram_size2 },
	{ 0xe000, 0xe7ff, colorram_w, &colorram },
	{ 0xe800, 0xefff, videoram_w, &videoram, &videoram_size },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, wc90b_shared_w, &wc90b_shared },
	{ 0xfc00, 0xfc00, wc90b_bankswitch_w },
	{ 0xfd00, 0xfd00, wc90b_sound_command_w },
	/*  */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress wc90b_writemem2[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xe7ff, wc90b_palette_w, &wc90b_palette },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, wc90b_shared_w },
	{ 0xfc00, 0xfc00, wc90b_bankswitch1_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xe800, 0xe800, YM2203_status_port_0_r },
	{ 0xe801, 0xe801, YM2203_read_port_0_r },
	{ 0xec00, 0xec00, YM2203_status_port_1_r },
	{ 0xec01, 0xec01, YM2203_read_port_1_r },
	{ 0xf800, 0xf800, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xe800, 0xe800, YM2203_control_port_0_w },
	{ 0xe801, 0xe801, YM2203_write_port_0_w },
	{ 0xec00, 0xec00, YM2203_control_port_1_w },
	{ 0xec01, 0xec01, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( wc90b_input_ports )
	PORT_START	/* IN0 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "9 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "8 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "7 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0e, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Continue Game countdown speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal (1 sec per number)" )
	PORT_DIPSETTING(    0x00, "Faster (56/60 per number)" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* DSWB */
	PORT_DIPNAME( 0x03, 0x03, "1 Player Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1:00" )
	PORT_DIPSETTING(    0x02, "1:30" )
	PORT_DIPSETTING(    0x03, "2:00" )
	PORT_DIPSETTING(    0x00, "2:30" )
	PORT_DIPNAME( 0x1c, 0x1c, "2 Player Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1:00" )
	PORT_DIPSETTING(    0x14, "1:30" )
	PORT_DIPSETTING(    0x04, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPSETTING(    0x1c, "3:00" )
	PORT_DIPSETTING(    0x08, "3:30" )
	PORT_DIPSETTING(    0x10, "4:00" )
	PORT_DIPSETTING(    0x00, "5:00" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x80, "Japanese" )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 0x4000*8, 0x8000*8, 0xc000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 0x20000*8, 0x40000*8, 0x60000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		(0x1000*8)+0, (0x1000*8)+1, (0x1000*8)+2, (0x1000*8)+3, (0x1000*8)+4, (0x1000*8)+5, (0x1000*8)+6, (0x1000*8)+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		0x800*8, 0x800*8+1*8, 0x800*8+2*8, 0x800*8+3*8, 0x800*8+4*8, 0x800*8+5*8, 0x800*8+6*8, 0x800*8+7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 32*32 characters */
	4096,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 0x20000*8, 0x40000*8, 0x60000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		(16*8)+0, (16*8)+1, (16*8)+2, (16*8)+3, (16*8)+4, (16*8)+5, (16*8)+6, (16*8)+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 8*8+1*8, 8*8+2*8, 8*8+3*8, 8*8+4*8, 8*8+5*8, 8*8+6*8, 8*8+7*8 },
	32*8	/* every char takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      	1*16*16, 16*16 },

	{ 1, 0x10000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x12000, &tilelayout,			2*16*16, 16*16 },

	{ 1, 0x14000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x16000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x18000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x1A000, &tilelayout,			2*16*16, 16*16 },

	{ 1, 0x1C000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x1E000, &tilelayout,			2*16*16, 16*16 },

	{ 1, 0x20000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x22000, &tilelayout,			3*16*16, 16*16 },

	{ 1, 0x24000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x26000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x28000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x2A000, &tilelayout,			3*16*16, 16*16 },

	{ 1, 0x2C000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x2E000, &tilelayout,			3*16*16, 16*16 },

	{ 1, 0x90000, &spritelayout,		0*16*16, 16*16 }, // sprites
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(2,Z80_NMI_INT);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	3000000,	/* 3 MHz ????? */
	{ YM2203_VOL(255,255), YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct MachineDriver wc90b_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,	/* 6.0 Mhz ??? */
			0,
			wc90b_readmem1, wc90b_writemem1,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			6000000,	/* 6.0 Mhz ??? */
			2,
			wc90b_readmem2, wc90b_writemem2,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ???? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* NMIs are triggered by the YM2203 */
								/* IRQs are triggered by the main CPU */
		}

	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 4*16*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	wc90b_vh_start,
	wc90b_vh_stop,
	wc90b_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

ROM_START( wc90b_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "A02.BIN",  0x00000, 0x10000, 0xef0cef88 )	/* c000-ffff is not used */
	ROM_LOAD( "A03.BIN",  0x10000, 0x10000, 0x5551ca5f )	/* banked at f000-f7ff */

	ROM_REGION(0x110000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "A06.BIN",  0x00000, 0x04000, 0xaddacdf6 )
	ROM_LOAD( "A08.BIN",  0x04000, 0x04000, 0xb141722b )
	ROM_LOAD( "A10.BIN",  0x08000, 0x04000, 0xa1981564 )
	ROM_LOAD( "A20.BIN",  0x0C000, 0x04000, 0x6f22be54 )

	ROM_LOAD( "A07.BIN",  0x10000, 0x20000, 0x9008f22c )
	ROM_LOAD( "A09.BIN",  0x30000, 0x20000, 0x6e26fe7a )
	ROM_LOAD( "A11.BIN",  0x50000, 0x20000, 0x1476fe84 )
	ROM_LOAD( "A21.BIN",  0x70000, 0x20000, 0x8a404c94 )

	ROM_LOAD( "146_A12.BIN",  0x090000, 0x10000, 0xdd097e9d )
	ROM_LOAD( "147_A13.BIN",  0x0a0000, 0x10000, 0x7378c0c0 )
	ROM_LOAD( "148_A14.BIN",  0x0b0000, 0x10000, 0x20efeb67 )
	ROM_LOAD( "149_A15.BIN",  0x0c0000, 0x10000, 0x1ea1204d )
	ROM_LOAD( "150_A16.BIN",  0x0d0000, 0x10000, 0x18da1942 )
	ROM_LOAD( "151_A17.BIN",  0x0e0000, 0x10000, 0xe1776b79 )
	ROM_LOAD( "152_A18.BIN",  0x0f0000, 0x10000, 0x872a2e5a )
	ROM_LOAD( "153_A19.BIN",  0x100000, 0x10000, 0x122c1dde )

	ROM_REGION(0x20000)	/* 96k for code */  /* Second CPU */
	ROM_LOAD( "A04.BIN",  0x00000, 0x10000, 0xfde1ea3f )	/* c000-ffff is not used */
	ROM_LOAD( "A05.BIN",  0x10000, 0x10000, 0xe9b1aeb7 )	/* banked at f000-f7ff */

	ROM_REGION(0x10000)	/* 192k for the audio CPU */
	ROM_LOAD( "A01.BIN",  0x00000, 0x10000, 0x746c3214 )
ROM_END


struct GameDriver wc90b_driver =
{
	"World Cup 90 (bootleg)",
	"wc90b",
    "Ernesto Corvi",
	&wc90b_machine_driver,

	wc90b_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	wc90b_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
