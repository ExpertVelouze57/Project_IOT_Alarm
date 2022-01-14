#include <string.h>

#include "net/loramac.h"
#include "semtech_loramac.h"

/*Header for sleep*/
#include "xtimer.h"

/*Header for thread and comunication*/
#include "thread.h"
#include "msg.h"

/*header for temp sensor*/
#include "bmx280.h"
#include "bmx280_params.h"

#include "board.h"

/* TODO: Add the cayenne_lpp header here */
#include "cayenne_lpp.h"

#define DELAY_led_off          (3000000U)
#define DELAY_led_on           (100000U)
#define DELAY_led_wait           (500000U)

#define DELAY_temp           (30000000U) //30s

#define DELAY           (500000U)
#define NOTE_1           (2500U)
#define NOTE_2           (1250U)
#define NOTE_3           (2500U)
#define NOTE_4           (2500U)
#define DELAY_B_2           (2)
#define STEPS           (1000U)


/**/
kernel_pid_t p_led, p_alarm, p_bmx;
//on attaque le catpteur de température 

/* Allocate the writer stack here */
static char led_stack[THREAD_STACKSIZE_MAIN];
static char alarm_stack[THREAD_STACKSIZE_MAIN];
static char bmx_stack[THREAD_STACKSIZE_MAIN];

static msg_t led_queue[4];
static msg_t alarm_queue[4];

//bt interupt
static bool erreur = false;
static bmx280_t my_bmp;
static int bt_panic =0, fire =0;

/*Déclaration of function thread*/
static void *flashing_thread(void *arg){
    (void) arg;
    xtimer_ticks32_t clin = xtimer_now();

    /*Initialisation de la led*/
    gpio_t led = GPIO_PIN(PORT_B ,5);
    gpio_init( led, GPIO_OUT );

    /*Initialisation du protocole de communication*/
    msg_init_queue(led_queue, 4);

    msg_t msg;
    int valeur_clin =0;

    while(1){
        if(msg_try_receive(&msg) != -1){
           valeur_clin = msg.content.value;   
        }
           
        if(valeur_clin == 0){
            /*Cas off*/
            gpio_write(led,0);
            xtimer_periodic_wakeup(&clin, DELAY_led_wait); 

        }

        else if(valeur_clin == 1){
            /*Clignottement lent == tt va bien*/
            gpio_write(led,1);
            xtimer_periodic_wakeup(&clin, DELAY_led_on); 
            gpio_write(led,0);
            xtimer_periodic_wakeup(&clin, DELAY_led_off); 

            
        }
        
        else if(valeur_clin == 2){
            /*Alumer fixe == erreur*/
            printf("herre\n");
            gpio_write(led,1);
            xtimer_periodic_wakeup(&clin, DELAY_led_wait); 

        }

        else{
            /*Cas default off*/
            gpio_write(led,0);
            xtimer_periodic_wakeup(&clin, DELAY_led_wait); 
        }
    }

    
    return NULL;
}

static void *alarm_thread(void *arg){
    (void) arg;
    xtimer_usleep(1);

    xtimer_ticks32_t pwm = xtimer_now();
    gpio_t buzzer = GPIO_PIN(PORT_B ,4);
    gpio_init( buzzer, GPIO_OUT );

    msg_init_queue(alarm_queue, 4);
    
    int temps;
    int temp=0;
    msg_t msg;

    while (1) {
        if(msg_try_receive(&msg) != -1){
           temp = msg.content.value;   
        }
           
        if(temp == 1){
            temps= 0;
            while(temps < 400){
                xtimer_periodic_wakeup(&pwm, NOTE_1/2);      
                gpio_write(buzzer,1);
                xtimer_periodic_wakeup(&pwm, NOTE_1/2);  
                gpio_write(buzzer, 0);
                temps ++;
            }
        
            temps= 0;
            while(temps < 400){
                xtimer_periodic_wakeup(&pwm, NOTE_2/2);      
                gpio_write(buzzer,1);
                xtimer_periodic_wakeup(&pwm, NOTE_2/2);  
                gpio_write(buzzer, 0);
                temps ++;
            }

            temps= 0;
            while(temps < 200){
                xtimer_periodic_wakeup(&pwm, NOTE_1); 
                temps ++;
            }
        }
        
        else{
            xtimer_periodic_wakeup(&pwm, DELAY); 
        }
    }

    
    return NULL;
}

#define T_MAX_ALARM 60
#define T_MIN_ALARM 25

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
        printf("%d    %d     %d\n", temp, temp_old,coef);
        xtimer_periodic_wakeup(&bmx_time, DELAY_temp); 
    }

    
    return NULL;
}


static void bt_user(void *arg){
    printf("Pressed User%d\n", (int)arg); 
    gpio_t B_temp= GPIO_PIN(PORT_A ,10);
    msg_t msg;

    /*Desactivation interuption == eviter les cliques multiple*/
    gpio_irq_disable(B_temp);

    msg.content.value= 0;
    msg_send(&msg, p_alarm);
    bt_panic = 0;
    fire = 0;
    
    xtimer_usleep(500);
    gpio_irq_enable(B_temp); 
}

static void bt_emergency(void *arg){
    printf("Pressed Emergency%d\n", (int)arg);
    gpio_t B_temp= GPIO_PIN(PORT_B ,14);
    msg_t msg;

    /*Desactivation interuption == eviter les cliques multiple*/
    gpio_irq_disable(B_temp);

    msg.content.value= 1;
    msg_send(&msg, p_alarm);
    bt_panic = 1;
    
    xtimer_usleep(500);
    gpio_irq_enable(B_temp); 
}

int main(void){
    /**/
    xtimer_ticks32_t time_main = xtimer_now();

    /*Déclaration des boutons*/
    gpio_t B_user = GPIO_PIN(PORT_A ,10);
    if(gpio_init_int (B_user, GPIO_IN_PU, GPIO_RISING , bt_user,(void*)0) < 0){
        puts("[FAILED] init BTN!");
        erreur = true;
        return 1;
    }
    gpio_t B_emergency = GPIO_PIN(PORT_B ,14);
    if(gpio_init_int (B_emergency, GPIO_IN_PU, GPIO_RISING , bt_emergency,(void*)0) < 0){
        puts("[FAILED] init BTN!");
        erreur = true;
        return 1;
    }

    /*intitalisation bme */
    switch (bmx280_init(&my_bmp, &bmx280_params[0])) {
        case BMX280_ERR_BUS:
            puts("[Error] Something went wrong when using the I2C bus");
            erreur = true;
            return 1;
        case BMX280_ERR_NODEV:
            puts("[Error] Unable to communicate with any BMX280 device");
            erreur = true;
            return 1;
        default:
            /* all good -> do nothing */
            break;
    }

    /*Création des thread*/
    p_alarm = thread_create(alarm_stack, sizeof(alarm_stack), THREAD_PRIORITY_MAIN - 1, 0, alarm_thread, NULL, "alarm thread");
    p_led= thread_create(led_stack, sizeof(led_stack), THREAD_PRIORITY_MAIN - 1, 0, flashing_thread, NULL, "led thread");
    p_bmx= thread_create(bmx_stack, sizeof(bmx_stack), THREAD_PRIORITY_MAIN - 1, 0, monitor_bmx_thread, NULL, "bmx thread");
    

    /*Déclaration est initialisation de la variable message*/

    while(1){
        xtimer_periodic_wakeup(&time_main, DELAY_led_wait);
    }

    return 0;
}


/*
if(val_led == 3){
        val_led = 0;
    }
    else{
        val_led = 1+val_led;
    }
    
    msg_t msg1;
    if(val_led == 0){
        printf("Cas eteints\n");
        msg1.content.value = 0;
    }
    else if(val_led ==1){
        printf("Cas clignontement\n");
        msg1.content.value = 1;
    }
    else if(val_led ==2){
        printf("Cas fixe\n");
        msg1.content.value = 2;
    }
    else{
        printf("ne sait pas\n");
    }
    msg_send(&msg1, p_led);


*/