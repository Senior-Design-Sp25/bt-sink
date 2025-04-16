#include "common.h"
#include "bt_sink.h"
#include "i2s_sink.h"
#include "a2dp_sink.h"
#include "RNN.h"
#include "helper_functions.h"

  void app_main(void)
  {
    

  /* Initialize Bluetooth controller and Bluedroid stack */
  init_bt();

  // init avrc

  /* Initialize GAP profile */
  init_gap();

  /* Initialize the A2DP sink profile */
  init_a2dp();

  /* init i2s and create ring buffer */
  init_i2s();

  init_rnn();

  }

