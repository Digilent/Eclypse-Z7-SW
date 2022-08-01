#ifndef TRAFFIC_GENERATOR_H_   /* prevent circular inclusions */
#define TRAFFIC_GENERATOR_H_

#include "AxiStreamSourceMonitor.h"

#define SWITCH_SOURCE_SCOPE 0
#define SWITCH_SOURCE_GENERATOR 1

void AxiStreamSourceMonitorSetControl(AxiStreamSourceMonitor *InstPtr, u8 Enable, u8 Freerun, u8 GeneratorSelect);
void AxiStreamSourceMonitorSetEnable(AxiStreamSourceMonitor *InstPtr, u8 Enable);
void AxiStreamSourceMonitorSetSelect(AxiStreamSourceMonitor *InstPtr, u8 GeneratorSelect);

#endif /* end of protection macro */
