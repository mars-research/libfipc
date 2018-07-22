/**
 * @File     : test.h
 * @Author   : Abdullah Younis
 *
 * This tests times the required cache transactions to 
 * send request and response messages in one cache line
 *
 * NOTE: This test assumes an x86 architecture.
 */

#ifndef LIBFIPC_TEST_PING_PONG_BARE_1
#define LIBFIPC_TEST_PING_PONG_BARE_1

#include "../libfipc_test.h"

// Test Variables
static uint32_t transactions   = 1000000;
static uint8_t  requester_cpu  = 0;
static uint8_t  responder_cpu  = 1;

module_param( transactions,     uint, 0 );
module_param( requester_cpu,    byte, 0 );
module_param( responder_cpu,    byte, 0 );

// Thread Locks
struct completion requester_comp;
struct completion responder_comp;

// Cache Variables
volatile cache_line_t CACHE_ALIGNED line;

volatile cache_line_t CACHE_ALIGNED resp_sequence; 
volatile cache_line_t CACHE_ALIGNED req_sequence;

#endif
