/**
 * @File     : main.c
 * @Author   : Abdullah Younis
 */

#include <thc.h>
#include <thcinternal.h>
#include <awe_mapper.h>

#include "test.h"
//#define FINE_GRAINED
static uint64_t transactions   = 1000000;
static uint32_t num_inner_asyncs = 10; 

#define REQUESTER_CPU	1
#define RESPONDER_CPU	3
#define CHANNEL_ORDER	ilog2(sizeof(message_t)) + 7
#define QUEUE_DEPTH	1


static int queue_depth = 1;


#if defined(FINE_GRAINED)
register uint64_t CACHE_ALIGNED start;
register uint64_t CACHE_ALIGNED end;
register int64_t* CACHE_ALIGNED times;
register uint64_t CACHE_ALIGNED correction;
#endif
uint64_t CACHE_ALIGNED whole_start;
uint64_t CACHE_ALIGNED whole_end;

void print_stats(int64_t *times, uint64_t transactions){

	// End test
#if defined(FINE_GRAINED) 
	register uint64_t CACHE_ALIGNED transaction_id;

	fipc_test_stat_get_and_print_stats( times, transactions );
	printf("Correction value: %llu\n", (unsigned long long) correction);
  	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		printf("%llu\n", (unsigned long long) times[transaction_id]);
	}

#endif
	// Print count
	printf("-------------------------------------------------\n");
	
}


void request ( header_t* chan )
{
	message_t* request;
	message_t* response;

	int i;

	for ( i = 0; i < queue_depth; ++i ) {
		fipc_test_blocking_send_start( chan, &request );
		fipc_send_msg_end ( chan, request );
	}

	for ( i = 0; i < queue_depth; ++i )
	{
		fipc_test_blocking_recv_start( chan, &response );
		fipc_recv_msg_end( chan, response );
	}
	
}

static inline
int fipc_test_blocking_recv_start_block ( header_t* channel, message_t** out, uint64_t id )
{
	int ret;
retry:
	while ( 1 )
	{
		// Poll until we get a message or error
		*out = get_current_rx_slot( channel);

		if ( ! check_rx_slot_msg_waiting( *out ) )
		{
			// No messages to receive, yield to next async
			//printf("No messages to recv, yield and save into id:%llu\n", id);
			THCYieldAndSave(id);
			continue; 
		}

		break;
	}

	if((*out)->regs[0] == id) {
		//printf("Message is ours id:%llu\n", (*out)->regs[0]);
		inc_rx_slot( channel ); 
		return 0;
	}
	
	//printf("Message is not ours yielding to id:%llu\n", (*out)->regs[0]);
	ret = THCYieldToIdAndSave((*out)->regs[0], id);
	 
	//ret = THCYieldToId((*out)->regs[0]);
	if (ret) {
		printf("ALERT: wrong id\n");
		return ret;
	}

	// We came back here but maybe we're the last AWE and 
        // we're re-started by do finish
	goto retry; 
	return 0;
}

void request_blk ( header_t* chan)
{
	message_t* req;
	message_t* resp;

	int id = awe_mapper_create_id();
	//printf("Got id:%d\n", id);

 	// Call
	fipc_test_blocking_send_start( chan, &req );
        req->regs[0] = (uint64_t)id;
	fipc_send_msg_end ( chan, req );

	// Reply (msg id is in reg[0]
	fipc_test_blocking_recv_start_block (chan, &resp, id);
	fipc_recv_msg_end( chan, resp );
	awe_mapper_remove_id(id);
	return; 	
}
void respond_blk ( header_t* chan )
{
	message_t* req;
	message_t* resp;

	uint64_t id;
	
	fipc_test_blocking_recv_start( chan, &req);
	id = req->regs[0];
	fipc_recv_msg_end( chan, req );
	
	fipc_test_blocking_send_start( chan, &resp );
	resp->regs[0] = id; 
	fipc_send_msg_end( chan, resp );
}


void respond ( header_t* chan )
{
	message_t* request;
	message_t* response;

	int i;
	for ( i = 0; i < queue_depth; ++i )
	{
		fipc_test_blocking_recv_start( chan, &request );
		fipc_recv_msg_end( chan, request );
	}

	for ( i = 0; i < queue_depth; ++i )
	{
		fipc_test_blocking_send_start( chan, &response );
		fipc_send_msg_end( chan, response );
	}
}

void ping_pong_rsp(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		respond( chan );
	}
}

void no_async_10_rsp(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		int j; 
		for (j = 0; j < num_inner_asyncs; j++) {
			respond( chan );
		}
	}
}

void async_10_rsp_blk(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		int j; 
		for (j = 0; j < num_inner_asyncs; j++) {
			respond_blk( chan );
		}
	}
}

void async_10_rsp_blk_srv_async_dispatch(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		int j; 
		DO_FINISH({
			for (j = 0; j < num_inner_asyncs; j++) {
				ASYNC({
					respond_blk( chan );
				});
			}
		});
	}
}

void ping_pong_req(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	// Wait to begin test
	pthread_mutex_lock( &requester_mutex );

	whole_start = RDTSC_START();

	// Begin test
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
#if defined(FINE_GRAINED)		
		start = RDTSC_START();
#endif
		request( chan );
#if defined(FINE_GRAINED)		
		end = RDTSCP();
		times[transaction_id] = (end - start) - correction;
#endif		
	}

	whole_end = RDTSCP();
#if defined(FINE_GRAINED)		
 	print_stats(times, transactions);
#endif
 	printf("ping-pong 1 msg: %llu\n",  
			(unsigned long long) (whole_end - whole_start) / transactions);


	pthread_mutex_unlock( &requester_mutex );
	return;

}

void no_async_10_req(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	// Wait to begin test
	pthread_mutex_lock( &requester_mutex );

	whole_start = RDTSC_START();

	// Begin test
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		int j;
#if defined(FINE_GRAINED)		
		start = RDTSC_START();
#endif
		DO_FINISH({
			for (j = 0; j < num_inner_asyncs; j++) {
				request( chan );
			};
		});
#if defined(FINE_GRAINED)		
		end = RDTSCP();
		times[transaction_id] = (end - start) - correction;
#endif		
	}

	whole_end = RDTSCP();
#if defined(FINE_GRAINED)		
 	print_stats(times, transactions);
#endif
 	printf("do_finish (10 msgs): %llu\n",  
			(unsigned long long) (whole_end - whole_start) / transactions);


	pthread_mutex_unlock( &requester_mutex );
	return;

}

void async_10_req(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	// Wait to begin test
	pthread_mutex_lock( &requester_mutex );

	whole_start = RDTSC_START();

	// Begin test
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		int j;
#if defined(FINE_GRAINED)		
		start = RDTSC_START();
#endif
		DO_FINISH({
			for (j = 0; j < num_inner_asyncs; j++) {
				ASYNC({
					request( chan );
				});
			};
		});
#if defined(FINE_GRAINED)		
		end = RDTSCP();
		times[transaction_id] = (end - start) - correction;
#endif		
	}

	whole_end = RDTSCP();
#if defined(FINE_GRAINED)		
 	print_stats(times, transactions);
#endif
 	printf("do{async{}}finish(), 10 msgs: %llu\n",  
			(unsigned long long) (whole_end - whole_start) / transactions);


	pthread_mutex_unlock( &requester_mutex );
	return;

}

void async_10_req_blk(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	// Wait to begin test
	pthread_mutex_lock( &requester_mutex );

	whole_start = RDTSC_START();

	// Begin test
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		int j;
#if defined(FINE_GRAINED)		
		start = RDTSC_START();
#endif
		DO_FINISH({
			for (j = 0; j < num_inner_asyncs; j++) {
				ASYNC({
					request_blk( chan );
				});
			};
		});
#if defined(FINE_GRAINED)		
		end = RDTSCP();
		times[transaction_id] = (end - start) - correction;
#endif		
	}

	whole_end = RDTSCP();
#if defined(FINE_GRAINED)		
 	print_stats(times, transactions);
#endif
 	printf("do{async{send_blk}}finish(), 10 msgs: %llu\n",  
			(unsigned long long) (whole_end - whole_start) / transactions);


	pthread_mutex_unlock( &requester_mutex );
	return;

}

void async_10_req_blk_srv_async_dispatch(header_t *chan) {
	register uint64_t CACHE_ALIGNED transaction_id;

	// Wait to begin test
	pthread_mutex_lock( &requester_mutex );

	whole_start = RDTSC_START();

	// Begin test
	for ( transaction_id = 0; transaction_id < transactions; transaction_id++ )
	{
		int j;
#if defined(FINE_GRAINED)		
		start = RDTSC_START();
#endif
		DO_FINISH({
			for (j = 0; j < num_inner_asyncs; j++) {
				ASYNC({
					request_blk( chan );
				});
			};
		});
#if defined(FINE_GRAINED)		
		end = RDTSCP();
		times[transaction_id] = (end - start) - correction;
#endif		
	}

	whole_end = RDTSCP();
#if defined(FINE_GRAINED)		
 	print_stats(times, transactions);
#endif
 	printf("do{async{send_blk}}finish(), 10 msgs, srv async dispatch: %llu\n",  
			(unsigned long long) (whole_end - whole_start) / transactions);


	pthread_mutex_unlock( &requester_mutex );
	return;

}


void* requester ( void* data )
{
	header_t* chan = (header_t*) data;
#if defined(FINE_GRAINED)		
	register int64_t* CACHE_ALIGNED times = malloc( transactions * sizeof( int64_t ) );
	correction = fipc_test_time_get_correction();
#endif
	thc_init();

	ping_pong_req(chan);
	no_async_10_req(chan);
	async_10_req(chan);
	async_10_req_blk(chan);
        async_10_req_blk_srv_async_dispatch(chan);
	thc_done();

#if defined(FINE_GRAINED)		
	free( times );
#endif
	pthread_exit( 0 );

	return NULL;
}

void* responder ( void* data )
{
	header_t* chan = (header_t*) data;
	
	thc_init();
	// Wait to begin test
	pthread_mutex_lock( &responder_mutex );

	ping_pong_rsp(chan); 
	no_async_10_rsp(chan);
	no_async_10_rsp(chan); 
	async_10_rsp_blk(chan);
	async_10_rsp_blk_srv_async_dispatch(chan);

	// End test
	pthread_mutex_unlock( &responder_mutex );
	thc_done();
	pthread_exit( 0 );
	return NULL;
}


int main ( void )
{
	// Begin critical section
	pthread_mutex_init( &requester_mutex, NULL );
	pthread_mutex_init( &responder_mutex, NULL );
	
	pthread_mutex_lock( &requester_mutex );
	pthread_mutex_lock( &responder_mutex );
	
	header_t* requester_header = NULL;
	header_t* responder_header = NULL;

	fipc_init();
	fipc_test_create_channel( CHANNEL_ORDER, &requester_header, &responder_header );

	if ( requester_header == NULL || responder_header == NULL )
	{
		fprintf( stderr, "%s\n", "Error while creating channel" );
		return -1;
	}

	// Create Threads
	pthread_t* requester_thread = fipc_test_thread_spawn_on_CPU ( requester, requester_header, REQUESTER_CPU );
	if ( requester_thread == NULL )
	{
		fprintf( stderr, "%s\n", "Error while creating thread" );
		return -1;
	}
	
	pthread_t* responder_thread = fipc_test_thread_spawn_on_CPU ( responder, responder_header, RESPONDER_CPU );
	if ( responder_thread == NULL )
	{
		fprintf( stderr, "%s\n", "Error while creating thread" );
		return -1;
	}
	
	// End critical section = start threads
	pthread_mutex_unlock( &responder_mutex );
	pthread_mutex_unlock( &requester_mutex );
	
	// Wait for thread completion
	fipc_test_thread_wait_for_thread( requester_thread );
	fipc_test_thread_wait_for_thread( responder_thread );
	
	// Clean up
	fipc_test_thread_free_thread( requester_thread );
	fipc_test_thread_free_thread( responder_thread );
	fipc_test_free_channel( CHANNEL_ORDER, requester_header, responder_header );
	fipc_fini();
	pthread_exit( NULL );
	return 0;
}
