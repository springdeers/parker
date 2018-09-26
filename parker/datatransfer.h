#ifndef __DATATRANSFER_H__
#define __DATATRANSFER_H__

#include <time.h>
#include "pthread.h"
#include "job.h"
#include "cutil/util.h"

#ifdef __cplusplus
extern "C"{
#endif

	typedef struct _transfer_st{
		time_t          _ctime;
		pthread_t       _pth;
		pthread_cond_t  _cond;
		pthread_mutex_t _mt_signal;
		int             _runing;
		jqueue_t        _workqueue;
		
	}transfer_st, *transfer_t;

	transfer_t transfer_new();
	void       transfer_free (transfer_t transfer);
	int        transfer_start(transfer_t transfer);
	int        transfer_stop (transfer_t transfer);

	int        transfer_postAjob (transfer_t transfer,job_st job);
#ifdef __cplusplus
}
#endif

#endif