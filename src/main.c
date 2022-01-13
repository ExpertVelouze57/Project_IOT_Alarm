#include <string.h>

#include "net/loramac.h"
#include "semtech_loramac.h"

/*Header for sleep*/
#include "xtimer.h"

/*Header for thread and comunication*/
#include "thread.h"
#include "msg.h"

#include "board.h"

/* TODO: Add the cayenne_lpp header here */
#include "cayenne_lpp.h"

#define DELAY           (500000U)

/*Ajout d'un bouton*/

/**/
kernel_pid_t p_led;


/* Allocate the writer stack here */
static char led_stack[THREAD_STACKSIZE_MAIN];
static msg_t led_queue[4];

/*Déclaration of function thread*/
static void *flashing_thread(void *arg){
    (void) arg;
    xtimer_ticks32_t clin = xtimer_now();

    gpio_t led = GPIO_PIN(PORT_B ,5);
    gpio_init( led, GPIO_OUT );

    msg_init_queue(led_queue, 4);

    msg_t msg;
    int temp =0;

    while(1){
    if(msg_try_receive(&msg) != -1){
           temp = msg.content.value;   
        }
           
        if(temp == 1){
            xtimer_periodic_wakeup(&clin, DELAY);      
            gpio_write(led,1);
            xtimer_periodic_wakeup(&clin, DELAY); 
            gpio_write(led,0);
        }
        
        else{
            gpio_write(led,1);
            xtimer_periodic_wakeup(&clin, DELAY); 
        }
    }

    
    return NULL;
}

int main(void){
    /*Déclaration de la LED*/
    gpio_t Led = GPIO_PIN(PORT_B ,5);
    gpio_init( Led, GPIO_OUT );

    /*Création des thread*/
    p_led= thread_create(led_stack, sizeof(led_stack), THREAD_PRIORITY_MAIN - 1, 0, flashing_thread, NULL, "led thread");

    /*Déclaration est initialisation de la variable message*/
    msg_t msg1;
    msg1.content.value = 1;
    msg_send(&msg1, p_led);

    while(1);

    return 0;
}
