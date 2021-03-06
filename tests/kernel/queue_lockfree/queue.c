/**
 * @File     : queue.c
 * @Author   : Abdullah Younis
 */

#include "queue.h"

// Constructor

int init_queue ( queue_t* q )
{
	q->header.next = NULL;

	q->head = &(q->header);
	q->tail = &(q->header);

	return SUCCESS;
}

// Destructor

int free_queue ( queue_t* q )
{
	// STUB
	return SUCCESS;
}

// Enqueue
// We will enqueue *r at the end of *q.

int enqueue ( queue_t* q, request_t* r )
{
	request_t* tail = NULL;
	request_t* next = NULL;

	// 1. Ground r.
	r->next = NULL;

	while ( 1 )
	{
		// 2. Make private copy of tail and tail->next
		tail = q->tail;
		next = q->tail->next;

		if ( tail == q->tail )
		{
			if ( next == NULL )
			{
				if ( fipc_test_CAS( &tail->next, next, r ) )
					break;
			}
			else
			{
				fipc_test_CAS( &q->tail, tail, next );
			}
		}
	}

	fipc_test_CAS( &q->tail, tail, r );

	// // 3. Attempt to change q->tail until success (starvation possible)
	// while( !fipc_test_stone_CAS( &q->tail, pTail, r ) );

	// // 4. Finish Linking
	// if ( pTail != NULL )
	// 	// 4a. Nonempty queue, update previous tail
	// 	pTail->next = r;
	// else
	// 	// 4b. Empty queue, reset head
	// 	q->head = r;


	return SUCCESS;
}

// Dequeue
// We will dequeue a node from the front of *q and place it in **r

int dequeue ( queue_t* q, uint64_t* data )
{
	request_t* head = NULL;
	request_t* tail = NULL;
	request_t* next = NULL;

	while ( 1 )
	{
		head = q->head;
		tail = q->tail;
		next = head->next;

		if ( head == q->head )
		{
			if ( head == tail )
			{
				if ( next == NULL )
				{
					return EMPTY_COLLECTION;
				}
				fipc_test_CAS( &q->tail, tail, next );
			}
			else
			{
				*data = next->data;
				if ( fipc_test_CAS( &q->head, head, next ) )
				{
					break;
				}
			}
		}
	}

	return SUCCESS;

	// int finished = 0;

	// // 1. Make private copy of head
	// request_t* pHead = q->head;

	// // 2. If queue isn't empty
	// if ( pHead != NULL )
	// {
	// 	// 3. Make private copy of tail
	// 	request_t* pTail = q->tail;

	// 	// 4. Attempt to dequeue
	// 	while ( !finished )
	// 	{
	// 		request_t* next = pHead->next;

	// 		// 4a. If queue is empty, finish
	// 		if ( pHead == NULL )
	// 		{
	// 			finished = true;
	// 		}

	// 		// 4b. If queue has one item, update both head and tail
	// 		else if ( pHead == pTail )
	// 		{
	// 			// Try to set tail to null
	// 			finished = fipc_test_stone_CAS( &q->tail, pTail, NULL );

	// 			// If that worked, try to set head to null,
	// 			// unless enqueuer reset it
	// 			if ( finished )
	// 				fipc_test_stone_CAS( &q->head, pHead, NULL );
	// 		}

	// 		// 4c. Multi-item list and no enqueue operation in progress
	// 		else if ( next != NULL )
	// 		{
	// 			finished = fipc_test_stone_CAS( &q->head, pHead, next );
	// 		}

	// 		// 4d. Else last item already being dequeued or intermediate state
	// 		else
	// 		{
	// 			pHead = NULL;
	// 			finished = true;
	// 		}
	// 	}

	// 	// 5. If dequeue succeeded then ground dequeued item
	// 	if ( pHead != NULL )
	// 		pHead->next = NULL;
	// }

	// // 6. Return dequeued item
	// *r = pHead;
	return SUCCESS;
}
