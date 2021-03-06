/**
 * @File     : main.c
 * @Author   : Abdullah Younis
 */

#include <linux/module.h>
#include "test.h"

static inline
void request ( header_t* chan )
{
	message_t* request;
	message_t* response;

	int i;
	int j;
	for ( i = 0; i < queue_depth; ++i )
	{
		fipc_test_blocking_send_start( chan, &request );

		// Marshalling
		request->flags = marshall_count;
		for ( j = 0; j < marshall_count; ++j )
			request->regs[j] = j;

		fipc_send_msg_end ( chan, request );
	}

	for ( i = 0; i < queue_depth; ++i )
	{
		fipc_test_blocking_recv_start( chan, &response );
		fipc_recv_msg_end( chan, response );
	}
}

static inline
void respond ( header_t* chan )
{
	message_t* request;
	message_t* response;
	uint64_t   answer;

	int i;
	for ( i = 0; i < queue_depth; ++i )
	{
		fipc_test_blocking_recv_start( chan, &request );

		// Dispatch loop
		switch ( request->flags )
		{
			case 0:
				answer = null_invocation();
				break;

			case 1:
				answer = increment( request->regs[0] );
				break;

			case 2:
				answer = add_2_nums( request->regs[0], request->regs[1] );
				break;

			case 3:
				answer = add_3_nums( request->regs[0], request->regs[1], request->regs[2] );
				break;

			case 4:
				answer = add_4_nums( request->regs[0], request->regs[1], request->regs[2], request->regs[3] );
				break;
				
			case 5:
				answer = add_5_nums( request->regs[0], request->regs[1], request->regs[2], request->regs[3], request->regs[4] );
				break;
				
			case 6:
				answer = add_6_nums( request->regs[0], request->regs[1], request->regs[2], request->regs[3], request->regs[4], request->regs[5] );
				break;
				
			case 7:
				answer = add_7_nums( request->regs[0], request->regs[1], request->regs[2], request->regs[3], request->regs[4], request->regs[5], request->regs[6] );
				break;
		}

		fipc_recv_msg_end( chan, request );
	}

	for ( i = 0; i < queue_depth; ++i )
	{
		fipc_test_blocking_send_start( chan, &response );
		response->regs[0] = answer;
		fipc_send_msg_end( chan, response );
	}
}

int requester ( void* data )
{
	header_t* chan = (header_t*) data;
	
	register uint64_t CACHE_ALIGNED transaction_id;
	register uint64_t CACHE_ALIGNED start;
	register uint64_t CACHE_ALIGNED end;
	register uint64_t CACHE_ALIGNED correction = fipc_test_time_get_correction();
	register int32_t* CACHE_ALIGNED times = vmalloc( transactions * sizeof( int32_t ) );

	// Begin test
	fipc_test_thread_take_control_of_CPU();

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		start = RDTSC_START();

		request( chan );

		end = RDTSCP();
		times[transaction_id] = (end - start) - correction;
	}

	// End test
	fipc_test_thread_release_control_of_CPU();
	fipc_test_stat_get_and_print_stats( times, transactions );
	vfree( times );
	complete( &requester_comp );
	return 0;
}

int responder ( void* data )
{
	header_t* chan = (header_t*) data;
	
	register uint64_t CACHE_ALIGNED transaction_id;

	// Begin test
	fipc_test_thread_take_control_of_CPU();

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		respond( chan );
	}

	// End test
	fipc_test_thread_release_control_of_CPU();
	complete( &responder_comp );
	return 0;
}


int main ( void )
{
	init_completion( &requester_comp );
	init_completion( &responder_comp );

	header_t*  requester_header = NULL;
	header_t*  responder_header = NULL;
	kthread_t* requester_thread = NULL;
	kthread_t* responder_thread = NULL;

	fipc_init();
	fipc_test_create_channel( CHANNEL_ORDER, &requester_header, &responder_header );

	if ( requester_header == NULL || responder_header == NULL )
	{
		pr_err( "%s\n", "Error while creating channel" );
		return -1;
	}

	// Create Threads
	requester_thread = fipc_test_thread_spawn_on_CPU ( requester, requester_header, requester_cpu );
	if ( requester_thread == NULL )
	{
		pr_err( "%s\n", "Error while creating thread" );
		return -1;
	}
	
	responder_thread = fipc_test_thread_spawn_on_CPU ( responder, responder_header, responder_cpu );
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
	fipc_test_free_channel( CHANNEL_ORDER, requester_header, responder_header );
	fipc_fini();
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
