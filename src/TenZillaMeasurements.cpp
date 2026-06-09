#include "TenZillaMeasurements.h"
#include "TenZillaNTP.h"
#include <Preferences.h>
#include <time.h>

#pragma pack(push, 1)
struct MeasEntry {
  uint32_t timestamp;
  uint8_t programType;
  uint8_t outcome;
  float weight;
};
#pragma pack(pop)

static const size_t ENTRY_SIZE = sizeof(MeasEntry);
static const size_t BLOB_HEADER = 4;
static const size_t BLOB_SIZE = BLOB_HEADER + MEAS_MAX_ENTRIES * ENTRY_SIZE;

static uint16_t head = 0;
static uint16_t count = 0;
static MeasEntry entries[MEAS_MAX_ENTRIES];

static const char* NVS_NS = "tenzilla-meas";
static const char* NVS_KEY = "meas";

static uint32_t nowUnix() {
  if (TenZillaNTP::isTimeSynced()) {
    time_t t = time(nullptr);
    return (t > 0) ? (uint32_t)t : 0;
  }
  return 0;
}

void TenZillaMeasurements::load() {
  Preferences prefs;
  if (!prefs.begin(NVS_NS, true)) return;
  uint8_t buf[BLOB_SIZE];
  size_t len = prefs.getBytesLength(NVS_KEY);
  if (len != BLOB_SIZE) {
    prefs.end();
    return;
  }
  if (prefs.getBytes(NVS_KEY, buf, BLOB_SIZE) != BLOB_SIZE) {
    prefs.end();
    return;
  }
  prefs.end();
  head = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
  count = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
  if (count > MEAS_MAX_ENTRIES) count = MEAS_MAX_ENTRIES;
  memcpy(entries, buf + BLOB_HEADER, MEAS_MAX_ENTRIES * ENTRY_SIZE);
}

void TenZillaMeasurements::save() {
  Preferences prefs;
  if (!prefs.begin(NVS_NS, false)) return;
  uint8_t buf[BLOB_SIZE];
  buf[0] = (uint8_t)(head & 0xFF);
  buf[1] = (uint8_t)((head >> 8) & 0xFF);
  buf[2] = (uint8_t)(count & 0xFF);
  buf[3] = (uint8_t)((count >> 8) & 0xFF);
  memcpy(buf + BLOB_HEADER, entries, MEAS_MAX_ENTRIES * ENTRY_SIZE);
  prefs.putBytes(NVS_KEY, buf, BLOB_SIZE);
  prefs.end();
}

void TenZillaMeasurements::begin() {
  head = 0;
  count = 0;
  memset(entries, 0, sizeof(entries));
  load();
}

void TenZillaMeasurements::record(uint8_t programType, uint8_t outcome, float weightN) {
  uint32_t ts = nowUnix();
  MeasEntry e;
  e.timestamp = ts;
  e.programType = programType;
  e.outcome = outcome;
  e.weight = weightN;

  if (count < MEAS_MAX_ENTRIES) {
    int idx = (head + count) % MEAS_MAX_ENTRIES;
    entries[idx] = e;
    count++;
  } else {
    entries[head] = e;
    head = (head + 1) % MEAS_MAX_ENTRIES;
  }
  save();
}

int TenZillaMeasurements::getCount() {
  return (int)count;
}

void TenZillaMeasurements::getEntry(int index, uint32_t& timestamp, uint8_t& programType, uint8_t& outcome, float& weightN) {
  if (index < 0 || index >= (int)count) {
    timestamp = 0;
    programType = 0;
    outcome = 0;
    weightN = 0.0f;
    return;
  }
  int i = (head + count - 1 - index) % MEAS_MAX_ENTRIES;
  const MeasEntry& e = entries[i];
  timestamp = e.timestamp;
  programType = e.programType;
  outcome = e.outcome;
  weightN = e.weight;
}
