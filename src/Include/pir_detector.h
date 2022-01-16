#include "pir.h"
#include "pir_params.h"

#define PIR_port PORT_A
#define PIR_pin  0
#define PIR_active_high true

#define DELAY_to_start      3
#define DELAY_Pir           (100000U)
#define TIME_PRESENCE       (10) //Periode en seconde ou on veut savoir si il y a quelq'un