/**
 * @File     : main.c
 * @Author   : Abdullah Younis
 */

#include <linux/module.h>
#include "test.h"

static inline
void load ( uint64_t index )
{
	while ( unlikely( cache[index].regs[0] != MSG_READY ) )
		fipc_test_pause();
}

static inline
void stage ( uint64_t index )
{
	cache[index].regs[0] = MSG_READY;
}

int loader ( void* data )
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

	// Wait for stager to complete
	load( transactions );
	fipc_test_mfence();

	// Start counting
	for ( i = 0; i < ev_num; ++i )
		PROG_EVENT(&ev[i], i);

	start = RDTSC_START();

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		load( load_order[transaction_id] );
		fipc_test_mfence();
	}

	end = RDTSCP();

	// Stop counting
	for ( i = 0; i < ev_num; ++i )
		STOP_EVENT(i);

	// Read count
	for ( i = 0; i < ev_num; ++i )
		READ_PMC(&ev_val[i], i);

	// Reset count
	for ( i = 0; i < ev_num; ++i )
		RESET_COUNTER(i);

	// Print count
	pr_err("-------------------------------------------------\n");

	pr_err("Average Cycles: %llu\n", (end - start) / transactions);

	for ( i = 0; i < ev_num; ++i )
		pr_err("Event id:%2x   mask:%2x   count: %llu\n", ev_idx[i], ev_msk[i], ev_val[i]);
	
	pr_err("-------------------------------------------------\n");

	// End test
	fipc_test_thread_release_control_of_CPU();
	complete( &loader_comp );
	return 0;
}

int stager ( void* data )
{
	register uint64_t CACHE_ALIGNED transaction_id;

	// Begin test
	fipc_test_thread_take_control_of_CPU();

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		stage( transaction_id );
	}

	stage( transactions ); // Used a flag to start loader

	// End test
	fipc_test_thread_release_control_of_CPU();
	complete( &stager_comp );
	return 0;
}


int main ( void )
{
	init_completion( &loader_comp );
	init_completion( &stager_comp );

	kthread_t* loader_thread = NULL;
	kthread_t* stager_thread = NULL;

	/**
	 * Shared memory region is 16kb, which is meant to fit into L1.
	 */
	cache = (cache_line_t*) kmalloc( 16*1024, GFP_KERNEL );

	// Create Threads
	loader_thread = fipc_test_thread_spawn_on_CPU ( loader, NULL, loader_cpu );
	if ( loader_thread == NULL )
	{
		pr_err( "%s\n", "Error while creating thread" );
		return -1;
	}
	
	stager_thread = fipc_test_thread_spawn_on_CPU ( stager, NULL, stager_cpu );
	if ( stager_thread == NULL )
	{
		pr_err( "%s\n", "Error while creating thread" );
		return -1;
	}
	
	// Start threads
	wake_up_process( loader_thread );
	wake_up_process( stager_thread );
	
	// Wait for thread completion
	wait_for_completion( &loader_comp );
	wait_for_completion( &stager_comp );
	
	// Clean up
	fipc_test_thread_free_thread( loader_thread );
	fipc_test_thread_free_thread( stager_thread );
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
