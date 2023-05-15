#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/driver/gpio.h>
#include <zephyr/sys/printk.h>      /* for printk()*/

#define SLEEP_TIME_MS 60*1000
#define LED1_NODE DT_NODELABEL(led1)
#define SW0_NODE DT_NODELABEL(button0)
#define SW1_NODE DT_NODELABEL(button1)
#define SW2_NODE DT_NODELABEL(button2)
#define SW3_NODE DT_NODELABEL(button3)  //botoes da placa 1,2,3,4
#define SW4_NODE DT_NODELABEL(button4)  //botoes fora da placa
#define SW5_NODE DT_NODELABEL(button5)
#define SW6_NODE DT_NODELABEL(button6)
#define SW7_NODE DT_NODELABEL(button7)


#define MAX_SESSIONS 3
#define MAX_MOVIES 2
#define MOVIE_A 0
#define MOVIE_B 1
#define MENU 2
#define h_19 0
#define h_21 1
#define h_23 2

volatile int But1 = 0;      // UP
volatile int But2 = 0;      //DOWN
volatile int But3 = 0;      //SELECT
volatile int But4 = 0;      //RETURN
volatile int But5 = 0;      //1 euro
volatile int But6 = 0;      //2 euros
volatile int But7 = 0;      //5 euros
volatile int But8 = 0;      //10 euros

static struct gpio_callback button_cb_data;
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    gpio_pin_toggle_dt(&led1);
}

void config(void) {
    if (!device_is_ready(led1.port)) {
		printk("Error: led1 device %s is not ready\n", led1.port->name);
		return;
	}

	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return;
	}
    int ret = 0;

    /* Configure the GPIO pins - led for output and button for input
	 * Use internal pull-up to avoid the need for an external resitor (button)
	 */
	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: gpio_pin_configure_dt failed for led1, error:%d", ret);
		return;
	}

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		printk("Error: gpio_pin_configure_dt failed for button, error:%d", ret);
		return;
	}

    /* Configure the interrupt on the button's pin */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE );
	if (ret < 0) {
		printk("Error: gpio_pin_interrupt_configure_dt failed for button, error:%d", ret);
		return;
	}

    printk("All devices initialized sucesfully!\n\r");

	/* Initialize the static struct gpio_callback variable   */
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));

    /* Add the callback function by calling gpio_add_callback()   */
	 gpio_add_callback(button.port, &button_cb_data);
}

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


void StateMachine(void) {
    struct session {
        int horas;
        int custo;
    };

    struct movie {
        int n_sessions;
        struct session sessions[MAX_SESSIONS];
    };

    struct movie movies[MAX_MOVIES];

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