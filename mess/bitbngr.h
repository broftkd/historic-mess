struct bitbanger_config
{
	/* filter function; returns non-zero if input accepted */
	int (*filter)(int id, const int *pulses, int total_pulses, int total_duration);
	double pulse_threshhold;			/* the maximum duration pulse that we will consider */
	double pulse_tolerance;				/* deviation tolerance for pulses */
	int minimum_pulses;					/* the minimum amount of pulses before we start analyzing */
	int maximum_pulses;					/* the maximum amount of pulses that we will track */
	int begin_pulse_value;				/* the begining value of a pulse */
	int initial_value;					/* the initial value of the bitbanger line */
};

int bitbanger_init(int id, const struct bitbanger_config *config);
void bitbanger_output(int id, int value);