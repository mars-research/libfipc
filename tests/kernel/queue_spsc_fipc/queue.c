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

int enqueue ( queue_t* q, node_t* r )
{
	node_t* msg;

	if (fipc_send_msg_start( q->head, &msg ) != 0)
		return NO_MEMORY;

	msg->regs[0] = r->regs[0];
	fipc_send_msg_end ( q->head, msg );

	return SUCCESS;
}

// Dequeue

int dequeue ( queue_t* q, data_t* data )
{
	node_t* msg;

	if (fipc_recv_msg_start( q->tail, &msg) != 0)
		return EMPTY_COLLECTION;

	*data = msg->regs[0];
	fipc_recv_msg_end( q->tail, msg );
	
	return SUCCESS;
}
