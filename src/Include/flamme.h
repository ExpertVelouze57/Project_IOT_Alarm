#include "periph/adc.h"

#define FLAMME_PORT 0  //adc line 
#define PRECISION_ADC ADC_RES_10BIT

#define FLAMME_SEUIL_DETEC      1000
#define DELAY_FLAMME_REFRESH    (500000U)