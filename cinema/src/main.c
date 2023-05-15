/** @file main.c
 * @brief main.c file brief decription 
 *
 * Program that emulates a ticket vending machine for a cinema
 * 
 * @author Bernardo Tavares bernardot@ua.pt and João Rodrigues jpcr@ua.pt
 * @date 15 May 2023
 * @bug No known bugs.
 */

/* Includes */
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

/* Defines */
#define SLEEP_TIME_MS 300
#define MAX_SESSIONS 3  // Maximum number of session for one movie
#define MOVIE_A 0       // Movie A state
#define MOVIE_B 1       // Movie B state
#define MENU 2          // Menu state
#define h_19 0          
#define h_21 1
#define h_23 2

/* Get node ID for GPI0, which has buttons*/
#define GPIO0_NODE DT_NODELABEL(gpio0)
#define LED1_PIN 13

const uint8_t buttons_pins[] = {11,12,24,25,3,4,28,29}; /*Vector with pins where buttons are connected*/

/* Now get the device pointer for GPIO0 */
static const struct device * gpio0_dev = DEVICE_DT_GET(GPIO0_NODE);

/* It defines which pin triggers the callback and the address of the function */
static struct gpio_callback button_cb_data;

/* Variables to use when a button is pressed */
volatile int But1 = 0;      // UP
volatile int But2 = 0;      //DOWN
volatile int But3 = 0;      //SELECT
volatile int But4 = 0;      //RETURN
volatile int But5 = 0;      //1 euro
volatile int But6 = 0;      //2 euros
volatile int But7 = 0;      //5 euros
volatile int But8 = 0;      //10 euros

/* Resets all the button states to not pressed */
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
/* Interrupt function to detect if button is pressed and determine what button was pressed. LED1 switches state when a button is pressed */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	int i=0;

    /* Toggle led1 */
	gpio_pin_toggle(gpio0_dev,LED1_PIN);

	/* Identify the button(s) that was(ere) hit*/
	for(i=0; i<sizeof(buttons_pins); i++){		
		if(BIT(buttons_pins[i]) & pins) {
			//printk("Button %d pressed\n\r",i+1);
            switch(i){
                case(0):
                    But1 = 1;       //Botao Up
                break;

                case(1):
                    But2 = 1;       //Botao Down
                break;

                case(2):
                    But3 = 1;       //Botao Select
                break;

                case(3):
                    But4 = 1;       //Botao Return
                break;

                case(4):
                    But5 = 1;       // 1 euro
                break;

                case(5):
                    But6 = 1;       //2 euros
                break;

                case(6):
                    But7 = 1;       //5 euros
                break;

                case(7):
                    But8 = 1;       //10 euros
                break;

                default:
                break;
            }
		}
	}

}

/* Function to configure the buttons and the interruptions for the same. Configures also LED1*/
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

}
/* Function wich handles the state machine and all the events/actions that happen inside it */
void StateMachine(void) {
    /* Structure to define hours and price for each session */
    struct session {
        int horas;
        int custo;
    };

    struct session movie_a[] = {
        {19,9},
        {21,11},
        {23,9}
    };

    struct session movie_b[] = {
        {19,10},
        {21,12}
    };
    
    int state = MENU;
    int saldo = 0;
    int select = 0;


    while(1) {
        k_msleep(SLEEP_TIME_MS);
        switch(state){
            case MENU:
                printk("\033[2J\033[H");
                if(select == 0) {   //menu filme A selecionado
                    printk("------------------------Cinema 3000------------------------\n\n\r -> Filme A\n\n\r    Filme B\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                if(select == 1) {   //menu filme B selecionado
                    printk("------------------------Cinema 3000------------------------\n\n\r    Filme A\n\n\r -> Filme B\n\n\r Saldo:%d euros\n\n\n\r",saldo);
                }
                                
                if(But1) {          //UP mudar select
                    if(select == 1) {
                        select=0;
                    }
                    reset_Buttons();
                }
                if(But2) {          //DOWN mudar select
                    if(select == 0) {
                        select=1;
                    }
                    reset_Buttons();
                }
                if(But3) {          //Select
                    if(select == 0){
                        state = MOVIE_A;
                        select = 0;
                    }else{
                        state = MOVIE_B;
                        select = 0;
                    }
                    reset_Buttons();
                }
                if(But4) {          //Return 
                    printk("%d euros devolvidos",saldo);
                    saldo = 0;
                    reset_Buttons();
                    k_msleep(SLEEP_TIME_MS*3);
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
                printk("\033[2J\033[H");
                if(select == 0) {   //movie A sessao 19 horas
                    printk("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao : -> 19 horas  %d euros\n\n\r                21 horas  %d euros\n\n\r                23 horas  %d euros\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",movie_a[h_19].custo,movie_a[h_21].custo,movie_a[h_23].custo,saldo);
                }
                if(select == 1) {   //movie A sessao 21 horas
                    printk("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao :    19 horas  %d euros\n\n\r             -> 21 horas  %d euros\n\n\r                23 horas  %d euros\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",movie_a[h_19].custo,movie_a[h_21].custo,movie_a[h_23].custo,saldo);
                }
                if(select == 2) {   //movie A sessao 23 horas
                    printk("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao :    19 horas  %d euros\n\n\r                21 horas  %d euros\n\n\r             -> 23 horas  %d euros\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",movie_a[h_19].custo,movie_a[h_21].custo,movie_a[h_23].custo,saldo);
                }
                if(select == 3) {   //movie A voltar atras
                    printk("------------------------Cinema 3000------------------------\n\n\r  Filme A\n\n\r    Sessao :    19 horas  %d euros\n\n\r                21 horas  %d euros\n\n\r                23 horas  %d euros\n\n\r             -> Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",movie_a[h_19].custo,movie_a[h_21].custo,movie_a[h_23].custo,saldo);
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
                        if(saldo >= movie_a[h_19].custo){
                            saldo -= movie_a[h_19].custo;
                            state = 0;
                            state = MENU;
                            printk("Bilhete comprado para Filme A as %d horas.\n\rSaldo:%d\n\n\r",movie_a[h_19].horas, saldo);
                        }else{
                            printk("Saldo insuficiente. Inserir %d euros\n\n\r",(movie_a[h_19].custo-saldo));
                        }
                        k_msleep(SLEEP_TIME_MS*3);
                    }
                    if(select == 1){
                        if(saldo >= movie_a[h_21].custo){
                            saldo -= movie_a[h_21].custo;
                            select = 0;
                            state = MENU;
                            printk("Bilhete comprado para Filme A as %d horas.\n\rSaldo:%d\n\n\r",movie_a[h_21].horas, saldo);
                        }else{
                            printk("Saldo insuficiente. Inserir %d euros\n\n\r",(movie_a[h_21].custo-saldo));
                        }
                        k_msleep(SLEEP_TIME_MS*3);
                    }
                    if(select == 2){
                        if(saldo >= movie_a[h_23].custo){
                            saldo -= movie_a[h_23].custo;
                            select = 0;
                            state = MENU;
                            printk("Bilhete comprado para Filme A as %d horas.\n\rSaldo:%d\n\n\r",movie_a[h_23].horas, saldo);
                        }else{
                            printk("Saldo insuficiente. Inserir %d euros\n\n\r",(movie_a[h_23].custo-saldo));
                        }
                        k_msleep(SLEEP_TIME_MS*3);
                    }
                    if(select == 3){
                        select = 0;
                        state = MENU;
                    }
                    reset_Buttons();
                }
                if(But4) {          //Return 
                    printk("%d euros devolvidos",saldo);
                    saldo = 0;
                    reset_Buttons();
                    k_msleep(SLEEP_TIME_MS*3);
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
                
                printk("\033[2J\033[H");
                if(select == 0) {   //movie B sessao 19 horas
                    printk("------------------------Cinema 3000------------------------\n\n\r  Filme B\n\n\r    Sessao : -> 19 horas  %d euros\n\n\r                21 horas  %d euros\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",movie_b[h_19].custo,movie_b[h_21].custo,saldo);
                }
                if(select == 1) {   //movie B sessao 21 horas
                    printk("------------------------Cinema 3000------------------------\n\n\r  Filme B\n\n\r    Sessao :    19 horas  %d euros\n\n\r             -> 21 horas  %d euros\n\n\r                Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",movie_b[h_19].custo,movie_b[h_21].custo,saldo);
                }
                if(select == 2) {   //movie B voltar
                    printk("------------------------Cinema 3000------------------------\n\n\r  Filme B\n\n\r    Sessao :    19 horas  %d euros\n\n\r                21 horas  %d euros\n\n\r             -> Voltar atras\n\n\r Saldo:%d euros\n\n\n\r",movie_b[h_19].custo,movie_b[h_21].custo,saldo);
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
                        if(saldo >= movie_b[h_19].custo){
                            saldo -= movie_b[h_19].custo;
                            select = 0;
                            state = MENU;
                            printk("Bilhete comprado para Filme B as %d horas.\n\rSaldo:%d\n\n\r",movie_b[h_19].horas, saldo);
                        }else{
                            printk("Saldo insuficiente. Inserir %d euros\n\n\r",(movie_b[h_19].custo-saldo));
                        }
                        k_msleep(SLEEP_TIME_MS*3);
                    }
                    if(select == 1){
                        if(saldo >= movie_b[h_21].custo){
                            saldo -= movie_b[h_21].custo;
                            select = 0;
                            state = MENU;
                            printk("Bilhete comprado para Filme B as %d horas.\n\rSaldo:%d\n\n\r",movie_b[h_21].horas, saldo);
                        }else{
                            printk("Saldo insuficiente. Inserir %d euros\n\n\r",(movie_b[h_21].custo-saldo));
                        }
                        k_msleep(SLEEP_TIME_MS*3);
                    }
                    if(select == 2) {
                        select = 0;
                        state = MENU;
                    }
                    reset_Buttons();
                }
                if(But4) {          //Return 
                    printk("%d euros devolvidos",saldo);
                    saldo = 0;
                    reset_Buttons();
                    k_msleep(SLEEP_TIME_MS*3);
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
    config();
    k_msleep(SLEEP_TIME_MS*10);
    StateMachine();
    return 0;
}
