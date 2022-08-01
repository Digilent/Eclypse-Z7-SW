#ifndef TRIGGER_H_   /* prevent circular inclusions */
#define TRIGGER_H_

#include "TriggerControl.h"

u32 TriggerGetIdle (TriggerControl *InstPtr);
void TriggerSetPosition (TriggerControl *InstPtr, u32 BufferLength, u32 TriggerPosition);
void TriggerSetEnable (TriggerControl *InstPtr, u32 EnableMask);
void TriggerStart (TriggerControl *InstPtr);
u32 TriggerGetDetected (TriggerControl *InstPtr);

#endif /* end of protection macro */
