/***************************************************************************

  advision.c

  Machine file to handle emulation of the Atari 7800.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern void advision_vh_write(int data);
extern void advision_vh_update(int data);
extern int advision_vh_hpos;

unsigned char *advision_ram;
int advision_rambank;
int advision_framestart;
int advision_videoenable;
int advision_videobank;

static UINT8 *ROM;

void advision_init_machine(void) {

	advision_ram = memory_region(REGION_CPU1) + 0x2000;
    advision_rambank = 0x300;
    cpu_setbank(1,memory_region(REGION_CPU1) + 0x1000);
    advision_framestart = 0;
    advision_videoenable = 0;
}

int advision_id_rom (int id)
{
    return 0;
}


int advision_load_rom (int id)
{
    const char *rom_name = device_filename(IO_CARTSLOT,id);
    FILE *cartfile;

	if(rom_name==NULL)
	{
		printf("%s requires Cartridge!\n", Machine->gamedrv->name);
		return INIT_FAILED;
    }

    ROM = memory_region(REGION_CPU1);
    cartfile = NULL;
	if (!(cartfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		if (errorlog) fprintf(errorlog,"Advision - Unable to locate cartridge: %s\n",rom_name);
		return 1;
	}
	osd_fread (cartfile, &ROM[0x0000], 4096);
    osd_fclose (cartfile);

	return 0;
}


/****** External RAM ******************************/

int advision_MAINRAM_r(int offset) {
    int d;

    d = advision_ram[advision_rambank + offset];

	/* the video hardware interprets reads as writes */
    if (!advision_videoenable) advision_vh_write(d);
    return d;
}

void advision_MAINRAM_w(int offset, int data) {
    advision_ram[advision_rambank + offset] = data;
}

/***** 8048 Ports ************************/

void advision_putp1(int offset, int data) {

	  ROM = memory_region(REGION_CPU1);
      if (data & 0x04) {
        cpu_setbank(1,&ROM[0x0000]);
      }
      else {
        cpu_setbank(1,&ROM[0x1000]);
      }
      advision_rambank = (data & 0x03) << 8;
}

void advision_putp2(int offset, int data) {

      if ((advision_videoenable == 0x00) && (data & 0x10)) {
		advision_vh_update(advision_vh_hpos);
        advision_vh_hpos++;
        if (advision_vh_hpos > 255) {
            advision_vh_hpos = 0;
            if (errorlog) fprintf(errorlog,"HPOS OVERFLOW\n");
        }
      }
      advision_videoenable = data & 0x10;
	  advision_videobank = (data & 0xE0) >> 5;
}

int  advision_getp1(int offset) {
    int d,in;

    if (errorlog) fprintf(errorlog,"P1 READ PC=%x\n",cpu_get_pc());
    in = input_port_0_r(0);
    d = in | 0x0F;
    if (in & 0x02) d = d & 0xF7;    /* Button 3 */
    if (in & 0x08) d = d & 0xCF;    /* Button 1 */
    if (in & 0x04) d = d & 0xAF;    /* Button 2 */
    if (in & 0x01) d = d & 0x6F;    /* Button 4 */

    d = d & 0xF8;
    return d;
}

int  advision_getp2(int offset) {
    return 0;
}

int  advision_gett0(int offset) {
    return 0;
}

int  advision_gett1(int offset) {
    if (advision_framestart) {
        advision_framestart = 0;
        return 0;
    }
    else {
        return 1;
    }
}
