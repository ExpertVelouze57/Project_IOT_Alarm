#include "net/loramac.h"
#include "semtech_loramac.h"
#include "cayenne_lpp.h"

#define NB_TENTATIVE_LORA       1
#define TIME_BT_TENTATIVE       500 //in us

#define NORMALE_send            (30*60) //30 min faire * 1s
#define EMERGENCY_send          (2*60) //2 min faire *1s
#define UNE_S                   (1000000U)




static const uint8_t deveui[LORAMAC_DEVEUI_LEN] = { 0x7c, 0xd0, 0x79, 0x4e, 0x00, 0xe3, 0xcf, 0x3f};
static const uint8_t appeui[LORAMAC_APPEUI_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t appkey[LORAMAC_APPKEY_LEN] = { 0x8B, 0x79, 0xA3, 0x46, 0xE1, 0x60, 0x79, 0x66, 0x09, 0x7A, 0x47, 0x4E, 0x45, 0x23, 0x1A, 0xAD};