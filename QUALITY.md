Quality and speed of the tested rng functions
=============================================
v3.31.2beta https://rurban.github.io/dieharder/QUALITY.md (WIP)

`rng_name _g<num> :<num_weak>`

BEST
----
_no FAILED, 0 WEAK_
| rng function              |    ints/sec| doubles/sec |
|:--------------------------|-----------:|------------:|
|wyrand_g210_a_Y1.out:0	    |    319345  |     283551  |
|xoshiro128+_g213_a.out:0  	|    289544  |     257579  |
|xoroshiro64ss_g214_a.out:0	|    261519  |     267279  |
|xoshiro128++_g211_a.out:0 	|    260260  |     246457  |
|xoroshiro64s_g215_a_Y1.out:0|   254123  |     272197  |
|lxm_232_a_Y1.out:0         |    215959  |     205161  |
|efiix64_g231_a.out 	    |    173178  |     171191  |

GOOD
----
_no FAILED, 1 WEAK_
| rng function              |    ints/sec| doubles/sec |
|:--------------------------|-----------:|------------:|
|xoroshiro128+_g221_a.out:1	|    250752  |     257546  |
|xoroshiro128ss_g220_a.out:1|    258438  |     263123  |
|xoshiro128ss_g212_a.out:1	|    256337  |     234257  |
|romutrio_g234_a_Y1.out:1   |    275307  |     286574  |
|mt19937_g013_a.out:1		|    118291  |     111515  |
|ca_g203_a.out:1			|    117518  |      70488  |
|mt19937_1998_g015_a.out:1	|    106327  |     101035  |
|R_mersenne_twister_g403_a.out:1| 88174  |     121088  |
|ranlux389_g044_a.out:1     |     10980  |      11295  |

WEAK
----
_no FAILED, >1 WEAK_
| rng function              |    ints/sec| doubles/sec |
|:--------------------------|-----------:|------------:|
|pcg64_g225_a.out:2         |    237129  |     227795  |
|taus_g052_a.out:2          |    215197  |     226952  |
|taus2_g053_a.out:2         |    214293  |     226557  |
|kiss_g208_a.out:2          |    193869  |     188893  |
|hc-128_g232_a_Y1.out:2     |    153583  |      99089  |
|Threefish_OFB_g206_a.out:2 |     92264  |      87436  |
|ranlxd2_g046_a.out:2       |      7344  |       7381  |
|tt800_g056_a.out:3         |    201869  |     192733  |
|uvag_g204_a.out:3          |    184928  |     168279  |
|mt19937_1999_g014_a.out:3  |    125415  |     106326  |
|R_wichmann_hill_g400_a.out:3|    91092  |     107046  |
|AES_OFB_g205_a.out:3       |     41845  |      43167  |
|ranlxd1_g045_a.out:3       |     12999  |      13124  |
|jsf_g222_a.out:4           |    268269  |     267802  |
|romuquad_g235_a_Y1.out:4   |    277531  |     258264  |
|pcg32_g224_a.out:5         |    289259  |     290343  |

BAD
---
`rng_name _g<num> :<num_failed> :<num_weak>`

_some FAILED, sorted from better to worst. (Note: 2 always fail)_

    knuthran_g007_a.out:3:1
    ran1_g018_a.out:3:1
    R_knuth_taocp2_g405_a.out:3:1
    ran2_g019_a.out:3:2
    cmrg_g001_a.out:3:2
    mrg_g012_a.out:3:2
    knuthran2002_g009_a.out:3:2
    fishman2x_g005_a.out:3:3
    knuthran2_g008_a.out:3:3
    random256-libc5_g028_a.out:3:3
    random256-glibc2_g027_a.out:3:5
    random256-bsd_g026_a.out:3:6
    R_knuth_taocp_g404_a.out:3:7
    superkiss_g209_a.out:4:1
    zuf_g061_a.out:4:1
    ranlux_g043_a.out:4:1
    ranlux389_g044_a.out:4:1
    ranlxs0_g047_a.out:4:2
    uni32_g058_a.out:4:2
    random128-libc5_g025_a.out:4:3
    ranlxs2_g049_a.out:4:3
    random-libc5_g040_a.out:4:4
    ranlxs1_g048_a.out:4:4
    ranmar_g050_a.out:4:4
    random128-bsd_g023_a.out:4:5
    random128-glibc2_g024_a.out:4:5
    random-glibc2_g039_a.out:4:5
    random-bsd_g038_a.out:4:6
    dev_random-5.7.15-x86_64_g500_a.out:5:0
    rand48_g022_a.out:6:2
    ranf_g042_a.out:6:2
    fishman18_g003_a.out:6:4
    R_super_duper_g402_a.out:6:5
    fishman20_g004_a.out:6:6
    lecuyer21_g010_a.out:7:3
    R_marsaglia_multic._g401_a.out:7:4
    random64-glibc2_g033_a.out:7:8
    uni_g057_a.out:8:2
    ran0_g017_a.out:8:3
    minstd_g011_a.out:8:5
    random64-bsd_g032_a.out:8:6
    random64-libc5_g034_a.out:10:4


VERY BAD
--------
_>20 FAILED_

    random32-libc5_g031_a.out:44:3
    random32-glibc2_g030_a.out:44:7
    random32-bsd_g029_a.out:44:8
    jsf64_g223_a.out:45:1
    ran3_g020_a.out:48:4
    vax_g059_a.out:54:8
    rand_g021_a.out:57:6
    random8-bsd_g035_a.out:57:7
    random8-glibc2_g036_a.out:59:3
    random8-libc5_g037_a.out:59:2
    coveyou_g002_a.out:61:4
    transputer_g055_a.out:63:2
    waterman14_g060_a.out:64:1
    borosh13_g000_a.out:64:4
    randu_g041_a.out:70:5
    slatec_g051_a.out:77:4
