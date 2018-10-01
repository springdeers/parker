#include "datatransfers.h"

#define __transfer_count 1

#define abstime_set(__time_,__timeout_ms_)\
{\
	__time_.tv_sec = time(NULL) + __timeout_ms_ / 1000;\
	__time_.tv_nsec = (__timeout_ms_ % 1000) * 1000000;\
}

static transfer_t   g_transfers = NULL;

void transfers_startup(config_t config)
{
	if (g_transfers == NULL){
		g_transfers = calloc(__transfer_count, sizeof(transfer_st));
		for (int i = 0; i < __transfer_count; i++){
			transfer_init (&g_transfers[i]);
			transfer_start(&g_transfers[i]);
		}
	}
}

int    transfers_pushAjob(job_st job)
{
	transfer_t mostidle = &g_transfers[0];
	for (int i = 1; i < __transfer_count; i++){
		if (transfer_jobs_remain_cnt(mostidle) > transfer_jobs_remain_cnt(&g_transfers[i]))
			mostidle = &g_transfers[i];
	}

	transfer_postAjob(mostidle, job);

	return 1;
}

int  transfers_stop()
{
	for (int i = 0; i < __transfer_count; i++){
		transfer_stop (&g_transfers[i]);
		transfer_clear(&g_transfers[i]);
	}

	return 1;
}

int   transfers_free()
{
	transfers_stop();

	free(g_transfers);

	g_transfers = NULL;

	return 1;
}