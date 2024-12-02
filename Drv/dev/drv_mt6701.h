#ifndef __DRV_MT6701_H_
#define __DRV_MT6701_H_

#include "spi.h"

uint16_t MT6701_GetRawData(void);
float MT6701_GetRawAngle(void);
float MT6701_GetFullAngle(void);
float MT6701_GetVelocity(void);

#endif
