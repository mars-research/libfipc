/**
 * @File     : main.c
 * @Author   : Abdullah Younis
 */

#include <linux/module.h>
#include "test.h"

static inline
void request ( uint64_t index )
{
	// Read Request
	while ( likely(cache_tx[index].regs[0] != MSG_AVAIL) )
		fipc_test_pause();

	// Write Request
	cache_tx[index].regs[0] = MSG_READY;

	// Read Response
	while ( likely(cache_rx[index].regs[0] != MSG_READY) )
		fipc_test_pause();

	// Write Response
	cache_rx[index].regs[0] = MSG_AVAIL;
}

static inline
void request_send ( uint64_t index )
{
	int i;

	// Start counting
	for ( i = 0; i < ev_num; ++i )
		PROG_EVENT(&ev[i], i);

	// Read Request
	while ( likely(cache_tx[index].regs[0] != MSG_AVAIL) )
		fipc_test_pause();

	// Write Request
	cache_tx[index].regs[0] = MSG_READY;

	// Stop counting
	for ( i = 0; i < ev_num; ++i )
		STOP_EVENT(i);

	// Read Response
	while ( likely(cache_rx[index].regs[0] != MSG_READY) )
		fipc_test_pause();

	// Write Response
	cache_rx[index].regs[0] = MSG_AVAIL;
}

static inline
void request_recv ( uint64_t index )
{
	int i;

	// Read Request
	while ( likely(cache_tx[index].regs[0] != MSG_AVAIL) )
		fipc_test_pause();

	// Write Request
	cache_tx[index].regs[0] = MSG_READY;

	// Start counting
	for ( i = 0; i < ev_num; ++i )
		PROG_EVENT(&ev[i], i);

	// Read Response
	while ( likely(cache_rx[index].regs[0] != MSG_READY) )
		fipc_test_pause();

	// Write Response
	cache_rx[index].regs[0] = MSG_AVAIL;

	// Stop counting
	for ( i = 0; i < ev_num; ++i )
		STOP_EVENT(i);
}

static inline
void respond ( uint64_t index )
{
	// Read Request
	while ( likely(cache_tx[index].regs[0] != MSG_READY ))
		fipc_test_pause();

	// Write Request
	cache_tx[index].regs[0] = MSG_AVAIL;

	// Read Response
	while ( likely(cache_rx[index].regs[0] != MSG_AVAIL ))
		fipc_test_pause();

	// Write Response
	cache_rx[index].regs[0] = MSG_READY;
}

int requester ( void* data )
{	
	register uint64_t CACHE_ALIGNED transaction_id;
	register uint64_t CACHE_ALIGNED start;
	register uint64_t CACHE_ALIGNED end;

	// Program the events to count
	uint i;
	for ( i = 0; i < ev_num; ++i )
		FILL_EVENT_OS(&ev[i], ev_idx[i], ev_msk[i]);

	// Begin test
	fipc_test_thread_take_control_of_CPU();

	// Start counting
	for ( i = 0; i < ev_num; ++i )
		PROG_EVENT(&ev[i], i);

	start = RDTSC_START();

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
		request( transaction_id );

	end = RDTSCP();

	// Stop counting
	for ( i = 0; i < ev_num; ++i )
		STOP_EVENT(i);

	// Read count
	for ( i = 0; i < ev_num; ++i )
		READ_PMC(&ev_val[i], i);

	// Print count
	pr_err("-------------------------------------------------\n");

	pr_err("Average Cycles: %llu\n", (end - start) / transactions);

	for ( i = 0; i < ev_num; ++i )
		pr_err("Event id:%2x   mask:%2x   count: %llu\n", ev_idx[i], ev_msk[i], ev_val[i]);
	
	pr_err("-------------------------------------------------\n");

	// Reset count
	for ( i = 0; i < ev_num; ++i )
		RESET_COUNTER(i);

	// ================================
	// Send Test
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
		request_send( transaction_id );

	// Read count
	for ( i = 0; i < ev_num; ++i )
		READ_PMC(&ev_val[i], i);

	// Print count
	pr_err("-------------------------------------------------\n");

	for ( i = 0; i < ev_num; ++i )
		pr_err("(send) Event id:%2x   mask:%2x   count: %llu\n", ev_idx[i], ev_msk[i], ev_val[i]);
	
	pr_err("-------------------------------------------------\n");

	// Reset count
	for ( i = 0; i < ev_num; ++i )
		RESET_COUNTER(i);

	// ================================
	// Recv Test
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
		request_recv( transaction_id );

	// Read count
	for ( i = 0; i < ev_num; ++i )
		READ_PMC(&ev_val[i], i);

	// Print count
	pr_err("-------------------------------------------------\n");

	for ( i = 0; i < ev_num; ++i )
		pr_err("(recv) Event id:%2x   mask:%2x   count: %llu\n", ev_idx[i], ev_msk[i], ev_val[i]);
	
	pr_err("-------------------------------------------------\n");

	// Reset count
	for ( i = 0; i < ev_num; ++i )
		RESET_COUNTER(i);

	// End test
	fipc_test_thread_release_control_of_CPU();
	complete( &requester_comp );
	return 0;
}

int responder ( void* data )
{
	register uint64_t CACHE_ALIGNED transaction_id;

	// Begin test
	fipc_test_thread_take_control_of_CPU();

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
		respond( transaction_id );

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
		respond( transaction_id );

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
		respond( transaction_id );

	// End test
	fipc_test_thread_release_control_of_CPU();
	complete( &responder_comp );
	return 0;
}


int main ( void )
{
	init_completion( &requester_comp );
	init_completion( &responder_comp );

	kthread_t* requester_thread = NULL;
	kthread_t* responder_thread = NULL;

	/**
	 * Shared memory regions are 4kb each, which is meant to fit into L1.
	 */
	cache_tx = kmalloc( 4*1024, GFP_KERNEL );
	cache_rx = kmalloc( 4*1024, GFP_KERNEL );

	// Init Variables
	int i;
	for ( i = 0; i < transactions; ++i )
	{
		cache_tx[i].regs[0] = MSG_AVAIL;
		cache_rx[i].regs[0] = MSG_AVAIL;
	}

	// Create Threads
	requester_thread = fipc_test_thread_spawn_on_CPU ( requester, NULL, requester_cpu );
	if ( requester_thread == NULL )
	{
		pr_err( "%s\n", "Error while creating thread" );
		return -1;
	}
	
	responder_thread = fipc_test_thread_spawn_on_CPU ( responder, NULL, responder_cpu );
	if ( responder_thread == NULL )
	{
		pr_err( "%s\n", "Error while creating thread" );
		return -1;
	}
	
	// Start threads
	wake_up_process( requester_thread );
	wake_up_process( responder_thread );
	
	// Wait for thread completion
	wait_for_completion( &requester_comp );
	wait_for_completion( &responder_comp );
	
	// Clean up
	fipc_test_thread_free_thread( requester_thread );
	fipc_test_thread_free_thread( responder_thread );
	return 0;
}

int init_module(void)
{
    return main();
}

void cleanup_module(void)
{
	return;
}

MODULE_LICENSE("GPL");
