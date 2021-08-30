
#include "24hr_temp_logger.h"


static struct temp_logger_ring_buffer_t {
  int16_t max_t[NUMBER_OF_INTERVALS];
  int16_t min_t[NUMBER_OF_INTERVALS];
  unsigned size_;
  unsigned cur_index;
  int16_t last_min, last_max;
} buf;


void logger_init(void) {
  buf.size_ = 0;
  buf.cur_index = 0;
  buf.last_min = INT16_MAX;
  buf.last_max = INT16_MIN;
}


void logger_push_min_max(const int16_t min_, const int16_t max_) {
  buf.min_t[buf.cur_index] = min_;
  buf.max_t[buf.cur_index] = max_;

  buf.cur_index = (buf.cur_index+1) % NUMBER_OF_INTERVALS;

  if (buf.size_ < NUMBER_OF_INTERVALS) buf.size_++;
}


int16_t logger_get_max(void) {
   return buf.last_max;
}

int16_t logger_get_min(void){
   return buf.last_min;
}


__attribute__((optimize("unroll-loops")))
int16_t logger_find_max(void) {
  buf.last_max = buf.max_t[0];
  if (buf.size_ == NUMBER_OF_INTERVALS) {
    for (unsigned i = 1; i < NUMBER_OF_INTERVALS; i++) {
      buf.last_max = logger_max(buf.last_max, buf.max_t[i]);
    }

    return buf.last_max;
  }

  for (unsigned i = 1; i < buf.size_; i++) {
    buf.last_max = logger_max(buf.last_max, buf.max_t[i]);
  }
  return buf.last_max;
}


__attribute__((optimize("unroll-loops")))
int16_t logger_find_min(void) {
  //buf.last_min = buf.min_t[0];

  //if (buf.size_ == NUMBER_OF_INTERVALS)
  //{
  //  for (unsigned i = 1; i < NUMBER_OF_INTERVALS; i++)
  //  {
  //    buf.last_min = logger_min(buf.last_min, buf.min_t[i]);
  //  }
  //  return buf.last_min;
  //}

  for (unsigned i = 0; i < buf.size_; i++)
  {
    buf.last_min = logger_min(buf.last_min, buf.min_t[i]);
  }
  return buf.last_min;
}