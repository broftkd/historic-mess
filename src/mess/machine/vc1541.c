/***************************************************************************

       commodore vc1541 floppy disk drive

***************************************************************************/

/*
 vc1540
  higher serial bus timing than the vc1541, to fast for c64
 vc1541
  (also build in in sx64)
  used with commodore vc20, c64, c16, c128
  floppy disk drive 5 1/4 inch, single sided, double density, 
   40 tracks (tone head to big to use 80 tracks?)
  gcr encoding

  computer M6502, 2x VIA6522, 2 KByte RAM, 16 KByte ROM
  1 Commodore serial bus (2 connectors)

 vc1541 ieee488 hardware modification
  additional ieee488 connection, modified rom

 vc1541 II
  ?

 dolphin drives
  vc1541 clone?
  additional 8 KByte RAM at 0x8000 (complete track buffer ?)
  24 KByte rom

 c1551
  used with commodore c16
  m6510 processor, VIA6522, CIA6526, 2 KByte RAM, 16 KByte ROM
  connector commodore C16 expansion cartridge
  IEEE488 protocoll

 1750
  single sided 1751
 1751
  (also build in in c128d series)
  used with c128 (vic20,c16,c64 possible?)
  double sided
  con read some other disk formats (mfm encoded?)
  modified commodore serial bus (with quick serial transmission)
  (m6510?, cia6526?)

 1581
  3 1/2 inch, double sided, double density, 80 tracks
  used with c128 (vic20,c16,c64 possible?)
  only mfm encoding?
 */


#include "cpu/m6502/m6502.h"

#define VERBOSE_DBG 1
#include "cbm.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/6522via.h"
#include "driver.h"

/*
 * only for testing at the moment
 */

#define WRITEPROTECTED 0

/*
 * gcr encoding 4 bits to 5 bits
 * 1 change, 0 none
 * 
 * physical encoding of the data on a track
 * sector header
 * sync (5x 0xff not gcr encoded)
 * sync mark (0x08)
 * checksum
 * sector#
 * track#
 * id2 (disk id to prevent writing to disk after disk change)
 * id1
 * 0x0f
 * 0x0f
 * cap normally 10 (min 5) byte?
 * sector data
 * sync (5x 0xff not gcr encoded)
 * sync mark (0x07)
 * 256 bytes
 * checksum
 * cap
 * 
 * max 42 tracks, stepper resolution 84 tracks
 */

#if 0
static int gcr[] =
{
	0xa, 0xb, 0x12, 0x13, 0xe, 0xf, 0x16, 0x17,
	9, 0x19, 0x1a, 0x1b, 0xd, 0x1d, 0x1e, 0x15
};
#endif

/* 0 summarized, 1 computer, 2 vc1541 */
static struct
{
	int atn[3], data[3], clock[3];
}
serial;

static struct
{
	int cpunumber;
	int via0irq, via1irq;
	int deviceid;
	int serial_atn, serial_clock, serial_data;
	int led, motor, stepper, timer;
}
vc1541_static, *vc1541 = &vc1541_static;

struct MemoryReadAddress vc1541_readmem[] =
{
	{0x0000, 0x07ff, MRA_RAM},
	{0x1800, 0x180f, via_2_r},		   /* 0 and 1 used in vc20 */
	{0x1810, 0x189f, MRA_NOP}, /* for debugger */
	{0x1c00, 0x1c0f, via_3_r},
	{0x1c10, 0x1c9f, MRA_NOP}, /* for debugger */
	{0xc000, 0xffff, MRA_ROM},
	{-1}							   /* end of table */
};

struct MemoryWriteAddress vc1541_writemem[] =
{
	{0x0000, 0x07ff, MWA_RAM},
	{0x1800, 0x180f, via_2_w},
	{0x1c00, 0x1c0f, via_3_w},
	{0xc000, 0xffff, MWA_ROM},
	{-1}							   /* end of table */
};

struct MemoryReadAddress dolphin_readmem[] =
{
	{0x0000, 0x07ff, MRA_RAM},
	{0x1800, 0x180f, via_2_r},		   /* 0 and 1 used in vc20 */
	{0x1c00, 0x1c0f, via_3_r},
	{0x8000, 0x9fff, MRA_RAM},
	{0xa000, 0xffff, MRA_ROM},
	{-1}							   /* end of table */
};

struct MemoryWriteAddress dolphin_writemem[] =
{
	{0x0000, 0x07ff, MWA_RAM},
	{0x1800, 0x180f, via_2_w},
	{0x1c00, 0x1c0f, via_3_w},
	{0x8000, 0x9fff, MWA_RAM},
	{0xa000, 0xffff, MWA_ROM},
	{-1}							   /* end of table */
};

struct MemoryReadAddress c1551_readmem[] =
{
#if 0
    {0x0000, 0x0001, m6510_port_r},
#endif
	{0x0002, 0x07ff, MRA_RAM},
	{0x1800, 0x180f, via_2_r},		   /* 0 and 1 used in vc20 */
	{0x1c00, 0x1c0f, via_3_r},
#if 0
    {0x4000, 0x4007, tia6523_port_r},
#endif
	{0xc000, 0xffff, MRA_ROM},
	{-1}							   /* end of table */
};

struct MemoryWriteAddress c1551_writemem[] =
{
#if 0
    {0x0000, 0x0001, m6510_port_w},
#endif
	{0x0002, 0x07ff, MWA_RAM},
	{0x1800, 0x180f, via_2_w},
	{0x1c00, 0x1c0f, via_3_w},
#if 0
    {0x4000, 0x4007, tia6523_port_w},
#endif
	{0xc000, 0xffff, MWA_ROM},
	{-1}							   /* end of table */
};

#if 0
INPUT_PORTS_START (vc1541)
PORT_START
PORT_DIPNAME (0x60, 0x00, "Device #", IP_KEY_NONE)
PORT_DIPSETTING (0x00, "8")
PORT_DIPSETTING (0x40, "9")
PORT_DIPSETTING (0x20, "10")
PORT_DIPSETTING (0x60, "11")
INPUT_PORTS_END
#endif

/*
 * via 6522 at 0x1800
 * port b 
 * 0 inverted serial data in
 * 1 inverted serial data out
 * 2 inverted serial clock in
 * 3 inverted serial clock out
 * 4 inverted serial atn out
 * 5 input device id 1
 * 6 input device id 2
 * id 1+id 2/0+0 devicenumber 8/0+1 9/1+0 10/1+1 11
 * 7 inverted serial atn in
 * also ca1 (books says cb2)
 * irq to m6502 irq connected (or with second via irq)
 */
static void vc1541_via0_irq (int level)
{
	vc1541->via0irq = level;
	cpu_set_irq_line (vc1541->cpunumber, 
					  M6502_INT_IRQ, vc1541->via1irq || vc1541->via0irq);
}

static int vc1541_via0_read_portb (int offset)
{
	int value = 0xff;

	if (!vc1541->serial_data && serial.data[0])
		value &= ~1;
	if (!vc1541->serial_clock && serial.clock[0])
		value &= ~4;
	if (serial.atn[0]) value &= ~0x80;

	DBG_LOG(2, "vc1541 serial read",(errorlog, "%s %s %s\n", 
									 serial.atn[0]?"ATN":"atn",
									 serial.clock[0]?"CLOCK":"clock",
									 serial.data[0]?"DATA":"data"));

	switch (vc1541->deviceid)
	{
	case 8:
		value &= ~0x60;
		break;
	case 9:
		value &= ~0x20;
		break;
	case 10:
		value &= ~0x40;
		break;
	case 11:
		break;
	}

	return value;
}

static void vc1541_via0_write_portb (int offset, int data)
{
	DBG_LOG(1, "vc1541 serial write",(errorlog, "%s %s %s\n", 
									 data&0x10?"ATN":"atn",
									 data&8?"CLOCK":"clock",
									 data&2?"DATA":"data"));

	if ((!(data & 2)) != vc1541->serial_data)
	{
		vc1541_serial_data_write (1, vc1541->serial_data = !(data & 2));
	}
	if ((!(data & 8)) != vc1541->serial_clock)
	{
		vc1541_serial_clock_write (1, vc1541->serial_clock = !(data & 8));
	}
	if ((!(data & 0x10)) != vc1541->serial_atn)
	{
		vc1541_serial_atn_write (1, vc1541->serial_atn = !(data & 0x10));
	}
}

/* 
 * via 6522 at 0x1c00
 * port b
 * 0 output steppermotor for drive 1 (not connected)
 * 1 output steppermotor (track)
 * 2 output motor (rotation) (300 revolutions per minute)
 * 3 output led
 * 4 input disk not write protected
 * 5 timer adjustment
 * 6 timer adjustment
 * 4 different speed zones (track dependend)
 * frequency select?
 * 3 slowest
 * 0 highest
 * 7 input sync signal when reading from disk (more then 9 1 bits)
 * ca1 byte ready
 * ca2 set overflow enable for 6502
 * ca3 read/write
 * 
 * irq to m6502 irq connected
 */
static void vc1541_via1_irq (int level)
{
	vc1541->via1irq = level;
	cpu_set_irq_line (vc1541->cpunumber, 
					  M6502_INT_IRQ, vc1541->via1irq || vc1541->via0irq);
}

static int vc1541_via1_read_portb (int offset)
{
	int value = 0xff;

	if (WRITEPROTECTED)
		value &= ~0x10;
	return value;
}

static void vc1541_via1_write_portb (int offset, int data)
{
	vc1541->led = data & 8;
	vc1541->stepper = data & 2;
	vc1541->motor = data & 4;
	vc1541->timer = data & 0x60;
}

static struct via6522_interface via2 =
{
	0,								   /*vc1541_via0_read_porta, */
	vc1541_via0_read_portb,
	0,								   /*via2_read_ca1, */
	0,								   /*via2_read_cb1, */
	0,								   /*via2_read_ca2, */
	0,								   /*via2_read_cb2, */
	0,								   /*via2_write_porta, */
	vc1541_via0_write_portb,
	0,								   /*via2_write_ca2, */
	0,								   /*via2_write_cb2, */
	vc1541_via0_irq
}, via3 =
{
	0,								   /*via3_read_porta, */
	vc1541_via1_read_portb,
	0,								   /*via3_read_ca1, */
	0,								   /*via3_read_cb1, */
	0,								   /*via3_read_ca2, */
	0,								   /*via3_read_cb2, */
	0,								   /*via3_write_porta, */
	vc1541_via1_write_portb,
	0,								   /*via3_write_ca2, */
	0,								   /*via3_write_cb2, */
	vc1541_via1_irq
};

void vc1541_driver_init (int cpunumber)
{
	via_config (2, &via2);
	via_config (3, &via3);
	vc1541->cpunumber = cpunumber;
	vc1541->deviceid = 8;
}

void vc1541_machine_init (void)
{
	int i;

	for (i = 0; i < sizeof (serial.atn) / sizeof (serial.atn[0]); i++)
	{
		serial.atn[i] = serial.data[i] = serial.clock[i] = 1;
	}
	via_reset ();
}

/* delivers status for displaying */
extern void vc1541_drive_status (char *text, int size)
{
#if VERBOSE_DBG
	snprintf (text, size, "%s %s %s %.2x %s %s %s", 
			  vc1541->led ? "LED" : "led",
			  vc1541->stepper ? "STEPPER" : "stepper", 
			  vc1541->motor ? "MOTOR" : "motor",
			  vc1541->timer,
			  serial.atn[0]?"ATN":"atn",
			  serial.clock[0]?"CLOCK":"clock",
			  serial.data[0]?"DATA":"data");
#else
	text[0] = 0;
#endif
	return;
}

void vc1541_serial_reset_write (int which, int level)
{
}

int vc1541_serial_atn_read (int which)
{
	return serial.atn[0];
}

void vc1541_serial_atn_write (int which, int level)
{
	if (serial.atn[1 + which] != level)
	{
		serial.atn[1 + which] = level;
		if (serial.atn[0] != level)
		{
			serial.atn[0] = serial.atn[1] && serial.atn[2];
			if (serial.atn[0] == level)
			{
				DBG_LOG(1, "vc1541",(errorlog, "%d atn %s\n", 
									 cpu_getactivecpu (),
									 serial.atn[0]?"ATN":"atn"));
				via_set_input_ca1 (2, !level);
				cpu_yield ();
			}
		}
	}
}

int vc1541_serial_data_read (int which)
{
	return serial.data[0];
}

void vc1541_serial_data_write (int which, int level)
{
	if (serial.data[1 + which] != level)
	{
		serial.data[1 + which] = level;
		if (serial.data[0] != level)
		{
			serial.data[0] = serial.data[1] && serial.data[2];
			if (serial.data[0] == level)
			{
				DBG_LOG(1, "vc1541",(errorlog, "%d data %s\n", 
									 cpu_getactivecpu (),
									 serial.data[0]?"DATA":"data"));
				cpu_yield ();
			}
		}
	}
}

int vc1541_serial_clock_read (int which)
{
	return serial.clock[0];
}

void vc1541_serial_clock_write (int which, int level)
{
	if (serial.clock[1 + which] != level)
	{
		serial.clock[1 + which] = level;
		if (serial.clock[0] != level)
		{
			serial.clock[0] = serial.clock[1] && serial.clock[2];
			if (serial.clock[0] == level)
			{
				DBG_LOG(1, "vc1541",(errorlog, "%d clock %s\n", 
									 cpu_getactivecpu (),
									 serial.clock[0]?"CLOCK":"clock"));
				cpu_yield ();
			}
		}
	}
}

int vc1541_serial_request_read (int which)
{
	return 1;
}

void vc1541_serial_request_write (int which, int level)
{

}
