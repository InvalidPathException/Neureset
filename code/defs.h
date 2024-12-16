#ifndef DEFS_H
#define DEFS_H

#define NUM_BRAIN_SITES 21
#define NUM_BRAIN_FREQ 4
#define MAX_SAMPLES 4

#define MAX_FREQ 51     // 0 - 51 min max span
#define REFRESH_PERIOD 62 // 62ms, 16 samples per second
#define NOISE_FLOOR 10.
#define NUM_OFFSETS 4

// anyone who change the total time here (in seconds)
// pre treatment delay: 5. before and after delay of treatments 2 * 4 offsets: 8
// 13 * 21 = 273
#define TREATMENT_TIME 273
// three full treatments: 273 * 3 = 819 total battery life
#define BATTERY_CAPACITY TREATMENT_TIME * 3

#define LOW_BATTERY_THRESHOLD 0.1
#define LOW_BATTERY_SHUTOFF 0.01

#define NORMAL_BATTERY_STYLESHEET "QProgressBar {border: 1px solid black;border-radius: 5px;background-color: transparent;} QProgressBar::chunk { background-color: rgb(46, 194, 126); }"
#define LOW_BATTERY_STYLESHEET "QProgressBar {border: 1px solid black;border-radius: 5px;background-color: transparent;} QProgressBar::chunk { background: red; }"
#define PI M_PIl

#define DB_PATH "/neureset.db"

#endif // DEFS_H
