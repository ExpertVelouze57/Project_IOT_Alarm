#include <string.h>

#include "net/loramac.h"
#include "semtech_loramac.h"

/*Header for sleep*/
#include "xtimer.h"

/*Header for thread and comunication*/
#include "thread.h"
#include "msg.h"

/*header for temp sensor*/

#include "board.h"

#include "Include/buzzer.h"
#include "Include/led.h"
#include "Include/bmp280.h"
#include "Include/flamme.h"
#include "Include/pir_detector.h"
#include "Include/bouton.h"

/* TODO: Add the cayenne_lpp header here */
#include "cayenne_lpp.h"


#define DELAY_temp              (30000000U) //30s

#define NORMALE_send            (30*60) //30 min faire * 1s
#define EMERGENCY_send          (2*60) //2 min faire *1s
#define UNE_S                   (1000000U)



/**/
kernel_pid_t p_led, p_alarm, p_bmx, p_flame, p_pir, p_sender;

/* Declare globally the loramac descriptor */
extern semtech_loramac_t loramac;
static cayenne_lpp_t lpp;


//Data LoRa et app
static const uint8_t deveui[LORAMAC_DEVEUI_LEN] = { 0x7c, 0xd0, 0x79, 0x4e, 0x00, 0xe3, 0xcf, 0x3f};
static const uint8_t appeui[LORAMAC_APPEUI_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t appkey[LORAMAC_APPKEY_LEN] = { 0x8B, 0x79, 0xA3, 0x46, 0xE1, 0x60, 0x79, 0x66, 0x09, 0x7A, 0x47, 0x4E, 0x45, 0x23, 0x1A, 0xAD};

/* Allocate the writer stack here */
static char sender_stack[THREAD_STACKSIZE_MAIN];
static char led_stack[THREAD_STACKSIZE_MAIN];
static char alarm_stack[THREAD_STACKSIZE_MAIN];
static char bmx_stack[THREAD_STACKSIZE_MAIN];
static char flame_stack[THREAD_STACKSIZE_MAIN];
static char pir_stack[THREAD_STACKSIZE_MAIN];

static msg_t led_queue[4];
static msg_t alarm_queue[4];
static msg_t send_queue[4];

//bt interupt
static bool erreur = false;
static bmx280_t my_bmp;
static pir_t dev;
static int bt_panic =0, fire =0, flame =0, PIR_min_10 = 0, alarm_enable =0;

/*Déclaration of function thread*/
static void *flashing_thread(void *arg){
    (void) arg;
    xtimer_ticks32_t clin = xtimer_now();

    /*Initialisation de la led*/
    gpio_t led = GPIO_PIN(Led_port ,Led_pin);
    gpio_init( led, GPIO_OUT );

    /*Initialisation du protocole de communication*/
    msg_init_queue(led_queue, 4);
    msg_t msg;

    //Déclaration variable local
    int valeur_clin =-1;

    while(1){
        //Recherche nouveau status
        if(msg_try_receive(&msg) != -1){
           valeur_clin = msg.content.value;   
        }
           
        if(valeur_clin == 0){
            /*Cas Incendi*/
            gpio_write(led,1);
            xtimer_periodic_wakeup(&clin, DELAY_LED_INC); 
            gpio_write(led,0);
            xtimer_periodic_wakeup(&clin, DELAY_LED_INC); 
        }

        else if(valeur_clin == 1){
            /*Clignottement lent == tt va bien*/
            gpio_write(led,1);
            xtimer_periodic_wakeup(&clin, DELAY_LED_NORM_on); 
            gpio_write(led,0);
            xtimer_periodic_wakeup(&clin, DELAY_LED_NORM_off); 

            
        }
        
        else if(valeur_clin == 2){
            /*Alumer fixe == erreur*/
            gpio_write(led,1);
            xtimer_periodic_wakeup(&clin, DELAY_LED_ERR); 

        }

        else{
            /*Cas default off*/
            printf("Led default\n");
            gpio_write(led,0);
            xtimer_periodic_wakeup(&clin, DELAY_LED_OFF); 
        }
    }

    
    return NULL;
}

static void *alarm_thread(void *arg){
    (void) arg;
    xtimer_usleep(1);

    //Definition du timer
    xtimer_ticks32_t pwm = xtimer_now();

    //Definition de la sortie du buzzer
    gpio_t buzzer = GPIO_PIN(Buzzer_port ,Buzzer_pin);
    gpio_init( buzzer, GPIO_OUT );

    //Creation du système de message
    msg_init_queue(alarm_queue, 4);
    msg_t msg, msg_led;

    //Definition variables local
    int temps;
    int temp=0, premiere_fois=0;
    

    while (1) {
        //Regarde si un message de changement d'état à été recu
        if(msg_try_receive(&msg) != -1){
           temp = msg.content.value;   
        }
           
        if(temp == 1){
            alarm_enable = 1;

            if(premiere_fois == 0){
                /*active la led urgence*/
                msg_led.content.value =0;
                msg_send(&msg_led, p_led);

                /*active l'envoie d'urgence*/
                msg_led.content.value =1;
                msg_send(&msg_led, p_sender);

                premiere_fois = 1;
            }
            

            //Joue la note 1
            temps= 0;
            while(temps < 400){
                xtimer_periodic_wakeup(&pwm, NOTE_1/2);      
                gpio_write(buzzer,1);
                xtimer_periodic_wakeup(&pwm, NOTE_1/2);  
                gpio_write(buzzer, 0);
                temps ++;
            }

            //Joue la note 2
            temps= 0;
            while(temps < 400){
                xtimer_periodic_wakeup(&pwm, NOTE_2/2);      
                gpio_write(buzzer,1);
                xtimer_periodic_wakeup(&pwm, NOTE_2/2);  
                gpio_write(buzzer, 0);
                temps ++;
            }

            //Pause 
            temps= 0;
            while(temps < 200){
                xtimer_periodic_wakeup(&pwm, DELAY_B_2); 
                temps ++;
            }
        }
        
        else{
            xtimer_periodic_wakeup(&pwm, DELAY_WAIT_BUZZER); 
            premiere_fois = 0;
        }
    }

    
    return NULL;
}

static void *monitor_bmx_thread(void *arg){
    (void) arg;
    xtimer_usleep(1);

    xtimer_ticks32_t bmx_time = xtimer_now();

    /*Déclaration des varibale et mise en place d'un old et new pour calcul de difference*/
    int16_t temp_old;
    int16_t temp = bmx280_read_temperature(&my_bmp)/100;
    int16_t coef;

    msg_t msg;


    while (1) {
        temp_old = temp;
        temp = bmx280_read_temperature(&my_bmp)/100;

        if(temp > T_MAX_ALARM){
            /*Trop chaud -> alarm*/
            fire =1;
            msg.content.value= 1;
            msg_send(&msg, p_alarm);
        }
        else if(temp<= T_MIN_ALARM && bt_panic != 1){
            fire = 0;
            msg.content.value= 0;
            msg_send(&msg, p_alarm);
        }

        if( temp == temp_old ){
             coef = 0;
        }
        else{
            coef = temp - temp_old;
        }
       
        if( coef > 5 ){
            fire = 1;
            msg.content.value= 1;
            msg_send(&msg, p_alarm);
        }
        xtimer_periodic_wakeup(&bmx_time, DELAY_temp); 
    }

    
    return NULL;
}

static void *monitor_flamme_thread(void *arg){
    (void) arg;
    xtimer_usleep(1);

    xtimer_ticks32_t flame_time = xtimer_now();

    msg_t msg;


    int xvalue=0; 
    while(1){
        xvalue = adc_sample(ADC_LINE(FLAMME_PORT), PRECISION_ADC);
        
        if(xvalue < FLAMME_SEUIL_DETEC){
            printf("Flame detected !!! \n");
            flame = 1;
            msg.content.value= 1;
            msg_send(&msg, p_alarm);
        }


        xtimer_periodic_wakeup(&flame_time, DELAY_FLAMME_REFRESH);
    }
    
    return NULL;
}

static void *monitor_PIR_thread(void *arg){
    (void) arg;

    /*Pause de 3 s avant de demarer*/
    xtimer_sleep(DELAY_to_start);

    //Gestion du temps
    xtimer_ticks32_t PIR_time = xtimer_now();
    uint32_t time_1 = xtimer_now_usec();;
    uint32_t time_2;

    //Déclaration variables interne
    int presence = 0, presence_avant =0;
    
    while (1) {
        presence =  (pir_get_status(&dev) == PIR_STATUS_INACTIVE ? 0 : 1);
        if(presence == 1){
            time_1 = xtimer_now_usec();
            presence_avant = presence;
            PIR_min_10 = 1;
        }
        else if(presence_avant == 1){
            time_2 = xtimer_now_usec();

            if((time_2 - time_1) < (TIME_PRESENCE)* 1000000){
                PIR_min_10 = 1;
            }
            else{
                PIR_min_10 = 0;
                presence_avant = 0;
            }

        }

        xtimer_periodic_wakeup(&PIR_time, DELAY_Pir);
    }
    
    return NULL;
}


static void bt_user(void *arg){
    (void) arg;
    printf("Pressed User\n"); 
    gpio_t B_temp= GPIO_PIN(BT_USER_PORT ,BT_USER_PIN);
    msg_t msg;

    /*Desactivation interuption == eviter les cliques multiple*/
    gpio_irq_disable(B_temp);

    msg.content.value = ((erreur == true) ? 2 : 1);
    msg_send(&msg, p_led);

    //Desactivation Alarm
    msg.content.value= 0;
    msg_send(&msg, p_alarm);

    //Retour normal frequence send data
    msg.content.value= 0;
    msg_send(&msg, p_sender);


    bt_panic = 0;
    fire = 0;
    flame=0;
    alarm_enable =0;
    
    xtimer_usleep(DELAY_BT);
    gpio_irq_enable(B_temp); 
}

static void bt_emergency(void *arg){
    (void) arg;
    printf("Pressed Emergency\n");
    gpio_t B_temp= GPIO_PIN(BT_EMRG_PORT , BT_EMRG_PIN);
    msg_t msg;

    /*Desactivation interuption == eviter les cliques multiple*/
    gpio_irq_disable(B_temp);

    msg.content.value= 1;
    msg_send(&msg, p_alarm);
    bt_panic = 1;
    
    xtimer_usleep(DELAY_BT);
    gpio_irq_enable(B_temp); 
}


static void *sender(void *arg){
    (void) arg;
    
    xtimer_ticks32_t SEND_time = xtimer_now();

    int16_t temperature, temps;
    int8_t mode = 0;   //Mode 0 = normale mode 1 = alarme

    /*Initialisation du protocole de communication*/
    msg_init_queue(send_queue, 4);
    msg_t msg;

    while (1) {
        temperature = bmx280_read_temperature(&my_bmp);        


        /* prepare cayenne lpp payload */
        cayenne_lpp_add_digital_input(&lpp, 0, fire);
        cayenne_lpp_add_temperature(&lpp, 1,(float)temperature/100);
        cayenne_lpp_add_digital_input(&lpp, 2, PIR_min_10);
        cayenne_lpp_add_digital_input(&lpp,3, flame);
        cayenne_lpp_add_digital_input(&lpp,4,bt_panic);

        /* //Une fois pour ajouter les boutons sur l'interface cayenne
        int temp1=0, temp2=0, temp3=0;
        cayenne_lpp_add_digital_output(&lpp,5, temp1);
        cayenne_lpp_add_analog_output(&lpp,6, temp2);
        cayenne_lpp_add_analog_output(&lpp,7,temp3);
        */

        printf("Sending LPP data\n");

        /* send the LoRaWAN message */
        uint8_t ret = semtech_loramac_send(&loramac, lpp.buffer, lpp.cursor);
        if (ret != SEMTECH_LORAMAC_TX_DONE) {
            printf("Cannot send lpp message, ret code: %d\n", ret);
        }

        /*clear buffer */
        cayenne_lpp_reset(&lpp);

        /*patiennte*/
        switch(mode){
            case 0:
                temps = NORMALE_send;
                break;
            case 1:
                temps = EMERGENCY_send;
                break;
        }

        printf("Next send in %d min\n", temps/60);
        for(int i=0; i < temps; i++){
            if(msg_try_receive(&msg) != -1){
                mode = msg.content.value;
                
                if( mode  == 1) //en cas durgence uniquement qui la pause et envoie les donnée 
                    break;   
            }

            xtimer_periodic_wakeup(&SEND_time, UNE_S);
        }

        
    }

    /* this should never be reached */
    return NULL;
}

void recv(void){
    while (1) {
        /* blocks until something is received */
        if(semtech_loramac_recv(&loramac) == SEMTECH_LORAMAC_RX_DATA) {
            loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
            printf("Data received: %s, port: %d\n",(char *)loramac.rx_data.payload, loramac.rx_data.port);

            /*
                formatage de donnée pour 1bytes to alarm,, 2 bytes to time send normal et 2 bytes to time send emergncy


            */


            break;
        }
        xtimer_usleep(100000);
    }
}

int main(void){
    /**/
    
   

    msg_t msg_main;

    /*Déclaration des boutons*/
    gpio_t B_user = GPIO_PIN( BT_USER_PORT , BT_USER_PIN );
    if(gpio_init_int (B_user, BT_RES, BT_DETEC , bt_user,(void*)0) < 0){
        puts("[FAILED] init BTN!");
        erreur = true;
    }
    gpio_t B_emergency = GPIO_PIN( BT_EMRG_PORT , BT_EMRG_PIN );
    if(gpio_init_int (B_emergency, BT_RES, BT_DETEC , bt_emergency,(void*)0) < 0){
        puts("[FAILED] init BTN!");
        erreur = true;
    }

    /*intitalisation bme */
    switch (bmx280_init(&my_bmp, &bmx280_params[0])) {
        case BMX280_ERR_BUS:
            puts("[Error] Something went wrong when using the I2C bus");
            erreur = true;
            break;
        case BMX280_ERR_NODEV:
            puts("[Error] Unable to communicate with any BMX280 device");
            erreur = true;
            break;
        default:
            /* all good -> do nothing */
            break;
    }

    //Initialaisation flam
    if (adc_init(0) < 0) {
        printf("Initialization of ADC_LINE(0) failed\n");
        erreur = true;
    } else {
        printf("Successfully initialized ADC_LINE(0)\n");  
    }

    //PIR motion sensor init
    pir_params_t my_pir_parm;
    my_pir_parm.gpio = GPIO_PIN(PIR_port ,PIR_port);
    my_pir_parm.active_high = PIR_active_high;


    if (pir_init(&dev, &my_pir_parm) == 0) {
        puts("[OK]\n");
    }
    else {
        puts("[Failfed]");
        erreur = true;
    }

    /* use a fast datarate so we don't use the physical layer too much */
    semtech_loramac_set_dr(&loramac, 5);

    /* set the LoRaWAN keys */
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    /* start the OTAA join procedure */
    
    puts("Starting join procedure");
    bool temp_erreur;
    for(int i =0; i<1;i++){
        if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
            puts("Join procedure failed");
            temp_erreur = true;
        }
        else{
            puts("Join procedure succeeded");
            temp_erreur = false;
            break;
        }

        //toutes les 500us
        xtimer_usleep(500);
    }
    printf("%d\n", temp_erreur);

    //erreur = erreur || temp_erreur;

    /*Création des thread*/
    p_alarm = thread_create(alarm_stack, sizeof(alarm_stack), THREAD_PRIORITY_MAIN - 1, 0, alarm_thread, NULL, "alarm thread");
    p_led= thread_create(led_stack, sizeof(led_stack), THREAD_PRIORITY_MAIN - 1, 0, flashing_thread, NULL, "led thread");
    p_bmx= thread_create(bmx_stack, sizeof(bmx_stack), THREAD_PRIORITY_MAIN - 1, 0, monitor_bmx_thread, NULL, "bmx thread");
    p_flame= thread_create(flame_stack, sizeof(flame_stack), THREAD_PRIORITY_MAIN - 1, 0, monitor_flamme_thread, NULL, "flame thread");
    p_pir= thread_create(pir_stack, sizeof(pir_stack), THREAD_PRIORITY_MAIN - 1, 0, monitor_PIR_thread, NULL, "PIr thread");
    p_sender = thread_create(sender_stack, sizeof(sender_stack), THREAD_PRIORITY_MAIN - 1, 0, sender, NULL, "sender thread");


    //fin initialisation envoie statue led
    msg_main.content.value = ((erreur == true) ? 2 : 1);
    msg_send(&msg_main, p_led);

    /*Déclaration est initialisation de la variable message*/
    while(1){
        recv();
    }

    return 0;
}