/***************************************************************************

Cobra Command:

  Video hardware looks like early version of Bad Dudes, we appear to have
  2 playfields and 1 text layer.  Each playfield has 2 scroll registers and
  4 control bytes:

  Register 1: MSB set is reverse screen (playfield 1 only), always seems to be 2
  Register 2: Unknown, seems to be always 0
  Register 3: Unknown, 3 in foreground, 0 in background (so transparency setting?)
  Register 4: Probably playfield shape, only 2 * 2 supported at moment (1)

  Sprite hardware appears to be the same as Bad Dudes.
  256 colours, palette generated by ram.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static unsigned char *pf_video,*pf_dirty,*oscar_shared_mem;
static int scroll1[4],scroll2[4],pf1_attr[8],pf2_attr[8];
static struct osd_bitmap *pf1_bitmap,*pf2_bitmap;

void dec8_pf1_w(int offset, int data)
{
	switch (offset)
	{
  	case 0:
    case 2:
    case 4:
    case 6:
      pf1_attr[offset]=data;
  }
  if (errorlog) fprintf(errorlog,"Write %d to playfield 1 register %d\n",data,offset);
}

void dec8_pf2_w(int offset, int data)
{
	switch (offset)
	{
  	case 0:
    case 2:
    case 4:
    case 6:
      pf2_attr[offset]=data;
  }
  if (errorlog) fprintf(errorlog,"Write %d to playfield 2 register %d\n",data,offset);
}

void dec8_scroll1_w(int offset, int data)
{
	scroll1[offset]=data;
}

void dec8_scroll2_w(int offset, int data)
{
	scroll2[offset]=data;
}



void dec8_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{

	int my,mx,offs,color,tile;

	int scrollx,scrolly;


	if (palette_recalc())
	{
		memset(pf_dirty,1,0x800);
	}


  /* Playfield 2 - Foreground */

  mx=-1; my=0;

  for (offs = 0x000;offs < 0x400; offs += 2) {

    mx++;

    if (mx==16) {mx=0; my++;}

    if (!pf_dirty[offs/2]) continue; else pf_dirty[offs/2]=0;



    tile=pf_video[offs+1]+(pf_video[offs]<<8);

    color = ((tile & 0xf000) >> 12);

    tile=tile&0xfff;



		drawgfx(pf2_bitmap,Machine->gfx[2],tile,

			color+12, 0,0, 16*mx,16*my,

		 	0,TRANSPARENCY_NONE,0);

	}



  mx=-1; my=0;

  for (offs = 0x400;offs < 0x800; offs += 2) {

    mx++;

    if (mx==16) {mx=0; my++;}

    if (!pf_dirty[offs/2]) continue; else pf_dirty[offs/2]=0;



    tile=pf_video[offs+1]+(pf_video[offs]<<8);

    color = ((tile & 0xf000) >> 12);

    tile=tile&0xfff;



		drawgfx(pf2_bitmap,Machine->gfx[2],tile,

			color+12, 0,0, (16*mx)+256,16*my,

		 	0,TRANSPARENCY_NONE,0);

	}



  /* Playfield 1 *

  my=-1; mx=0;

  for (offs = 0x800;offs < 0xc00; offs += 2) {

    my++;

    if (my==16) {my=0; mx++;}

    if (!pf_dirty[offs/2]) continue; else pf_dirty[offs/2]=0;



    tile=pf_video[offs+1]+(pf_video[offs]<<8);

    color = ((tile & 0xf000) >> 12);

    tile=tile&0xfff;



		drawgfx(pf1_bitmap,Machine->gfx[3],tile,

			color+4, 0,0, (16*mx)+256,16*my,

		 	0,TRANSPARENCY_NONE,0);

	}



  my=-1; mx=0;

  for (offs = 0xc00;offs < 0x1000; offs += 2) {

    my++;

    if (my==16) {my=0; mx++;}

    if (!pf_dirty[offs/2]) continue; else pf_dirty[offs/2]=0;



    tile=pf_video[offs+1]+(pf_video[offs]<<8);

    color = ((tile & 0xf000) >> 12);

    tile=tile&0xfff;



		drawgfx(pf1_bitmap,Machine->gfx[3],tile,

			color+4, 0,0, 16*mx,(16*my)+256,

		 	0,TRANSPARENCY_NONE,0);

	}



  scrolly=-((scroll1[2]<<8)+scroll1[3]);

  scrollx=-((scroll1[0]<<8)+scroll1[1]);

//  copyscrollbitmap(bitmap,pf1_bitmap,1,&scrollx,1,&scrolly,0,TRANSPARENCY_NONE,0);

          */

  scrolly=-((scroll2[2]<<8)+scroll2[3]);

  scrollx=-((scroll2[0]<<8)+scroll2[1]);

  copyscrollbitmap(bitmap,pf2_bitmap,1,&scrollx,1,&scrolly,0,TRANSPARENCY_NONE,0);



	/* Sprites */

	for (offs = 0;offs < 0x800;offs += 8)

	{

		int x,y,sprite,colour,multi,fx,fy,inc;



		y =spriteram[offs+1]+(spriteram[offs]<<8);

		if ((y&0x8000) == 0) continue;



		x = spriteram[offs+5]+(spriteram[offs+4]<<8);

		colour = ((x & 0x3000) >> 12)+4;



		fx = y & 0x2000;

		fy = y & 0x4000;

		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */

											/* multi = 0   1   3   7 */



		sprite = spriteram[offs+3]+(spriteram[offs+2]<<8);

    sprite &= 0x0fff;



		x = x & 0x01ff;

		y = y & 0x01ff;

		if (x >= 256) x -= 512;

		if (y >= 256) y -= 512;

		x = 240 - x;

		y = 240 - y;



		sprite &= ~multi;

		if (fy)

			inc = -1;

		else

		{

			sprite += multi;

			inc = 1;

		}



		while (multi >= 0)

		{

			drawgfx(bitmap,Machine->gfx[1],

					sprite - multi * inc,

					colour,

					fx,fy,

					x,y - 16 * multi,

					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);



			multi--;

		}

	}



   /* Draw character tiles */

   for (offs = 0x800 - 2;offs >= 0;offs -= 2) {

      tile=videoram[offs+1]+((videoram[offs]&0xF)<<8);

      if (!tile) continue;

      color=(videoram[offs]&0x70)>>4;



			mx = (offs/2) % 32;

			my = (offs/2) / 32;

			drawgfx(bitmap,Machine->gfx[0],

				tile,color,0,0,8*mx,8*my,

				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

   }

   /*
    {
   	int i,j;

	char buf[80];

	int trueorientation;

	struct osd_bitmap *bitmap = Machine->scrbitmap;



	trueorientation = Machine->orientation;

	Machine->orientation = ORIENTATION_DEFAULT;



	sprintf(buf,"%04X %04X %02X %02X %02X %02X",(scroll1[0]<<8)+scroll1[1],(scroll1[2]<<8)+scroll1[3],pf1_attr[0],pf1_attr[2],pf1_attr[4],pf1_attr[6]);

	for (j = 0;j < 22;j++)

		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*0+8*j,8*2,0,TRANSPARENCY_NONE,0);



	sprintf(buf,"%04X %04X %02X %02X %02X %02X",(scroll2[0]<<8)+scroll2[1],(scroll2[2]<<8)+scroll2[3],pf2_attr[0],pf2_attr[2],pf2_attr[4],pf2_attr[6]);

	for (j = 0;j < 22;j++)

		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*0+8*j,8*4,0,TRANSPARENCY_NONE,0);





	Machine->orientation = trueorientation;

}
  */

}

int dec8_video_r(int offset)
{
	return pf_video[offset];
}

void dec8_video_w(int offset, int data)
{
	if (pf_video[offset]!=data)
  {
  	pf_video[offset]=data;
		pf_dirty[offset/2] = 1;
  }
}

int oscar_share_r(int offset)
{
	return oscar_shared_mem[offset];
}

void oscar_share_w(int offset, int data)
{
	oscar_shared_mem[offset]=data;
}

int dec8_vh_start (void)
{

	pf1_bitmap=osd_create_bitmap(512,512);

  pf2_bitmap=osd_create_bitmap(512,512);



  pf_video=malloc(0x1000);
  pf_dirty=malloc(0x800);
  oscar_shared_mem=malloc(0x1000);

  memset(pf_dirty,1,0x800);

{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
  /* Should really go in a machine start but hey.. */
  cpu_setbank(1,&RAM[0x10000]);
}

  return 0;
}

void dec8_vh_stop (void)
{
	osd_free_bitmap(pf1_bitmap);
	osd_free_bitmap(pf2_bitmap);
	free(pf_video);
	free(pf_dirty);
	free(oscar_shared_mem);
	generic_vh_stop();
}
