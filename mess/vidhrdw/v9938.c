
/***************************************************************************

	v9938 / v9958 emulation

***************************************************************************/

/*
 todo:

 - mode2 sprites
 - graphic 5 -- check sprites and transparency
 - vdp engine -- make run at correct speed
 - vr/hr/fh flags: double-check all of that
 - make vdp engine work in exp. ram
 - interlaced mode
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "v9938.h"

typedef struct {
	/* general */
	int model;
	int offset_x, offset_y, visible_y, mode;
	/* palette */
	int	pal_write_first, cmd_write_first;
	UINT8 pal_write, cmd_write;
	UINT8 palReg[32], statReg[10], contReg[48], read_ahead;
	/* memory */
	UINT16 address_latch;
	UINT8 *vram, *vram_exp;
	int vram_size;
	struct osd_bitmap *bmp;
    /* interrupt */
    UINT8 INT;
    void (*INTCallback)(int);
	int scanline;
    /* blinking */
    int blink, blink_count;
    /* sprites */
    int sprite_limit;
} V9938;

static V9938 vdp;

static UINT16 pal_ind16[16], pal_ind256[256], *pal_indYJK;

#define V9938_MODE_TEXT1	(0)
#define V9938_MODE_MULTI	(1)
#define V9938_MODE_GRAPHIC1	(2)
#define V9938_MODE_GRAPHIC2	(3)
#define V9938_MODE_GRAPHIC3	(4)
#define V9938_MODE_GRAPHIC4	(5)
#define V9938_MODE_GRAPHIC5	(6)
#define V9938_MODE_GRAPHIC6	(7)
#define V9938_MODE_GRAPHIC7	(8)
#define V9938_MODE_TEXT2	(9)
#define V9938_MODE_UNKNOWN	(10)

static const char *v9938_modes[] = {
	"TEXT 1", "MULTICOLOR", "GRAPHIC 1", "GRAPHIC 2", "GRAPHIC 3",
	"GRAPHIC 4", "GRAPHIC 5", "GRAPHIC 6", "GRAPHIC 7", "TEXT 2",
	"UNKNOWN" };

static void v9938_register_write (int reg, int data);
static void v9938_update_command (void);
static void v9938_cpu_to_vdp (UINT8 V);
static UINT8 v9938_command_unit_w (UINT8 Op);
static UINT8 v9938_vdp_to_cpu (void);
static void v9938_set_mode (void);
static void v9938_refresh_line (struct osd_bitmap *bmp, int line);

/***************************************************************************

	Palette functions

***************************************************************************/

/*
About the colour burst registers:

The color burst registers will only have effect on the composite video outputfrom
the V9938. but the output is only NTSC (Never The Same Color ,so the
effects are already present) . this system is not used in europe
the european machines use a separate PAL  (Phase Alternating Line) encoder
or no encoder at all , only RGB output.

Erik de Boer.

--
Right now they're not emulated. For completeness sake they should -- with
a dip-switch to turn them off. I really don't know how they work though. :(
*/

/*
 In screen 8, the colors are encoded as:

 7  6  5  4  3  2  1  0
+--+--+--+--+--+--+--+--+
|g2|g1|g0|r2|r1|r0|b2|b1|
+--+--+--+--+--+--+--+--+

b0 is set if b2 and b1 are set (remember, color bus is 3 bits)

*/

void v9938_init_palette (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
	{
	int	i,red;

	if (Machine->scrbitmap->depth == 8)
		{
		/* create 256 colour palette -- this is actually the graphic 7
		   palette, with duplicate entries so the core fill shrink it to
		   256 colours */
		for (i=0;i<512;i++)
			{
			(*palette++) = (unsigned char)(((i >> 6) & 7) * 36); /* red */
			(*palette++) = (unsigned char)(((i >> 3) & 7) * 36); /* green */
			red = (i & 6); if (red == 6) red++;
			(*palette++) = (unsigned char)red * 36; /* blue */
			}
		}
	else
		{
		/* create the full 512 colour palette */
		for (i=0;i<512;i++)
			{
			(*palette++) = (unsigned char)(((i >> 6) & 7) * 36); /* red */
			(*palette++) = (unsigned char)(((i >> 3) & 7) * 36); /* green */
			(*palette++) = (unsigned char)((i & 7) * 36); /* blue */
			}
		}
	}

/*

The v9958 can display up to 19286 colours. For this we need a larger palette.

The colours are encoded in 17 bits; however there are just 19268 different colours.
Here we calculate the palette and a 2^17 reference table to the palette,
which is: pal_indYJK. It's 256K in size, but I can't think of a faster way
to emulate this. Also it keeps the palette a reasonable size. :)

*/

void v9958_init_palette (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
    {
	int r,g,b,y,j,k,i,k0,j0,n;
	unsigned char *pal;

	pal = palette + 512*3;
	v9938_init_palette (palette, colortable, color_prom);

	/* set up YJK table */
	if (!pal_indYJK)
		{
		pal_indYJK = malloc (0x20000 * sizeof (UINT16) );
		if (!pal_indYJK)
			{
			logerror ("Fatal: cannot malloc () in v9958_init_palette (), cannot exit\n");
			return;
			}
		}

	logerror ("Building YJK table for V9958 screens, may take a while ... \n");
	i = 0;
	for (y=0;y<32;y++) for (k=0;k<64;k++) for (j=0;j<64;j++)
		{
		/* calculate the color */
		if (k >= 32) k0 = (k - 64); else k0 = k;
		if (j >= 32) j0 = (j - 64); else j0 = j;
		r = y + j0;
        b = (y * 5 - 2 * j0 - k0) / 4;
        g = y + k0;
		if (r < 0) r = 0; else if (r > 31) r = 31;
		if (g < 0) g = 0; else if (g > 31) g = 31;
		if (b < 0) b = 0; else if (b > 31) b = 31;

		if (Machine->scrbitmap->depth == 8)
			{
			/* we don't have the space for more entries, so map it to the
			   256 colours we already have */
			r /= 4; g /= 4; b /= 4;
			pal_indYJK[y | j << 5 | k << (5 + 6)] = (r << 6) | (g << 3) | b;
			}
		else
			{
			r = (255 * r) / 31;
			b = (255 * g) / 31;
			g = (255 * r) / 31;
			/* have we seen this one before */
			n = 0;
			while (n < i)
				{
				if (pal[n*3+0] == r && pal[n*3+1] == g && pal[n*3+2] == b)
					{
					pal_indYJK[y | j << 5 | k << (5 + 6)] = n + 512;
					break;
					}
				n++;
				}

			if (i == n)
				{
				/* so we haven't; add it */
				pal[i*3+0] = r;
				pal[i*3+1] = g;
				pal[i*3+2] = b;
				pal_indYJK[y | j << 5 | k << (5 + 6)] = i + 512;
				i++;
				}
			}
		}

	if (i != 19268)
		logerror ("Table creation failed - %d colours out of 19286 created\n", i);
	}

/*

 so lookups for screen 12 will look like:

 int ind;

 ind = (*data & 7) << 11 | (*(data + 1) & 7) << 14 |
	   (*(data + 2) & 7) << 5 | (*(data + 3) & 7) << 8;

 pixel0 = pal_indYJK[ind | (*data >> 3) & 31];
 pixel1 = pal_indYJK[ind | (*(data + 1) >> 3) & 31];
 pixel2 = pal_indYJK[ind | (*(data + 2) >> 3) & 31];
 pixel3 = pal_indYJK[ind | (*(data + 3) >> 3) & 31];

and for screen 11:

pixel0 = (*data) & 8 ? pal_ind16[(*data) >> 4] : pal_indYJK[ind | (*data >> 3) & 30];
pixel1 = *(data+1) & 8 ? pal_ind16[*(data+1) >> 4] : pal_indYJK[ind | *(data+1) >> 3) & 30];
pixel2 = *(data+2) & 8 ? pal_ind16[*(data+2) >> 4] : pal_indYJK[ind | *(data+2) >> 3) & 30];
pixel3 = *(data+3) & 8 ? pal_ind16[*(data+3) >> 4] : pal_indYJK[ind | *(data+3) >> 3) & 30];

*/

WRITE_HANDLER (v9938_palette_w)
	{
	int indexp;

	if (vdp.pal_write_first)
		{
		/* store in register */
		indexp = vdp.contReg[0x10] & 15;
		vdp.palReg[indexp*2] = vdp.pal_write & 0x77;
		vdp.palReg[indexp*2+1] = data & 0x07;
		/* update palette */
		pal_ind16[indexp] = (((int)vdp.pal_write << 2) & 0x01c0)  |
						 	   (((int)data << 3) & 0x0038)  |
						 		((int)vdp.pal_write & 0x0007);

		vdp.contReg[0x10] = (vdp.contReg[0x10] + 1) & 15;
		vdp.pal_write_first = 0;
		}
	else
		{
		vdp.pal_write = data;
		vdp.pal_write_first = 1;
		}
	}

static void v9938_reset_palette (void)
	{
	/* taken from V9938 Technical Data book, page 148. it's in G-R-B format */
	static const UINT8 pal16[16*3] = {
		0, 0, 0, /* 0: black/transparent */
		0, 0, 0, /* 1: black */
		6, 1, 1, /* 2: medium green */
		7, 3, 3, /* 3: light green */
		1, 1, 7, /* 4: dark blue */
		3, 2, 7, /* 5: light blue */
		1, 5, 1, /* 6: dark red */
		6, 2, 7, /* 7: cyan */
		1, 7, 1, /* 8: medium red */
		3, 7, 3, /* 9: light red */
		6, 6, 1, /* 10: dark yellow */
		6, 6, 4, /* 11: light yellow */
		4, 1, 1, /* 12: dark green */
		2, 6, 5, /* 13: magenta */
		5, 5, 5, /* 14: gray */
		7, 7, 7  /* 15: white */
        };
	int i, red, ind;

	for (i=0;i<16;i++)
		{
		/* set the palette registers */
		vdp.palReg[i*2+0] = pal16[i*3+1] << 4 | pal16[i*3+2];
		vdp.palReg[i*2+1] = pal16[i*3];
		/* set the reference table */
		pal_ind16[i] = pal16[i*3+1] << 6 | pal16[i*3] << 3 | pal16[i*3+2];
		}

	/* set internal palette GRAPHIC 7 */
	for (i=0;i<256;i++)
		{
		ind = (i << 4) & 0x01c0;
		ind |= (i >> 2) & 0x0038;
		red = (i << 1) & 6; if (red == 6) red++;
		ind |= red;

		pal_ind256[i] = ind;
		}
	}

/***************************************************************************

	Memory functions

***************************************************************************/

static void v9938_vram_write (int offset, int data)
	{
	int newoffset;

	if ( (vdp.mode == V9938_MODE_GRAPHIC6) || (vdp.mode == V9938_MODE_GRAPHIC7) )
		{
        newoffset = ((offset & 1) << 16) | (offset >> 1);
   		if (newoffset < vdp.vram_size)
        	vdp.vram[newoffset] = data;
		}
	else
		{
		if (offset < vdp.vram_size)
			vdp.vram[offset] = data;
        }
	}

static int v9938_vram_read (int offset)
	{
	if ( (vdp.mode == V9938_MODE_GRAPHIC6) || (vdp.mode == V9938_MODE_GRAPHIC7) )
		return vdp.vram[((offset & 1) << 16) | (offset >> 1)];
	else
		return vdp.vram[offset];
	}

WRITE_HANDLER (v9938_vram_w)
	{
	int address;

	v9938_update_command ();

	vdp.cmd_write_first = 0;

    address = ((int)vdp.contReg[14] << 14) | vdp.address_latch;

    if (vdp.contReg[47] & 0x20)
        {
        if (vdp.vram_exp && address < 0x10000)
            vdp.vram_exp[address] = data;
        }
    else
        {
		v9938_vram_write (address, data);
        }

	vdp.address_latch = (vdp.address_latch + 1) & 0x3fff;
	if (!vdp.address_latch && (vdp.contReg[0] & 0x0c) ) /* correct ??? */
		{
		vdp.contReg[14] = (vdp.contReg[14] + 1) & 7;
		}
	}

READ_HANDLER (v9938_vram_r)
	{
	UINT8 ret;
	int address;

	address = ((int)vdp.contReg[14] << 14) | vdp.address_latch;

	vdp.cmd_write_first = 0;

	ret = vdp.read_ahead;

	if (vdp.contReg[47] & 0x20)
		{
		/* correct? */
		if (vdp.vram_exp && address < 0x10000)
			vdp.read_ahead = vdp.vram_exp[address];
		else
			vdp.read_ahead = 0xff;
		}
	else
		{
		vdp.read_ahead = v9938_vram_read (address);
		}

	vdp.address_latch = (vdp.address_latch + 1) & 0x3fff;
	if (!vdp.address_latch && (vdp.contReg[0] & 0x0c) ) /* correct ??? */
		{
		vdp.contReg[14] = (vdp.contReg[14] + 1) & 7;
		}

	return ret;
	}

WRITE_HANDLER (v9938_command_w)
	{
	if (vdp.cmd_write_first)
		{
		if (data & 0x80)
			v9938_register_write (data & 0x3f, vdp.cmd_write);
		else
			{
			vdp.address_latch =
				(((UINT16)data << 8) | vdp.cmd_write) & 0x3fff;
			if ( !(data & 0x40) ) v9938_vram_r (0); /* read ahead! */
			}

		vdp.cmd_write_first = 0;
		}
	else
		{
		vdp.cmd_write = data;
		vdp.cmd_write_first = 1;
		}
	}

/***************************************************************************

	Init/stop/reset/Interrupt functions

***************************************************************************/

int v9938_init (int model, int vram_size, void (*callback)(int) )
	{
	memset (&vdp, 0, sizeof (vdp) );

	vdp.model = model;
	vdp.vram_size = vram_size;
	vdp.INTCallback = callback;

	/* allocate back-buffer */
	vdp.bmp = osd_alloc_bitmap (512 + 32, (212 + 16) * 2, Machine->scrbitmap->depth);
	if (!vdp.bmp)
		{
		logerror ("V9938: Unable to allocate back-buffer bitmap\n");
		return 1;
		}

	/* allocate VRAM */
	vdp.vram = malloc (0x20000);
	if (!vdp.vram) return 1;
	memset (vdp.vram, 0, 0x20000);
	if (vdp.vram_size < 0x20000)
		{
		/* set unavailable RAM to 0xff */
		memset (vdp.vram + vdp.vram_size, 0xff, (0x20000 - vdp.vram_size) );
		}
	/* do we have expanded memory? */
	if (vdp.vram_size > 0x20000)
		{
		vdp.vram_exp = malloc (0x10000);
		if (!vdp.vram_exp)
			{
			free (vdp.vram);
			return 1;
			}
		memset (vdp.vram_exp, 0, 0x10000);
		}
	else
		vdp.vram_exp = NULL;

	return 0;
	}

void v9938_reset (void)
	{
	int i;

	/* offset reset */
	vdp.offset_x = 14;
	vdp.offset_y = 7 + 10;
	vdp.visible_y = 192;
	/* register reset */
	v9938_reset_palette (); /* palette registers */
	for (i=0;i<10;i++) vdp.statReg[i] = 0;
	vdp.statReg[2] = 0x0c;
	if (vdp.model == MODEL_V9958) vdp.statReg[1] |= 4;
	for (i=0;i<48;i++) vdp.contReg[i] = 0;
	vdp.cmd_write_first = vdp.pal_write_first = 0;
	vdp.INT = 0;
	vdp.read_ahead = 0; vdp.address_latch = 0; /* ??? */
	vdp.scanline = 0;
	}

void v9938_exit (void)
	{
	free (vdp.vram);
	if (vdp.vram_exp) free (vdp.vram_exp);
	if (pal_indYJK)
		{ free (pal_indYJK); pal_indYJK = NULL; }
	}

static void v9938_check_int (void)
	{
	UINT8 n;

	n = ( (vdp.contReg[1] & 0x20) && (vdp.statReg[0] & 0x80) ) ||
		( (vdp.statReg[1] & 0x01) && (vdp.contReg[0] & 0x10) );

	if (n != vdp.INT)
		{
		vdp.INT = n;
		vdp.INTCallback (n);
		logerror ("V9938: IRQ line %s\n", n ? "up" : "down");
		}
	}

void v9938_set_sprite_limit (int i)
	{
	vdp.sprite_limit = i;
	}

/***************************************************************************

	Register functions

***************************************************************************/

WRITE_HANDLER (v9938_register_w)
	{
	int reg;

	reg = vdp.contReg[17] & 0x3f;
	if (reg != 17) v9938_register_write (reg, data); /* true ? */
	if ( !(vdp.contReg[17] & 0x80) )
		vdp.contReg[17] = (vdp.contReg[17] + 1) & 0x3f;
	}

static void v9938_register_write (int reg, int data)
	{
	static UINT8 const reg_mask[] = {
		0x7e, 0x7b, 0x7f, 0xff, 0x3f, 0xff, 0x3f, 0xff,
		0xfb, 0xbf, 0x07, 0x03, 0xff, 0xff, 0x07, 0x0f,
		0x0f, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x00, 0x7f, 0x3f, 0x07 };

	if (reg <= 27)
		{
		data &= reg_mask[reg];
		if (vdp.contReg[reg] == data) return;
		}

	if (reg > 46)
		{
		logerror ("V9938: Attempted to write to non-existant R#%d\n", reg);
		return;
		}

	switch (reg)
		{
		/* registers that affect interrupt and display mode */
		case 0:
		case 1:
			v9938_update_command ();
			vdp.contReg[reg] = data;
			v9938_set_mode ();
			v9938_check_int ();
			logerror ("V9938: mode = %s\n", v9938_modes[vdp.mode]);
			break;
		case 18:
		case 9:
			vdp.contReg[reg] = data;
			/* recalc offset */
			vdp.offset_x = ( (~vdp.contReg[18] - 8) & 0x0f) * 2;
			vdp.offset_y = (~(vdp.contReg[18]>>4) - 8) & 0x0f;
			if (vdp.contReg[9] & 0x80)
				vdp.visible_y = 212;
			else
				{
				vdp.visible_y = 192;
				vdp.offset_y += 10;
				}

			break;
		case 15:
			vdp.pal_write_first = 0;
			break;
		/* color burst registers aren't emulated */
		case 20:
		case 21:
		case 22:
			logerror ("V9938: Write %02xh to R#%d; color burst not emulated\n",
				data, reg);
			break;
		case 25:
		case 26:
		case 27:
			if (vdp.model != MODEL_V9958)
				{
				logerror ("V9938: Attempting to write %02xh to V9958 R#%d\n");
				data = 0;
				}
			break;
		case 44:
			v9938_cpu_to_vdp (data);
			break;
		case 46:
			v9938_command_unit_w (data);
			break;
		}

	if (reg != 15)
		logerror ("V9938: Write %02x to R#%d\n", data, reg);

	vdp.contReg[reg] = data;
	}

READ_HANDLER (v9938_status_r)
	{
	int reg;
	UINT8 ret;

	vdp.cmd_write_first = 0;

	reg = vdp.contReg[15] & 0x0f;
	if (reg > 9)
		return 0xff;

	switch (reg)
		{
		case 0:
			ret = vdp.statReg[0];
			vdp.statReg[0] &= 0x5f;
			break;
		case 1:
			ret = vdp.statReg[1];
			vdp.statReg[1] &= 0xfe;
			break;
		case 2:
			v9938_update_command ();
			if ( (cpu_getcurrentcycles () % 227) > 170) vdp.statReg[2] |= 0x20;
			else vdp.statReg[2] &= ~0x20;
			ret = vdp.statReg[2];
			break;
		case 7:
			ret = vdp.statReg[7];
			vdp.statReg[7] = vdp.contReg[44] = v9938_vdp_to_cpu () ;
			break;
		default:
			ret = vdp.statReg[reg];
			break;
		}

	logerror ("V9938: Read %02x from S#%d\n", ret, reg);
	v9938_check_int ();

	return ret;
	}

/***************************************************************************

	Refresh / render function

***************************************************************************/

#define V9938_SECOND_FIELD ( !(vdp.contReg[9] & 0x04) || (!vdp.blink && (vdp.statReg[2] & 2) ) )

static void v9938_default_border_8 (UINT8 *ln)
	{
	UINT8 pen;

	pen = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];
	memset (ln, pen, 512 + 32);
	}

static void v9938_default_border_16 (UINT16 *ln)
	{
	UINT16 pen;
	int i;

	pen = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];
	i = 512 + 32;
	while (i--) *ln++ = pen;
	}

static void v9938_graphic7_border_8 (UINT8 *ln)
	{
	UINT8 pen;

	pen = Machine->pens[pal_ind256[vdp.contReg[7]]];
	memset (ln, pen, 512 + 32);
	}

static void v9938_graphic7_border_16 (UINT16 *ln)
	{
	UINT16 pen;
	int i;

	pen = Machine->pens[pal_ind256[vdp.contReg[7]]];
	i = 512 + 32;
	while (i--) *ln++ = pen;
	}

static void v9938_graphic5_border_8 (UINT8 *ln)
	{
	UINT8 pen0, pen1;
	int i;

	pen1 = Machine->pens[pal_ind16[(vdp.contReg[7]&0x03)]];
	pen0 = Machine->pens[pal_ind16[((vdp.contReg[7]>>2)&0x03)]];
	i = (512 + 32) / 2;
	while (i--) { *ln++ = pen0; *ln++ = pen1; }
	}

static void v9938_graphic5_border_16 (UINT16 *ln)
	{
	UINT16 pen0, pen1;
	int i;

	pen1 = Machine->pens[pal_ind16[(vdp.contReg[7]&0x03)]];
	pen0 = Machine->pens[pal_ind16[((vdp.contReg[7]>>2)&0x03)]];
	i = (512 + 32) / 2;
	while (i--) { *ln++ = pen0; *ln++ = pen1; }
	}

#define TEXT1(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	int pattern, x, xx, name, xxx;	\
	pen_type fg, bg, pen;	\
	UINT8 *nametbl, *patterntbl;	\
	\
	patterntbl = vdp.vram + (vdp.contReg[4] << 11);	\
	nametbl = vdp.vram + (vdp.contReg[2] << 10);	\
	\
    fg = Machine->pens[pal_ind16[vdp.contReg[7] >> 4]];	\
    bg = Machine->pens[pal_ind16[vdp.contReg[7] & 15]];	\
	\
	name = (line/8)*40;	\
	\
	pen = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];	\
	\
	xxx = vdp.offset_x + 16;	\
	while (xxx--) *ln++ = pen;	\
	\
	for (x=0;x<40;x++)	\
		{	\
		pattern = patterntbl[(nametbl[name] * 8) + 	\
			((line + vdp.contReg[23]) & 7)];	\
		for (xx=0;xx<6;xx++)	\
			{	\
			*ln++ = (pattern & 0x80) ? fg : bg;	\
			*ln++ = (pattern & 0x80) ? fg : bg;	\
			pattern <<= 1;	\
			}	\
		/* width height 212, characters start repeating at the bottom */ \
		name = (name + 1) & 0x3ff;	\
		}	\
	\
	xxx = (32 - vdp.offset_x) + 16;	\
	while (xxx--) *ln++ = pen;	\
	}

TEXT1 (v9938_mode_text1, UINT8)
TEXT1 (v9938_mode_text1_16, UINT16)

#define TEXT2(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	int pattern, x, xx, charcode, name, xxx, patternmask, colourmask;	\
	pen_type fg, bg, fg0, bg0, pen;	\
	UINT8 *nametbl, *patterntbl, *colourtbl;	\
	\
	patterntbl = vdp.vram + (vdp.contReg[4] << 11);	\
	colourtbl = vdp.vram + ((vdp.contReg[3] & 0xf8) << 6) + (vdp.contReg[10] << 14);	\
	colourmask = ((vdp.contReg[3] & 7) << 5) | 0x1f; /* verify! */	\
	nametbl = vdp.vram + ((vdp.contReg[2] & 0xfc) << 10);	\
	patternmask = ((vdp.contReg[2] & 3) << 10) | 0x3ff; /* seems correct */	\
	\
    fg = Machine->pens[pal_ind16[vdp.contReg[7] >> 4]];	\
    bg = Machine->pens[pal_ind16[vdp.contReg[7] & 15]];	\
    fg0 = Machine->pens[pal_ind16[vdp.contReg[13] >> 4]];	\
    bg0 = Machine->pens[pal_ind16[vdp.contReg[13] & 15]];	\
	\
	name = (line/8)*80;	\
	\
	xxx = vdp.offset_x + 16;	\
	pen = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];	\
	while (xxx--) *ln++ = pen;	\
	\
	for (x=0;x<80;x++)	\
		{	\
		charcode = nametbl[name&patternmask];	\
		if (vdp.blink)	\
			{	\
			pattern = colourtbl[(name/8)&colourmask];	\
			if (pattern & (1 << (name & 7) ) )	\
				{	\
				pattern = patterntbl[(charcode * 8) + 	\
					((line + vdp.contReg[23]) & 7)];	\
				for (xx=0;xx<6;xx++)	\
					{	\
					*ln++ = (pattern & 0x80) ? fg0 : bg0;	\
					pattern <<= 1;	\
					}	\
				name++;	\
				continue;	\
				}	\
			}	\
	\
		pattern = patterntbl[(charcode * 8) + 	\
			((line + vdp.contReg[23]) & 7)];	\
		for (xx=0;xx<6;xx++)	\
			{	\
			*ln++ = (pattern & 0x80) ? fg : bg;	\
			pattern <<= 1;	\
			}	\
		name++;	\
		}	\
	\
	xxx = 32 - vdp.offset_x + 16;	\
	while (xxx--) *ln++ = pen;	\
	}

TEXT2 (v9938_mode_text2, UINT8)
TEXT2 (v9938_mode_text2_16, UINT16)

#define MULTI(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	UINT8 *nametbl, *patterntbl, colour;	\
	int name, line2, x, xx;	\
	pen_type pen, pen_bg;	\
	\
	nametbl = vdp.vram + (vdp.contReg[2] << 10);	\
	patterntbl = vdp.vram + (vdp.contReg[4] << 11);	\
	\
	line2 = (line - vdp.contReg[23]) & 255;	\
	name = (line2/8)*32;	\
	\
	pen_bg = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];	\
	xx = vdp.offset_x;	\
	while (xx--) *ln++ = pen_bg;	\
	\
	for (x=0;x<32;x++)	\
		{	\
		colour = patterntbl[(nametbl[name] * 8) + ((line2/4)&7)];	\
		pen = Machine->pens[pal_ind16[colour>>4]];	\
		/* eight pixels */	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		pen = Machine->pens[pal_ind16[colour&15]];	\
		/* eight pixels */	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		name++;	\
		}	\
		\
	xx = 32 - vdp.offset_x;	\
	while (xx--) *ln++ = pen_bg;	\
	}

MULTI (v9938_mode_multi, UINT8)
MULTI (v9938_mode_multi_16, UINT16)

#define GRAPH1(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	pen_type fg, bg, pen;	\
	UINT8 *nametbl, *patterntbl, *colourtbl;	\
	int pattern, x, xx, line2, name, charcode, colour, xxx;	\
	\
	nametbl = vdp.vram + (vdp.contReg[2] << 10);	\
	colourtbl = vdp.vram + (vdp.contReg[3] << 6) + (vdp.contReg[10] << 14);	\
	patterntbl = vdp.vram + (vdp.contReg[4] << 11);	\
	\
	line2 = (line - vdp.contReg[23]) & 255;	\
	\
	name = (line2/8)*32;	\
	\
	pen = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];	\
	xxx = vdp.offset_x;	\
	while (xxx--) *ln++ = pen;	\
	\
	for (x=0;x<32;x++)	\
		{	\
		charcode = nametbl[name];	\
		colour = colourtbl[charcode/8];	\
		fg = Machine->pens[pal_ind16[colour>>4]];	\
		bg = Machine->pens[pal_ind16[colour&15]];	\
		pattern = patterntbl[charcode * 8 + (line2 & 7)];	\
		for (xx=0;xx<8;xx++)	\
			{	\
			*ln++ = (pattern & 0x80) ? fg : bg;	\
			*ln++ = (pattern & 0x80) ? fg : bg;	\
			pattern <<= 1;	\
			}	\
		name++;	\
		}	\
	\
	xx = 32 - vdp.offset_x;	\
	while (xx--) *ln++ = pen;	\
	}

GRAPH1 (v9938_mode_graphic1, UINT8)
GRAPH1 (v9938_mode_graphic1_16, UINT16)

#define GRAPH23(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	pen_type fg, bg, pen;	\
	UINT8 *nametbl, *patterntbl, *colourtbl;	\
	int pattern, x, xx, line2, name, charcode, 	\
		colour, colourmask, patternmask, xxx;	\
	\
	colourmask = (vdp.contReg[3] & 0x7f) * 8 | 7;	\
	patternmask = (vdp.contReg[4] & 0x03) * 256 | (colourmask & 255);	\
	\
	nametbl = vdp.vram + (vdp.contReg[2] << 10);	\
 	colourtbl = vdp.vram + ((vdp.contReg[3] & 0x80) << 6) + (vdp.contReg[10] << 14);	\
	patterntbl = vdp.vram + ((vdp.contReg[4] & 0x3c) << 11);	\
	\
	line2 = (line + vdp.contReg[23]) & 255;	\
	name = (line2/8)*32;	\
	\
	pen = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];	\
	xxx = vdp.offset_x;	\
	while (xxx--) *ln++ = pen;	\
	\
	for (x=0;x<32;x++)	\
		{	\
		charcode = nametbl[name] + (line2&0xc0)*4;	\
		colour = colourtbl[(charcode&colourmask)*8+(line2&7)];	\
		pattern = patterntbl[(charcode&patternmask)*8+(line2&7)];	\
        fg = Machine->pens[pal_ind16[colour>>4]];	\
        bg = Machine->pens[pal_ind16[colour&15]];	\
		for (xx=0;xx<8;xx++)	\
			{	\
			*ln++ = (pattern & 0x80) ? fg : bg;	\
			*ln++ = (pattern & 0x80) ? fg : bg;	\
            pattern <<= 1;	\
			}	\
		name++;	\
		}	\
	\
	xx = 32 - vdp.offset_x;	\
	while (xx--) *ln++ = pen;	\
	}

GRAPH23 (v9938_mode_graphic23, UINT8)
GRAPH23 (v9938_mode_graphic23_16, UINT16)

#define GRAPH4(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	UINT8 *nametbl, colour;	\
	int line2, linemask, x, xx;	\
	pen_type pen, pen_bg;	\
	\
	linemask = ((vdp.contReg[2] & 0x1f) << 3) | 7;	\
	\
	line2 = ((line & linemask) + vdp.contReg[23]) & 255;	\
	\
	nametbl = vdp.vram + ((vdp.contReg[2] & 0x40) << 10) + line2 * 128;	\
	if ( (vdp.contReg[2] & 0x20) && (V9938_SECOND_FIELD) )	\
		nametbl += 0x8000;	\
	\
	pen_bg = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];	\
	xx = vdp.offset_x;	\
	while (xx--) *ln++ = pen_bg;	\
	\
	for (x=0;x<128;x++)	\
		{	\
		colour = *nametbl++;	\
        pen = Machine->pens[pal_ind16[colour>>4]];	\
		*ln++ = pen;	\
		*ln++ = pen;	\
        pen = Machine->pens[pal_ind16[colour&15]];	\
		*ln++ = pen;	\
		*ln++ = pen;	\
		}	\
	\
	xx = 32 - vdp.offset_x;	\
	while (xx--) *ln++ = pen_bg;	\
	}

GRAPH4 (v9938_mode_graphic4, UINT8)
GRAPH4 (v9938_mode_graphic4_16, UINT16)

#define GRAPH5(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	UINT8 *nametbl, colour;	\
	int line2, linemask, x, xx;	\
	pen_type pen_bg0, pen_bg1;	\
	\
	linemask = ((vdp.contReg[2] & 0x1f) << 3) | 7;	\
	\
	line2 = ((line & linemask) + vdp.contReg[23]) & 255;	\
	\
	nametbl = vdp.vram + ((vdp.contReg[2] & 0x40) << 10) + line2 * 128;	\
	if ( (vdp.contReg[2] & 0x20) && (V9938_SECOND_FIELD) )	\
		nametbl += 0x8000;	\
	\
	pen_bg1 = Machine->pens[pal_ind16[(vdp.contReg[7]&0x03)]];	\
	pen_bg0 = Machine->pens[pal_ind16[((vdp.contReg[7]>>2)&0x03)]];	\
	xx = vdp.offset_x / 2;	\
	while (xx--) { *ln++ = pen_bg0; *ln++ = pen_bg1; }	\
	\
	for (x=0;x<128;x++)	\
		{	\
		colour = *nametbl++;	\
        *ln++ = Machine->pens[pal_ind16[colour>>6]];	\
        *ln++ = Machine->pens[pal_ind16[(colour>>4)&3]];	\
        *ln++ = Machine->pens[pal_ind16[(colour>>2)&3]];	\
        *ln++ = Machine->pens[pal_ind16[(colour&3)]];	\
		}	\
	\
	xx = 32 - vdp.offset_x;	\
	while (xx--) { *ln++ = pen_bg0; *ln++ = pen_bg1; }	\
	}

GRAPH5 (v9938_mode_graphic5, UINT8)
GRAPH5 (v9938_mode_graphic5_16, UINT16)

#define GRAPH6(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
    UINT8 colour;	\
    int line2, linemask, x, xx, nametbl;	\
    pen_type pen_bg;	\
	\
    linemask = ((vdp.contReg[2] & 0x1f) << 3) | 7;	\
	\
    line2 = ((line & linemask) + vdp.contReg[23]) & 255;	\
	\
    nametbl = line2 << 8 ;	\
   	if ( (vdp.contReg[2] & 0x20) && (V9938_SECOND_FIELD) )	\
        nametbl += 0x10000;	\
	\
	pen_bg = Machine->pens[pal_ind16[(vdp.contReg[7]&0x0f)]];	\
	xx = vdp.offset_x;	\
	while (xx--) *ln++ = pen_bg;	\
	\
    for (x=0;x<256;x++)	\
        {	\
		colour = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)];	\
        *ln++ = Machine->pens[pal_ind16[colour>>4]];	\
        *ln++ = Machine->pens[pal_ind16[colour&15]];	\
		nametbl++;	\
        }	\
	\
	xx = 32 - vdp.offset_x;	\
	while (xx--) *ln++ = pen_bg;	\
	}

GRAPH6 (v9938_mode_graphic6, UINT8)
GRAPH6 (v9938_mode_graphic6_16, UINT16)

#if 0

static void v9938_mode_graphic7_yjk (struct osd_bitmap *bmp, int line)
	{
    UINT8 data0, data1, data2, data3;
    int line2, linemask, x, xx, nametbl, jk;
    UINT16 pen, pen_bg;

    linemask = ((vdp.contReg[2] & 0x1f) << 3) | 7;

    line2 = ((line & linemask) - vdp.contReg[23]) & 255;

    nametbl = line2 << 8 ;
   	if ( (vdp.contReg[2] & 0x20) && (V9938_SECOND_FIELD) )
        nametbl += 0x10000;

	xx = 0;
	pen_bg = Machine->pens[pal_ind256[vdp.contReg[7]]];
	while (xx < vdp.offset_x)
		plot_pixel (bmp, xx++, vdp.offset_y + line, pen_bg);

    for (x=0;x<64;x++)
        {
		data0 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		data1 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		data2 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		data3 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		jk = ((data0 & 7) << 11) | ((data1 & 7) << 14) |
			((data2 & 7) << 5) | ((data3 & 7) << 8);
		pen = Machine->pens[pal_indYJK[jk | ((data0 >> 3) & 31)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
		pen = Machine->pens[pal_indYJK[jk | ((data1 >> 3) & 31)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
		pen = Machine->pens[pal_indYJK[jk | ((data2 >> 3) & 31)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
		pen = Machine->pens[pal_indYJK[jk | ((data3 >> 3) & 31)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
        }

	while (xx < (512 + 32) )
		plot_pixel (bmp, xx++, vdp.offset_y + line, pen_bg);
	}

static void v9938_mode_graphic7_yae (struct osd_bitmap *bmp, int line)
	{
    UINT8 data0, data1, data2, data3;
    int line2, linemask, x, xx, nametbl, jk;
    UINT16 pen, pen_bg;

    linemask = ((vdp.contReg[2] & 0x1f) << 3) | 7;

    line2 = ((line & linemask) + vdp.contReg[23]) & 255;

	nametbl = line2 << 8 ;
   	if ( (vdp.contReg[2] & 0x20) && (V9938_SECOND_FIELD) )
        nametbl += 0x10000;

	xx = 0;
	pen_bg = Machine->pens[pal_ind256[vdp.contReg[7]]];
	while (xx < vdp.offset_x)
		plot_pixel (bmp, xx++, vdp.offset_y + line, pen_bg);

    for (x=0;x<64;x++)
        {
		/* read four bytes */
		data0 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		data1 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		data2 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		data3 = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)]; nametbl++;
		/* extract jk information */
		jk = ((data0 & 7) << 11) | ((data1 & 7) << 14) |
			((data2 & 7) << 5) | ((data3 & 7) << 8);
		/* first pixel */
		if (data0 & 0x08)
			pen = Machine->pens[pal_ind16[data0 >> 4]];
		else
			pen = Machine->pens[pal_indYJK[jk | ((data0 >> 3) & 30)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
		/* second pixel */
		if (data1 & 0x08)
			pen = Machine->pens[pal_ind16[data1 >> 4]];
		else
			pen = Machine->pens[pal_indYJK[jk | ((data1 >> 3) & 30)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
		/* third pixel */
		if (data2 & 0x08)
			pen = Machine->pens[pal_ind16[data2 >> 4]];
		else
			pen = Machine->pens[pal_indYJK[jk | ((data2 >> 3) & 30)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
		/* fourth pixel */
		if (data3 & 0x08)
			pen = Machine->pens[pal_ind16[data3 >> 4]];
		else
			pen = Machine->pens[pal_indYJK[jk | ((data3 >> 3) & 30)]];
        plot_pixel (bmp, xx++, line, pen);
        plot_pixel (bmp, xx++, line, pen);
        }

	while (xx < (512 + 32) )
		plot_pixel (bmp, xx++, vdp.offset_y + line, pen_bg);
	}

#endif

#define GRAPH7(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
    UINT8 colour;	\
    int line2, linemask, x, xx, nametbl;	\
    pen_type pen, pen_bg;	\
	\
/* if (vdp.contReg[25] & 0x10)	\
		{	\
		if (vdp.contReg[25] & 0x20)	\
			v9938_mode_graphic7_yae (bmp, line);	\
		else	\
			v9938_mode_graphic7_yjk (bmp, line);	\
		}	\
	else */	\
		{	\
    	linemask = ((vdp.contReg[2] & 0x1f) << 3) | 7;	\
	\
    	line2 = ((line & linemask) + vdp.contReg[23]) & 255;	\
	\
		nametbl = line2 << 8;	\
    	if ( (vdp.contReg[2] & 0x20) && (V9938_SECOND_FIELD) )	\
            nametbl += 0x10000;	\
	\
		pen_bg = Machine->pens[pal_ind256[vdp.contReg[7]]];	\
		xx = vdp.offset_x;	\
		while (xx--) *ln++ = pen_bg;	\
	\
    	for (x=0;x<256;x++)	\
	        {	\
			colour = vdp.vram[((nametbl&1) << 16) | (nametbl>>1)];	\
			pen = Machine->pens[pal_ind256[colour]];	\
			*ln++ = pen; *ln++ = pen;	\
			nametbl++;	\
        	}	\
	\
		xx = 32 - vdp.offset_x;	\
		while (xx--) *ln++ = pen_bg;	\
		}	\
	}

GRAPH7 (v9938_mode_graphic7, UINT8)
GRAPH7 (v9938_mode_graphic7_16, UINT16)

#define UNKNOWN(fnname,pen_type) \
\
static void fnname (int line, pen_type *ln)	\
	{	\
	pen_type fg, bg;	\
	int x;	\
	\
    fg = Machine->pens[pal_ind16[vdp.contReg[7] >> 4]];	\
    bg = Machine->pens[pal_ind16[vdp.contReg[7] & 15]];	\
	\
	x = vdp.offset_x;	\
	while (x--) *ln++ = bg;	\
	\
	x = 512;	\
	while (x--) *ln++ = fg;	\
	\
	x = 32 - vdp.offset_x;	\
	while (x--) *ln++ = bg;	\
	}

UNKNOWN (v9938_mode_unknown, UINT8)
UNKNOWN (v9938_mode_unknown_16, UINT16)

static void v9938_sprite_mode1 (int line, UINT8 *col)
	{
	UINT8	*attrtbl, *patterntbl, *patternptr;
	int x, y, p, height, c, p2, i, n, pattern;

	memset (col, 0, 256);

	/* are sprites disabled? */
	if (vdp.contReg[8] & 0x02) return;

	attrtbl = vdp.vram + (vdp.contReg[5] << 7) + (vdp.contReg[11] << 15);
	patterntbl = vdp.vram + (vdp.contReg[6] << 11);

	/* 16x16 or 8x8 sprites */
	height = (vdp.contReg[1] & 2) ? 16 : 8;
	/* magnified sprites (zoomed) */
	if (vdp.contReg[1] & 1) height *= 2;

	p2 = p = 0;
	while (1)
		{
		y = attrtbl[0];
		if (y == 208) break;
		y = (y - vdp.contReg[23]) & 255;
		if (y > 208)
			y = -(~y&255);
		else
			y++;

		/* if sprite in range, has to be drawn */
		if ( (line >= y) && (line  < (y + height) ) )
			{
			if (p2 == 4)
				{
				/* max maximum sprites per line! */
				if ( !(vdp.statReg[0] & 0x40) )
					vdp.statReg[0] = (vdp.statReg[0] & 0xa0) | 0x40 | p;

				if (vdp.sprite_limit) break;
				}
			/* get x */
			x = attrtbl[1];
			if (attrtbl[3] & 0x80) x -= 32;

			/* get pattern */
			pattern = attrtbl[2];
			if (vdp.contReg[1] & 2)
				pattern &= 0xfc;
			n = line - y;
			patternptr = patterntbl + pattern * 8 +
				((vdp.contReg[1] & 1) ? n/2  : n);
			pattern = patternptr[0] << 8 | patternptr[16];

			/* get colour */
			c = attrtbl[3] & 0x0f;

			/* draw left part */
			n = 0;
			while (1)
				{
				if (n == 0) pattern = patternptr[0];
				else if ( (n == 1) && (vdp.contReg[1] & 2) ) pattern = patternptr[16];
				else break;

				n++;

				for (i=0;i<8;i++)
					{
					if (pattern & 0x80)
						{
						if ( (x >= 0) && (x < 256) )
							{
							if (col[x] & 0x40)
								{
								/* we have a collision! */
								if (p2 < 4)
									vdp.statReg[0] |= 0x20;
								}
							if ( !(col[x] & 0x80) )
								{
								if (c || (vdp.contReg[8] & 0x20) )
									col[x] |= 0xc0 | c;
								else
									col[x] |= 0x40;
								}

							/* if zoomed, draw another pixel */
							if (vdp.contReg[1] & 1)
								{
								if (col[x+1] & 0x40)
    	                        	{
       		                    	/* we have a collision! */
									if (p2 < 4)
										vdp.statReg[0] |= 0x20;
                            		}
                        		if ( !(col[x+1] & 0x80) )
	                            	{
   		                         	if (c || (vdp.contReg[8] & 0x20) )
										col[x+1] |= 0xc0 | c;
									else
										col[x+1] |= 0x80;
                            		}
								}
							}
						}
					if (vdp.contReg[1] & 1) x += 2; else x++;
					pattern <<= 1;
					}
				}

			p2++;
			}

		if (p >= 31) break;
		p++;
		attrtbl += 4;
		}

	if ( !(vdp.statReg[0] & 0x40) )
		vdp.statReg[0] = (vdp.statReg[0] & 0xa0) | p;
	}

static void v9938_sprite_mode2 (int line, UINT8 *col)
	{
	int attrtbl, patterntbl, patternptr, colourtbl;
	int x, i, y, p, height, c, p2, n, pattern, colourmask;

	memset (col, 0, 256);

	/* are sprites disabled? */
	if (vdp.contReg[8] & 0x02) return;

	attrtbl = ( (vdp.contReg[5] & 0xfc) << 7) + (vdp.contReg[11] << 15);
	colourtbl =  ( (vdp.contReg[5] & 0xf8) << 7) + (vdp.contReg[11] << 15);
	patterntbl = (vdp.contReg[6] << 11);
	colourmask = ( (vdp.contReg[5] & 3) << 3) | 0x7; /* check this! */

	/* 16x16 or 8x8 sprites */
	height = (vdp.contReg[1] & 2) ? 16 : 8;
	/* magnified sprites (zoomed) */
	if (vdp.contReg[1] & 1) height *= 2;

	p2 = p = 0;
	while (1)
		{
		y = v9938_vram_read (attrtbl);
		if (y == 216) break;
		y = (y - vdp.contReg[23]) & 255;
		if (y > 216)
			y = -(~y&255);
		else
			y++;

		/* if sprite in range, has to be drawn */
		if ( (line >= y) && (line  < (y + height) ) )
			{
			if (p2 == 8)
				{
				/* max maximum sprites per line! */
				if ( !(vdp.statReg[0] & 0x40) )
					vdp.statReg[0] = (vdp.statReg[0] & 0xa0) | 0x40 | p;

				if (vdp.sprite_limit) break;
				}

			/* get pattern */
			pattern = v9938_vram_read (attrtbl + 2);
			if (vdp.contReg[1] & 2)
				pattern &= 0xfc;
			n = line - y; if (vdp.contReg[1] & 1) n /= 2;
			patternptr = patterntbl + pattern * 8 + n;
			pattern = (v9938_vram_read (patternptr) << 8) |
				v9938_vram_read (patternptr + 16);

			/* get colour */
			c = v9938_vram_read (colourtbl + (((p&colourmask)*16) + n));

			/* get x */
			x = v9938_vram_read (attrtbl + 1);
			if (c & 0x80) x -= 32;

			/* draw left part */
			n = (vdp.contReg[1] & 2) ? 16 : 8;
			while (n--)
				{
				if (pattern & 0x8000)
					{
					for (i=0;i<=(vdp.contReg[1] & 1);i++)
						{
						if ( (x >= 0) && (x < 256) )
							{
							/* the bits in col[x] are:
								7 = non-transparent colour drawn (1)
								6 = priority enable (1)
								5 = colission (1)
								4 = unused, 3 - 0 = colour */
							if (col[x] & 0x20)
								{
								/* we have a collision! */
								if (p2 < 8)
									vdp.statReg[0] |= 0x20;
								}

							if ( !(col[x] & 0x80) )
								{
								col[x] |= c & 15;
								if (c & 0x40)
									col[x] |= 0x80;
								else
									col[x] |= 0x40;
								}
							}
						x++;
						}
					}
				else
					{
/*
					if ( (x >= 0) && (x < 256) )
						{
						if ( (c & 0x40) && ( (col[x] & 0xc0) == 0x40) )
							col[x] |= 0x80;
						}
*/

					if (vdp.contReg[1] & 1) x += 2; else x++;
					}

				pattern <<= 1;
				}

			p2++;
			}

		if (p >= 31) break;
		p++;
		attrtbl += 4;
		}

	if ( !(vdp.statReg[0] & 0x40) )
		vdp.statReg[0] = (vdp.statReg[0] & 0xa0) | p;
	}

static void v9938_default_draw_sprite_8 (UINT8 *ln, UINT8 *col)
	{
	int i;

	ln += vdp.offset_x;

	for (i=0;i<256;i++)
		{
		if ( (col[i] & 0xc0) && ( (col[i] & 0x0f) || (vdp.contReg[8] & 0x20) ) )
			{
			*ln++ = Machine->pens[pal_ind16[col[i]&0x0f]];
			*ln++ = Machine->pens[pal_ind16[col[i]&0x0f]];
			}
		else
			ln += 2;
		}
	}

static void v9938_default_draw_sprite_16 (UINT16 *ln, UINT8 *col)
	{
	int i;

	ln += vdp.offset_x;

	for (i=0;i<256;i++)
		{
		if ( (col[i] & 0xc0) && ( (col[i] & 0x0f) || (vdp.contReg[8] & 0x20) ) )
			{
			*ln++ = Machine->pens[pal_ind16[col[i]&0x0f]];
			*ln++ = Machine->pens[pal_ind16[col[i]&0x0f]];
			}
		else
			ln += 2;
		}
	}

static void v9938_graphic7_draw_sprite_8 (UINT8 *ln, UINT8 *col)
	{
	static const UINT16 g7_ind16[16] = {
		0, 2, 192, 194, 48, 50, 240, 242,
		482, 7, 448, 455, 56, 63, 504, 511  };
	int i;

	ln += vdp.offset_x;

	for (i=0;i<256;i++)
		{
		if ( (col[i] & 0xc0) && ( (col[i] & 0x0f) || (vdp.contReg[8] & 0x20) ) )
			{
			*ln++ = Machine->pens[g7_ind16[col[i]&0x0f]];
			*ln++ = Machine->pens[g7_ind16[col[i]&0x0f]];
			}
		else
			ln += 2;
		}
	}

static void v9938_graphic7_draw_sprite_16 (UINT16 *ln, UINT8 *col)
	{
	static const UINT16 g7_ind16[16] = {
		0, 2, 192, 194, 48, 50, 240, 242,
		482, 7, 448, 455, 56, 63, 504, 511  };
	int i;

	ln += vdp.offset_x;

	for (i=0;i<256;i++)
		{
		if ( (col[i] & 0xc0) && ( (col[i] & 0x0f) || (vdp.contReg[8] & 0x20) ) )
			{
			*ln++ = Machine->pens[g7_ind16[col[i]&0x0f]];
			*ln++ = Machine->pens[g7_ind16[col[i]&0x0f]];
			}
		else
			ln += 2;
		}
	}

typedef struct {
	UINT8 m;
	void (*visible_8)(int, UINT8*);
	void (*visible_16)(int, UINT16*);
	void (*border_8)(UINT8*);
	void (*border_16)(UINT16*);
	void (*sprites)(int, UINT8*);
	void (*draw_sprite_8)(UINT8*, UINT8*);
	void (*draw_sprite_16)(UINT16*, UINT8*);
} V9938_MODE;

static const V9938_MODE modes[] = {
	{ 0x02,
		v9938_mode_text1, v9938_mode_text1_16,
		v9938_default_border_8, v9938_default_border_16,
		NULL, NULL, NULL },
	{ 0x01,
		v9938_mode_multi, v9938_mode_multi_16,
		v9938_default_border_8, v9938_default_border_16,
		v9938_sprite_mode1, v9938_default_draw_sprite_8, v9938_default_draw_sprite_16 },
	{ 0x00,
		v9938_mode_graphic1, v9938_mode_graphic1_16,
		v9938_default_border_8, v9938_default_border_16,
		v9938_sprite_mode1, v9938_default_draw_sprite_8, v9938_default_draw_sprite_16 },
	{ 0x04,
		v9938_mode_graphic23, v9938_mode_graphic23_16,
		v9938_default_border_8, v9938_default_border_16,
		v9938_sprite_mode1, v9938_default_draw_sprite_8, v9938_default_draw_sprite_16 },
	{ 0x08,
		v9938_mode_graphic23, v9938_mode_graphic23_16,
		v9938_default_border_8, v9938_default_border_16,
		v9938_sprite_mode2, v9938_default_draw_sprite_8, v9938_default_draw_sprite_16 },
	{ 0x0c,
		v9938_mode_graphic4, v9938_mode_graphic4_16,
		v9938_default_border_8, v9938_default_border_16,
		v9938_sprite_mode2, v9938_default_draw_sprite_8, v9938_default_draw_sprite_16 },
	{ 0x10,
		v9938_mode_graphic5, v9938_mode_graphic5_16,
		v9938_graphic5_border_8, v9938_graphic5_border_16,
		v9938_sprite_mode2, v9938_default_draw_sprite_8, v9938_default_draw_sprite_16 },
	{ 0x14,
		v9938_mode_graphic6, v9938_mode_graphic6_16,
		v9938_default_border_8, v9938_default_border_16,
		v9938_sprite_mode2, v9938_default_draw_sprite_8, v9938_default_draw_sprite_16 },
	{ 0x1c,
		v9938_mode_graphic7, v9938_mode_graphic7_16,
		v9938_graphic7_border_8, v9938_graphic7_border_16,
		v9938_sprite_mode2, v9938_graphic7_draw_sprite_8, v9938_graphic7_draw_sprite_16 },
	{ 0x0a,
		v9938_mode_text2, v9938_mode_text2_16,
		v9938_default_border_8, v9938_default_border_16,
		NULL, NULL, NULL },
	{ 0xff,
		v9938_mode_unknown, v9938_mode_unknown_16,
		v9938_default_border_8, v9938_default_border_16,
		NULL, NULL, NULL },
};

static void v9938_set_mode (void)
	{
	int n,i;

	n = (((vdp.contReg[0] & 0x0e) << 1) | ((vdp.contReg[1] & 0x18) >> 3));
	for (i=0;;i++)
		{
		if ( (modes[i].m == n) || (modes[i].m == 0xff) ) break;
		}
	vdp.mode = i;
	}

static void v9938_refresh_8 (struct osd_bitmap *bmp, int line)
	{
	int i;
	UINT8 col[256], *ln, *ln2;

	ln = bmp->line[line*2];
	ln2 = bmp->line[line*2+1];

	if ( !(vdp.contReg[1] & 0x40) || (vdp.statReg[2] & 0x40) )
		{
		modes[vdp.mode].border_8 (ln);
		}
	else
		{
		i = (line - vdp.offset_y) & 255;
		modes[vdp.mode].visible_8 (i, ln);
		if (modes[vdp.mode].sprites)
			{
			modes[vdp.mode].sprites (i, col);
			modes[vdp.mode].draw_sprite_8(ln, col);
			}
		}

	memcpy (ln2, ln, (512 + 32) );
	}

static void v9938_refresh_16 (struct osd_bitmap *bmp, int line)
	{
	int i;
	UINT8 col[256];
	UINT16 *ln, *ln2;

	ln = (UINT16*)bmp->line[line*2];
	ln2 = (UINT16*)bmp->line[line*2+1];

	if ( !(vdp.contReg[1] & 0x40) || (vdp.statReg[2] & 0x40) )
		{
		modes[vdp.mode].border_16 (ln);
		}
	else
		{
		i = (line - vdp.offset_y) & 255;
		modes[vdp.mode].visible_16 (i, ln);
		if (modes[vdp.mode].sprites)
			{
			modes[vdp.mode].sprites (i, col);
			modes[vdp.mode].draw_sprite_16 (ln, col);
			}
		}

	memcpy (ln2, ln, (512 + 32) * 2);
	}

static void v9938_refresh_line (struct osd_bitmap *bmp, int line)
	{
	int ind16, ind256;

	ind16 = pal_ind16[0];
	ind256 = pal_ind256[0];

	if ( !(vdp.contReg[8] & 0x20) )
		{
		pal_ind16[0] = pal_ind16[(vdp.contReg[7] & 0x0f)];
		pal_ind256[0] = pal_ind256[vdp.contReg[7]];
		}

	if (Machine->scrbitmap->depth == 8)
		v9938_refresh_8 (bmp, line);
	else
		v9938_refresh_16 (bmp, line);

	if ( !(vdp.contReg[8] & 0x20) )
		{
		pal_ind16[0] = ind16;
		pal_ind256[0] = ind256;
		}
	}

void v9938_refresh (struct osd_bitmap *bmp, int fullrefresh)
	{
	copybitmap (bmp, vdp.bmp, 0, 0, 0, 0,
		&Machine->visible_area, TRANSPARENCY_NONE, 0);
	}

/*

From: awulms@inter.nl.net (Alex Wulms)
*** About the HR/VR topic: this is how it works according to me:

*** HR:
HR is very straightforward:
-HR=1 during 'display time'
-HR=0 during 'horizontal border, horizontal retrace'
I have put 'display time' and 'horizontal border, horizontal retrace' between
quotes because HR does not only flip between 0 and 1 during the display of
the 192/212 display lines, but also during the vertical border and during the
vertical retrace.

*** VR:
VR is a little bit tricky
-VR always gets set to 0 when the VDP starts with display line 0
-VR gets set to 1 when the VDP reaches display line (192 if LN=0) or (212 if
LN=1)
-The VDP displays contents of VRAM as long as VR=0

As a consequence of this behaviour, it is possible to program the famous
overscan trick, where VRAM contents is shown in the borders:
Generate an interrupt at line 230 (or so) and on this interrupt: set LN=1
Generate an interrupt at line 200 (or so) and on this interrupt: set LN=0
Repeat the above two steps

*** The top/bottom border contents during overscan:
On screen 0:
1) The VDP keeps increasing the name table address pointer during bottom
border, vertical retrace and top border
2) The VDP resets the name table address pointer when the first display line
is reached

On the other screens:
1) The VDP keeps increasing the name table address pointer during the bottom
border
2) The VDP resets the name table address pointer such that the top border
contents connects up with the first display line. E.g., when the top border
is 26 lines high, the VDP will take:
'logical'      vram line
TOPB000  256-26
...
TOPB025  256-01
DISPL000 000
...
DISPL211 211
BOTB000  212
...
BOTB024  236



*** About the horizontal interrupt

All relevant definitions on a row:
-FH: Bit 0 of status register 1
-IE1: Bit 4 of mode register 0
-IL: Line number in mode register 19
-DL: The line that the VDP is going to display (corrected for vertical scroll)
-IRQ: Interrupt request line of VDP to Z80

At the *start* of every new line (display, bottom border, part of vertical
display), the VDP does:
-FH = (FH && IE1) || (IL==DL)

After reading of status register 1 by the CPU, the VDP does:
-FH = 0

Furthermore, the following is true all the time:
-IRQ = FH && IE1

The resulting behaviour:
When IE1=0:
-FH will be set as soon as display of line IL starts
-FH will be reset as soon as status register 1 is read
-FH will be reset as soon as the next display line is reached

When IE=1:
-FH and IRQ will be set as soon as display line IL is reached
-FH and IRQ will be reset as soon as status register 1 is read

Another subtile result:
If, while FH and IRQ are set, IE1 gets reset, the next happens:
-IRQ is reset immediately (since IRQ is always FH && IE1)
-FH will be reset as soon as display of the next line starts (unless the next
line is line IL)


*** About the vertical interrupt:
Another relevant definition:
-FV: Bit 7 of status register 0
-IE0: Bit 5 of mode register 1

I only know for sure the behaviour when IE0=1:
-FV and IRQ will be set as soon as VR changes from 0 to 1
-FV and IRQ will be reset as soon as status register 0 is read

A consequence is that NO vertical interrupts will be generated during the
overscan trick, described in the VR section above.

I do not know the behaviour of FV when IE0=0. That is the part that I still
have to test.
*/

static void v9938_interrupt_bottom (void)
	{
#if 0
	if (keyboard_pressed (KEYCODE_D) )
		{
	FILE *fp;
	int i;

	fp = fopen ("vram.dmp", "wb");
	if (fp)
		{
		fwrite (vdp.vram, 0x10000, 1, fp);
		fclose (fp);
		usrintf_showmessage ("saved");
		}

	for (i=0;i<24;i++) printf ("R#%d = %02x\n", i, vdp.contReg[i]);
		}
#endif

	/* at every interrupt, vdp switches fields */
	vdp.statReg[2] = (vdp.statReg[2] & 0xfd) | (~vdp.statReg[2] & 2);

	v9938_update_command ();

	/* color blinking */
	if (!(vdp.contReg[12] & 0xf0))
		vdp.blink = 0;
	else if (!(vdp.contReg[12] & 0x0f))
		vdp.blink = 1;
	else
		{
		/* both on and off counter are non-zero: timed blinking */
		if (vdp.blink_count)
			vdp.blink_count--;
		if (!vdp.blink_count)
			{
			vdp.blink = !vdp.blink;
			if (vdp.blink)
				vdp.blink_count = (vdp.contReg[12] >> 4) * 10;
			else
				vdp.blink_count = (vdp.contReg[12] & 0x0f) * 10;
			}
		}
	}

int v9938_interrupt (void)
	{
	UINT8 col[256];
	int scanline, max;

	if (vdp.scanline == vdp.offset_y)
		{
		vdp.statReg[2] &= ~0x40;
		}
	else if (vdp.scanline == (vdp.offset_y + vdp.visible_y) )
		{
		vdp.statReg[2] |= 0x40;
		vdp.statReg[0] |= 0x80;
		}
	if (vdp.scanline < (212 + 16) )
		{
		if (osd_skip_this_frame () )
			{
			if ( !(vdp.statReg[2] & 0x40) && (modes[vdp.mode].sprites) )
				modes[vdp.mode].sprites ((vdp.scanline - vdp.offset_y) & 255, col);
			}
		else
			{
			v9938_refresh_line (vdp.bmp, vdp.scanline);
			}
		}
	max = (vdp.contReg[9] & 2) ? 255 : (vdp.contReg[9] & 0x80) ? 234 : 244;
	scanline = (vdp.scanline - vdp.offset_y);
	if ( (scanline >= 0) && (scanline <= max) &&
	   ( ( (scanline + vdp.contReg[23]) & 255) == vdp.contReg[19]) )
		{
		vdp.statReg[1] |= 1;
		logerror ("V9938: scanline interrupt (%d)\n", scanline);
		}
	else
		if ( !(vdp.contReg[0] & 0x10) ) vdp.statReg[1] &= 0xfe;

	if (vdp.scanline == (212 + 32) ) /* check this!! */
		v9938_interrupt_bottom ();

	max = (vdp.contReg[9] & 2) ? 313 : 262;
	if (++vdp.scanline > max)
		vdp.scanline = 0;

	v9938_check_int ();

	return vdp.INT;
	}

/***************************************************************************

	Command unit

***************************************************************************/

#define VDP vdp.contReg
#define VDPStatus vdp.statReg
#define VRAM vdp.vram
#define ScrMode vdp.mode

/*************************************************************/
/** Completely rewritten by Alex Wulms:                     **/
/**  - VDP Command execution 'in parallel' with CPU         **/
/**  - Corrected behaviour of VDP commands                  **/
/**  - Made it easier to implement correct S7/8 mapping     **/
/**    by concentrating VRAM access in one single place     **/
/**  - Made use of the 'in parallel' VDP command exec       **/
/**    and correct timing. You must call the function       **/
/**    LoopVDP() from LoopZ80 in MSX.c. You must call it    **/
/**    exactly 256 times per screen refresh.                **/
/** Started on       : 11-11-1999                           **/
/** Beta release 1 on:  9-12-1999                           **/
/** Beta release 2 on: 20-01-2000                           **/
/**  - Corrected behaviour of VRM <-> Z80 transfer          **/
/**  - Improved performance of the code                     **/
/** Public release 1.0: 20-04-2000                          **/
/*************************************************************/

#define VDP_VRMP5(X, Y) (VRAM + ((Y&1023)<<7) + ((X&255)>>1))
#define VDP_VRMP6(X, Y) (VRAM + ((Y&1023)<<7) + ((X&511)>>2))
//#define VDP_VRMP7(X, Y) (VRAM + ((Y&511)<<8) + ((X&511)>>1))
#define VDP_VRMP7(X, Y) (VRAM + ((X&2)<<15) + ((Y&511)<<7) + ((X&511)>>2))
//#define VDP_VRMP8(X, Y) (VRAM + ((Y&511)<<8) + (X&255))
#define VDP_VRMP8(X, Y) (VRAM + ((X&1)<<16) + ((Y&511)<<7) + ((X>>1)&127))

#define VDP_VRMP(M, X, Y) VDPVRMP(M, X, Y)
#define VDP_POINT(M, X, Y) VDPpoint(M, X, Y)
#define VDP_PSET(M, X, Y, C, O) VDPpset(M, X, Y, C, O)

#define CM_ABRT  0x0
#define CM_POINT 0x4
#define CM_PSET  0x5
#define CM_SRCH  0x6
#define CM_LINE  0x7
#define CM_LMMV  0x8
#define CM_LMMM  0x9
#define CM_LMCM  0xA
#define CM_LMMC  0xB
#define CM_HMMV  0xC
#define CM_HMMM  0xD
#define CM_YMMM  0xE
#define CM_HMMC  0xF

/*************************************************************/
/* Many VDP commands are executed in some kind of loop but   */
/* essentially, there are only a few basic loop structures   */
/* that are re-used. We define the loop structures that are  */
/* re-used here so that they have to be entered only once    */
/*************************************************************/
#define pre_loop \
    while ((cnt-=delta) > 0) {


/* Loop over DX, DY */
#define post__x_y(MX) \
    if (!--ANX || ((ADX+=TX)&MX)) { \
      if (!(--NY&1023) || (DY+=TY)==-1) \
        break; \
      else { \
        ADX=DX; \
        ANX=NX; \
      } \
    } \
  }

/* Loop over DX, SY, DY */
#define post__xyy(MX) \
    if ((ADX+=TX)&MX) { \
      if (!(--NY&1023) || (SY+=TY)==-1 || (DY+=TY)==-1) \
        break; \
      else \
        ADX=DX; \
    } \
  }

/* Loop over SX, DX, SY, DY */
#define post_xxyy(MX) \
    if (!--ANX || ((ASX+=TX)&MX) || ((ADX+=TX)&MX)) { \
      if (!(--NY&1023) || (SY+=TY)==-1 || (DY+=TY)==-1) \
        break; \
      else { \
        ASX=SX; \
        ADX=DX; \
        ANX=NX; \
      } \
    } \
  }

/*************************************************************/
/** Structures and stuff                                    **/
/*************************************************************/
static struct {
  int SX,SY;
  int DX,DY;
  int TX,TY;
  int NX,NY;
  int MX;
  int ASX,ADX,ANX;
  UINT8 CL;
  UINT8 LO;
  UINT8 CM;
} MMC;

/*************************************************************/
/** Function prototypes                                     **/
/*************************************************************/
static UINT8 *VDPVRMP(register UINT8 M, register int X, register int Y);

static UINT8 VDPpoint5(register int SX, register int SY);
static UINT8 VDPpoint6(register int SX, register int SY);
static UINT8 VDPpoint7(register int SX, register int SY);
static UINT8 VDPpoint8(register int SX, register int SY);

static UINT8 VDPpoint(register UINT8 SM,
                     register int SX, register int SY);

static void VDPpsetlowlevel(register UINT8 *P, register UINT8 CL,
                            register UINT8 M, register UINT8 OP);

static void VDPpset5(register int DX, register int DY,
                     register UINT8 CL, register UINT8 OP);
static void VDPpset6(register int DX, register int DY,
                     register UINT8 CL, register UINT8 OP);
static void VDPpset7(register int DX, register int DY,
                     register UINT8 CL, register UINT8 OP);
static void VDPpset8(register int DX, register int DY,
                     register UINT8 CL, register UINT8 OP);

static void VDPpset(register UINT8 SM,
                    register int DX, register int DY,
                    register UINT8 CL, register UINT8 OP);

static int GetVdpTimingValue(register int *);

static void SrchEngine(void);
static void LineEngine(void);
static void LmmvEngine(void);
static void LmmmEngine(void);
static void LmcmEngine(void);
static void LmmcEngine(void);
static void HmmvEngine(void);
static void HmmmEngine(void);
static void YmmmEngine(void);
static void HmmcEngine(void);

static void ReportVdpCommand(register UINT8 Op);

/*************************************************************/
/** Variables visible only in this module                   **/
/*************************************************************/
static UINT8 Mask[4] = { 0x0F,0x03,0x0F,0xFF };
static int  PPB[4]  = { 2,4,2,1 };
static int  PPL[4]  = { 256,512,512,256 };
static int  VdpOpsCnt=1;
static void (*VdpEngine)(void)=0;

                      /*  SprOn SprOn SprOf SprOf */
                      /*  ScrOf ScrOn ScrOf ScrOn */
static int srch_timing[8]={ 818, 1025,  818,  830,   /* ntsc */
                            696,  854,  696,  684 }; /* pal  */
static int line_timing[8]={ 1063, 1259, 1063, 1161,
                            904,  1026, 904,  953 };
static int hmmv_timing[8]={ 439,  549,  439,  531,
                            366,  439,  366,  427 };
static int lmmv_timing[8]={ 873,  1135, 873, 1056,
                            732,  909,  732,  854 };
static int ymmm_timing[8]={ 586,  952,  586,  610,
                            488,  720,  488,  500 };
static int hmmm_timing[8]={ 818,  1111, 818,  854,
                            684,  879,  684,  708 };
static int lmmm_timing[8]={ 1160, 1599, 1160, 1172,
                            964,  1257, 964,  977 };


/** VDPVRMP() **********************************************/
/** Calculate addr of a pixel in vram                       **/
/*************************************************************/
INLINE UINT8 *VDPVRMP(UINT8 M,int X,int Y)
{
  switch(M)
  {
    case 0: return VDP_VRMP5(X,Y);
    case 1: return VDP_VRMP6(X,Y);
    case 2: return VDP_VRMP7(X,Y);
    case 3: return VDP_VRMP8(X,Y);
  }

  return(VRAM);
}

/** VDPpoint5() ***********************************************/
/** Get a pixel on screen 5                                 **/
/*************************************************************/
INLINE UINT8 VDPpoint5(int SX, int SY)
{
  return (*VDP_VRMP5(SX, SY) >>
          (((~SX)&1)<<2)
         )&15;
}

/** VDPpoint6() ***********************************************/
/** Get a pixel on screen 6                                 **/
/*************************************************************/
INLINE UINT8 VDPpoint6(int SX, int SY)
{
  return (*VDP_VRMP6(SX, SY) >>
          (((~SX)&3)<<1)
         )&3;
}

/** VDPpoint7() ***********************************************/
/** Get a pixel on screen 7                                 **/
/*************************************************************/
INLINE UINT8 VDPpoint7(int SX, int SY)
{
  return (*VDP_VRMP7(SX, SY) >>
          (((~SX)&1)<<2)
         )&15;
}

/** VDPpoint8() ***********************************************/
/** Get a pixel on screen 8                                 **/
/*************************************************************/
INLINE UINT8 VDPpoint8(int SX, int SY)
{
  return *VDP_VRMP8(SX, SY);
}

/** VDPpoint() ************************************************/
/** Get a pixel on a screen                                 **/
/*************************************************************/
INLINE UINT8 VDPpoint(UINT8 SM, int SX, int SY)
{
  switch(SM)
  {
    case 0: return VDPpoint5(SX,SY);
    case 1: return VDPpoint6(SX,SY);
    case 2: return VDPpoint7(SX,SY);
    case 3: return VDPpoint8(SX,SY);
  }

  return(0);
}

/** VDPpsetlowlevel() ****************************************/
/** Low level function to set a pixel on a screen           **/
/** Make it inline to make it fast                          **/
/*************************************************************/
INLINE void VDPpsetlowlevel(UINT8 *P, UINT8 CL, UINT8 M, UINT8 OP)
{
  switch (OP)
  {
    case 0: *P = (*P & M) | CL; break;
    case 1: *P = *P & (CL | M); break;
    case 2: *P |= CL; break;
    case 3: *P ^= CL; break;
    case 4: *P = (*P & M) | ~(CL | M); break;
    case 8: if (CL) *P = (*P & M) | CL; break;
    case 9: if (CL) *P = *P & (CL | M); break;
    case 10: if (CL) *P |= CL; break;
    case 11:  if (CL) *P ^= CL; break;
    case 12:  if (CL) *P = (*P & M) | ~(CL|M); break;
  }
}

/** VDPpset5() ***********************************************/
/** Set a pixel on screen 5                                 **/
/*************************************************************/
INLINE void VDPpset5(int DX, int DY, UINT8 CL, UINT8 OP)
{
  register UINT8 SH = ((~DX)&1)<<2;

  VDPpsetlowlevel(VDP_VRMP5(DX, DY),
                  CL << SH, ~(15<<SH), OP);
}

/** VDPpset6() ***********************************************/
/** Set a pixel on screen 6                                 **/
/*************************************************************/
INLINE void VDPpset6(int DX, int DY, UINT8 CL, UINT8 OP)
{
  register UINT8 SH = ((~DX)&3)<<1;

  VDPpsetlowlevel(VDP_VRMP6(DX, DY),
                  CL << SH, ~(3<<SH), OP);
}

/** VDPpset7() ***********************************************/
/** Set a pixel on screen 7                                 **/
/*************************************************************/
INLINE void VDPpset7(int DX, int DY, UINT8 CL, UINT8 OP)
{
  register UINT8 SH = ((~DX)&1)<<2;

  VDPpsetlowlevel(VDP_VRMP7(DX, DY),
                  CL << SH, ~(15<<SH), OP);
}

/** VDPpset8() ***********************************************/
/** Set a pixel on screen 8                                 **/
/*************************************************************/
INLINE void VDPpset8(int DX, int DY, UINT8 CL, UINT8 OP)
{
  VDPpsetlowlevel(VDP_VRMP8(DX, DY),
                  CL, 0, OP);
}

/** VDPpset() ************************************************/
/** Set a pixel on a screen                                 **/
/*************************************************************/
INLINE void VDPpset(UINT8 SM, int DX, int DY, UINT8 CL, UINT8 OP)
{
  switch (SM) {
    case 0: VDPpset5(DX, DY, CL, OP); break;
    case 1: VDPpset6(DX, DY, CL, OP); break;
    case 2: VDPpset7(DX, DY, CL, OP); break;
    case 3: VDPpset8(DX, DY, CL, OP); break;
  }
}

/** GetVdpTimingValue() **************************************/
/** Get timing value for a certain VDP command              **/
/*************************************************************/
static int GetVdpTimingValue(register int *timing_values)
{
  return(timing_values[((VDP[1]>>6)&1)|(VDP[8]&2)|((VDP[9]<<1)&4)]);
}

/** SrchEgine()** ********************************************/
/** Search a dot                                            **/
/*************************************************************/
void SrchEngine(void)
{
  register int SX=MMC.SX;
  register int SY=MMC.SY;
  register int TX=MMC.TX;
  register int ANX=MMC.ANX;
  register UINT8 CL=MMC.CL;
  register int cnt;
  register int delta;

  delta = GetVdpTimingValue(srch_timing);
  cnt = VdpOpsCnt;

#define pre_srch \
    pre_loop \
      if ((
#define post_srch(MX) \
           ==CL) ^ANX) { \
      VDPStatus[2]|=0x10; /* Border detected */ \
      break; \
    } \
    if ((SX+=TX) & MX) { \
      VDPStatus[2]&=0xEF; /* Border not detected */ \
      break; \
    } \
  }

  switch (ScrMode) {
    case 5: pre_srch VDPpoint5(SX, SY) post_srch(256)
            break;
    case 6: pre_srch VDPpoint6(SX, SY) post_srch(512)
            break;
    case 7: pre_srch VDPpoint7(SX, SY) post_srch(512)
            break;
    case 8: pre_srch VDPpoint8(SX, SY) post_srch(256)
            break;
  }

  if ((VdpOpsCnt=cnt)>0) {
    /* Command execution done */
    VDPStatus[2]&=0xFE;
    VdpEngine=0;
    /* Update SX in VDP registers */
    VDPStatus[8]=SX&0xFF;
    VDPStatus[9]=(SX>>8)|0xFE;
  }
  else {
    MMC.SX=SX;
  }
}

/** LineEgine()** ********************************************/
/** Draw a line                                             **/
/*************************************************************/
void LineEngine(void)
{
  register int DX=MMC.DX;
  register int DY=MMC.DY;
  register int TX=MMC.TX;
  register int TY=MMC.TY;
  register int NX=MMC.NX;
  register int NY=MMC.NY;
  register int ASX=MMC.ASX;
  register int ADX=MMC.ADX;
  register UINT8 CL=MMC.CL;
  register UINT8 LO=MMC.LO;
  register int cnt;
  register int delta;

  delta = GetVdpTimingValue(line_timing);
  cnt = VdpOpsCnt;

#define post_linexmaj(MX) \
      DX+=TX; \
      if ((ASX-=NY)<0) { \
        ASX+=NX; \
        DY+=TY; \
      } \
      ASX&=1023; /* Mask to 10 bits range */ \
      if (ADX++==NX || (DX&MX)) \
        break; \
    }
#define post_lineymaj(MX) \
      DY+=TY; \
      if ((ASX-=NY)<0) { \
        ASX+=NX; \
        DX+=TX; \
      } \
      ASX&=1023; /* Mask to 10 bits range */ \
      if (ADX++==NX || (DX&MX)) \
        break; \
    }

  if ((VDP[45]&0x01)==0)
    /* X-Axis is major direction */
    switch (ScrMode) {
      case 5: pre_loop VDPpset5(DX, DY, CL, LO); post_linexmaj(256)
              break;
      case 6: pre_loop VDPpset6(DX, DY, CL, LO); post_linexmaj(512)
              break;
      case 7: pre_loop VDPpset7(DX, DY, CL, LO); post_linexmaj(512)
              break;
      case 8: pre_loop VDPpset8(DX, DY, CL, LO); post_linexmaj(256)
              break;
    }
  else
    /* Y-Axis is major direction */
    switch (ScrMode) {
      case 5: pre_loop VDPpset5(DX, DY, CL, LO); post_lineymaj(256)
              break;
      case 6: pre_loop VDPpset6(DX, DY, CL, LO); post_lineymaj(512)
              break;
      case 7: pre_loop VDPpset7(DX, DY, CL, LO); post_lineymaj(512)
              break;
      case 8: pre_loop VDPpset8(DX, DY, CL, LO); post_lineymaj(256)
              break;
    }

  if ((VdpOpsCnt=cnt)>0) {
    /* Command execution done */
    VDPStatus[2]&=0xFE;
    VdpEngine=0;
    VDP[38]=DY & 0xFF;
    VDP[39]=(DY>>8) & 0x03;
  }
  else {
    MMC.DX=DX;
    MMC.DY=DY;
    MMC.ASX=ASX;
    MMC.ADX=ADX;
  }
}

/** LmmvEngine() *********************************************/
/** VDP -> Vram                                             **/
/*************************************************************/
void LmmvEngine(void)
{
  register int DX=MMC.DX;
  register int DY=MMC.DY;
  register int TX=MMC.TX;
  register int TY=MMC.TY;
  register int NX=MMC.NX;
  register int NY=MMC.NY;
  register int ADX=MMC.ADX;
  register int ANX=MMC.ANX;
  register UINT8 CL=MMC.CL;
  register UINT8 LO=MMC.LO;
  register int cnt;
  register int delta;

  delta = GetVdpTimingValue(lmmv_timing);
  cnt = VdpOpsCnt;

  switch (ScrMode) {
    case 5: pre_loop VDPpset5(ADX, DY, CL, LO); post__x_y(256)
            break;
    case 6: pre_loop VDPpset6(ADX, DY, CL, LO); post__x_y(512)
            break;
    case 7: pre_loop VDPpset7(ADX, DY, CL, LO); post__x_y(512)
            break;
    case 8: pre_loop VDPpset8(ADX, DY, CL, LO); post__x_y(256)
            break;
  }

  if ((VdpOpsCnt=cnt)>0) {
    /* Command execution done */
    VDPStatus[2]&=0xFE;
    VdpEngine=0;
    if (!NY)
      DY+=TY;
    VDP[38]=DY & 0xFF;
    VDP[39]=(DY>>8) & 0x03;
    VDP[42]=NY & 0xFF;
    VDP[43]=(NY>>8) & 0x03;
  }
  else {
    MMC.DY=DY;
    MMC.NY=NY;
    MMC.ANX=ANX;
    MMC.ADX=ADX;
  }
}

/** LmmmEngine() *********************************************/
/** Vram -> Vram                                            **/
/*************************************************************/
void LmmmEngine(void)
{
  register int SX=MMC.SX;
  register int SY=MMC.SY;
  register int DX=MMC.DX;
  register int DY=MMC.DY;
  register int TX=MMC.TX;
  register int TY=MMC.TY;
  register int NX=MMC.NX;
  register int NY=MMC.NY;
  register int ASX=MMC.ASX;
  register int ADX=MMC.ADX;
  register int ANX=MMC.ANX;
  register UINT8 LO=MMC.LO;
  register int cnt;
  register int delta;

  delta = GetVdpTimingValue(lmmm_timing);
  cnt = VdpOpsCnt;

  switch (ScrMode) {
    case 5: pre_loop VDPpset5(ADX, DY, VDPpoint5(ASX, SY), LO); post_xxyy(256)
            break;
    case 6: pre_loop VDPpset6(ADX, DY, VDPpoint6(ASX, SY), LO); post_xxyy(512)
            break;
    case 7: pre_loop VDPpset7(ADX, DY, VDPpoint7(ASX, SY), LO); post_xxyy(512)
            break;
    case 8: pre_loop VDPpset8(ADX, DY, VDPpoint8(ASX, SY), LO); post_xxyy(256)
            break;
  }

  if ((VdpOpsCnt=cnt)>0) {
    /* Command execution done */
    VDPStatus[2]&=0xFE;
    VdpEngine=0;
    if (!NY) {
      SY+=TY;
      DY+=TY;
    }
    else
      if (SY==-1)
        DY+=TY;
    VDP[42]=NY & 0xFF;
    VDP[43]=(NY>>8) & 0x03;
    VDP[34]=SY & 0xFF;
    VDP[35]=(SY>>8) & 0x03;
    VDP[38]=DY & 0xFF;
    VDP[39]=(DY>>8) & 0x03;
  }
  else {
    MMC.SY=SY;
    MMC.DY=DY;
    MMC.NY=NY;
    MMC.ANX=ANX;
    MMC.ASX=ASX;
    MMC.ADX=ADX;
  }
}

/** LmcmEngine() *********************************************/
/** Vram -> CPU                                             **/
/*************************************************************/
void LmcmEngine()
{
  if ((VDPStatus[2]&0x80)!=0x80) {

    VDPStatus[7]=VDP[44]=VDP_POINT(ScrMode-5, MMC.ASX, MMC.SY);
    VdpOpsCnt-=GetVdpTimingValue(lmmv_timing);
    VDPStatus[2]|=0x80;

    if (!--MMC.ANX || ((MMC.ASX+=MMC.TX)&MMC.MX)) {
      if (!(--MMC.NY & 1023) || (MMC.SY+=MMC.TY)==-1) {
        VDPStatus[2]&=0xFE;
        VdpEngine=0;
        if (!MMC.NY)
          MMC.DY+=MMC.TY;
        VDP[42]=MMC.NY & 0xFF;
        VDP[43]=(MMC.NY>>8) & 0x03;
        VDP[34]=MMC.SY & 0xFF;
        VDP[35]=(MMC.SY>>8) & 0x03;
      }
      else {
        MMC.ASX=MMC.SX;
        MMC.ANX=MMC.NX;
      }
    }
  }
}

/** LmmcEngine() *********************************************/
/** CPU -> Vram                                             **/
/*************************************************************/
void LmmcEngine(void)
{
  if ((VDPStatus[2]&0x80)!=0x80) {
    register UINT8 SM=ScrMode-5;

    VDPStatus[7]=VDP[44]&=Mask[SM];
    VDP_PSET(SM, MMC.ADX, MMC.DY, VDP[44], MMC.LO);
    VdpOpsCnt-=GetVdpTimingValue(lmmv_timing);
    VDPStatus[2]|=0x80;

    if (!--MMC.ANX || ((MMC.ADX+=MMC.TX)&MMC.MX)) {
      if (!(--MMC.NY&1023) || (MMC.DY+=MMC.TY)==-1) {
        VDPStatus[2]&=0xFE;
        VdpEngine=0;
        if (!MMC.NY)
          MMC.DY+=MMC.TY;
        VDP[42]=MMC.NY & 0xFF;
        VDP[43]=(MMC.NY>>8) & 0x03;
        VDP[38]=MMC.DY & 0xFF;
        VDP[39]=(MMC.DY>>8) & 0x03;
      }
      else {
        MMC.ADX=MMC.DX;
        MMC.ANX=MMC.NX;
      }
    }
  }
}

/** HmmvEngine() *********************************************/
/** VDP --> Vram                                            **/
/*************************************************************/
void HmmvEngine(void)
{
  register int DX=MMC.DX;
  register int DY=MMC.DY;
  register int TX=MMC.TX;
  register int TY=MMC.TY;
  register int NX=MMC.NX;
  register int NY=MMC.NY;
  register int ADX=MMC.ADX;
  register int ANX=MMC.ANX;
  register UINT8 CL=MMC.CL;
  register int cnt;
  register int delta;

  delta = GetVdpTimingValue(hmmv_timing);
  cnt = VdpOpsCnt;

  switch (ScrMode) {
    case 5: pre_loop *VDP_VRMP5(ADX, DY) = CL; post__x_y(256)
            break;
    case 6: pre_loop *VDP_VRMP6(ADX, DY) = CL; post__x_y(512)
            break;
    case 7: pre_loop *VDP_VRMP7(ADX, DY) = CL; post__x_y(512)
            break;
    case 8: pre_loop *VDP_VRMP8(ADX, DY) = CL; post__x_y(256)
            break;
  }

  if ((VdpOpsCnt=cnt)>0) {
    /* Command execution done */
    VDPStatus[2]&=0xFE;
    VdpEngine=0;
    if (!NY)
      DY+=TY;
    VDP[42]=NY & 0xFF;
    VDP[43]=(NY>>8) & 0x03;
    VDP[38]=DY & 0xFF;
    VDP[39]=(DY>>8) & 0x03;
  }
  else {
    MMC.DY=DY;
    MMC.NY=NY;
    MMC.ANX=ANX;
    MMC.ADX=ADX;
  }
}

/** HmmmEngine() *********************************************/
/** Vram -> Vram                                            **/
/*************************************************************/
void HmmmEngine(void)
{
  register int SX=MMC.SX;
  register int SY=MMC.SY;
  register int DX=MMC.DX;
  register int DY=MMC.DY;
  register int TX=MMC.TX;
  register int TY=MMC.TY;
  register int NX=MMC.NX;
  register int NY=MMC.NY;
  register int ASX=MMC.ASX;
  register int ADX=MMC.ADX;
  register int ANX=MMC.ANX;
  register int cnt;
  register int delta;

  delta = GetVdpTimingValue(hmmm_timing);
  cnt = VdpOpsCnt;

  switch (ScrMode) {
    case 5: pre_loop *VDP_VRMP5(ADX, DY) = *VDP_VRMP5(ASX, SY); post_xxyy(256)
            break;
    case 6: pre_loop *VDP_VRMP6(ADX, DY) = *VDP_VRMP6(ASX, SY); post_xxyy(512)
            break;
    case 7: pre_loop *VDP_VRMP7(ADX, DY) = *VDP_VRMP7(ASX, SY); post_xxyy(512)
            break;
    case 8: pre_loop *VDP_VRMP8(ADX, DY) = *VDP_VRMP8(ASX, SY); post_xxyy(256)
            break;
  }

  if ((VdpOpsCnt=cnt)>0) {
    /* Command execution done */
    VDPStatus[2]&=0xFE;
    VdpEngine=0;
    if (!NY) {
      SY+=TY;
      DY+=TY;
    }
    else
      if (SY==-1)
        DY+=TY;
    VDP[42]=NY & 0xFF;
    VDP[43]=(NY>>8) & 0x03;
    VDP[34]=SY & 0xFF;
    VDP[35]=(SY>>8) & 0x03;
    VDP[38]=DY & 0xFF;
    VDP[39]=(DY>>8) & 0x03;
  }
  else {
    MMC.SY=SY;
    MMC.DY=DY;
    MMC.NY=NY;
    MMC.ANX=ANX;
    MMC.ASX=ASX;
    MMC.ADX=ADX;
  }
}

/** YmmmEngine() *********************************************/
/** Vram -> Vram                                            **/
/*************************************************************/
void YmmmEngine(void)
{
  register int SY=MMC.SY;
  register int DX=MMC.DX;
  register int DY=MMC.DY;
  register int TX=MMC.TX;
  register int TY=MMC.TY;
  register int NY=MMC.NY;
  register int ADX=MMC.ADX;
  register int cnt;
  register int delta;

  delta = GetVdpTimingValue(ymmm_timing);
  cnt = VdpOpsCnt;

  switch (ScrMode) {
    case 5: pre_loop *VDP_VRMP5(ADX, DY) = *VDP_VRMP5(ADX, SY); post__xyy(256)
            break;
    case 6: pre_loop *VDP_VRMP6(ADX, DY) = *VDP_VRMP6(ADX, SY); post__xyy(512)
            break;
    case 7: pre_loop *VDP_VRMP7(ADX, DY) = *VDP_VRMP7(ADX, SY); post__xyy(512)
            break;
    case 8: pre_loop *VDP_VRMP8(ADX, DY) = *VDP_VRMP8(ADX, SY); post__xyy(256)
            break;
  }

  if ((VdpOpsCnt=cnt)>0) {
    /* Command execution done */
    VDPStatus[2]&=0xFE;
    VdpEngine=0;
    if (!NY) {
      SY+=TY;
      DY+=TY;
    }
    else
      if (SY==-1)
        DY+=TY;
    VDP[42]=NY & 0xFF;
    VDP[43]=(NY>>8) & 0x03;
    VDP[34]=SY & 0xFF;
    VDP[35]=(SY>>8) & 0x03;
    VDP[38]=DY & 0xFF;
    VDP[39]=(DY>>8) & 0x03;
  }
  else {
    MMC.SY=SY;
    MMC.DY=DY;
    MMC.NY=NY;
    MMC.ADX=ADX;
  }
}

/** HmmcEngine() *********************************************/
/** CPU -> Vram                                             **/
/*************************************************************/
void HmmcEngine(void)
{
  if ((VDPStatus[2]&0x80)!=0x80) {

    *VDP_VRMP(ScrMode-5, MMC.ADX, MMC.DY)=VDP[44];
    VdpOpsCnt-=GetVdpTimingValue(hmmv_timing);
    VDPStatus[2]|=0x80;

    if (!--MMC.ANX || ((MMC.ADX+=MMC.TX)&MMC.MX)) {
      if (!(--MMC.NY&1023) || (MMC.DY+=MMC.TY)==-1) {
        VDPStatus[2]&=0xFE;
        VdpEngine=0;
        if (!MMC.NY)
          MMC.DY+=MMC.TY;
        VDP[42]=MMC.NY & 0xFF;
        VDP[43]=(MMC.NY>>8) & 0x03;
        VDP[38]=MMC.DY & 0xFF;
        VDP[39]=(MMC.DY>>8) & 0x03;
      }
      else {
        MMC.ADX=MMC.DX;
        MMC.ANX=MMC.NX;
      }
    }
  }
}

/** VDPWrite() ***********************************************/
/** Use this function to transfer pixel(s) from CPU to VDP. **/
/*************************************************************/
static void v9938_cpu_to_vdp (UINT8 V)
{
  VDPStatus[2]&=0x7F;
  VDPStatus[7]=VDP[44]=V;
  if(VdpEngine&&(VdpOpsCnt>0)) VdpEngine();
}

/** VDPRead() ************************************************/
/** Use this function to transfer pixel(s) from VDP to CPU. **/
/*************************************************************/
static UINT8 v9938_vdp_to_cpu (void)
{
  VDPStatus[2]&=0x7F;
  if(VdpEngine&&(VdpOpsCnt>0)) VdpEngine();
  return(VDP[44]);
}

/** ReportVdpCommand() ***************************************/
/** Report VDP Command to be executed                       **/
/*************************************************************/
static void ReportVdpCommand(register UINT8 Op)
{
  static char *Ops[16] =
  {
    "SET ","AND ","OR  ","XOR ","NOT ","NOP ","NOP ","NOP ",
    "TSET","TAND","TOR ","TXOR","TNOT","NOP ","NOP ","NOP "
  };
  static char *Commands[16] =
  {
    " ABRT"," ????"," ????"," ????","POINT"," PSET"," SRCH"," LINE",
    " LMMV"," LMMM"," LMCM"," LMMC"," HMMV"," HMMM"," YMMM"," HMMC"
  };
  register UINT8 CL, CM, LO;
  register int SX,SY, DX,DY, NX,NY;

  /* Fetch arguments */
  CL = VDP[44];
  SX = (VDP[32]+((int)VDP[33]<<8)) & 511;
  SY = (VDP[34]+((int)VDP[35]<<8)) & 1023;
  DX = (VDP[36]+((int)VDP[37]<<8)) & 511;
  DY = (VDP[38]+((int)VDP[39]<<8)) & 1023;
  NX = (VDP[40]+((int)VDP[41]<<8)) & 1023;
  NY = (VDP[42]+((int)VDP[43]<<8)) & 1023;
  CM = Op>>4;
  LO = Op&0x0F;

  logerror ("V9938: Opcode %02Xh %s-%s (%d,%d)->(%d,%d),%d [%d,%d]%s\n",
         Op, Commands[CM], Ops[LO],
         SX,SY, DX,DY, CL, VDP[45]&0x04? -NX:NX,
         VDP[45]&0x08? -NY:NY,
         VDP[45]&0x70? " on ExtVRAM":""
        );
}

/** VDPDraw() ************************************************/
/** Perform a given V9938 operation Op.                     **/
/*************************************************************/
static UINT8 v9938_command_unit_w (UINT8 Op)
{
  register int SM;

  /* V9938 ops only work in SCREENs 5-8 */
  if (ScrMode<5)
    return(0);

  SM = ScrMode-5;         /* Screen mode index 0..3  */

  MMC.CM = Op>>4;
  if ((MMC.CM & 0x0C) != 0x0C && MMC.CM != 0)
    /* Dot operation: use only relevant bits of color */
    VDPStatus[7]=(VDP[44]&=Mask[SM]);

/*  if(Verbose&0x02) */
    ReportVdpCommand(Op);

  switch(Op>>4) {
    case CM_ABRT:
      VDPStatus[2]&=0xFE;
      VdpEngine=0;
      return 1;
    case CM_POINT:
      VDPStatus[2]&=0xFE;
      VdpEngine=0;
      VDPStatus[7]=VDP[44]=
                   VDP_POINT(SM, VDP[32]+((int)VDP[33]<<8),
                                 VDP[34]+((int)VDP[35]<<8));
      return 1;
    case CM_PSET:
      VDPStatus[2]&=0xFE;
      VdpEngine=0;
      VDP_PSET(SM,
               VDP[36]+((int)VDP[37]<<8),
               VDP[38]+((int)VDP[39]<<8),
               VDP[44],
               Op&0x0F);
      return 1;
    case CM_SRCH:
      VdpEngine=SrchEngine;
      break;
    case CM_LINE:
      VdpEngine=LineEngine;
      break;
    case CM_LMMV:
      VdpEngine=LmmvEngine;
      break;
    case CM_LMMM:
      VdpEngine=LmmmEngine;
      break;
    case CM_LMCM:
      VdpEngine=LmcmEngine;
      break;
    case CM_LMMC:
      VdpEngine=LmmcEngine;
      break;
    case CM_HMMV:
      VdpEngine=HmmvEngine;
      break;
    case CM_HMMM:
      VdpEngine=HmmmEngine;
      break;
    case CM_YMMM:
      VdpEngine=YmmmEngine;
      break;
    case CM_HMMC:
      VdpEngine=HmmcEngine;
      break;
    default:
      logerror("V9938: Unrecognized opcode %02Xh\n",Op);
        return(0);
  }

  /* Fetch unconditional arguments */
  MMC.SX = (VDP[32]+((int)VDP[33]<<8)) & 511;
  MMC.SY = (VDP[34]+((int)VDP[35]<<8)) & 1023;
  MMC.DX = (VDP[36]+((int)VDP[37]<<8)) & 511;
  MMC.DY = (VDP[38]+((int)VDP[39]<<8)) & 1023;
  MMC.NY = (VDP[42]+((int)VDP[43]<<8)) & 1023;
  MMC.TY = VDP[45]&0x08? -1:1;
  MMC.MX = PPL[SM];
  MMC.CL = VDP[44];
  MMC.LO = Op&0x0F;

  /* Argument depends on UINT8 or dot operation */
  if ((MMC.CM & 0x0C) == 0x0C) {
    MMC.TX = VDP[45]&0x04? -PPB[SM]:PPB[SM];
    MMC.NX = ((VDP[40]+((int)VDP[41]<<8)) & 1023)/PPB[SM];
  }
  else {
    MMC.TX = VDP[45]&0x04? -1:1;
    MMC.NX = (VDP[40]+((int)VDP[41]<<8)) & 1023;
  }

  /* X loop variables are treated specially for LINE command */
  if (MMC.CM == CM_LINE) {
    MMC.ASX=((MMC.NX-1)>>1);
    MMC.ADX=0;
  }
  else {
    MMC.ASX = MMC.SX;
    MMC.ADX = MMC.DX;
  }

  /* NX loop variable is treated specially for SRCH command */
  if (MMC.CM == CM_SRCH)
    MMC.ANX=(VDP[45]&0x02)!=0; /* Do we look for "==" or "!="? */
  else
    MMC.ANX = MMC.NX;

  /* Command execution started */
  VDPStatus[2]|=0x01;

  /* Start execution if we still have time slices */
  if(VdpEngine&&(VdpOpsCnt>0)) VdpEngine();

  /* Operation successfull initiated */
  return(1);
}

/** LoopVDP() ************************************************/
/** Run X steps of active VDP command                       **/
/*************************************************************/
static void v9938_update_command (void)
{
  if(VdpOpsCnt<=0)
  {
    VdpOpsCnt+=12500*60;
    if(VdpEngine&&(VdpOpsCnt>0)) VdpEngine();
  }
  else
  {
    VdpOpsCnt=12500*60;
    if(VdpEngine) VdpEngine();
  }
}
