#ifndef __CRTC6845_H_
#define __CRTC6845_H_
/***************************************************************************

  motorola cathode ray tube controller 6845

  praster version

  copyright peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include "praster.h"

/* call to init videodriver */
extern void crtc6845_init (UINT8 *memory);
extern void crtc6845_pet_init (UINT8 *memory);
extern void crtc6845_cbm600_init (UINT8 *memory);
extern void crtc6845_cbm600pal_init (UINT8 *memory);
extern void crtc6845_cbm700_init (UINT8 *memory);

extern void crtc6845_set_rastering(int on);

#define crtc6845_vh_start praster_vh_start
#define crtc6845_vh_stop praster_vh_stop
#define crtc6845_vh_screenrefresh praster_vh_screenrefresh
#define crtc6845_raster_irq praster_raster_irq

//#define crtc6845_update praster_2_update
#define crtc6845_videoram_w praster_2_videoram_w

/* to be called when writting to port */
extern void crtc6845_port_w (int offset, int data);

/* special level of address line 11 for the cbm pet series */
extern void crtc6845_address_line_11(int level);
/* special level of address line 12 for the cbm b series */
extern void crtc6845_address_line_12(int level);

/* special port write for the 80 column hardware of the commodore pet */
extern void crtc6845_pet_port_w (int offset, int data);

/* to be called when reading from port */
extern int crtc6845_port_r (int offset);

extern void crtc6845_status (char *text, int size);
#endif
