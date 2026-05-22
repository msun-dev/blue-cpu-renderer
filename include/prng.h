// Original code: https://www.pcg-random.org

#define pcguint32_t unsigned long
#define pcguint64_t unsigned long long

typedef struct { pcguint64_t state; pcguint64_t inc; } prng_t;

pcguint32_t pcg32_random_r(prng_t* rng)
{
	pcguint64_t oldstate = rng->state;
	// Advance internal state
	rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
	// Calculate output function (XSH RR), uses old state for max ILP
	pcguint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	pcguint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
