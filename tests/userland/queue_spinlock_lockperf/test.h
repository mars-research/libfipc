/**
 * @File     : test.h
 * @Author   : Abdullah Younis
 *
 * NOTE: This test assumes an x86 architecture.
 */

#ifndef LIBFIPC_TEST_PING_PONG
#define LIBFIPC_TEST_PING_PONG

#include "../libfipc_test.h"
#include "queue.h"

// Queue Variable
static queue_t queue;

// Test Variables
static uint32_t transactions = 1000000;

static uint8_t producer_count = 4;
static uint8_t consumer_count = 4;

static uint8_t producer_cpus[32] = { 0, 4, 8, 12 };
static uint8_t consumer_cpus[32] = { 16, 20, 24, 28 };

// Request Types
#define HALT            0
#define NULL_INVOCATION 1

// Mutex Locks
static uint64_t mutex_num_producer = 0;
static uint64_t mutex_num_consumer = 0;

pthread_mutex_t producer_mutex[4];
pthread_mutex_t consumer_mutex[4];

#endif