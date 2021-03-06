/**
 * @File     : queue.c
 * @Author   : Abdullah Younis
 */

#include "queue.h"

// Constructor

int init_queue ( queue_t* q )
{
	fipc_test_create_channel( CHANNEL_ORDER, &q->head, &q->tail );

	if ( q->head == NULL || q->tail == NULL )
	{
		pr_err( "%s\n", "Error while creating channel" );
		return -1;
	}

	return SUCCESS;
}

// Destructor

int free_queue ( queue_t* q )
{
	fipc_test_free_channel( CHANNEL_ORDER, q->head, q->tail );
	return SUCCESS;
}

// Enqueue

int enqueue ( queue_t* q, request_t* r )
{
	request_t* msg;

	fipc_test_blocking_send_start( q->head, &msg );
	msg->regs[0] = r->regs[0];
	fipc_send_msg_end ( q->head, msg );

	return SUCCESS;
}

// Dequeue

int dequeue ( queue_t* q, request_t** r )
{
	request_t* msg;

	fipc_test_blocking_recv_start( q->tail, &msg );
	(*r)->regs[0] = msg->regs[0];
	fipc_recv_msg_end( q->tail, msg );
	
	return SUCCESS;
}
