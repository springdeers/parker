#ifndef __DATATRANSFERS_H__
#define __DATATRANSFERS_H__

#include "datatransfer.h"

void       transfers_startup(config_t config);
int        transfers_pushAjob(job_st job);
int        transfers_stop();
int        transfers_free();

#endif