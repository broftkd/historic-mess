/***************************************************************************

  machine/dai.c

  Functions to emulate general aspects of DAI (RAM, ROM, interrupts, I/O ports)

  Krzysztof Strzecha
  Nathan Woods

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "devices/cassette.h"
#include "cpu/i8085/i8085.h"
#include "includes/dai.h"
#include "includes/pit8253.h"
#include "machine/8255ppi.h"
#include "machine/tms5501.h"

#if DAI_DEBUG
#define LOG_SOUND	0
#define LOG_PADDLE	0
#define LOG_TAPE	0
#define LOG_MEMORY	0
#define LOG_IO_ERRORS	1
#else /* !DAI_DEBUG */
#define LOG_SOUND	0
#define LOG_PADDLE	0
#define LOG_TAPE	0
#define LOG_MEMORY	0
#define LOG_IO_ERRORS	0
#endif /* DAI_DEBUG */

/* Discrete I/O devices */
static UINT8 dai_noise_volume;
static UINT8 dai_osc_volume[3];
static UINT8 dai_paddle_select;
static UINT8 dai_paddle_enable;
static UINT8 dai_cassette_motor[2];


static OPBASE_HANDLER(dai_opbaseoverride)
{
	tms5501_set_pio_bit_7 (0, (readinputport(8) & 0x04) ? 1:0);
	return address;
}

/* Memory */
static void dai_update_memory (int dai_rom_bank)
{
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_w(2, 0, MWA_ROM);

	cpu_setbank(2, memory_region(REGION_CPU1) + 0x010000 + dai_rom_bank*0x1000);
}

static void dai_bootstrap_callback (int param)
{
	cpunum_set_reg(0, I8080_PC, 0xc000);
}

static UINT8 dai_keyboard_scan_mask = 0;

static UINT8 dai_keyboard_read (void)
{
	UINT8 data = 0x00;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (dai_keyboard_scan_mask & (1 << i))
			data |= readinputport(i);
	}
	return data;
}

static void dai_keyboard_write (UINT8 data)
{
	dai_keyboard_scan_mask = data;
}

static void dai_interrupt_callback(int intreq, UINT8 vector)
{
	if (intreq)
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, vector);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static const tms5501_init_param tms5501_init_param_dai =
{
	dai_keyboard_read,
	dai_keyboard_write,
	dai_interrupt_callback,
	2000000.
};

static ppi8255_interface ppi82555_intf =
{
	1, 					/* 1 chip */
	{ NULL, NULL },		/* Port A read */
	{ NULL, NULL },		/* Port B read */
	{ NULL, NULL },		/* Port C read */
};

static PIT8253_CONFIG pit8253_intf =
{
	TYPE8253,
	{
		{ 0.0, NULL, NULL }
	}
};

MACHINE_INIT( dai )
{
	memory_set_opbase_handler(0, dai_opbaseoverride);

	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);

	memory_set_bankhandler_w(1, 0, MWA_BANK1);
	memory_set_bankhandler_w(2, 0, MWA_ROM);

	cpu_setbank(1, mess_ram);
	cpu_setbank(2, memory_region(REGION_CPU1) + 0x010000);

	tms5501_init(0, &tms5501_init_param_dai);
	ppi8255_init(&ppi82555_intf);
	pit8253_config(0, &pit8253_intf);

	timer_set(0, 0, dai_bootstrap_callback);
}

/***************************************************************************

	Discrete Devices IO

	FD00	POR1:	IN	bit 0	-
				bit 1	-
				bit 2	PIPGE: Page signal
				bit 3	PIDTR: Serial output ready
				bit 4	PIBU1: Button on paddle 1 (1 = closed)
				bit 5	PIBU1: Button on paddle 2 (1 = closed)
				bit 6	PIRPI: Random data
				bit 7	PICAI: Cassette input data

	FD01	PDLST:	IN	Single pulse used to trigger paddle timer circuit

	FD04	POR1:	OUT	bit 0-3	Volume oscillator channel 0
				bit 4-7	Volume oscillator channel 1

	FD05	POR1:	OUT	bit 0-3	Volume oscillator channel 2
				bit 4-7	Volume random noise generator

	FD06	POR0:	OUT	bit 0	POCAS: Cassette data output
				bit 1-2	PDLMSK: Paddle select
				bit 3	PDPNA:	Paddle enable
				bit 4	POCM1:	Cassette 1 motor control (0 = run)
				bit 5	POCM2:	Cassette 2 motor control (0 = run)
				bit 6-7			ROM bank switching

***************************************************************************/

READ_HANDLER( dai_io_discrete_devices_r )
{
	data8_t data = 0x00;

	switch(offset & 0x000f) {
	case 0x00:
		data = readinputport(8);
		data |= 0x08;	// serial ready
		logerror ("Discrete devices read 0xfd00: %02x\n", data);
		if (device_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 255)
			data |= 0x80;
		break;

	default:
		data = 0xff;
		break;
	}
	return data;
}

WRITE_HANDLER( dai_io_discrete_devices_w )
{
	switch(offset & 0x000f) {
	case 0x04:
		dai_osc_volume[0] = data&0x0f;
		dai_osc_volume[1] = (data&0xf0)>>4;
#if LOG_SOUND
		logerror ("Osc. 0 volume: %02x\n", dai_osc_volume[0]);
		logerror ("Osc. 1 volume: %02x\n", dai_osc_volume[1]);
#endif
		break;

	case 0x05:
		dai_osc_volume[2] = data&0x0f;
		dai_noise_volume = (data&0xf0)>>4;
#if LOG_SOUND
		logerror ("Osc. 2 volume: %02x\n", dai_osc_volume[0]);
		logerror ("Osc. noise generator volume: %02x\n", dai_noise_volume);
#endif
		break;

	case 0x06:
		dai_paddle_select = (data&0x06)>>2;
		dai_paddle_enable = (data&0x08)>>3;
#if LOG_PADDLE
		logerror ("Paddle select: %02x\n", dai_paddle_select);
		logerror ("Paddle enable: %02x\n", dai_paddle_enable);
#endif
		dai_cassette_motor[0] = (data&0x10)>>4;
		dai_cassette_motor[1] = (data&0x20)>>5;
		device_status(image_from_devtype_and_index(IO_CASSETTE, 0), !dai_cassette_motor[0]);
		device_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x01) ? -32768 : 32767);
#if LOG_TAPE
		logerror ("Cassette: motor 1: %02x motor 2: %02x\n", dai_cassette_motor[0], dai_cassette_motor[1]);
#endif
		dai_update_memory ((data&0xc0)>>6);
#if LOG_MEMORY
		logerror ("ROM bank: %02x\n", (data&0xc0)>>6);
#endif
		break;

	default:
#if LOG_IO_ERRORS
		logerror("Writing to discrete port: %04x, %02x\n", offset, data);
#endif
		break;
	}
}

/***************************************************************************

	AMD 9911 mathematical coprocesor

***************************************************************************/

READ_HANDLER( amd9511_r )
{
	/* optional and no present at this moment */
	return 0xff;
}

WRITE_HANDLER( amd9511_w )
{
}


DEVICE_LOAD( dai_cassette )
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.initial_status = WAVE_STATUS_MOTOR_INHIBIT;
	args.create_smpfreq = 44100;
	return cassette_init(image, file, &args);
}
