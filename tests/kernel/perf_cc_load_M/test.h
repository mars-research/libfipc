/**
 * @File     : test.h
 * @Author   : Abdullah Younis
 *
 * This tests times a load from modified cache transaction
 *
 * This test uses a shared-continuous region of memory
 *
 * The events can be programmed using the ev_idx and ev_msk parameters
 * Event ids and mask ids can be found in your cpu's architecture manual
 * Emulab's d710 (table 19-17, 19-19) and d820 (table 19-13, 19-15) machines can use this link:
 * https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-3b-part-2-manual.html
 *
 * NOTE: This test assumes an x86 architecture.
 */

#ifndef LIBFIPC_TEST_CC_LOAD_M
#define LIBFIPC_TEST_CC_LOAD_M

#include "../libfipc_test.h"
#include "perf_counter_helper.h"

// Test Variables
static uint32_t transactions   = 250;
static uint8_t  loader_cpu     = 0;
static uint8_t  stager_cpu     = 1;

module_param( transactions,     uint, 0 );
module_param( loader_cpu,       byte, 0 );
module_param( stager_cpu,       byte, 0 );

// Thread Locks
struct completion loader_comp;
struct completion stager_comp;

// Events
static evt_sel_t ev[8]     = { 0 };
static uint64_t  ev_val[8] = { 0 };

static uint32_t ev_num    = 0;
static uint8_t  ev_idx[8] = { 0 };
static uint8_t  ev_msk[8] = { 0 };

module_param_array( ev_idx, byte, &ev_num, 0 );
module_param_array( ev_msk, byte, NULL,    0 );

// Cache Variable
volatile cache_line_t volatile * cache;

#define MSG_READY 0xF00DF00D

static uint32_t load_order[256] =
{
	40, 125, 37, 59, 239, 21, 8, 189,
	31, 68, 172, 142, 200, 3, 39, 101,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

static uint32_t load_order2[256] = 
{
	211,
	3,
	68,
	195,
	246,
	241,
	109,
	49,
	104,
	73,
	155,
	232,
	171,
	193,
	103,
	151,
	9,
	178,
	126,
	34,
	159,
	87,
	124,
	146,
	173,
	117,
	18,
	64,
	245,
	156,
	23,
	185,
	38,
	164,
	197,
	157,
	129,
	74,
	186,
	84,
	17,
	149,
	5,
	142,
	244,
	254,
	119,
	236,
	169,
	180,
	227,
	161,
	128,
	168,
	66,
	248,
	85,
	62,
	138,
	116,
	100,
	123,
	22,
	89,
	80,
	191,
	201,
	214,
	78,
	50,
	98,
	127,
	105,
	110,
	60,
	125,
	137,
	46,
	95,
	29,
	141,
	121,
	174,
	132,
	102,
	221,
	203,
	217,
	63,
	54,
	165,
	24,
	32,
	88,
	206,
	12,
	255,
	134,
	21,
	33,
	86,
	30,
	111,
	90,
	2,
	69,
	10,
	147,
	44,
	202,
	59,
	143,
	135,
	106,
	207,
	76,
	79,
	25,
	77,
	158,
	167,
	96,
	6,
	223,
	182,
	139,
	253,
	93,
	130,
	198,
	228,
	230,
	234,
	75,
	170,
	61,
	53,
	122,
	210,
	45,
	99,
	213,
	176,
	179,
	154,
	144,
	108,
	219,
	70,
	166,
	92,
	43,
	37,
	216,
	175,
	91,
	72,
	11,
	215,
	107,
	247,
	115,
	163,
	97,
	131,
	71,
	229,
	172,
	7,
	152,
	235,
	226,
	120,
	208,
	36,
	222,
	39,
	233,
	40,
	183,
	112,
	153,
	27,
	56,
	101,
	13,
	194,
	19,
	81,
	224,
	220,
	26,
	145,
	113,
	218,
	47,
	133,
	41,
	249,
	199,
	82,
	189,
	20,
	14,
	242,
	196,
	0,
	51,
	148,
	239,
	118,
	8,
	114,
	243,
	192,
	4,
	231,
	57,
	181,
	16,
	209,
	52,
	188,
	162,
	136,
	28,
	31,
	160,
	237,
	48,
	55,
	140,
	187,
	15,
	94,
	204,
	42,
	150,
	251,
	240,
	252,
	212,
	250,
	177,
	58,
	190,
	200,
	225,
	67,
	205,
	35,
	83,
	1,
	65,
	238,
	184
};

#endif
