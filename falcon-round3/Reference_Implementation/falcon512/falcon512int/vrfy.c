/*
 * Falcon signature verification.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

#include "inner.h"

/* ===================================================================== */
/*
 * Constants for NTT.
 *
 *   n = 2^logn  (2 <= n <= 1024)
 *   phi = X^n + 1
 *   q = 12289
 *   q0i = -1/q mod 2^16
 *   R = 2^16 mod q
 *   R2 = 2^32 mod q
 */

#define Q     12289
#define Q0I   12287
#define R      4091
#define R2    10952

/*
 * Table for NTT, binary case:
 *   GMb[x] = R*(g^rev(x)) mod q
 * where g = 7 (it is a 2048-th primitive root of 1 modulo q)
 * and rev() is the bit-reversal function over 10 bits.
 */
static const uint16_t GMb[] = {
	 4091,  7888, 11060, 11208,  6960,  4342,  6275,  9759,
	 1591,  6399,  9477,  5266,   586,  5825,  7538,  9710,
	 1134,  6407,  1711,   965,  7099,  7674,  3743,  6442,
	10414,  8100,  1885,  1688,  1364, 10329, 10164,  9180,
	12210,  6240,   997,   117,  4783,  4407,  1549,  7072,
	 2829,  6458,  4431,  8877,  7144,  2564,  5664,  4042,
	12189,   432, 10751,  1237,  7610,  1534,  3983,  7863,
	 2181,  6308,  8720,  6570,  4843,  1690,    14,  3872,
	 5569,  9368, 12163,  2019,  7543,  2315,  4673,  7340,
	 1553,  1156,  8401, 11389,  1020,  2967, 10772,  7045,
	 3316, 11236,  5285, 11578, 10637, 10086,  9493,  6180,
	 9277,  6130,  3323,   883, 10469,   489,  1502,  2851,
	11061,  9729,  2742, 12241,  4970, 10481, 10078,  1195,
	  730,  1762,  3854,  2030,  5892, 10922,  9020,  5274,
	 9179,  3604,  3782, 10206,  3180,  3467,  4668,  2446,
	 7613,  9386,   834,  7703,  6836,  3403,  5351, 12276,
	 3580,  1739, 10820,  9787, 10209,  4070, 12250,  8525,
	10401,  2749,  7338, 10574,  6040,   943,  9330,  1477,
	 6865,  9668,  3585,  6633, 12145,  4063,  3684,  7680,
	 8188,  6902,  3533,  9807,  6090,   727, 10099,  7003,
	 6945,  1949,  9731, 10559,  6057,   378,  7871,  8763,
	 8901,  9229,  8846,  4551,  9589, 11664,  7630,  8821,
	 5680,  4956,  6251,  8388, 10156,  8723,  2341,  3159,
	 1467,  5460,  8553,  7783,  2649,  2320,  9036,  6188,
	  737,  3698,  4699,  5753,  9046,  3687,    16,   914,
	 5186, 10531,  4552,  1964,  3509,  8436,  7516,  5381,
	10733,  3281,  7037,  1060,  2895,  7156,  8887,  5357,
	 6409,  8197,  2962,  6375,  5064,  6634,  5625,   278,
	  932, 10229,  8927,  7642,   351,  9298,   237,  5858,
	 7692,  3146, 12126,  7586,  2053, 11285,  3802,  5204,
	 4602,  1748, 11300,   340,  3711,  4614,   300, 10993,
	 5070, 10049, 11616, 12247,  7421, 10707,  5746,  5654,
	 3835,  5553,  1224,  8476,  9237,  3845,   250, 11209,
	 4225,  6326,  9680, 12254,  4136,  2778,   692,  8808,
	 6410,  6718, 10105, 10418,  3759,  7356, 11361,  8433,
	 6437,  3652,  6342,  8978,  5391,  2272,  6476,  7416,
	 8418, 10824, 11986,  5733,   876,  7030,  2167,  2436,
	 3442,  9217,  8206,  4858,  5964,  2746,  7178,  1434,
	 7389,  8879, 10661, 11457,  4220,  1432, 10832,  4328,
	 8557,  1867,  9454,  2416,  3816,  9076,   686,  5393,
	 2523,  4339,  6115,   619,   937,  2834,  7775,  3279,
	 2363,  7488,  6112,  5056,   824, 10204, 11690,  1113,
	 2727,  9848,   896,  2028,  5075,  2654, 10464,  7884,
	12169,  5434,  3070,  6400,  9132, 11672, 12153,  4520,
	 1273,  9739, 11468,  9937, 10039,  9720,  2262,  9399,
	11192,   315,  4511,  1158,  6061,  6751, 11865,   357,
	 7367,  4550,   983,  8534,  8352, 10126,  7530,  9253,
	 4367,  5221,  3999,  8777,  3161,  6990,  4130, 11652,
	 3374, 11477,  1753,   292,  8681,  2806, 10378, 12188,
	 5800, 11811,  3181,  1988,  1024,  9340,  2477, 10928,
	 4582,  6750,  3619,  5503,  5233,  2463,  8470,  7650,
	 7964,  6395,  1071,  1272,  3474, 11045,  3291, 11344,
	 8502,  9478,  9837,  1253,  1857,  6233,  4720, 11561,
	 6034,  9817,  3339,  1797,  2879,  6242,  5200,  2114,
	 7962,  9353, 11363,  5475,  6084,  9601,  4108,  7323,
	10438,  9471,  1271,   408,  6911,  3079,   360,  8276,
	11535,  9156,  9049, 11539,   850,  8617,   784,  7919,
	 8334, 12170,  1846, 10213, 12184,  7827, 11903,  5600,
	 9779,  1012,   721,  2784,  6676,  6552,  5348,  4424,
	 6816,  8405,  9959,  5150,  2356,  5552,  5267,  1333,
	 8801,  9661,  7308,  5788,  4910,   909, 11613,  4395,
	 8238,  6686,  4302,  3044,  2285, 12249,  1963,  9216,
	 4296, 11918,   695,  4371,  9793,  4884,  2411, 10230,
	 2650,   841,  3890, 10231,  7248,  8505, 11196,  6688,
	 4059,  6060,  3686,  4722, 11853,  5816,  7058,  6868,
	11137,  7926,  4894, 12284,  4102,  3908,  3610,  6525,
	 7938,  7982, 11977,  6755,   537,  4562,  1623,  8227,
	11453,  7544,   906, 11816,  9548, 10858,  9703,  2815,
	11736,  6813,  6979,   819,  8903,  6271, 10843,   348,
	 7514,  8339,  6439,   694,   852,  5659,  2781,  3716,
	11589,  3024,  1523,  8659,  4114, 10738,  3303,  5885,
	 2978,  7289, 11884,  9123,  9323, 11830,    98,  2526,
	 2116,  4131, 11407,  1844,  3645,  3916,  8133,  2224,
	10871,  8092,  9651,  5989,  7140,  8480,  1670,   159,
	10923,  4918,   128,  7312,   725,  9157,  5006,  6393,
	 3494,  6043, 10972,  6181, 11838,  3423, 10514,  7668,
	 3693,  6658,  6905, 11953, 10212, 11922,  9101,  8365,
	 5110,    45,  2400,  1921,  4377,  2720,  1695,    51,
	 2808,   650,  1896,  9997,  9971, 11980,  8098,  4833,
	 4135,  4257,  5838,  4765, 10985, 11532,   590, 12198,
	  482, 12173,  2006,  7064, 10018,  3912, 12016, 10519,
	11362,  6954,  2210,   284,  5413,  6601,  3865, 10339,
	11188,  6231,   517,  9564, 11281,  3863,  1210,  4604,
	 8160, 11447,   153,  7204,  5763,  5089,  9248, 12154,
	11748,  1354,  6672,   179,  5532,  2646,  5941, 12185,
	  862,  3158,   477,  7279,  5678,  7914,  4254,   302,
	 2893, 10114,  6890,  9560,  9647, 11905,  4098,  9824,
	10269,  1353, 10715,  5325,  6254,  3951,  1807,  6449,
	 5159,  1308,  8315,  3404,  1877,  1231,   112,  6398,
	11724, 12272,  7286,  1459, 12274,  9896,  3456,   800,
	 1397, 10678,   103,  7420,  7976,   936,   764,   632,
	 7996,  8223,  8445,  7758, 10870,  9571,  2508,  1946,
	 6524, 10158,  1044,  4338,  2457,  3641,  1659,  4139,
	 4688,  9733, 11148,  3946,  2082,  5261,  2036, 11850,
	 7636, 12236,  5366,  2380,  1399,  7720,  2100,  3217,
	10912,  8898,  7578, 11995,  2791,  1215,  3355,  2711,
	 2267,  2004,  8568, 10176,  3214,  2337,  1750,  4729,
	 4997,  7415,  6315, 12044,  4374,  7157,  4844,   211,
	 8003, 10159,  9290, 11481,  1735,  2336,  5793,  9875,
	 8192,   986,  7527,  1401,   870,  3615,  8465,  2756,
	 9770,  2034, 10168,  3264,  6132,    54,  2880,  4763,
	11805,  3074,  8286,  9428,  4881,  6933,  1090, 10038,
	 2567,   708,   893,  6465,  4962, 10024,  2090,  5718,
	10743,   780,  4733,  4623,  2134,  2087,  4802,   884,
	 5372,  5795,  5938,  4333,  6559,  7549,  5269, 10664,
	 4252,  3260,  5917, 10814,  5768,  9983,  8096,  7791,
	 6800,  7491,  6272,  1907, 10947,  6289, 11803,  6032,
	11449,  1171,  9201,  7933,  2479,  7970, 11337,  7062,
	 8911,  6728,  6542,  8114,  8828,  6595,  3545,  4348,
	 4610,  2205,  6999,  8106,  5560, 10390,  9321,  2499,
	 2413,  7272,  6881, 10582,  9308,  9437,  3554,  3326,
	 5991, 11969,  3415, 12283,  9838, 12063,  4332,  7830,
	11329,  6605, 12271,  2044, 11611,  7353, 11201, 11582,
	 3733,  8943,  9978,  1627,  7168,  3935,  5050,  2762,
	 7496, 10383,   755,  1654, 12053,  4952, 10134,  4394,
	 6592,  7898,  7497,  8904, 12029,  3581, 10748,  5674,
	10358,  4901,  7414,  8771,   710,  6764,  8462,  7193,
	 5371,  7274, 11084,   290,  7864,  6827, 11822,  2509,
	 6578,  4026,  5807,  1458,  5721,  5762,  4178,  2105,
	11621,  4852,  8897,  2856, 11510,  9264,  2520,  8776,
	 7011,  2647,  1898,  7039,  5950, 11163,  5488,  6277,
	 9182, 11456,   633, 10046, 11554,  5633,  9587,  2333,
	 7008,  7084,  5047,  7199,  9865,  8997,   569,  6390,
	10845,  9679,  8268, 11472,  4203,  1997,     2,  9331,
	  162,  6182,  2000,  3649,  9792,  6363,  7557,  6187,
	 8510,  9935,  5536,  9019,  3706, 12009,  1452,  3067,
	 5494,  9692,  4865,  6019,  7106,  9610,  4588, 10165,
	 6261,  5887,  2652, 10172,  1580, 10379,  4638,  9949
};


static const uint32_t GMb_large[] = {
	65536, 12973364, 8770880, 13947155, 17328434, 4908543, 2878718, 6062702, 12572472, 
	3936934, 13208275, 19730568, 10953681, 11828114, 5236040, 11931819, 15752049, 
	23890657, 12070276, 1823739, 841701, 12216673, 2601177, 22866132, 12980370, 
	1520237, 13597699, 931887, 4629263, 3227807, 3389581, 6127157, 3254322, 
	6188301, 4741894, 20835948, 14262742, 15327555, 16946294, 24046780, 1079758, 
	24225514, 1735605, 10099602, 17762729, 19645267, 9065257, 20325140, 15278006, 
	22433110, 10193483, 2941733, 17892086, 17352139, 16819658, 19470335, 1162895, 
	22739847, 18463305, 11839852, 3258887, 12405694, 13146887, 88184, 5410697, 
	1296011, 19448050, 19975479, 5718759, 6877814, 10575742, 22423368, 16998768, 
	21775054, 16200878, 7351923, 16742486, 16355343, 4068438, 4573753, 6691027, 
	2570063, 22571028, 13057943, 21387537, 1072286, 21107115, 6456005, 20246003, 
	3158926, 8434322, 6943141, 23714099, 5440458, 7320822, 19879639, 22248231, 
	16385861, 18503807, 15188859, 15392526, 6407116, 24185279, 10959711, 13517846, 
	9258789, 19459106, 4836055, 4517356, 17123890, 22711297, 18216301, 1883190, 
	24136781, 8391602, 5112513, 13556640, 5303905, 7442153, 19075396, 714466, 
	11780668, 3854380, 2864788, 3367557, 17176662, 4284549, 13361436, 10909690, 
	5662648, 20013538, 658903, 15972170, 21217981, 4609315, 9951066, 19402596, 
	7053667, 8322016, 23434228, 13404924, 13913031, 2105925, 18838326, 14163140, 
	4921107, 9575009, 9937000, 19546739, 19375632, 21630522, 10147723, 22484054, 
	13439928, 15861482, 16419956, 9656808, 12932610, 10839328, 18265275, 3374002, 
	18629773, 11606388, 17084139, 1765019, 20048768, 1514547, 13666815, 5352612, 
	3480411, 17990635, 12952689, 9211702, 13882855, 19008526, 10986181, 23116824, 
	19964664, 18697601, 9466084, 2506326, 24073793, 14753044, 8790820, 9708221, 
	24228971, 7208849, 17688928, 14334567, 9481274, 7371674, 16473456, 6537685, 
	12028204, 4098617, 1289514, 13983317, 14471698, 9033902, 20981163, 5469416, 
	2022832, 17891765, 5676480, 312359, 22656615, 5503238, 22819544, 5322849, 
	20146931, 9407208, 13480499, 23469114, 11819381, 9165999, 2396970, 16234572, 
	18398616, 17370133, 472538, 20332087, 4772723, 4224102, 15248084, 15396923, 
	22709210, 6825987, 4331205, 5194852, 7049686, 4504340, 3475913, 19390757, 
	19945079, 6237032, 18023580, 9445552, 13002846, 4494503, 24328270, 6236605, 
	10095847, 12427577, 22809131, 14906987, 18607544, 2655341, 12416880, 6939996, 
	22030390, 4097841, 19977467, 6773786, 17673213, 11837888, 9955962, 3211264, 
	1111770, 14853423, 13447, 19253872, 20855238, 19021977, 4187906, 5875103, 
	22060479, 12622409, 14923233, 24182508, 18215643, 12495550, 23297188, 15232130, 
	23511266, 5674540, 16142088, 16836308, 12847993, 5422468, 22123623, 1455064, 
	1270490, 7297144, 21255422, 7170518, 11720297, 19647223, 7346201, 13019532, 
	10342257, 12689437, 20272771, 15477210, 18838965, 529012, 6754252, 4094060, 
	15512218, 11823522, 6739678, 16127286, 10743484, 4870855, 19650220, 16411064, 
	905545, 11339847, 22109712, 22465779, 20415417, 18730889, 2171816, 8167773, 
	15935658, 1641428, 18790805, 13243217, 22110022, 9614397, 4321016, 21053743, 
	14690457, 1079851, 2516831, 11741740, 19721353, 5663497, 428187, 3100238, 
	17474883, 12817710, 18545653, 14949461, 20386495, 4097134, 4450528, 10568790, 
	3897882, 7663527, 5256141, 22893591, 3727932, 9152913, 23459753, 15772507, 
	8345128, 22769122, 22922376, 14859924, 22512032, 17021704, 22227712, 16253515, 
	21881877, 3626026, 12042861, 22022544, 21064192, 13540703, 70937, 3384347, 
	14353923, 1621595, 17303326, 1687075, 9231216, 14536708, 13945273, 19055187, 
	11164301, 20675842, 6442727, 5285253, 15820935, 22966923, 7226846, 10601793, 
	15890789, 18015333, 18339407, 18568047, 11817044, 14686573, 20127298, 22026949, 
	8992301, 4381722, 7879206, 1611018, 14585347, 6193066, 23868455, 23259646, 
	3931109, 17266128, 1146245, 22258210, 22748412, 5562161, 20017457, 10596712, 
	21470874, 5441662, 23179221, 5915612, 21938410, 10392815, 9097607, 3401801, 
	23973406, 20594347, 23552532, 9449813, 23521865, 18579211, 16344999, 18883852, 
	9798360, 7351069, 7283417, 13264808, 6107992, 991680, 10683828, 18207578, 
	24097893, 2887639, 98695, 12046660, 21269788, 3950216, 1367967, 10000490, 
	1986896, 13121932, 104337, 774769, 8077889, 15094967, 15830483, 11969050, 
	15681611, 11535027, 12511037, 18996635, 848647, 19513452, 1766991, 3055032, 
	3613012, 5575905, 14372104, 1785385, 1309013, 3334460, 2981265, 23930974, 
	1490604, 22450050, 9670069, 15305591, 11857290, 1181211, 19840811, 16749191, 
	10917979, 21626454, 1554344, 2855659, 17787726, 9807213, 19823366, 14468716, 
	22878708, 21297123, 23154362, 19990623, 14200058, 11724670, 14944886, 22237997, 
	14434445, 17181830, 16972717, 10477338, 3736040, 1049291, 23877491, 22679535, 
	1027231, 12730076, 4501944, 23505310, 2556388, 567278, 20547262, 12709153, 
	6555683, 23182289, 19330574, 22638174, 8709139, 8076504, 22658136, 22768271, 
	5579306, 5537881, 2614243, 14623981, 11741002, 18694569, 24108359
};



/*
 * Table for inverse NTT, binary case:
 *   iGMb[x] = R*((1/g)^rev(x)) mod q
 * Since g = 7, 1/g = 8778 mod 12289.
 */
static const uint16_t iGMb[] = {
	 4091,  4401,  1081,  1229,  2530,  6014,  7947,  5329,
	 2579,  4751,  6464, 11703,  7023,  2812,  5890, 10698,
	 3109,  2125,  1960, 10925, 10601, 10404,  4189,  1875,
	 5847,  8546,  4615,  5190, 11324, 10578,  5882, 11155,
	 8417, 12275, 10599,  7446,  5719,  3569,  5981, 10108,
	 4426,  8306, 10755,  4679, 11052,  1538, 11857,   100,
	 8247,  6625,  9725,  5145,  3412,  7858,  5831,  9460,
	 5217, 10740,  7882,  7506, 12172, 11292,  6049,    79,
	   13,  6938,  8886,  5453,  4586, 11455,  2903,  4676,
	 9843,  7621,  8822,  9109,  2083,  8507,  8685,  3110,
	 7015,  3269,  1367,  6397, 10259,  8435, 10527, 11559,
	11094,  2211,  1808,  7319,    48,  9547,  2560,  1228,
	 9438, 10787, 11800,  1820, 11406,  8966,  6159,  3012,
	 6109,  2796,  2203,  1652,   711,  7004,  1053,  8973,
	 5244,  1517,  9322, 11269,   900,  3888, 11133, 10736,
	 4949,  7616,  9974,  4746, 10270,   126,  2921,  6720,
	 6635,  6543,  1582,  4868,    42,   673,  2240,  7219,
	 1296, 11989,  7675,  8578, 11949,   989, 10541,  7687,
	 7085,  8487,  1004, 10236,  4703,   163,  9143,  4597,
	 6431, 12052,  2991, 11938,  4647,  3362,  2060, 11357,
	12011,  6664,  5655,  7225,  5914,  9327,  4092,  5880,
	 6932,  3402,  5133,  9394, 11229,  5252,  9008,  1556,
	 6908,  4773,  3853,  8780, 10325,  7737,  1758,  7103,
	11375, 12273,  8602,  3243,  6536,  7590,  8591, 11552,
	 6101,  3253,  9969,  9640,  4506,  3736,  6829, 10822,
	 9130,  9948,  3566,  2133,  3901,  6038,  7333,  6609,
	 3468,  4659,   625,  2700,  7738,  3443,  3060,  3388,
	 3526,  4418, 11911,  6232,  1730,  2558, 10340,  5344,
	 5286,  2190, 11562,  6199,  2482,  8756,  5387,  4101,
	 4609,  8605,  8226,   144,  5656,  8704,  2621,  5424,
	10812,  2959, 11346,  6249,  1715,  4951,  9540,  1888,
	 3764,    39,  8219,  2080,  2502,  1469, 10550,  8709,
	 5601,  1093,  3784,  5041,  2058,  8399, 11448,  9639,
	 2059,  9878,  7405,  2496,  7918, 11594,   371,  7993,
	 3073, 10326,    40, 10004,  9245,  7987,  5603,  4051,
	 7894,   676, 11380,  7379,  6501,  4981,  2628,  3488,
	10956,  7022,  6737,  9933,  7139,  2330,  3884,  5473,
	 7865,  6941,  5737,  5613,  9505, 11568, 11277,  2510,
	 6689,   386,  4462,   105,  2076, 10443,   119,  3955,
	 4370, 11505,  3672, 11439,   750,  3240,  3133,   754,
	 4013, 11929,  9210,  5378, 11881, 11018,  2818,  1851,
	 4966,  8181,  2688,  6205,  6814,   926,  2936,  4327,
	10175,  7089,  6047,  9410, 10492,  8950,  2472,  6255,
	  728,  7569,  6056, 10432, 11036,  2452,  2811,  3787,
	  945,  8998,  1244,  8815, 11017, 11218,  5894,  4325,
	 4639,  3819,  9826,  7056,  6786,  8670,  5539,  7707,
	 1361,  9812,  2949, 11265, 10301,  9108,   478,  6489,
	  101,  1911,  9483,  3608, 11997, 10536,   812,  8915,
	  637,  8159,  5299,  9128,  3512,  8290,  7068,  7922,
	 3036,  4759,  2163,  3937,  3755, 11306,  7739,  4922,
	11932,   424,  5538,  6228, 11131,  7778, 11974,  1097,
	 2890, 10027,  2569,  2250,  2352,   821,  2550, 11016,
	 7769,   136,   617,  3157,  5889,  9219,  6855,   120,
	 4405,  1825,  9635,  7214, 10261, 11393,  2441,  9562,
	11176,   599,  2085, 11465,  7233,  6177,  4801,  9926,
	 9010,  4514,  9455, 11352, 11670,  6174,  7950,  9766,
	 6896, 11603,  3213,  8473,  9873,  2835, 10422,  3732,
	 7961,  1457, 10857,  8069,   832,  1628,  3410,  4900,
	10855,  5111,  9543,  6325,  7431,  4083,  3072,  8847,
	 9853, 10122,  5259, 11413,  6556,   303,  1465,  3871,
	 4873,  5813, 10017,  6898,  3311,  5947,  8637,  5852,
	 3856,   928,  4933,  8530,  1871,  2184,  5571,  5879,
	 3481, 11597,  9511,  8153,    35,  2609,  5963,  8064,
	 1080, 12039,  8444,  3052,  3813, 11065,  6736,  8454,
	 2340,  7651,  1910, 10709,  2117,  9637,  6402,  6028,
	 2124,  7701,  2679,  5183,  6270,  7424,  2597,  6795,
	 9222, 10837,   280,  8583,  3270,  6753,  2354,  3779,
	 6102,  4732,  5926,  2497,  8640, 10289,  6107, 12127,
	 2958, 12287, 10292,  8086,   817,  4021,  2610,  1444,
	 5899, 11720,  3292,  2424,  5090,  7242,  5205,  5281,
	 9956,  2702,  6656,   735,  2243, 11656,   833,  3107,
	 6012,  6801,  1126,  6339,  5250, 10391,  9642,  5278,
	 3513,  9769,  3025,   779,  9433,  3392,  7437,   668,
	10184,  8111,  6527,  6568, 10831,  6482,  8263,  5711,
	 9780,   467,  5462,  4425, 11999,  1205,  5015,  6918,
	 5096,  3827,  5525, 11579,  3518,  4875,  7388,  1931,
	 6615,  1541,  8708,   260,  3385,  4792,  4391,  5697,
	 7895,  2155,  7337,   236, 10635, 11534,  1906,  4793,
	 9527,  7239,  8354,  5121, 10662,  2311,  3346,  8556,
	  707,  1088,  4936,   678, 10245,    18,  5684,   960,
	 4459,  7957,   226,  2451,     6,  8874,   320,  6298,
	 8963,  8735,  2852,  2981,  1707,  5408,  5017,  9876,
	 9790,  2968,  1899,  6729,  4183,  5290, 10084,  7679,
	 7941,  8744,  5694,  3461,  4175,  5747,  5561,  3378,
	 5227,   952,  4319,  9810,  4356,  3088, 11118,   840,
	 6257,   486,  6000,  1342, 10382,  6017,  4798,  5489,
	 4498,  4193,  2306,  6521,  1475,  6372,  9029,  8037,
	 1625,  7020,  4740,  5730,  7956,  6351,  6494,  6917,
	11405,  7487, 10202, 10155,  7666,  7556, 11509,  1546,
	 6571, 10199,  2265,  7327,  5824, 11396, 11581,  9722,
	 2251, 11199,  5356,  7408,  2861,  4003,  9215,   484,
	 7526,  9409, 12235,  6157,  9025,  2121, 10255,  2519,
	 9533,  3824,  8674, 11419, 10888,  4762, 11303,  4097,
	 2414,  6496,  9953, 10554,   808,  2999,  2130,  4286,
	12078,  7445,  5132,  7915,   245,  5974,  4874,  7292,
	 7560, 10539,  9952,  9075,  2113,  3721, 10285, 10022,
	 9578,  8934, 11074,  9498,   294,  4711,  3391,  1377,
	 9072, 10189,  4569, 10890,  9909,  6923,    53,  4653,
	  439, 10253,  7028, 10207,  8343,  1141,  2556,  7601,
	 8150, 10630,  8648,  9832,  7951, 11245,  2131,  5765,
	10343,  9781,  2718,  1419,  4531,  3844,  4066,  4293,
	11657, 11525, 11353,  4313,  4869, 12186,  1611, 10892,
	11489,  8833,  2393,    15, 10830,  5003,    17,   565,
	 5891, 12177, 11058, 10412,  8885,  3974, 10981,  7130,
	 5840, 10482,  8338,  6035,  6964,  1574, 10936,  2020,
	 2465,  8191,   384,  2642,  2729,  5399,  2175,  9396,
	11987,  8035,  4375,  6611,  5010, 11812,  9131, 11427,
	  104,  6348,  9643,  6757, 12110,  5617, 10935,   541,
	  135,  3041,  7200,  6526,  5085, 12136,   842,  4129,
	 7685, 11079,  8426,  1008,  2725, 11772,  6058,  1101,
	 1950,  8424,  5688,  6876, 12005, 10079,  5335,   927,
	 1770,   273,  8377,  2271,  5225, 10283,   116, 11807,
	   91, 11699,   757,  1304,  7524,  6451,  8032,  8154,
	 7456,  4191,   309,  2318,  2292, 10393, 11639,  9481,
	12238, 10594,  9569,  7912, 10368,  9889, 12244,  7179,
	 3924,  3188,   367,  2077,   336,  5384,  5631,  8596,
	 4621,  1775,  8866,   451,  6108,  1317,  6246,  8795,
	 5896,  7283,  3132, 11564,  4977, 12161,  7371,  1366,
	12130, 10619,  3809,  5149,  6300,  2638,  4197,  1418,
	10065,  4156,  8373,  8644, 10445,   882,  8158, 10173,
	 9763, 12191,   459,  2966,  3166,   405,  5000,  9311,
	 6404,  8986,  1551,  8175,  3630, 10766,  9265,   700,
	 8573,  9508,  6630, 11437, 11595,  5850,  3950,  4775,
	11941,  1446,  6018,  3386, 11470,  5310,  5476,   553,
	 9474,  2586,  1431,  2741,   473, 11383,  4745,   836,
	 4062, 10666,  7727, 11752,  5534,   312,  4307,  4351,
	 5764,  8679,  8381,  8187,     5,  7395,  4363,  1152,
	 5421,  5231,  6473,   436,  7567,  8603,  6229,  8230
};


static const iGMb_large[] = {
	65536, 10297394, 19869846, 18051901, 7501546, 18426595, 10914687, 1260573, 16515748, 
	16671460, 24165886, 12885082, 7892191, 14036538, 3738593, 17491948, 9027145, 
	16302368, 11856455, 8604124, 5453199, 4314442, 18674657, 13629263, 10344887, 
	8697056, 2330556, 1351009, 16848664, 5040771, 400872, 11950623, 3380018, 
	10935191, 10436338, 8048079, 4797666, 12078293, 19505381, 24145196, 14329827, 
	8965400, 22417400, 10937023, 6558706, 18672898, 11581732, 6704643, 20407780, 
	8642942, 4479307, 1287559, 8678271, 22069378, 17663488, 13198435, 10049976, 
	10391681, 12777013, 12479476, 13704156, 22143839, 13113312, 18470784, 14034478, 
	11120043, 6859656, 9967010, 19034086, 12044991, 10805844, 17946882, 17502966, 
	7039825, 9235181, 3986722, 4297258, 19632709, 16499315, 15764566, 20337662, 
	14302650, 5793481, 22951126, 2200034, 14838800, 942355, 12522322, 20156231, 
	13609867, 2195317, 14220537, 21215475, 21055994, 23333145, 15966653, 2391626, 
	10209284, 15848030, 9320, 20948866, 2053156, 6588612, 14122431, 2129005, 
	24352943, 18549373, 11251109, 8622068, 18686971, 15443469, 14222676, 12987305, 
	19526719, 14470205, 8753061, 13918342, 13733670, 22029640, 7059012, 12090685, 
	11462181, 12939902, 103633, 23284865, 22651828, 12621050, 8330996, 15065098, 
	22327430, 19688422, 11890830, 10859734, 22086847, 146861, 12036317, 20103805, 
	12947453, 12076353, 4559450, 17945156, 8138138, 2227772, 19809416, 16644462, 
	24363004, 22541552, 18961989, 10360778, 18085981, 17146583, 21027651, 20355369, 
	20669809, 23442302, 22466019, 860908, 8459683, 8722050, 16879483, 6639391, 
	7821720, 543111, 22377320, 19722806, 23812315, 3494844, 6576882, 10669436, 
	16552948, 18601789, 19725363, 4414500, 7276013, 5473790, 3194717, 14727948, 
	22133599, 11742862, 1637160, 4700013, 629279, 3107793, 10201358, 9762936, 
	13961374, 22948550, 4752423, 14836975, 8151680, 14003169, 13354817, 12285606, 
	17824516, 16023472, 17468248, 23479755, 4416785, 19715143, 5893221, 16393858, 
	23322279, 6651995, 22223148, 4911661, 22300822, 8118833, 13607824, 14148493, 
	5718892, 15718067, 16141993, 17322698, 890567, 15004471, 9103217, 15835868, 
	6430179, 22364717, 15782569, 2987288, 8822141, 24386263, 758887, 722737, 
	17498845, 8738649, 10145038, 10956823, 10389858, 2117140, 18425523, 16112990, 
	13611401, 6452567, 22043197, 11175327, 21405876, 10619073, 15172648, 17276364, 
	628220, 6755829, 13360935, 9286775, 20011158, 18977746, 18056628, 23222729, 
	16696303, 17408494, 162689, 16142132, 20116526, 24117502, 3185229, 5978572, 
	20134266, 12858079, 21288727, 19579105, 8345699, 7196189, 4010549, 8804805, 
	24249172, 5972313, 3749681, 659168, 8256106, 11532664, 18786785, 10146285, 
	22249228, 6219203, 1669903, 19537302, 7559593, 10343173, 5259177, 21131441, 
	12131960, 1541871, 2518086, 17777451, 24011811, 12460753, 21662314, 8038626, 
	17158665, 2205398, 22578876, 14542895, 19672508, 398069, 4975685, 10254503, 
	5662099, 8427144, 14668188, 21054172, 14826063, 24145300, 10098887, 13865263, 
	10636547, 14038295, 6999717, 10637268, 6427630, 5839611, 11725722, 14650085, 
	710178, 1755064, 13205358, 19207586, 8919664, 2758133, 19802967, 16225710, 
	19154849, 4124816, 1697717, 3377068, 21166137, 14167408, 20290378, 22273730, 
	20067785, 8656222, 15024448, 21008020, 18830474, 7310161, 7793269, 9879009, 
	11748257, 13567012, 9932345, 18972808, 19230742, 3007849, 6730895, 19339261, 
	22194279, 21961329, 16727610, 24341907, 17365212, 23388919, 12778422, 11007072, 
	3196970, 13274104, 8966042, 24336466, 20962222, 6111696, 24197151, 8511198, 
	7968542, 23291291, 6206849, 10636121, 23792202, 7786716, 11248522, 16702442, 
	6375739, 13744088, 20102749, 15725237, 4265102, 2441996, 6121296, 15189835, 
	12686494, 23176812, 7971761, 9939156, 9428134, 10219630, 9633975, 4292274, 
	7927205, 18831611, 242670, 5202656, 4435575, 16938495, 743742, 3398899, 
	3252851, 13197131, 93050, 15309314, 10128142, 3034082, 15845463, 20263798, 
	17432702, 15403118, 1881288, 10173502, 19297011, 21768354, 3417753, 11373679, 
	7893376, 9942369, 13409165, 14462553, 6647984, 20600219, 1340685, 19561510, 
	12114096, 19935199, 456680, 1398712, 2976480, 5550455, 8601971, 6693081, 
	21756239, 5858760, 14349439, 7561635, 12601062, 111710, 13513976, 1794879, 
	4934632, 1235856, 13980292, 23008651, 13959723, 5542556, 4193014, 6674581, 
	284926, 23879173, 5078017, 18234499, 11622727, 8255425, 17706148, 19178636, 
	23276498, 23239742, 3843215, 15422265, 6067373, 3889070, 11576636, 18764375, 
	24384903, 135755, 23366266, 4085061, 11911485, 6142925, 11235974, 23699580, 
	8584461, 24229715, 14276309, 10315582, 7489718, 11264477, 12140249, 4308004, 
	21051549, 15897612, 1318299, 12513537, 11138307, 7969222, 2007899, 3003367, 
	17790720, 19604352, 6682379, 23136341, 16151330, 11997676, 8843780, 18758643, 
	11236047, 16569080, 17385359, 20152183, 2927369, 19642728, 24218584, 3839299, 
	11469187, 12590446, 21192993, 20611744, 4393214, 12341770, 2859017, 1470139, 
	24249679, 22271802, 12953995, 21747855, 5889673, 16431486, 15008091
};

/*
 * Reduce a small signed integer modulo q. The source integer MUST
 * be between -q/2 and +q/2.
 */
static inline uint32_t
mq_conv_small(int x)
{
	/*
	 * If x < 0, the cast to uint32_t will set the high bit to 1.
	 */
	uint32_t y;

	y = (uint32_t)x;
	y += Q & -(y >> 31);
	return y;
}

/*
 * Addition modulo q. Operands must be in the 0..q-1 range.
 */
static inline uint32_t
mq_add(uint32_t x, uint32_t y)
{
	/*
	 * We compute x + y - q. If the result is negative, then the
	 * high bit will be set, and 'd >> 31' will be equal to 1;
	 * thus '-(d >> 31)' will be an all-one pattern. Otherwise,
	 * it will be an all-zero pattern. In other words, this
	 * implements a conditional addition of q.
	 */
	uint32_t d;

	d = x + y - Q;
	d += Q & -(d >> 31);
	return d;
}

/*
 * Subtraction modulo q. Operands must be in the 0..q-1 range.
 */
static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	/*
	 * As in mq_add(), we use a conditional addition to ensure the
	 * result is in the 0..q-1 range.
	 */
	uint32_t d;

	d = x - y;
	d += Q & -(d >> 31);
	return d;
}

/*
 * Division by 2 modulo q. Operand must be in the 0..q-1 range.
 */
static inline uint32_t
mq_rshift1(uint32_t x)
{
	x += Q & -(x & 1);
	return (x >> 1);
}

/*
 * Montgomery multiplication modulo q. If we set R = 2^16 mod q, then
 * this function computes: x * y / R mod q
 * Operands must be in the 0..q-1 range.
 */
static inline uint32_t
mq_montymul(uint32_t x, uint32_t y)
{
	uint32_t z, w;

	/*
	 * We compute x*y + k*q with a value of k chosen so that the 16
	 * low bits of the result are 0. We can then shift the value.
	 * After the shift, result may still be larger than q, but it
	 * will be lower than 2*q, so a conditional subtraction works.
	 */

	z = x * y;
	w = ((z * Q0I) & 0xFFFF) * Q;

	/*
	 * When adding z and w, the result will have its low 16 bits
	 * equal to 0. Since x, y and z are lower than q, the sum will
	 * be no more than (2^15 - 1) * q + (q - 1)^2, which will
	 * fit on 29 bits.
	 */
	z = (z + w) >> 16;

	/*
	 * After the shift, analysis shows that the value will be less
	 * than 2q. We do a subtraction then conditional subtraction to
	 * ensure the result is in the expected range.
	 */
	z -= Q;
	z += Q & -(z >> 31);
	return z;
}

/*
 * Montgomery squaring (computes (x^2)/R).
 */
static inline uint32_t
mq_montysqr(uint32_t x)
{
	return mq_montymul(x, x);
}

/*
 * Divide x by y modulo q = 12289.
 */
static inline uint32_t
mq_div_12289(uint32_t x, uint32_t y)
{
	/*
	 * We invert y by computing y^(q-2) mod q.
	 *
	 * We use the following addition chain for exponent e = 12287:
	 *
	 *   e0 = 1
	 *   e1 = 2 * e0 = 2
	 *   e2 = e1 + e0 = 3
	 *   e3 = e2 + e1 = 5
	 *   e4 = 2 * e3 = 10
	 *   e5 = 2 * e4 = 20
	 *   e6 = 2 * e5 = 40
	 *   e7 = 2 * e6 = 80
	 *   e8 = 2 * e7 = 160
	 *   e9 = e8 + e2 = 163
	 *   e10 = e9 + e8 = 323
	 *   e11 = 2 * e10 = 646
	 *   e12 = 2 * e11 = 1292
	 *   e13 = e12 + e9 = 1455
	 *   e14 = 2 * e13 = 2910
	 *   e15 = 2 * e14 = 5820
	 *   e16 = e15 + e10 = 6143
	 *   e17 = 2 * e16 = 12286
	 *   e18 = e17 + e0 = 12287
	 *
	 * Additions on exponents are converted to Montgomery
	 * multiplications. We define all intermediate results as so
	 * many local variables, and let the C compiler work out which
	 * must be kept around.
	 */
	uint32_t y0, y1, y2, y3, y4, y5, y6, y7, y8, y9;
	uint32_t y10, y11, y12, y13, y14, y15, y16, y17, y18;

	y0 = mq_montymul(y, R2);
	y1 = mq_montysqr(y0);
	y2 = mq_montymul(y1, y0);
	y3 = mq_montymul(y2, y1);
	y4 = mq_montysqr(y3);
	y5 = mq_montysqr(y4);
	y6 = mq_montysqr(y5);
	y7 = mq_montysqr(y6);
	y8 = mq_montysqr(y7);
	y9 = mq_montymul(y8, y2);
	y10 = mq_montymul(y9, y8);
	y11 = mq_montysqr(y10);
	y12 = mq_montysqr(y11);
	y13 = mq_montymul(y12, y9);
	y14 = mq_montysqr(y13);
	y15 = mq_montysqr(y14);
	y16 = mq_montymul(y15, y10);
	y17 = mq_montysqr(y16);
	y18 = mq_montymul(y17, y0);

	/*
	 * Final multiplication with x, which is not in Montgomery
	 * representation, computes the correct division result.
	 */
	return mq_montymul(y18, x);
}

/*
 * Compute NTT on a ring element.
 */
static void
mq_NTT(uint16_t *a, unsigned logn)
{
	size_t n, t, m;

	n = (size_t)1 << logn;
	t = n;
	for (m = 1; m < n; m <<= 1) {
		size_t ht, i, j1;

		ht = t >> 1;
		for (i = 0, j1 = 0; i < m; i ++, j1 += t) {
			size_t j, j2;
			uint32_t s;

			s = GMb[m + i];
			j2 = j1 + ht;
			for (j = j1; j < j2; j ++) {
				uint32_t u, v;

				u = a[j];
				v = mq_montymul(a[j + ht], s);
				a[j] = (uint16_t)mq_add(u, v);
				a[j + ht] = (uint16_t)mq_sub(u, v);
			}
		}
		t = ht;
	}
}

/*
 * Compute the inverse NTT on a ring element, binary case.
 */
static void
mq_iNTT(uint16_t *a, unsigned logn)
{
	size_t n, t, m;
	uint32_t ni;

	n = (size_t)1 << logn;
	t = 1;
	m = n;
	while (m > 1) {
		size_t hm, dt, i, j1;

		hm = m >> 1;
		dt = t << 1;
		for (i = 0, j1 = 0; i < hm; i ++, j1 += dt) {
			size_t j, j2;
			uint32_t s;

			j2 = j1 + t;
			s = iGMb[hm + i];
			for (j = j1; j < j2; j ++) {
				uint32_t u, v, w;

				u = a[j];
				v = a[j + t];
				a[j] = (uint16_t)mq_add(u, v);
				w = mq_sub(u, v);
				a[j + t] = (uint16_t)
					mq_montymul(w, s);
			}
		}
		t = dt;
		m = hm;
	}

	/*
	 * To complete the inverse NTT, we must now divide all values by
	 * n (the vector size). We thus need the inverse of n, i.e. we
	 * need to divide 1 by 2 logn times. But we also want it in
	 * Montgomery representation, i.e. we also want to multiply it
	 * by R = 2^16. In the common case, this should be a simple right
	 * shift. The loop below is generic and works also in corner cases;
	 * its computation time is negligible.
	 */
	ni = R;
	for (m = n; m > 1; m >>= 1) {
		ni = mq_rshift1(ni);
	}
	for (m = 0; m < n; m ++) {
		a[m] = (uint16_t)mq_montymul(a[m], ni);
	}
}

/*
 * Convert a polynomial (mod q) to Montgomery representation.
 */
static void
mq_poly_tomonty(uint16_t *f, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		f[u] = (uint16_t)mq_montymul(f[u], R2);
	}
}

/*
 * Multiply two polynomials together (NTT representation, and using
 * a Montgomery multiplication). Result f*g is written over f.
 */
static void
mq_poly_montymul_ntt(uint16_t *f, const uint16_t *g, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		f[u] = (uint16_t)mq_montymul(f[u], g[u]);
	}
}

/*
 * Subtract polynomial g from polynomial f.
 */
static void
mq_poly_sub(uint16_t *f, const uint16_t *g, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		f[u] = (uint16_t)mq_sub(f[u], g[u]);
	}
}

/* ===================================================================== */

/* see inner.h */
void
Zf(to_ntt_monty)(uint16_t *h, unsigned logn)
{
	mq_NTT(h, logn);
	mq_poly_tomonty(h, logn);
}


// void
// Zf(to_ntt_monty_large)(uint32_t *h, unsigned logn)
// {
// 	mq_NTT(h, logn);
// 	mq_poly_tomonty(h, logn);
// }

/* see inner.h */
int
Zf(verify_raw)(const uint16_t *c0, const int16_t *s2,
	const uint16_t *h, unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;

	n = (size_t)1 << logn;
	tt = (uint16_t *)tmp;

	/*
	 * Reduce s2 elements modulo q ([0..q-1] range).
	 */
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u];
		w += Q & -(w >> 31);
		tt[u] = (uint16_t)w;
	}

	/*
	 * Compute -s1 = s2*h - c0 mod phi mod q (in tt[]).
	 */
	mq_NTT(tt, logn);
	mq_poly_montymul_ntt(tt, h, logn);
	mq_iNTT(tt, logn);
	mq_poly_sub(tt, c0, logn);

	/*
	 * Normalize -s1 elements into the [-q/2..q/2] range.
	 */
	for (u = 0; u < n; u ++) {
		int32_t w;

		w = (int32_t)tt[u];
		w -= (int32_t)(Q & -(((Q >> 1) - (uint32_t)w) >> 31));
		((int16_t *)tt)[u] = (int16_t)w;
	}

	/*
	 * Signature is valid if and only if the aggregate (-s1,s2) vector
	 * is short enough.
	 */
	return Zf(is_short)((int16_t *)tt, s2, logn);
}

/* see inner.h */
int
Zf(compute_public)(uint16_t *h,
	const int8_t *f, const int8_t *g, unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;

	n = (size_t)1 << logn;
	tt = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		tt[u] = (uint16_t)mq_conv_small(f[u]);
		h[u] = (uint16_t)mq_conv_small(g[u]);
	}
	mq_NTT(h, logn);
	mq_NTT(tt, logn);
	for (u = 0; u < n; u ++) {
		if (tt[u] == 0) {
			return 0;
		}
		h[u] = (uint16_t)mq_div_12289(h[u], tt[u]);
	}
	mq_iNTT(h, logn);
	return 1;
}

/* see inner.h */
int
Zf(complete_private)(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *t1, *t2;

	n = (size_t)1 << logn;
	t1 = (uint16_t *)tmp;
	t2 = t1 + n;
	for (u = 0; u < n; u ++) {
		t1[u] = (uint16_t)mq_conv_small(g[u]);
		t2[u] = (uint16_t)mq_conv_small(F[u]);
	}
	mq_NTT(t1, logn);
	mq_NTT(t2, logn);
	mq_poly_tomonty(t1, logn);
	mq_poly_montymul_ntt(t1, t2, logn);
	for (u = 0; u < n; u ++) {
		t2[u] = (uint16_t)mq_conv_small(f[u]);
	}
	mq_NTT(t2, logn);
	for (u = 0; u < n; u ++) {
		if (t2[u] == 0) {
			return 0;
		}
		t1[u] = (uint16_t)mq_div_12289(t1[u], t2[u]);
	}
	mq_iNTT(t1, logn);
	for (u = 0; u < n; u ++) {
		uint32_t w;
		int32_t gi;

		w = t1[u];
		w -= (Q & ~-((w - (Q >> 1)) >> 31));
		gi = *(int32_t *)&w;
		if (gi < -127 || gi > +127) {
			return 0;
		}
		G[u] = (int8_t)gi;
	}
	return 1;
}

/* see inner.h */
int
Zf(is_invertible)(
	const int16_t *s2, unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;
	uint32_t r;

	n = (size_t)1 << logn;
	tt = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u];
		w += Q & -(w >> 31);
		tt[u] = (uint16_t)w;
	}
	mq_NTT(tt, logn);
	r = 0;
	for (u = 0; u < n; u ++) {
		r |= (uint32_t)(tt[u] - 1);
	}
	return (int)(1u - (r >> 31));
}

/* see inner.h */
int
Zf(verify_recover)(uint16_t *h,
	const uint16_t *c0, const int16_t *s1, const int16_t *s2,
	unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;
	uint32_t r;

	n = (size_t)1 << logn;

	/*
	 * Reduce elements of s1 and s2 modulo q; then write s2 into tt[]
	 * and c0 - s1 into h[].
	 */
	tt = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u];
		w += Q & -(w >> 31);
		tt[u] = (uint16_t)w;

		w = (uint32_t)s1[u];
		w += Q & -(w >> 31);
		w = mq_sub(c0[u], w);
		h[u] = (uint16_t)w;
	}

	/*
	 * Compute h = (c0 - s1) / s2. If one of the coefficients of s2
	 * is zero (in NTT representation) then the operation fails. We
	 * keep that information into a flag so that we do not deviate
	 * from strict constant-time processing; if all coefficients of
	 * s2 are non-zero, then the high bit of r will be zero.
	 */
	mq_NTT(tt, logn);
	mq_NTT(h, logn);
	r = 0;
	for (u = 0; u < n; u ++) {
		r |= (uint32_t)(tt[u] - 1);
		h[u] = (uint16_t)mq_div_12289(h[u], tt[u]);
	}
	mq_iNTT(h, logn);

	/*
	 * Signature is acceptable if and only if it is short enough,
	 * and s2 was invertible mod phi mod q. The caller must still
	 * check that the rebuilt public key matches the expected
	 * value (e.g. through a hash).
	 */
	r = ~r & (uint32_t)-Zf(is_short)(s1, s2, logn);
	return (int)(r >> 31);
}

/* see inner.h */
int
Zf(count_nttzero)(const int16_t *sig, unsigned logn, uint8_t *tmp)
{
	uint16_t *s2;
	size_t u, n;
	uint32_t r;

	n = (size_t)1 << logn;
	s2 = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)sig[u];
		w += Q & -(w >> 31);
		s2[u] = (uint16_t)w;
	}
	mq_NTT(s2, logn);
	r = 0;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u] - 1u;
		r += (w >> 31);
	}
	return (int)r;
}
