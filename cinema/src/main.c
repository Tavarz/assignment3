/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <kernel.h>

#define SLEEP_TIME_MS 60*1000

#define MAX_SESSIONS 3
#define MAX_MOVIES 2
#define MOVIE_A 0
#define MOVIE_B 1
#define MENU 2
#define h_19 0
#define h_21 1
#define h_23 2

const uint8_t buttons_pins[] = {11,12,24,25,3,4,28,29}; /*Vector with pins where buttons are connected*/
/* Get node ID for GPI0, which has buttons*/
#define GPIO0_NODE DT_NODELABEL(gpio0)
#define LED1_PIN 13

/* Now get the device pointer for GPIO0 */
static const struct device * gpio0_dev = DEVICE_DT_GET(GPIO0_NODE);

/* It defines which pin triggers the callback and the address of the function */
static struct gpio_callback button_cb_data;

volatile int But1 = 0;      // UP
volatile int But2 = 0;      //DOWN
volatile int But3 = 0;      //SELECT
volatile int But4 = 0;      //RETURN
volatile int But5 = 0;      //1 euro
volatile int But6 = 0;      //2 euros
volatile int But7 = 0;      //5 euros
volatile int But8 = 0;      //10 euros

void reset_Buttons(void) {
    But1 = 0;
    But2 = 0;
    But3 = 0;
    But4 = 0;
    But5 = 0;
    But6 = 0;
    But7 = 0;
    But8 = 0;
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    
	/*int i = 0;
	reset_Buttons();
	for(i = 0; i < sizeof(buttons_pins); i++){

		if(BIT(buttons_pins[i]) & pins){

			Buttons[i] = BIT(buttons_pins[i]) & pins; //Teriamos que usar um array de butoes
													  // em vez do que temos nas linhas 40
		}

	}*/

	int i=0;

    /* Toggle led1 */
	gpio_pin_toggle(gpio0_dev,LED1_PIN);

	/* Identify the button(s) that was(ere) hit*/
	for(i=0; i<sizeof(buttons_pins); i++){		
		if(BIT(buttons_pins[i]) & pins) {
			printk("Button %d pressed\n\r",i+1);
		}
	}

}

void config(void) {
    
	int ret, i;
	uint32_t pinmask = 0; /* Mask for setting the pins that shall generate interrupts */
	
	/* Welcome message */
	printk("Digital IO accessing IO pins not set via DT (external buttons in the case) \n\r");
	printk("Hit buttons 1-8 (1...4 internal, 5-8 external connected to A0...A3). Led toggles and button ID printed at console \n\r");

	/* Check if gpio0 device is ready */
	if (!device_is_ready(gpio0_dev)) {
		printk("Error: gpio0 device is not ready\n");
		return;
	} else {
		printk("Success: gpio0 device is ready\n");
	}

    /* Configure the GPIO pins - LED1 for output and buttons 1-4 + IOPINS 2,4,28 and 29 for input
	 * Use internal pull-up to avoid the need for an external resistor (buttons) */
	ret = gpio_pin_configure(gpio0_dev,LED1_PIN, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: gpio_pin_configure failed for led1, error:%d\n\r", ret);
		return;
	}

	for(i=0; i<sizeof(buttons_pins); i++) {
		ret = gpio_pin_configure(gpio0_dev, buttons_pins[i], GPIO_INPUT | GPIO_PULL_UP);
		if (ret < 0) {
			printk("Error: gpio_pin_configure failed for button %d/pin %d, error:%d\n\r", i+1,buttons_pins[i], ret);
			return;
		} else {
			printk("Success: gpio_pin_configure for button %d/pin %d\n\r", i+1,buttons_pins[i]);
		}
	}

	/* Configure the interrupt on the button's pin */
	for(i=0; i<sizeof(buttons_pins); i++) {
		ret = gpio_pin_interrupt_configure(gpio0_dev, buttons_pins[i], GPIO_INT_EDGE_TO_ACTIVE );
		if (ret < 0) {
			printk("Error: gpio_pin_interrupt_configure failed for button %d / pin %d, error:%d", i+1, buttons_pins[i], ret);
			return;
		}
	}

    /* HW init done!*/
	printk("All devices initialized sucesfully!\n\r");

	/* Initialize the static struct gpio_callback variable   */
	pinmask=0;
	for(i=0; i<sizeof(buttons_pins); i++) {
		pinmask |= BIT(buttons_pins[i]);
	}
    gpio_init_callback(&button_cb_data, button_pressed, pinmask); 	
	
	/* Add the callback function by calling gpio_add_callback()   */
	gpio_add_callback(gpio0_dev, &button_cb_data);

	/* 
 	 * The main loop
 	 */ 
	while (1) {
		/* Just sleep. Led on/off is done by the int callback */
		k_msleep(SLEEP_TIME_MS);
	}

}

void StateMachine(void) {
    struct session {
        int horas;
        int custo;
    };

    struct movie {
        int n_sessions;
        struct session sessions[MAX_SESSIONS];
    };

    //struct movie movies[MAX_MOVIES];

    struct session movie_a[] = {
        {19,9},
        {21,11},
        {23,9}
    };

    struct session movie_b[] = {
        {19,10},
        {21,12}
    };

    struct movie movies[] = {
        {3,movie_a},
        {2,movie_b}
    };
    
    int state = MENU;
    int saldo = 0;
    int select = 0;


    while(1) {
        switch(state){
            case MENU:
                if(select == 0) {   //menu filme A selecionado
                    printf("------------------------Cinema 3000------------------------\n\n\r -> Filme A\n\n\r    Filme B\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(select == 1) {   //menu filme B selecionado
                    printf("------------------------Cinema 3000------------------------\n\n\r    Filme A\n\n\r -> Filme B\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(But1) {          //UP mudar select
                    if(select == 1) {
                        select--;
                    }
                    reset_Buttons();
                }
                if(But2) {          //DOWN mudar select
                    if(select == 0) {
                        select++;
                    }
                    reset_Buttons();
                }
                if(But3) {          //Select
                    if(select == 0){
                        state = MOVIE_A;
                    }else{
                        state = MOVIE_B;
                    }
                    reset_Buttons();
                }
                if(But4) {          //Return 
                    printf("%d euros devolvidos",saldo);
                    saldo = 0;
                    reset_Buttons();
                }
                if(But5) {          //1 euro
                    saldo++;
                    reset_Buttons();
                }
                if(But6) {          //2 euros
                    saldo += 2;
                    reset_Buttons();
                }
                if(But7) {          //5 euros
                    saldo += 5;
                    reset_Buttons();
                }
                if(But8) {          //10 euros
                    saldo += 10;
                    reset_Buttons();
                }

            break;

            case MOVIE_A:
            select = 0;
                if(select == 0) {   //menu filme A selecionado
                    printf("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao : -> 19 horas\n\n\r                21 horas\n\n\r                23 horas\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(select == 1) {   //menu filme B selecionado
                    printf("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao :    19 horas\n\n\r             -> 21 horas\n\n\r                23 horas\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(select == 2) {
                    printf("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao :    19 horas\n\n\r                21 horas\n\n\r             -> 23 horas\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(select == 3) {
                    printf("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao :    19 horas\n\n\r                21 horas\n\n\r                23 horas\n\n\r             -> Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(But1) {          //UP mudar select
                    if((select == 1) || (select == 2) || (select == 3)) {
                        select--;
                    }
                    reset_Buttons();
                }
                if(But2) {          //DOWN mudar select
                    if((select == 0) || (select == 1) || (select == 2))  {
                        select++;
                    }
                    reset_Buttons();
                }
                if(But3) {          //Select
                    if(select == 0){
                        if(saldo >= movies[MOVIE_A].sessions[h_19].custo){
                            saldo -= movies[MOVIE_A].sessions[h_19].custo;
                            state = MENU;
                            printf("Bilhete comprado para Filme A as %d.\n\rSaldo:%d\n\n\r",movies[MOVIE_A].sessions[h_19].horas, saldo);
                        }else{
                            printf("Saldo insuficiente. Inserir %d euros\n\n\r",(movies[MOVIE_A].sessions[h_19].custo-saldo));
                        }
                    }
                    if(select == 1){
                        if(saldo >= movies[MOVIE_A].sessions[h_21].custo){
                            saldo -= movies[MOVIE_A].sessions[h_21].custo;
                            state = MENU;
                            printf("Bilhete comprado para Filme A as %d.\n\rSaldo:%d\n\n\r",movies[MOVIE_A].sessions[h_21].horas, saldo);
                        }else{
                            printf("Saldo insuficiente. Inserir %d euros\n\n\r",(movies[MOVIE_A].sessions[h_21].custo-saldo));
                        }
                    }
                    if(select == 2){
                        if(saldo >= movies[MOVIE_A].sessions[h_23].custo){
                            saldo -= movies[MOVIE_A].sessions[h_23].custo;
                            state = MENU;
                            printf("Bilhete comprado para Filme A as %d.\n\rSaldo:%d\n\n\r",movies[MOVIE_A].sessions[h_23].horas, saldo);
                        }else{
                            printf("Saldo insuficiente. Inserir %d euros\n\n\r",(movies[MOVIE_A].sessions[h_23].custo-saldo));
                        }
                    }
                    if(select == 3){
                        state = MENU;
                    }
                    reset_Buttons();
                }
                if(But4) {          //Return 
                    printf("%d euros devolvidos",saldo);
                    saldo = 0;
                    reset_Buttons();
                }
                if(But5) {          //1 euro
                    saldo++;
                    reset_Buttons();
                }
                if(But6) {          //2 euros
                    saldo += 2;
                    reset_Buttons();
                }
                if(But7) {          //5 euros
                    saldo += 5;
                    reset_Buttons();
                }
                if(But8) {          //10 euros
                    saldo += 10;
                    reset_Buttons();
                }
            break;

            case MOVIE_B:
            select = 0;
                if(select == 0) {   //menu filme A selecionado
                    printf("------------------------Cinema 3000------------------------\n\n\r  Filme B\n\n\r    Sessao : -> 19 horas\n\n\r                21 horas\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(select == 1) {   //menu filme B selecionado
                    printf("------------------------Cinema 3000------------------------\n\n\r  Filme B\n\n\r    Sessao :    19 horas\n\n\r             -> 21 horas\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(select == 2) {
                    printf("------------------------Cinema 3000------------------------\n\n\r  Filme B\n\n\r    Sessao :    19 horas\n\n\r                21 horas\n\n\r             -> Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(But1) {          //UP mudar select
                    if((select == 1) || (select == 2)) {
                        select--;
                    }
                    reset_Buttons();
                }
                if(But2) {          //DOWN mudar select
                    if((select == 0) || (select == 1))  {
                        select++;
                    }
                    reset_Buttons();
                }
                if(But3) {          //Select
                    if(select == 0){
                        if(saldo >= movies[MOVIE_A].sessions[h_19].custo){
                            saldo -= movies[MOVIE_A].sessions[h_19].custo;
                            state = MENU;
                            printf("Bilhete comprado para Filme B as %d.\n\rSaldo:%d\n\n\r",movies[MOVIE_B].sessions[h_19].horas, saldo);
                        }else{
                            printf("Saldo insuficiente. Inserir %d euros\n\n\r",(movies[MOVIE_B].sessions[h_19].custo-saldo));
                        }
                    }
                    if(select == 1){
                        if(saldo >= movies[MOVIE_A].sessions[h_21].custo){
                            saldo -= movies[MOVIE_A].sessions[h_21].custo;
                            state = MENU;
                            printf("Bilhete comprado para Filme B as %d.\n\rSaldo:%d\n\n\r",movies[MOVIE_B].sessions[h_21].horas, saldo);
                        }else{
                            printf("Saldo insuficiente. Inserir %d euros\n\n\r",(movies[MOVIE_B].sessions[h_21].custo-saldo));
                        }
                    }
                    if(select == 2) {
                        state = MENU;
                    }
                    reset_Buttons();
                }
                if(But4) {          //Return 
                    printf("%d euros devolvidos",saldo);
                    saldo = 0;
                    reset_Buttons();
                }
                if(But5) {          //1 euro
                    saldo++;
                    reset_Buttons();
                }
                if(But6) {          //2 euros
                    saldo += 2;
                    reset_Buttons();
                }
                if(But7) {          //5 euros
                    saldo += 5;
                    reset_Buttons();
                }
                if(But8) {          //10 euros
                    saldo += 10;
                    reset_Buttons();
                }
            break;

            default:
            break;
        }
    }
}

int main(void) {

    StateMachine();
    return 0;
}
