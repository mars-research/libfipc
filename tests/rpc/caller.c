#include "rpc.h"

static unsigned long add_constant(unsigned long trans)
{
	return trans + 50;
}

static unsigned long add_nums(unsigned long trans, unsigned long res1)
{
	return trans+res1;
}

static unsigned long add_3_nums(unsigned long trans, unsigned long res1,
				unsigned long res2)
{
	return add_nums(trans,res1) + res2;
}

static unsigned long add_4_nums(unsigned long trans, unsigned long res1,
				unsigned long res2, unsigned long res3)
{

	return add_2_nums(trans,res1) + add_2_nums(res2+res3);
}

static unsigned long add_5_nums(unsigned long trans, unsigned long res1,
				unsigned long res2, unsigned long res3,
				unsigned long res4)
{
	return add_4_nums(trans,res1,res2,res3) + res4;
}

static unsigned long add_6_nums(unsigned long trans, unsigned long res1,
				unsigned long res2, unsigned long res3,
				unsigned long res4, unsigned long res5)
{
	return add_3_nums(trans,res1,res2) + add_3_nums(res3,res4,res5);
}

void caller(struct ttd_ring_channel *chan)
{

	unsigned long num_transactions = 0;
	unsigned long temp_res = 0;
	struct ipc_message *msg;
	while (num_transactions < TRANSACTIONS) {
		msg = recv(chan);

		switch(msg->fn_type) {
		case ADD_CONSTANT:
			temp_res = add_constant(msg->reg1);
			transaction_complete(msg);
			msg = get_send_slot(chan);
			msg->fn_type = ADD_CONSTANT;
			msg->reg1 = temp_res;
			send(msg);
			break;
		case ADD_NUMS:
			temp_res = add_nums(msg->reg1, msg->reg2);
			transaction_complete(msg);
			msg = get_send_slot(chan);
			msg->fn_type = ADD_NUMS;
			msg->reg1 = temp_res;
			send(msg);
			break;
		case ADD_3_NUMS:
			temp_res = add_3_nums(msg->reg1, msg->reg2, msg->reg3);
			transaction_complete(msg);
			msg = get_send_slot(chan);
			msg->fn_type = ADD_3_NUMS;
			msg->reg1 = temp_res;
			send(msg);
			break;
		case ADD_4_NUMS:
			temp_res = add_3_nums(msg->reg1, msg->reg2, msg->reg3,
					      msg->reg4);
			transaction_complete(msg);
			msg = get_send_slot(chan);
			msg->fn_type = ADD_4_NUMS;
			msg->reg1 = temp_res;
			send(msg);
			break;
		case ADD_5_NUMS:
			temp_res = add_3_nums(msg->reg1, msg->reg2, msg->reg3,
					      msg->reg4, msg->reg5);
			transaction_complete(msg);
			msg = get_send_slot(chan);
			msg->fn_type = ADD_5_NUMS;
			msg->reg1 = temp_res;
			send(msg);
			break;
		case ADD_6_NUMS:
			temp_res = add_3_nums(msg->reg1, msg->reg2, msg->reg3,
					      msg->reg4, msg->reg5, msg->reg6);
			transaction_complete(msg);
			msg = get_send_slot(chan);
			msg->fn_type = ADD_6_NUMS;
			msg->reg1 = temp_res;
			send(msg);
			break;
		}
		num_transactions++;
}
