/******************************************************************************
 *
 *      cpm_bios.c
 *
 *      CP/M bios emulation (used for Kaypro 2x)
 *
 *      Juergen Buchmueller, July 1998
 *      Benjamin C. W. Sittler, July 1998 (minor modifications)
 *
 ******************************************************************************/

#include "driver.h"
#include "memory.h"
#include "Z80/Z80.h"
#include "cpm_bios.h"
#include "machine/wd179x.h"

#define VERBOSE         1
#define VERBOSE_FDD     1
#define VERBOSE_BIOS    1
#define VERBOSE_CONIO   1

#define REAL_FDD                ((void*)-1) /* using real floppy disk drive */

/* buffer for one physical sector */
typedef struct {
	byte    unit;                                   /* unit number to use for this drive */
	byte    cyl;                                    /* physical cylinder */
	byte    side;                                   /* physical side */
	byte    head;                                   /* logical head number */
	byte    sec;                                    /* physical sector number */
	byte    dirty;                                  /* non zero if the sector buffer is dirty */
	byte    buffer[4096];                   /* buffer to hold one physical sector */
}       SECBUF;

static int curdisk = 0;             /* currently selected disk */
static int num_disks = 0;                       /* number of supported disks */
static int fmt[NDSK] = {0,};            /* index of disk formats */
static int mode[NDSK] = {0,};           /* 0 read only, !0 read/write */
static int bdos_trk[NDSK] = {0,};       /* BDOS track number */
static int bdos_sec[NDSK] = {0,};       /* BDOS sector number */
static void *fp[NDSK] = {NULL, };       /* image file pointer */
static void *lp = NULL;                         /* list file handle (ie. PIP LST:=X:FILE.EXT) */
static void *pp = NULL;                         /* punch file handle (ie. PIP PUN:=X:FILE.EXT) */
static void *rp = NULL;                         /* reader file handle (ie. PIP X:FILE.EXE=RDR:) */
static int dma = 0;                             /* DMA transfer address */

static SECBUF dsk[NDSK] = {{0,}, };

static byte zeropage0[8] =
{
	0xc3,                           /* JP  BIOS+3 */
	BIOS & 0xff,
	BIOS >> 8,
	0x80,                           /* io byte */
	0x00,                           /* usr drive */
	0xc3,                           /* JP  BDOS */
	BDOS & 0xff,
	BDOS >> 8
};

cpm_dph dph[NDSK] = {
	{ 0, 0, 0, 0, DIRBUF, DPB0, 0, 0 },
	{ 0, 0, 0, 0, DIRBUF, DPB1, 0, 0 },
	{ 0, 0, 0, 0, DIRBUF, DPB2, 0, 0 },
	{ 0, 0, 0, 0, DIRBUF, DPB3, 0, 0 }
};

#include "cpm_disk.c"

/*****************************************************************************
 *      fdd_select
 *      The selected floppy is a real FDD drive, so set it's parameters
 *      and start the motor(s)
 *****************************************************************************/
static void fdd_select(void)
{
dsk_fmt *f = &formats[fmt[curdisk]];
int seclen = f->seclen;
byte secl = 0;
byte eot = 0;
int i;
	/* calculate sector length code */
	while (seclen > 128)
	{
		secl++;
		seclen >>= 1;
	}
	/* find highest sector number */
    for (i = 1; i <= f->spt; i++)
	{
		if (f->side1[i] > eot)
			eot = f->side1[i];
		if (f->sides == 2)
			if (f->side2[i] > eot)
				eot = f->side2[i];
    }
	if (errorlog)
		fprintf(errorlog, "DISK #%d select den:%d cyl:%d spt:%d eot:%d secl:%d\n",
			curdisk, f->density, f->cylinders, f->spt, eot, secl);
	osd_fdc_density(dsk[curdisk].unit, f->density, f->cylinders, f->spt, eot, secl);
}

/*****************************************************************************
 *      fdd_set_track
 *      Either recalibrate the drive (track == 0) or seek to the give track
 *****************************************************************************/
static void fdd_set_track(int t)
{
dsk_fmt *f = &formats[fmt[curdisk]];
byte pcn;
	if (errorlog)
		fprintf(errorlog, "DISK #%d settrk %d\n", curdisk, t);
	osd_fdc_motors(dsk[curdisk].unit);
    if (t == 0)
		osd_fdc_recal(&pcn);
	else
    if (t < f->cylinders)
		osd_fdc_seek(t, &pcn);
}

/*****************************************************************************
 *      fdd_access_sector
 *      Access a (new) physical sector from the floppy disk according to the
 *      BIOS track and sector numbers. Calculate the offset into a virtual
 *      linear image and convert it back into the physical cylinder, side,
 *      head id and sector numbers using the format description in formats[].
 *      Check them against the values for a buffered sector and only read
 *      the physical sector if they changed (after flushing previous changes).
 *****************************************************************************/
#define STA_2_ERROR (STA_2_LOST_DAT|STA_2_CRC_ERR|STA_2_REC_N_FND|STA_2_REC_TYPE|STA_2_NOT_READY)

static int fdd_access_sector(int * record_offset)
{
dsk_fmt *f;
byte cyl, head, side, sec, n = 0;
int recofs, o;

    /* get an index to the disk format */
    f = &formats[fmt[curdisk]];

    /* calculate offset into disk image */
	o = RECL * (f->dpb.spt * bdos_trk[curdisk] + bdos_sec[curdisk]);

	recofs = o % f->seclen;         /* record offset in sector */
	o /= f->seclen;
	switch (f->order)
	{
		case ORD_CYLINDERS:
			/* logical sector number (0 .. spt-1) */
			sec = o % f->spt;
			o /= f->spt;
			/* physical side number (0 .. sides-1) */
			side  = o % f->sides;
	    o /= f->sides;
			/* physical sector number */
			sec = (side) ? f->side2[sec+1] : f->side1[sec+1];
	    /* logical head number for this side */
			head = (side) ? f->side2[0] : f->side1[0];
			/* physical cylinder number (0 .. cylinders-1) */
			cyl  = o % f->cylinders;
			break;
		case ORD_SIDES:
			/* logical sector number (0 .. spt-1) */
			sec = o % f->spt;
	    o /= f->spt;
			/* physical side number (0 .. sides-1) */
			side = o % f->sides;
	    o /= f->sides;
			/* physical sector number */
			sec = (side) ? f->side2[sec+1] : f->side1[sec+1];
	    /* logical head number for this side */
			head = (side) ? f->side2[0] : f->side1[0];
			/* physical cylinder number (0 .. cylinders-1) */
	    cyl = o % f->cylinders;  
	    break;
		case ORD_EAGLE:
		default:
			/* logical sector number (0 .. spt-1) */
			sec = o % f->spt;
	    o /= f->spt;
			/* physical side number (0 .. sides-1) */
	    side = o % f->sides;    
	    o /= f->sides;
			/* physical sector number */
			sec = (side) ? f->side2[sec+1] : f->side1[sec+1];
	    /* logical head number for this side */
			head = (side) ? f->side2[0] : f->side1[0];
			/* physical cylinder number (0 .. cylinders-1) */
	    cyl  = o % f->cylinders;  
    }

	if (errorlog)
		fprintf(errorlog, "DISK #%d access CYL:%d SIDE:%d HEAD:%d SEC:%d RECOFS:0x%04x\n",
			curdisk, cyl, side, head, sec, recofs);
    /* changed cylinder, head or sector for this disk ? */
	if (cyl != dsk[curdisk].cyl ||
		side != dsk[curdisk].side ||
		head != dsk[curdisk].head ||
		sec != dsk[curdisk].sec)
	{
	int tries = 0;
		/* sector buffer dirty ? */
		if (dsk[curdisk].dirty)
		{
			/* seek to cylinder number */
			fdd_set_track(dsk[curdisk].cyl);
			/* put the sector back */
			do
			{
				n = osd_fdc_put_sector(dsk[curdisk].cyl, dsk[curdisk].side, dsk[curdisk].head, dsk[curdisk].sec, dsk[curdisk].buffer, 0);
				if (n & STA_2_ERROR)
				  {
				    if (errorlog)
				      {
					fprintf(errorlog, "cpm_access_sector: (put) status 0x%02x:", n);
					if (n & STA_2_LOST_DAT)
					  fprintf(errorlog, " LOST_DAT");
					if (n & STA_2_CRC_ERR)
					  fprintf(errorlog, " CRC_ERR");
					if (n & STA_2_REC_N_FND)
					  fprintf(errorlog, " REC_N_FND");
					if (n & STA_2_REC_TYPE)
					  fprintf(errorlog, "REC_TYPE");
					if (n & STA_2_NOT_READY)
					  fprintf(errorlog, " NOT_READY");
					fprintf(errorlog, " (try #%d)\n", tries);
				      }
				  }
			} while ((tries++ < 10) &&
				 (n & STA_2_ERROR));
			if (errorlog)
			  fprintf(errorlog, "DISK %d flush  CYL:%d SIDE:%d HEAD:%d SEC:%d -> 0x%02X\n",
				  curdisk, cyl, side, head, sec, n);
			dsk[curdisk].dirty = 0; /* seek to current
						   track number */
			fdd_set_track(cyl);
		}
		/*
		  store new cylinder, head and sector values
		  */
		dsk[curdisk].cyl = cyl;
		dsk[curdisk].side = side;
		dsk[curdisk].head = head;
		dsk[curdisk].sec = sec; /* set track number
					   (ie. seek) */
		fdd_set_track(cyl);
		tries = 0;
		do {
		  n = osd_fdc_get_sector(cyl, side, head,
					 sec, dsk[curdisk].buffer);
		  if (n & STA_2_ERROR) {
		    if (errorlog) {
		      fprintf(errorlog, "cpm_access_sector: (get) status 0x%02x:", n);
		      if (n & STA_2_LOST_DAT)
			fprintf(errorlog, " LOST_DAT");
		      if (n & STA_2_CRC_ERR)
			fprintf(errorlog, "CRC_ERR");
		      if (n & STA_2_REC_N_FND)
			fprintf(errorlog, " REC_N_FND");
		      if (n & STA_2_REC_TYPE)
			fprintf(errorlog, "REC_TYPE");
		      if (n & STA_2_NOT_READY)
			fprintf(errorlog, " NOT_READY");
		      fprintf(errorlog, " (try #%d)\n", tries);
		    }
		  }
		} while ((tries++ < 10) && (n &	STA_2_ERROR));
		
		if (errorlog)
		  fprintf(errorlog, "DISK #%d read   CYL:%d SIDE:%d HEAD:%d SEC:%d -> 0x%02X\n",
			  curdisk, cyl, side, head, sec, n);
	}
	*record_offset = recofs;
	/* mask DRQ and BUSY bits */
	return (n & 0xfc);
}

#define CP(n) ((n)<32?'.':(n))

/*****************************************************************************
 *      fdd_read_sector
 *      access the sector and copy a RECL (128 bytes) to the current DMA
 *****************************************************************************/
static int fdd_read_sector(void)
{
byte *DMA = &Machine->memory_region[0][dma];
int recofs, n = fdd_access_sector(&recofs);
	/* copy a record from the sector buffer to the DMA area */
	memcpy(DMA, &dsk[curdisk].buffer[recofs], RECL);
    return n;
}

/*****************************************************************************
 *      fdd_read_sector
 *      if the DMA memory contains changed data for the (partial) sector,
 *      copy it to the sector buffer and set the dirty flag
 *****************************************************************************/
static int fdd_write_sector(void)
{
byte *DMA = &Machine->memory_region[0][dma];
int recofs, n = fdd_access_sector(&recofs);
	/* did the record really change ? */
	if (memcmp(&dsk[curdisk].buffer[recofs], DMA, RECL))
	{
		/* copy it into the sector buffer */
		memcpy(&dsk[curdisk].buffer[recofs], DMA, RECL);
		/* and set the buffer dirty flag */
		dsk[curdisk].dirty = 1;
	}
    return n;
}

/*****************************************************************************
 * cpm_jumptable
 * initialize a jump table with 18 entries for bios functions
 * initialize bios functions at addresses BIOS_00 .. BIOS_11
 * and the central bios execute function at BIOS_EXEC
 *****************************************************************************/
void cpm_jumptable(void)
{
byte * RAM = Machine->memory_region[0];
int i;
int jmp_addr, fun_addr;

	/* reset RST7 vector (RET) */
	RAM[0x0038] = 0xc9;

	/* reset NMI vector (RETI) */
	RAM[0x0066] = 0xed;
	RAM[0x0067] = 0x5d;

	for (i = 0; i < NFUN; i++)
	{
		jmp_addr = BIOS + 3 * i;
		fun_addr = BIOS_00 + 4 * i;

	RAM[jmp_addr + 0] = 0xc3;
		RAM[jmp_addr + 1] = fun_addr & 0xff;
		RAM[jmp_addr + 2] = fun_addr >> 8;

		RAM[fun_addr + 0] = 0x3e;       /* LD   A,i */
	RAM[fun_addr + 1] = i;
		RAM[fun_addr + 2] = 0x18;       /* JR   BIOS_EXEC */
		RAM[fun_addr + 3] = BIOS_EXEC - fun_addr - 4;
    }

	/* initialize bios execute */
	RAM[BIOS_EXEC + 0] = 0xd3;                /* OUT (BIOS_CMD),A */
	RAM[BIOS_EXEC + 1] = BIOS_CMD;
	RAM[BIOS_EXEC + 2] = 0xc9;                /* RET */
}

/*****************************************************************************
 * cpm_init
 * initialize the BIOS simulation for 'n' drives of types stored in 'ids'
 *****************************************************************************/
int cpm_init(int n, const char *ids[])
{
byte * RAM = Machine->memory_region[0];
dsk_fmt *f;
int i, d;

	if (!osd_fdc_init())
		return 0;

    /* fill memory with HALT insn */
    memset(RAM + 0x100, 0x76, 0xff00);

	zeropage0[0] = 0xc3;
	zeropage0[1] = BIOS & 0xff;
	zeropage0[2] = BIOS >> 8;

    /* copy substantial zero page data */
	for (i = 0; i < 8; i++)
		RAM[i] = zeropage0[i];

#if VERBOSE
	if (errorlog)
	{
		fprintf(errorlog, "CPM CCP     %04X\n", CCP);
		fprintf(errorlog, "CPM BDOS    %04X\n", BDOS);
		fprintf(errorlog, "CPM BIOS    %04X\n", BIOS);
	}
#endif
    /* begin allocating csv & alv */
    i = DATA_START;            

	num_disks = n;

	for (d = 0; d < num_disks; d++)
	{
		if (ids[d])
		{
			/* select a format by name */
			if (cpm_disk_select_format(d, ids[d]))
			{
#if VERBOSE
				if (errorlog)
					fprintf(errorlog, "Error drive #%d id '%s' is unknown\n", d, ids[d]);
#endif
				return 1;
			}

			f = &formats[fmt[d]];

	    /* transfer DPB to memory */
			RAM[DPB0 + d * DPBL +  0] = f->dpb.spt & 0xff;
			RAM[DPB0 + d * DPBL +  1] = f->dpb.spt >> 8;
			RAM[DPB0 + d * DPBL +  2] = f->dpb.bsh;
			RAM[DPB0 + d * DPBL +  3] = f->dpb.blm;
			RAM[DPB0 + d * DPBL +  4] = f->dpb.exm;
			RAM[DPB0 + d * DPBL +  5] = f->dpb.dsm & 0xff;
			RAM[DPB0 + d * DPBL +  6] = f->dpb.dsm >> 8;
			RAM[DPB0 + d * DPBL +  7] = f->dpb.drm & 0xff;
			RAM[DPB0 + d * DPBL +  8] = f->dpb.drm >> 8;
			RAM[DPB0 + d * DPBL +  9] = f->dpb.al0;
			RAM[DPB0 + d * DPBL + 10] = f->dpb.al1;
			RAM[DPB0 + d * DPBL + 11] = f->dpb.cks & 0xff;
			RAM[DPB0 + d * DPBL + 12] = f->dpb.cks >> 8;
			RAM[DPB0 + d * DPBL + 13] = f->dpb.off & 0xff;
			RAM[DPB0 + d * DPBL + 14] = f->dpb.off >> 8;
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "CPM DPB%d    %04X\n", d, DPB0 + d * DPBL);
#endif

			/* TODO: generate sector translation table and
			   allocate space for it; for now it is always 0 */
			dph[d].xlat = 0;
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "CPM XLAT%d   %04X\n", d, dph[d].xlat);
#endif

			/* memory address for dirbuf */
			dph[d].dirbuf = DIRBUF + d * RECL;
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "CPM DIRBUF%d %04X\n", d, dph[d].dirbuf);
#endif

			/* memory address for CSV */
			dph[d].csv = i;
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "CPM CSV%d    %04X\n", d, dph[d].csv);
#endif

			/* length is CKS bytes */
			i += f->dpb.cks;

			/* memory address for ALV */
			dph[d].alv = i;
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "CPM ALV%d    %04X\n", d, dph[d].alv);
#endif

			/* length is DSM+1 bits */
			i += (f->dpb.dsm + 1 + 7) >> 3;

			if (i >= BIOS_00)
			{
#if VERBOSE
				if (errorlog)
					fprintf(errorlog, "Error configuring BIOS tables: out of memory!\n");
#endif
				return 1;
			}
		}
		else
		{
			memset(&dph[d], 0, DPHL);
		}

		if (errorlog)
			fprintf(errorlog, "CPM DPH%d    %04X\n", d, DPH0 + d * DPHL);

	/* transfer DPH to memory */
		RAM[DPH0 + d * DPHL +  0] = dph[d].xlat & 0xff;
		RAM[DPH0 + d * DPHL +  1] = dph[d].xlat >> 8;
		RAM[DPH0 + d * DPHL +  2] = dph[d].scr1 & 0xff;
		RAM[DPH0 + d * DPHL +  3] = dph[d].scr1 >> 8;
		RAM[DPH0 + d * DPHL +  4] = dph[d].scr2 & 0xff;
		RAM[DPH0 + d * DPHL +  5] = dph[d].scr2 >> 8;
		RAM[DPH0 + d * DPHL +  6] = dph[d].scr3 & 0xff;
		RAM[DPH0 + d * DPHL +  7] = dph[d].scr3 >> 8;
		RAM[DPH0 + d * DPHL +  8] = dph[d].dirbuf & 0xff;
		RAM[DPH0 + d * DPHL +  9] = dph[d].dirbuf >> 8;
		RAM[DPH0 + d * DPHL + 10] = dph[d].dpb & 0xff;
		RAM[DPH0 + d * DPHL + 11] = dph[d].dpb >> 8;
		RAM[DPH0 + d * DPHL + 12] = dph[d].csv & 0xff;
		RAM[DPH0 + d * DPHL + 13] = dph[d].csv >> 8;
		RAM[DPH0 + d * DPHL + 14] = dph[d].alv & 0xff;
		RAM[DPH0 + d * DPHL + 15] = dph[d].alv >> 8;

		/* now try to open the image if a floppy_name is given */
	if (strlen(floppy_name[d]))
		{
			/* fake name to access the real floppy disk drive A: */
			if (!stricmp(floppy_name[d], "fd0.dsk"))
			{
				fp[d] = REAL_FDD;
				dsk[d].unit = 0;
			}
			else
			/* fake name to access the real floppy disk drive B: */
			if (!stricmp(floppy_name[d], "fd1.dsk"))
			{
				fp[d] = REAL_FDD;
				dsk[d].unit = 1;
			}
	    else
	    {
				mode[d] = 1;
				fp[d] = osd_fopen(Machine->gamedrv->name, floppy_name[d], OSD_FILETYPE_IMAGE, 1);
				if (!fp[d])
				{
					mode[d] = 0;
					fp[d] = osd_fopen(Machine->gamedrv->name, floppy_name[d], OSD_FILETYPE_IMAGE, 0);
					if (!fp[d])
					{
						floppy_name[d][0] = '\0';
					}
				}
			}
		}
    }

	/* create a file to receive list output (ie. PIP LST:=FILE.EXT) */
	lp = osd_fopen(Machine->gamedrv->name, "kaypro.lst", OSD_FILETYPE_IMAGE, 1);

    cpm_jumptable();

    return 0;
}

/*****************************************************************************
 * cpm_exit
 * shut down the BIOS simulation; close image files; exit osd_fdc
 *****************************************************************************/
void cpm_exit(void)
{
int d;

	/* if a list file is still open close it now */
    if (lp)
	{
		osd_fclose(lp);
		lp = NULL;
	}

	/* close all opened disk images */
    for (d = 0; d < NDSK; d++)
	{
		if (fp[d])
		{
			if (fp[d] != REAL_FDD)
				osd_fclose(fp[d]);
			fp[d] = NULL;
		}
    }
	osd_fdc_exit();
}

/*****************************************************************************
 * cpm_conout_chr
 * send a character to the console
 *****************************************************************************/
void cpm_conout_chr(int data)
{
	cpu_writeport(BIOS_CONOUT, data);
}

/*****************************************************************************
 * cpm_conout_str
 * send a zero terminated string to the console
 *****************************************************************************/
void cpm_conout_str(char *src)
{
	while (*src)
		cpm_conout_chr(*src++);
}

/*****************************************************************************
 * cpm_conout_str
 * select a disk format for drive 'd' by searching formats for 'id'
 *****************************************************************************/
int cpm_disk_select_format(int d, const char *id)
{
int i, j;

	if (!id)
		return 1;

#if VERBOSE
    if (errorlog)
		fprintf(errorlog, "CPM search format '%s'\n", id);
#endif

	for (i = 0; formats[i].id && stricmp(formats[i].id, id); i++)
		;

	if (!formats[i].id)
	{
#if VERBOSE
	if (errorlog)
			fprintf(errorlog, "CPM format '%s' not supported\n", id);
#endif
	return 1;
	}

	/* references another type id? */
	if (formats[i].ref)
	{
#if VERBOSE
	if (errorlog)
			fprintf(errorlog, "CPM search format '%s' for '%s'\n", formats[i].ref, id);
#endif
	/* search for the referred id */
		for (j = 0; formats[j].id && stricmp(formats[j].id, formats[i].ref); j++)
			;
		if (!formats[j].id)
		{
#if VERBOSE
	    if (errorlog)
				fprintf(errorlog, "CPM format '%s' not supported\n", formats[i].ref);
#endif
	    return 1;
		}
	/* set current format */
	fmt[d] = j;
    }
	else
	{
	/* set current format */
	fmt[d] = i;
    }
#if VERBOSE
	if (errorlog)
	{
	dsk_fmt *f = &formats[fmt[d]];
		fprintf(errorlog, "CPM '%s' is '%s'\n", id, f->name);
		fprintf(errorlog, "CPM #%d %dK (%d cylinders, %d sides, %d %d bytes sectors/track)\n",
			d,
			f->cylinders * f->sides * f->spt * f->seclen / 1024,
			f->cylinders, f->sides, f->spt, f->seclen);
	}
#endif

	return 0;
}


/*****************************************************************************
 * cpm_disk_image_seek
 * seek to an image offset for the currently selected disk 'curdisk'
 * the track 'bdos_trk[curdisk]' and sector 'bdos_sec[curdisk]' are used
 * to calculate the offset.
 *****************************************************************************/
void cpm_disk_image_seek(void)
{
dsk_fmt *f = &formats[fmt[curdisk]];
int offs, o, r, s, h;
	switch (f->order)
	{
/* TRACK  0   1   2        n/2-1           n/2     n/2+1         n/2+2    n-1 */
/* F/B   f0, f1, f2 ... f(n/2)-1, b(n/2)-1, b(n/2)-2, b(n/2)-3 ... b0 */
		case ORD_CYLINDERS:
			offs = (bdos_trk[curdisk] % f->cylinders) * 2;
			offs *= f->dpb.spt * RECL;
			o = bdos_sec[curdisk] * RECL;
			r = (o % f->seclen) / RECL;
			s = o / f->seclen;
			h = (s >= f->spt) ? f->side2[0] : f->side1[0];
			s = (s >= f->spt) ? f->side2[s+1] : f->side1[s+1];
			s -= f->side1[1];       /* subtract first sector number */
			offs += (h * f->spt + s) * f->seclen + r * RECL;
			if (bdos_trk[curdisk] >= f->cylinders)
				offs += (int)f->cylinders * f->sides * f->spt * f->seclen / 2;
#if VERBOSE_FDD
	    if (errorlog)
				fprintf(errorlog, "CPM image #%d ord_cylinders C:%2d H:%d S:%2d REC:%d -> 0x%08X\n", curdisk, bdos_trk[curdisk], h, s, r, offs * RECL);
#endif
	    break;

/* TRACK  0   1   2        n/2-1 n/2  n/2+1  n/2+3                      n-1 */
/* F/B   f0, f1, f2 ... f(n/2)-1, b0,    b1,    b2 ... b(n/2)-1 */
	case ORD_EAGLE:
			offs = (bdos_trk[curdisk] % f->cylinders) * 2;
			if (bdos_trk[curdisk] >= f->cylinders)
				offs = f->cylinders * 2 - 1 - offs;
			offs *= f->dpb.spt * RECL;
			o = bdos_sec[curdisk] * RECL;
			r = (o % f->seclen) / RECL;
			s = o / f->seclen;
			h = (s >= f->spt) ? f->side2[0] : f->side1[0];
			s = (s >= f->spt) ? f->side2[s+1] : f->side1[s+1];
			s -= f->side1[1];       /* subtract first sector number */
			offs += (h * f->spt + s) * f->seclen + r * RECL;
#if VERBOSE_FDD
	    if (errorlog)
				fprintf(errorlog, "CPM image #%d ord_eagle     C:%2d H:%d S:%2d REC:%d -> 0x%08X\n", curdisk, bdos_trk[curdisk], h, s, r, offs);
#endif
	    break;

/* TRACK         0              1         2               n-1 */
/* F/B   f0/b0, f1/b1 f2/b2 ... fn-1/bn-1 */
		case ORD_SIDES:
		default:
			offs = bdos_trk[curdisk];
			offs *= f->dpb.spt * RECL;
			o = bdos_sec[curdisk] * RECL;
			r = (o % f->seclen) / RECL;
			s = o / f->seclen;
			h = (s >= f->spt) ? f->side2[0] : f->side1[0];
			s = (s >= f->spt) ? f->side2[s+1] : f->side1[s+1];
			s -= f->side1[1];       /* subtract first sector number */
			offs += (h * f->spt + s) * f->seclen + r * RECL;
#if VERBOSE_FDD
	    if (errorlog)
				fprintf(errorlog, "CPM image #%d ord_sides     C:%2d H:%d S:%2d REC:%d -> 0x%08X\n", curdisk, bdos_trk[curdisk], h, s, r, offs);
#endif
	    break;

    }
	osd_fseek(fp[curdisk], offs, SEEK_SET);
}


/*****************************************************************************
 * cpm_disk_select
 * select a disk drive and return it's DPH (disk parameter header)
 * offset in Z80s memory range.
 *****************************************************************************/
int  cpm_disk_select(int d)
{
int return_dph = 0;

    curdisk = d;
	switch (curdisk)
	{
		case 0:
			if (num_disks > 0)
				return_dph = DPH0;

	    break;
		case 1:
			if (num_disks > 1)
				return_dph	= DPH1;

			break;
		case 2:
			if (num_disks > 2)
				return_dph	= DPH2;

			break;
		case 3:
			if (num_disks > 3)
				return_dph	= DPH3;

			break;
	}
	if (fp[curdisk] == REAL_FDD)
		fdd_select();
	return return_dph;

}

void cpm_disk_set_track(int t)
{
    if (curdisk >= 0 && curdisk < num_disks)
		bdos_trk[curdisk] = t;
}

void cpm_disk_set_sector(int s)
{
	if (curdisk >= 0 && curdisk < num_disks)
		bdos_sec[curdisk] = s;
}

void cpm_disk_home(void)
{
    cpm_disk_set_track(0);
	if (fp[curdisk] == REAL_FDD)
		fdd_set_track(0);
}

void cpm_disk_set_dma(int d)
{
	dma = d;
}

static void dump_sector(void)
{
byte *DMA = &Machine->memory_region[0][dma];
    if (errorlog)
    {
    int i;
	for (i = 0x00; i < 0x80; i += 0x10)
	{
			fprintf(errorlog,"%02X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		i,
		DMA[i+0x00],DMA[i+0x01],DMA[i+0x02],DMA[i+0x03],
		DMA[i+0x04],DMA[i+0x05],DMA[i+0x06],DMA[i+0x07],
		DMA[i+0x08],DMA[i+0x09],DMA[i+0x0a],DMA[i+0x0b],
		DMA[i+0x0c],DMA[i+0x0d],DMA[i+0x0e],DMA[i+0x0f],
		CP(DMA[i+0x00]),CP(DMA[i+0x01]),CP(DMA[i+0x02]),CP(DMA[i+0x03]),
		CP(DMA[i+0x04]),CP(DMA[i+0x05]),CP(DMA[i+0x06]),CP(DMA[i+0x07]),
		CP(DMA[i+0x08]),CP(DMA[i+0x09]),CP(DMA[i+0x0a]),CP(DMA[i+0x0b]),
		CP(DMA[i+0x0c]),CP(DMA[i+0x0d]),CP(DMA[i+0x0e]),CP(DMA[i+0x0f]));
	}
    }
}

int cpm_disk_read_sector(void)
{
int result = -1;
	/* TODO: remove this */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    
    if (curdisk >= 0 &&
		curdisk < num_disks &&
		fmt[curdisk] != -1 &&
		bdos_trk[curdisk] >= 0 &&
		bdos_trk[curdisk] < formats[fmt[curdisk]].sides * formats[fmt[curdisk]].cylinders)
	{
		if (fp[curdisk] == REAL_FDD)
			result = fdd_read_sector();
		else
		{
			if (fp[curdisk])
			{
				cpm_disk_image_seek();
				if (osd_fread(fp[curdisk], &RAM[dma], RECL) == RECL)
					result = 0;
			}
		}
	}
	if (result)
		if (errorlog)
			fprintf(errorlog, "BDOS Err on %c: Select\n", curdisk + 'A');
	if (errorlog)
		dump_sector();
    return result;
}

int cpm_disk_write_sector(void)
{
int result = -1;
	/* TODO: remove this */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    if (curdisk >= 0 &&
		curdisk < num_disks &&
		fmt[curdisk] != -1 &&
		bdos_trk[curdisk] >= 0 &&
		bdos_trk[curdisk] < formats[fmt[curdisk]].sides * formats[fmt[curdisk]].cylinders)
	{
		if (fp[curdisk] == REAL_FDD)
			result = fdd_write_sector();
		else
		{
			if (fp[curdisk])
			{
				cpm_disk_image_seek();
				if (osd_fwrite(fp[curdisk], &RAM[dma], RECL) == RECL)
					result = 0;
			}
		}
	}
	if (result)
		if (errorlog)
			fprintf(errorlog, "BDOS Err on %c: Select\n", curdisk + 'A');
	return result;
}

int  cpm_bios_command_r(int offset)
{
	return 0xff;
}

void cpm_bios_command_w(int offset, int data)
{
  dsk_fmt *f;
  char buff[256];
  byte * RAM = Machine->memory_region[0];
  Z80_Regs r;
  int i;
  
  Z80_GetRegs(&r);
  
  switch (data)
    {
    case 0x00: /* CBOOT */
#if VERBOSE
      if (errorlog)
	fprintf(errorlog, "BIOS 00 cold boot\n");
#endif
      
      cpm_conout_str("MESS CP/M 2.2 Emulator\r\n\n");
      for (i = 0; i < NDSK; i++)
	{
	  if (fmt[i] != -1)
	    {
	      f = &formats[fmt[i]];
	      sprintf(buff, "Drive %c: '%s' %5dK (%d %s%s %2d,%4d) %d %dK blocks\r\n",
		      'A'+i,
		      f->id,
		      f->cylinders * f->sides * f->spt * f->seclen / 1024,
		      f->cylinders,
		      (f->sides == 2) ? "DS" : "SS",
		      (f->density == DEN_FM_LO || f->density == DEN_FM_HI) ? "SD" : "DD",
		      f->spt, f->seclen,
		      f->dpb.dsm + 1, (RECL << f->dpb.bsh) / 1024);
	      cpm_conout_str(buff);
	    }
	}
      /* change vector at 0000 to warm start */
      zeropage0[0] = 0xc3;
      zeropage0[1] = (BIOS + 3) & 0xff;
      zeropage0[2] = (BIOS + 3) >> 8;
      /* fall through */
      
    case 0x01: /* WBOOT */
#if VERBOSE
      if (errorlog)
	fprintf(errorlog, "BIOS 01 warm boot\n");
#endif
      zeropage0[4] = curdisk;
      /* copy substantial zero page data */
      for (i = 0; i < 8; i++)
	RAM[0x0000 + i] = zeropage0[i];
      
      /* copy the CP/M 2.2 image to Z80 memory space */
      for (i = 0; i < 0x1600; i++)
	RAM[CCP + i] = Machine->memory_region[2][i];
      
      /* build the bios jump table */
      cpm_jumptable();
      
      r.BC.W.l = curdisk;     /* current disk */
      r.PC.W.l = CCP + 3;
      r.SP.W.l = 0x0080;
      change_pc16(r.PC.D);
      break;
      
    case 0x02: /* CSTAT */
      r.AF.B.h = cpu_readport(BIOS_CONST);
#if VERBOSE_CONIO
      if (errorlog)
	fprintf(errorlog, "BIOS 02 console status      A:%02X\n", r.AF.B.h);
#endif
      break;
      
    case 0x03: /* CONIN */
      r.AF.B.h = cpu_readport(BIOS_CONIN);
#if VERBOSE_CONIO
      if (timer_iscpususpended(0))
	{
	  if (errorlog)
	    fprintf(errorlog, "BIOS 03 console input       CPU suspended\n");
	}
      else
	{
	  if (errorlog)
	    fprintf(errorlog, "BIOS 03 console input       A:%02X\n", r.AF.B.h);
	}
#endif
      break;
      
    case 0x04: /* CONOUT */
#if VERBOSE_CONIO
      if (errorlog)
	fprintf(errorlog, "BIOS 04 console output      C:%02X\n", r.BC.B.l);
#endif
      cpm_conout_chr(r.BC.B.l);
      break;
      
    case 0x05: /* LSTOUT */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 05 list output         C:%02X\n", r.BC.B.l);
#endif
      if (lp)
	osd_fwrite(lp, &r.BC.B.l, 1);
      break;
      
      
    case 0x06: /* PUNOUT */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 06 punch output        C:%02X\n", r.BC.B.l);
#endif
      break;
      
      
    case 0x07: /* RDRIN */
      r.AF.B.h = 0x00;
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 07 reader input        A:%02X\n", r.AF.B.h);
#endif
      break;
      
      
    case 0x08: /* HOME */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 08 home\n");
#endif
      cpm_disk_home();
      break;
      
      
    case 0x09: /* SELDSK */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 09 select disk         C:%02X\n", r.BC.B.l);
#endif
      r.HL.W.l = cpm_disk_select(r.BC.B.l);
      break;
      
      
    case 0x0a: /* SETTRK */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 0A set bdos_trk        BC:%04X\n", r.BC.W.l);
#endif
      cpm_disk_set_track(r.BC.W.l);
      break;
      
      
    case 0x0b: /* SETSEC */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 0B set bdos_sec        C:%02X\n", r.BC.B.l);
#endif
      cpm_disk_set_sector(r.BC.B.l);
      break;
      
      
    case 0x0c: /* SETDMA */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 0C set DMA             BC:%04X\n", r.BC.W.l);
#endif
      cpm_disk_set_dma(r.BC.W.l);
      break;
      
      
    case 0x0d: /* RDSEC */
      r.AF.B.h = cpm_disk_read_sector();
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 0D read sector         A:%02X\n", r.AF.B.h);
#endif
      break;
      
      
    case 0x0e: /* WRSEC */
      r.AF.B.h = cpm_disk_write_sector();
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 0E write sector        A:%02X\n", r.AF.B.h);
#endif
      break;
      
    case 0x0f: /* LSTSTA */
      r.AF.B.h = 0xff;        /* always ready */
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 0F list status         A:%02X\n", r.AF.B.h);
#endif
      break;
      
    case 0x10: /* SECTRA */
      /* mvi b,0 */
      r.BC.B.h = 0;
      if (r.DE.W.l)                           /* translation table ? */
	{
	  /* xchg    */
	  i = r.HL.D;
	  r.HL.D = r.DE.D;
	  r.DE.D = i;
	  /* dad b   */
	  r.HL.W.l += r.BC.W.l;
	  /* mov a,m */
	  r.AF.B.h = cpu_readmem16(r.HL.D);
	  /* mov l,a */
	  r.HL.B.l = r.AF.B.h;
	}
      else
	{
	  /* mov l,c */
	  /* mov h,b */
	  r.HL.D = r.BC.D;
	}
      if (curdisk >= 0 && curdisk < num_disks)
	bdos_sec[curdisk] = r.HL.B.l;
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 10 sector trans        BC:%04X DE:%04X HL:%04X A:%02X\n", r.BC.W.l, r.DE.W.l, r.HL.W.l, r.AF.B.h);
#endif
      break;
      
      
    case 0x11: /* TRAP */
      sprintf(buff,
	      "Program traps in BIOS:\r\n" \
	      "BC:%04X DE:%04X HL:%04X AF:%04X SP:%04X IX:%04X IY:%04X\r\n",
	      r.BC.W.l,r.DE.W.l,r.HL.W.l,r.AF.W.l,r.SP.W.l,r.IX.W.l,r.IY.W.l);
      cpm_conout_str(buff);
#if VERBOSE_BIOS
      if (errorlog)
	fprintf(errorlog, "BIOS 11 trap                BC:%04X DE:%04X HL:%04X A:%02X\n", r.BC.W.l, r.DE.W.l, r.HL.W.l, r.AF.B.h);
#endif
      break;
    }
  
  Z80_SetRegs(&r);
}

