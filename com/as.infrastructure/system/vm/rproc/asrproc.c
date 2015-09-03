/**
 * AS - the open source Automotive Software on https://github.com/parai
 *
 * Copyright (C) 2015  AS <parai@foxmail.com>
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation; See <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */
/* ============================ [ INCLUDES  ] ====================================================== */
#include "remoteproc.h"
#include <windows.h>
/* ============================ [ MACROS    ] ====================================================== */

/* ============================ [ TYPES     ] ====================================================== */
enum rp_fifo_messages {
	RP_FIFO_READY		= 0xFFFFFF00,
	RP_FIFO_PENDING_MSG	= 0xFFFFFF01,
	RP_FIFO_CRASH		= 0xFFFFFF02,
	RP_FIFO_ECHO_REQUEST	= 0xFFFFFF03,
	RP_FIFO_ECHO_REPLY	= 0xFFFFFF04,
	RP_FIFO_ABORT_REQUEST	= 0xFFFFFF05,
};

struct rsc_fifo {
	u32 count;
	u32 size;	/* size of identifier in u32 */
	u32 r_pos;
	u32 w_pos;
	u32 identifier[0];
} __packed;
/* ============================ [ DECLARES  ] ====================================================== */
static int start(struct rproc *rproc);
static int stop(struct rproc *rproc);
static void kick(struct rproc *rproc, int vqid);
/* ============================ [ DATAS     ] ====================================================== */
static struct device rpdev = {
	.name = "remote processor AUTOSAR ECU",
	.address = NULL,
	.size    = 0
};
static struct rproc* rproc;
static const struct rproc_ops rproc_ops=
{
	.start = start,
	.stop  = stop,
	.kick  = kick
};

struct rsc_fifo* r_fifo;
struct rsc_fifo* w_fifo;
/* ============================ [ LOCALS    ] ====================================================== */
static bool fifo_write(u32 id)
{
	bool ercd;
	WaitForSingleObject(rpdev.w_lock,INFINITE);
	if(w_fifo->count < w_fifo->size)
	{
		w_fifo->identifier[w_fifo->w_pos] = id;
		w_fifo->w_pos = (w_fifo->w_pos + 1)%(w_fifo->size);
		w_fifo->count += 1;
		ercd = true;
	}
	else
	{
		assert(0);
		ercd = false;
	}
	ReleaseMutex(rpdev.w_lock);
	SetEvent( rpdev.w_event );
	return ercd;
}
static bool fifo_read(u32* id)
{
	bool ercd;
	WaitForSingleObject(rpdev.r_lock,INFINITE);
    if(r_fifo->count > 0)
	{
		*id = r_fifo->identifier[r_fifo->r_pos];
		r_fifo->r_pos = (r_fifo->r_pos + 1)%(r_fifo->size);
		r_fifo->count -= 1;
		ercd = true;
	}
    else
    {
    	assert(0);
    	ercd  = false;
    }
	ReleaseMutex(rpdev.r_lock);
	return ercd;
}
static int start(struct rproc *rproc)
{
	/* refer omap_remoteproc.c code for linux, this rproc is gust */
	fifo_write(RP_FIFO_READY);
	return 0;
}
static int stop(struct rproc *rproc)
{
	return 0;
}
static void kick(struct rproc *rproc, int vqid)
{

}
/* ============================ [ FUNCTIONS ] ====================================================== */
void InitOS(void)
{
	struct device* dev;
	if(NULL != rpdev.address)
	{
		printf("  >> start rproc up!\n");
		dev = &rpdev;
		/* init FIFO */
		dev->size = dev->size - dev->sz_fifo*2*sizeof(u32);

		r_fifo = (struct rsc_fifo*)((unsigned long)dev->address + dev->size + 0*dev->sz_fifo*sizeof(u32));
		w_fifo = (struct rsc_fifo*)((unsigned long)dev->address + dev->size + 1*dev->sz_fifo*sizeof(u32));

		r_fifo->count = 0;
		r_fifo->size  = dev->sz_fifo - sizeof(struct rsc_fifo)/sizeof(u32);
		r_fifo->r_pos = 0;
		r_fifo->w_pos = 0;
		memset(r_fifo->identifier,0,r_fifo->size*sizeof(u32));

		w_fifo->count = 0;
		w_fifo->size  = dev->sz_fifo - sizeof(struct rsc_fifo)/sizeof(u32);
		w_fifo->r_pos = 0;
		w_fifo->w_pos = 0;
		memset(w_fifo->identifier,0,w_fifo->size*sizeof(u32));
		/* inti remote processor */
		rproc = rproc_alloc(&rpdev,"AsRproc",&rproc_ops,NULL,1024);
	}
	else
	{
		assert(0);
	}
}

bool AsRproc_Init(void* address, size_t size,HANDLE r_lock,HANDLE w_lock,HANDLE r_event, HANDLE w_event, size_t sz_fifo)
{

	bool bOK = false;
	if(NULL == rpdev.address)
	{
		rpdev.address = address;
		rpdev.size    = size;
		rpdev.sz_fifo = sz_fifo;
		rpdev.r_lock  = r_lock;
		rpdev.w_lock  = w_lock;
		rpdev.r_event = r_event;
		rpdev.w_event = w_event;
		bOK = true;
	}
	printf("  >> AsRproc_Init(0x%X,%d,0x%X,0x%X,0x%X,0x%X) = %s\n",
			address,size,r_lock,w_lock,r_event,w_event,bOK?"true":"false");
	return bOK;
}
