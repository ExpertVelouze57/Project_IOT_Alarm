#include "bmx280.h"
#include "bmx280_params.h"


#define T_MAX_ALARM 60  //seuil alerte maximal
#define T_MIN_ALARM 25  //seuil retour à la normal

#define DELAY_BMP_REFRESH (30000000U) //30s
