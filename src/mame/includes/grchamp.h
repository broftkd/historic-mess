/*************************************************************************

    Taito Grand Champ hardware

*************************************************************************/

#include "sound/discrete.h"

typedef struct _grchamp_state grchamp_state;
struct _grchamp_state
{
	UINT8		cpu0_out[16];
	UINT8		cpu1_out[16];

	UINT8		comm_latch;
	UINT8		comm_latch2[4];

	UINT16		ledlatch;
	UINT8		ledaddr;
	UINT16		ledram[8];

	UINT16		collide;
	UINT8		collmode;

	UINT8 *		radarram;
	UINT8 *		videoram;
	UINT8 *		leftram;
	UINT8 *		centerram;
	UINT8 *		rightram;
	UINT8 *		spriteram;

	mame_bitmap *work_bitmap;
	tilemap *	text_tilemap;
	tilemap *	left_tilemap;
	tilemap *	center_tilemap;
	tilemap *	right_tilemap;

	rgb_t		bgcolor[0x20];
};

/* Discrete Sound Input Nodes */
#define GRCHAMP_ENGINE_CS_EN				NODE_01
#define GRCHAMP_SIFT_DATA					NODE_02
#define GRCHAMP_ATTACK_UP_DATA				NODE_03
#define GRCHAMP_IDLING_EN					NODE_04
#define GRCHAMP_FOG_EN						NODE_05
#define GRCHAMP_PLAYER_SPEED_DATA			NODE_06
#define GRCHAMP_ATTACK_SPEED_DATA			NODE_07
#define GRCHAMP_A_DATA						NODE_08
#define GRCHAMP_B_DATA						NODE_09

/*----------- defined in audio/grchamp.c -----------*/

extern discrete_sound_block grchamp_discrete_interface[];

/*----------- defined in video/grchamp.c -----------*/

PALETTE_INIT( grchamp );
VIDEO_START( grchamp );
VIDEO_UPDATE( grchamp );
WRITE8_HANDLER( grchamp_left_w );
WRITE8_HANDLER( grchamp_center_w );
WRITE8_HANDLER( grchamp_right_w );
