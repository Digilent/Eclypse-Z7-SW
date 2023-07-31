#ifndef TEST_STREAM_SINK_H_   /* prevent circular inclusions */
#define TEST_STREAM_SINK_H_

#include "AxiStreamSinkMonitor.h"

void AxiStreamSinkMonitor_SetSelect(AxiStreamSinkMonitor *InstPtr, u8 SelectBit);
void AxiStreamSinkMonitor_SetLastCount(AxiStreamSinkMonitor *InstPtr, u32 LastCount);
void AxiStreamSinkMonitor_Start(AxiStreamSinkMonitor *InstPtr);
u32 AxiStreamSinkMonitor_GetIdle(AxiStreamSinkMonitor *InstPtr);
u32 AxiStreamSinkMonitor_GetBeatCount(AxiStreamSinkMonitor *InstPtr);
u32 AxiStreamSinkMonitor_GetMissCount(AxiStreamSinkMonitor *InstPtr);
u32 AxiStreamSinkMonitor_GetErrorCount(AxiStreamSinkMonitor *InstPtr);

#endif /* end of protection macro */
