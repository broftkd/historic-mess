#ifndef RIOT_H
#define RIOT_H

#include "osd_cpu.h"

#define MAX_RIOTS   4


struct RIOTinterface {
	int num_chips;
	int baseclock[MAX_RIOTS];
	int (*port_a_r[MAX_RIOTS])(int chip);
	int (*port_b_r[MAX_RIOTS])(int chip);
	void (*port_a_w[MAX_RIOTS])(int chip, int data);
	void (*port_b_w[MAX_RIOTS])(int chip, int data);
	void (*irq_callback[MAX_RIOTS])(int chip);
};

/* This has to be called from a driver at startup */
void riot_init(struct RIOTinterface *riot);

int riot_r(int chip, int offs);
void riot_w(int chip, int offs, int data);

int riot_0_r(int offs);
void riot_0_w(int offs, int data);
int riot_1_r(int offs);
void riot_1_w(int offs, int data);
int riot_2_r(int offs);
void riot_2_w(int offs, int data);
int riot_3_r(int offs);
void riot_3_w(int offs, int data);

#endif