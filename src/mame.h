#ifndef MACHINE_H
#define MACHINE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern char messversion[]; /* MESS */
extern FILE *errorlog;

extern int framecount; /* MESS */
extern int frameskip; /* MESS */

#define MAX_GFX_ELEMENTS 32 /* MESS */
#define MAX_MEMORY_REGIONS 10

#define MAX_LAYERS 4	/* MAX_LAYERS is the maximum number of gfx layers */
						/* which we can handle. Currently, 4 is enough. */

struct RunningMachine
{
	unsigned char *memory_region[MAX_MEMORY_REGIONS];
	struct GfxElement *gfx[MAX_GFX_ELEMENTS];	/* graphic sets (chars, sprites) */
	struct osd_bitmap *scrbitmap;	/* bitmap to draw into */
	struct GfxLayer *dirtylayer;	/* for GfxLayer games: keep track of dirty portions of scrbitmap */
	struct GfxLayer *layer[MAX_LAYERS];
	unsigned short *pens;	/* remapped palette pen numbers. When you write */
							/* directly to a bitmap, never use absolute values, */
							/* use this array to get the pen number. For example, */
							/* if you want to use color #6 in the palette, use */
							/* pens[6] instead of just 6. */
	unsigned short *colortable;	/* lookup table used to map gfx pen numbers to palette pen numbers */
	const struct GameDriver *gamedrv;	/* contains the definition of the game machine */
	const struct MachineDriver *drv;	/* same as gamedrv->drv */
	int sample_rate;	/* the digital audio sample rate; 0 if sound is disabled. */
						/* This is set to a default value, or a value specified by */
						/* the user; osd_init() is allowed to change it to the actual */
						/* sample rate supported by the audio card. */
	int sample_bits;	/* 8 or 16 */
	struct GameSamples *samples;	/* samples loaded from disk */
	struct InputPort *input_ports;	/* the input ports definition from the driver */
								/* is copied here and modified (load settings from disk, */
								/* remove cheat commands, and so on) */
	int orientation;	/* see #defines in driver.h */
	struct GfxElement *uifont;	/* font used by DisplayText() */
	int uiwidth;
	int uiheight;
	int uixmin;
	int uiymin;
};

struct GameOptions {
	FILE *errorlog;
	void *record;
	void *playback;
	int mame_debug;
	int frameskip;
	int cheat;
	int samplerate;
	int samplebits;
	int norotate;
	int ror;
	int rol;
	int flipx;
	int flipy;
	char rom_name[MAX_ROM][32];  /* MESS */
	char floppy_name[MAX_FLOPPY][32]; /* MESS */
	char hard_name[MAX_HARD][32]; /* MESS */
	char cassette_name[MAX_CASSETTE][32]; /* MESS */
};

extern struct RunningMachine *Machine;

int run_game (int game, struct GameOptions *options);
int updatescreen(void);
/* osd_fopen() must use this to know if high score files can be used */
int mame_highscore_enabled(void);


#endif
