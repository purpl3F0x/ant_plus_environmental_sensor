

#ifndef TEMP_LOGGER_H
#define TEMP_LOGGER_H

#include "inttypes.h"

#define logger_max(a,b)              \
 ({ __typeof__ (a) _a = (a);  \
     __typeof__ (b) _b = (b); \
   _a > _b ? _a : _b; })

#define logger_min(a,b)              \
 ({ __typeof__ (a) _a = (a);  \
     __typeof__ (b) _b = (b); \
   _a < _b ? _a : _b; })


#define MINUTES_OF_A_DAY                          24u * 60u
#define NUMBER_OF_INTERVALS                       512u
#define TEMP_LOGGER_INTERVAL_MS_PERIOD            ((MINUTES_OF_A_DAY * 60u * 1000u) / NUMBER_OF_INTERVALS)

void logger_init(void);

void logger_push_min_max(const int16_t min_, const int16_t max_);

int16_t logger_get_max(void);

int16_t logger_get_min(void);


int16_t logger_find_max(void);

int16_t logger_find_min(void);

#endif // TEMP_LOGGER_H