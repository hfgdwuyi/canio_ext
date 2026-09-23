#ifndef ROTATION_CONTROL_H
#define ROTATION_CONTROL_H

void rotation_detection_init(void);
void rotation_detection_handler(void);
void calibrate_angles(void);
void calibrate_orientation(void);

extern double acc_before[3];
extern double acc_after[3];

#endif