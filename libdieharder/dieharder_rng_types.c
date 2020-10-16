/*
 * This is a hack of the GSL's rng/types.c:
 *
 * Copyright (C) 2001 Brian Gough
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************
 * This is being directly adapted from and modified for use in dieharder
 * so it can maintain its own independent RNG space of types not limited
 * by its internal limit of 100.  To avoid collisions, we'll start our number
 * space above 100, and extend it to 1000.
 *
 * While we're at it, let's define the ranges:
 *
 *   0-199 gsl generators (fixed order from now on with room for growth)
 *   200-399 libdieharder generators (fixed order from now on)
 *   400-499 R-based generators (fixed order from now on)
 *   500-599 hardware generators (e.g. /dev/random and friends)
 *   600-699 user-defined generators (starting with dieharder example)
 *   700-999 reserved for future integration with R-like environments
 *
 * Naturally, we can simply bump MAXRNGS and add more, but 1000 seems
 * likely to last for "a while" and maybe "forever".
 */

#include <dieharder/libdieharder.h>
FILE *test_fp;

const gsl_rng_type *dh_rng_types[MAXRNGS];
const gsl_rng_type **gsl_types;    /* where all the rng types go */

unsigned int dh_num_rngs;           /* dh rngs available in dieharder */
unsigned int dh_num_gsl_rngs;       /* GSL rngs available in dieharder */
unsigned int dh_num_dieharder_rngs; /* dh rngs available in libdieharder */
unsigned int dh_num_R_rngs;         /* R-derived rngs available in libdieharder */
unsigned int dh_num_hardware_rngs;  /* hardware rngs supported in libdieharder */
unsigned int dh_num_user_rngs;      /* user-added rngs */
unsigned int dh_num_reserved_rngs;  /* ngs added in reserved space by new UI */

gsl_rng *rng;                  /* global gsl random number generator */

void dieharder_rng_types()
{

 int i;

 /*
  * Null the whole thing for starters
  */
 for(i=0;i<MAXRNGS;i++) dh_rng_types[i] = 0;

 /*
  * Initialize gsl_types to fill it with the current gsl rngs.
  */
 gsl_types = gsl_rng_types_setup();

 /*
  * Copy its contents over into dieharder_rng_generator_types.
  */
 i = 0;
 while(gsl_types[i] != NULL){
   dh_rng_types[i] = gsl_types[i];
   i++;
 }
 dh_num_gsl_rngs = i;
 MYDEBUG(D_TYPES){
   printf("# startup:  Found %u GSL rngs.\n",dh_num_gsl_rngs);
 }

 /*
  * Now add the new ones in.  These positions are to be locked in by
  * order within the ranges, so we need to be careful to get them
  * "right" the first time.
  *
  * These are the dieharder generators.  I expect many users to use
  * stdin-based raw input since it is by far the easiest one to come up
  * with (and actually will work with e.g. /dev/random).  The file-based
  * inputs will also be fairly common.  The rest are there for convenience,
  * and to expose users to some new/interesting rngs.
  */
 i = 200;
 dh_num_dieharder_rngs = 0;

#define ADD_RNG(x) \
 ADD(gsl_rng_##x); \
 dh_num_dieharder_rngs++
 
 ADD_RNG (stdin_input_raw);
 ADD_RNG (file_input_raw);
 ADD_RNG (file_input);
 ADD_RNG (ca);
 ADD_RNG (uvag);
 ADD_RNG (aes);
 ADD_RNG (threefish);
 ADD_RNG (XOR);
 ADD_RNG (kiss);
 ADD_RNG (superkiss);

 ADD_RNG (wyrand);
 // 32bit
 ADD_RNG (xoshiro128_pp);
 ADD_RNG (xoshiro128_ss);
 ADD_RNG (xoshiro128_p);
 ADD_RNG (xoroshiro64_ss);
 ADD_RNG (xoroshiro64_s);
 // 64bit
 ADD_RNG (xoshiro256_pp);
 ADD_RNG (xoshiro256_ss);
 ADD_RNG (xoshiro256_p);
 ADD_RNG (xoroshiro128_pp);
 ADD_RNG (xoroshiro128_ss);
 ADD_RNG (xoroshiro128_p);

 //ADD_RNG (jsf);
 //ADD_RNG (jsf64);

 MYDEBUG(D_TYPES){
   printf("# startup:  Found %u dieharder rngs.\n",dh_num_dieharder_rngs);
 }


 /*
  * These are the R-based generators.  Honestly it would be lovely
  * to merge them with the GSL permanently.
  */
 i = 400;
 dh_num_R_rngs = 0;

#undef ADD_RNG
#define ADD_RNG(x) \
 ADD(gsl_rng_##x); \
 dh_num_R_rngs++

 ADD_RNG (r_wichmann_hill);
 ADD_RNG (r_marsaglia_mc);
 ADD_RNG (r_super_duper);
 ADD_RNG (r_mersenne_twister);
 ADD_RNG (r_knuth_taocp);
 ADD_RNG (r_knuth_taocp2);

 MYDEBUG(D_TYPES){
   printf("# startup:  Found %u R rngs.\n",dh_num_R_rngs);
 }

 /*
  * These are hardware/system generators.  Again, it would be lovely to
  * merge them with the GSL permanently.  It would also be good to wrap
  * these in conditionals so that they are added iff the hardware
  * interface exists.  Perhaps we should try doing this -- it requires a
  * call to stat, I believe.  But not now.
  */
 i = 500;
 dh_num_hardware_rngs = 0;
 if ((test_fp = fopen("/dev/random","r"))) {
   ADD(gsl_rng_dev_random);
   fclose(test_fp);
   dh_num_hardware_rngs++;
 }
 if ((test_fp = fopen("/dev/urandom","r"))) {
   ADD(gsl_rng_dev_urandom);
   fclose(test_fp);
   dh_num_hardware_rngs++;
 }
 if ((test_fp = fopen("/dev/arandom","r"))) {
   ADD(gsl_rng_dev_arandom);
   fclose(test_fp);
   dh_num_hardware_rngs++;
 }
 MYDEBUG(D_TYPES){
   printf("# startup:  Found %u hardware rngs.\n",dh_num_hardware_rngs);
 }

 /*
  * Tally up all the generators we found.
  */
 dh_num_rngs = dh_num_gsl_rngs + dh_num_dieharder_rngs + dh_num_R_rngs +
			dh_num_hardware_rngs;

}
