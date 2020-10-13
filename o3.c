#include "o3.h"
#include "gpio.h"
#include "systick.h"
#include "math.h"

/**************************************************************************//**
 * @brief Konverterer nummer til string 
 * Konverterer et nummer mellom 0 og 99 til string
 *****************************************************************************/
void int_to_string(char *timestamp, unsigned int offset, int i) {
    if (i > 99) {
        timestamp[offset]   = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
	    if (i >= 10) {
		    i -= 10;
		    timestamp[offset]++;
		
	    } else {
		    timestamp[offset+1] = '0' + i;
		    i=0;
	    }
    }
}

/**************************************************************************//**
 * @brief Konverterer 3 tall til en timestamp-string
 * timestamp-argumentet mÃ¥ vÃ¦re et array med plass til (minst) 7 elementer.
 * Det kan deklareres i funksjonen som kaller som "char timestamp[7];"
 * Kallet blir dermed:
 * char timestamp[7];
 * time_to_string(timestamp, h, m, s);
 *****************************************************************************/
void time_to_string(char *timestamp, int h, int m, int s) {
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, h);
    int_to_string(timestamp, 2, m);
    int_to_string(timestamp, 4, s);
}

//---- Memory mapping ----
typedef struct {
	volatile word CTRL;
	volatile word MODEL;
	volatile word MODEH;
	volatile word DOUT;
	volatile word DOUTSET;
	volatile word DOUTCLR;
	volatile word DOUTTGL;
	volatile word DIN;
	volatile word PINLOCKN;
} gpio_port_map_t;

typedef struct {
	volatile gpio_port_map_t ports[6];
	volatile word unused_space[10];
	volatile word EXTIPSELL;
	volatile word EXTIPSELH;
	volatile word EXTIRISE;
	volatile word EXTIFALL;
	volatile word IEN;
	volatile word IF;
	volatile word IFS;
	volatile word IFC;
	volatile word ROUTE;
	volatile word INSENSE;
	volatile word LOCK;
	volatile word CTRL;
	volatile word CMD;
	volatile word EM4WUEN;
	volatile word EM4WUPOL;
	volatile word EM4WUCAUSE;
} gpio_map_t;

volatile gpio_map_t* gpio;

typedef struct {
	volatile word CTRL;
	volatile word LOAD;
	volatile word VAL;
	volatile word CALIB;
} systick_map_t;

volatile systick_map_t* systick;

//---- Ports & pins ----
port_pin_t led = {.port = GPIO_PORT_E, .pin = 2};
port_pin_t PB0 = {.port = GPIO_PORT_B, .pin = 9};
port_pin_t PB1 = {.port = GPIO_PORT_B, .pin = 10};

//---- Time and state globals ----
int state;
int secs;
int mins;
int hrs;

//---- Mask functions ----
word pos_bit_msk(int bitnr){
	word msk = 1;
	msk = msk << bitnr;
	return msk;
}

word bit_on(int bitnr, word orig){
	word msk = pos_bit_msk(bitnr);
	return orig | msk;
}

word bits_on(int bitnr_start, int bitnr_end, word orig){
	if(bitnr_start < bitnr_end){
		return bit_on(bitnr_start, orig) | bits_on(bitnr_start+1, bitnr_end, orig);
	}
	return bit_on(bitnr_start, orig);
}

word neg_bit_msk(int bitnr){
	word msk = 1;
	msk = msk << bitnr;
	return ~msk;
}

word bit_off(int bitnr, word orig){
	word msk = neg_bit_msk(bitnr);
	return orig & msk;
}

word bits_off(int bitnr_start, int bitnr_end, word orig){
	if(bitnr_start < bitnr_end){
		return bit_off(bitnr_start, orig) & bits_off(bitnr_start+1, bitnr_end, orig);
	}
	return bit_off(bitnr_start, orig);
}


// ---- Leds ----
void ledOn(){
	gpio->ports[led.port].DOUTSET = pos_bit_msk(led.pin);
}

void ledOff(){
	gpio->ports[led.port].DOUTCLR = pos_bit_msk(led.pin);
}

// ---- Countdown ----
//Functions
void print_time(){
	char str[7];
	time_to_string(str, hrs, mins, secs);
	lcd_write(str);
}

void alarm(){
	ledOn();
	state++;
}

void start_countdown(){
	systick->CTRL = bit_on(0, systick->CTRL);
}

void stop_countdown(){
	systick->CTRL = bit_off(0, systick->CTRL);
}

void reset_countdown(){
	secs = 0;
	mins = 0;
	hrs = 0;
}

void decrement_hrs(){
	if(hrs == 0){
		mins = 0;
	}
	else{
		secs = 59;
		hrs--;
	}
}

void decrement_mins(){
	if(mins == 0){
		secs = 0;
		mins = 59;
		decrement_hrs();
	}
	else{
		mins--;
	}
}

void decrement_secs(){
	if(secs == 1){
		secs--;
		if(mins == 0 && hrs == 0){
			stop_countdown();
			alarm();
		}
	}
	else if(secs == 0){
		secs = 59;
		decrement_mins();
	}
	else{
		secs--;
	}
	print_time();
}

// Interruption handler
void SysTick_Handler(){
	decrement_secs();
}


// ---- Buttons ----
// Functions
void clear_IF(port_pin_t button){
	gpio->IFC = pos_bit_msk(button.pin);
}

// Interruption handlers
void GPIO_ODD_IRQHandler(){
	switch(state){
	case 0:
		secs++;
		break;
	case 1:
		mins++;
		break;
	case 2:
		hrs++;
	}
	print_time();
	clear_IF(PB0);
}

void GPIO_EVEN_IRQHandler(){
	switch(state){
	case 0:
	case 1:
		state++;
		break;
	case 2:
		state++;
		start_countdown();
		break;
	case 4:
		reset_countdown();
		ledOff();
		state = 0;
		break;
	}
	clear_IF(PB1);
}


int main(void) {
    init();
    gpio = (gpio_map_t*) GPIO_BASE;
    systick = (systick_map_t*) SYSTICK_BASE;
    state = 0;
    secs = 0;
    mins = 0;
    hrs = 0;
    print_time();

    // ---- Systick setup ----
    systick->CTRL = bits_on(1, 2, systick->CTRL);
    stop_countdown();
    systick->LOAD = FREQUENCY;

    // ---- Led setup ----
    gpio->ports[led.port].MODEL = bits_off(8, 11, gpio->ports[led.port].MODEL);
    gpio->ports[led.port].MODEL = gpio->ports[led.port].MODEL | (GPIO_MODE_OUTPUT << 8);

    // ---- Button interruption setup ----
    // Set MODE
    gpio->ports[PB0.port].MODEH = bits_off(4, 7, gpio->ports[PB0.port].MODEH);
    gpio->ports[PB0.port].MODEH = gpio->ports[PB0.port].MODEH | (GPIO_MODE_INPUT << 4);
    gpio->ports[PB0.port].MODEH = bits_off(8, 11, gpio->ports[PB1.port].MODEH);
    gpio->ports[PB0.port].MODEH = gpio->ports[PB1.port].MODEH | (GPIO_MODE_INPUT << 8);

    // Set EXTIPSEL
    gpio->EXTIPSELH = bits_off((PB0.pin-8)*4, (PB0.pin-7)*4-1, gpio->EXTIPSELH);
    gpio->EXTIPSELH = gpio->EXTIPSELH | (PB0.port << 4);
    gpio->EXTIPSELH = bits_off((PB1.pin-8)*4, (PB1.pin-7)*4-1, gpio->EXTIPSELH);
    gpio->EXTIPSELH = gpio->EXTIPSELH | (PB1.port << 8);

    // Set EXTIFALL
    gpio->EXTIFALL = bit_on(PB0.pin, gpio->EXTIFALL);
    gpio->EXTIFALL = bit_on(PB1.pin, gpio->EXTIFALL);

    // Clear IF
    clear_IF(PB0);
    clear_IF(PB1);

    // Set IEN
    gpio->IEN = bit_on(PB0.pin, gpio->IEN);
    gpio->IEN = bit_on(PB1.pin, gpio->IEN);


    // ---- Endless loop to keep it running ----
    while(1){};

    return 0;
}
