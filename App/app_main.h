#ifndef __APP_MAIN_H_
#define __APP_MAIN_H_

#include "main.h"

/* Include Lib */
#include "arm_math.h"

/* Include Hardware */
#include "adc.h"
#include "opamp.h"
#include "tim.h"

/* Include Drivers */
#include "drv_uart.h"
#include "shell_port.h"
#include "RTT.h"

#include "drv_mt6701.h"

void app_main(void);

#endif
