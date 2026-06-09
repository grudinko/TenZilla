#ifndef TENZILLA_MEASUREMENTS_H
#define TENZILLA_MEASUREMENTS_H

#include <Arduino.h>

// Outcome: result of program run
#define MEAS_OUTCOME_COMPLETED  0
#define MEAS_OUTCOME_STOPPED    1
#define MEAS_OUTCOME_ERROR      2

// Program type
#define MEAS_TYPE_COMPRESSION   1
#define MEAS_TYPE_BREAK         2

#define MEAS_MAX_ENTRIES        50

class TenZillaMeasurements {
public:
  static void begin();
  static void record(uint8_t programType, uint8_t outcome, float weightN);

  static int getCount();
  static void getEntry(int index, uint32_t& timestamp, uint8_t& programType, uint8_t& outcome, float& weightN);

private:
  static void load();
  static void save();
};

#endif
