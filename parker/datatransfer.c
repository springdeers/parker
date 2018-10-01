#include "datatransfer.h"
#include "dbaccess.h"

#define abstime_set(__time_,__timeout_ms_)\
{\
	__time_.tv_sec = time(NULL) + __timeout_ms_ / 1000;\
	__time_.tv_nsec = (__timeout_ms_ % 1000) * 1000000;\
}

static void dojob(job_t job)
{
	if (job == NULL) return;

}

static void* _worker(void* param)
{
	transfer_t transfer = (transfer_t)param;

	if (transfer == NULL) return NULL;

	struct timespec abstime;

	while (transfer->_runing){
		pthread_mutex_lock(&transfer->_mt_signal);

		int wait = 0;

		abstime_set(abstime, 100);//Î´À´100msÊ±¿Ì¡£
		
		while (jqueue_size(transfer->_workqueue) == 0 && wait == 0 )//predicate in critical area..
			wait = pthread_cond_timedwait(&transfer->_cond, &transfer->_mt_signal, &abstime);

		if (wait != 0 && wait != ETIMEDOUT) {//error
			pthread_mutex_unlock(&transfer->_mt_signal);
			break; 
		}

		if (jqueue_size(transfer->_workqueue) > 0){
			job_t job = (job_t)jqueue_pull(transfer->_workqueue);
			
			dojob(job);

			free(job);
		}

		pthread_mutex_unlock(&transfer->_mt_signal);
	}

	return (void*)1;
}

transfer_t transfer_new()
{
	transfer_t rt = NULL;

	while ((rt = calloc(1, sizeof(transfer_st))) == NULL) Sleep(1);

	rt->_ctime = time(0);

	rt->_workqueue = jqueue_new();

	pthread_cond_init(&rt->_cond, NULL);

	pthread_mutex_init(&rt->_mt_signal, NULL);
	
	return rt;
}

void       transfer_free(transfer_t transfer)
{
	if (transfer)
	{
		transfer_stop(transfer);

		if (transfer->_workqueue) jqueue_free(transfer->_workqueue);

		pthread_mutex_destroy(&transfer->_mt_signal);

		pthread_cond_destroy(&transfer->_cond);
		
	}
}

int        transfer_start(transfer_t transfer)
{
	if (transfer == NULL)       return 0;
	if (transfer->_pth != NULL) return 0;

	transfer->_runing = 1;

	pthread_create(&transfer->_pth, NULL, _worker, transfer);

	return 1;
}

int        transfer_stop(transfer_t transfer)
{
	if (transfer == NULL || transfer->_pth == NULL)  return 0;

	transfer->_runing = 0;

	pthread_join(transfer->_pth, NULL);

	transfer->_pth = NULL;

	return 1;
}

int      transfer_postAjob(transfer_t transfer, job_st job)
{
	if (transfer == NULL) return 0;

	pthread_mutex_lock(&transfer->_mt_signal);

	job_t job2 = calloc(1, sizeof(job_st));
	memcpy(job2, &job, sizeof(job_st));

	jqueue_push(transfer->_workqueue, job2,0);

	pthread_cond_signal(&transfer->_cond);

	pthread_mutex_unlock(&transfer->_mt_signal);

	return 1;
}