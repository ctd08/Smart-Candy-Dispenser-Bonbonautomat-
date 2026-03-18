#include "bonbonautomat.h"
#include "Lib/bml_gpio.h"
#include "Lib/bml_timer3.h"
#include "Lib/bml_systick.h"
#include "Lib/bml_adc.h"
#include <stdbool.h>

static enum {
    STATE_IDLE,
    STATE_MOVING,
    STATE_COOLDOWN,
    STATE_LOCKED_EMPTY
} state = STATE_IDLE;

static int auswurf_counter = 0;
static volatile bool fuellstand_ok = true;

// === Initialisierung ===
void bonbonautomat_init(void) {
    // Servo
    bmlTim3InitBase(1, TIM3_UNIT_MHZ, 50, TIM3_UNIT_HZ);
    bmlTim3InitChannel(PA6, 0, TIM3_POLARITY_HIGH);
    bmlTim3SetTon(TIM3_CH1, SERVO_MIN_US, TIM3_UNIT_US);  //for safety reason


    // Taster
    bmlGpioInitSimple(PC13, GPIO_MODE_INPUT);

    // ADC
    bmlGpioInitSimple(PA1, GPIO_MODE_ANALOG);
    ADC_EasyInit(1, 1);

    //Pins für LEDs
    
    bmlTim3InitChannel(PA7, 0, TIM3_POLARITY_LOW); // Rot
    bmlTim3InitChannel(PC8, 0, TIM3_POLARITY_LOW); // Grün
    bmlTim3InitChannel(PC9, 0, TIM3_POLARITY_LOW); // Blau

    bmlTim3SetDutyCycle(TIM3_CH2, 2, DUTY_PERCENT);    //DAS IS hellBLAU
    bmlTim3SetDutyCycle(TIM3_CH3, 2, DUTY_PERCENT);  //DAS IS ROT (pink)
    bmlTim3SetDutyCycle(TIM3_CH4, 2, DUTY_PERCENT);    // gelbgrün
    
}

// === Hilfsfunktionen ===
void delay_ms(unsigned int ms) {
    bmlSystickInit(ms, SYSTICK_UNIT_MS, SYSTICK_IRQ_DISABLE);
    while (!bmlSystickGetCountFlag());
}


void servo_smooth_to(uint16_t from, uint16_t to, uint16_t steps, uint16_t delay) {     //pusht münze und bonbon mit bestimmter anzahl von steps langsam raus
    int step = (to - from) / (int)steps;   //1,73 - 1,22
    /*
    for (uint16_t i = 0; i < steps; ++i) {
        bmlTim3SetTon(TIM3_CH1, from - i * step, TIM3_UNIT_US);
        delay_ms(delay);
    }
    bmlTim3SetTon(TIM3_CH1, to, TIM3_UNIT_US); // Sicherheit: Endposition setzen
    delay_ms(200); // kleine Pause nach dem Auswurf

    // Rückweg: langsam von to nach from
    for (uint16_t i = 0; i < steps; ++i) {
        bmlTim3SetTon(TIM3_CH1, to + i * step, TIM3_UNIT_US);
        delay_ms(delay);
    }
    bmlTim3SetTon(TIM3_CH1, from, TIM3_UNIT_US); // Sicherheit: Endposition zurück*/
    for (uint16_t i = 0; i < steps; ++i) {
        bmlTim3SetTon(TIM3_CH1, from + i * step, TIM3_UNIT_US);
        delay_ms(delay);
    }
    bmlTim3SetTon(TIM3_CH1, to, TIM3_UNIT_US); // Sicherheit: Endposition setzen
    delay_ms(200); // kleine Pause nach dem Auswurf
    for (uint16_t i = 0; i < steps; ++i) {
        bmlTim3SetTon(TIM3_CH1, to - i * step, TIM3_UNIT_US);
        delay_ms(delay);
    }
}

void bonbon_auswurf(void) {                          
    servo_smooth_to(SERVO_MIN_US, SERVO_MAX_US, SERVO_STEPS, SERVO_DELAY_MS);
    delay_ms(200);
    //servo_smooth_to(SERVO_MAX_US, SERVO_MIN_US, SERVO_STEPS, SERVO_DELAY_MS);
    bmlTim3SetTon(TIM3_CH1, SERVO_MIN_US, TIM3_UNIT_US);
}

bool taster_gedrueckt(void) {
    return bmlGpioReadSimple(PC13) == 0;
}

void fuellstand_update(void) {
    ADC_SingleConversion(ADC1, 0);
    delay_ms(1);
    volatile uint16_t wert = ADC_Read(ADC1, 0);

    fuellstand_ok = (wert > FUELL_SCHWELLE);      //je höherer wert desto weniger bonbons
}

void led_set_blau(void) {
    bmlTim3SetDutyCycle(TIM3_CH2, 0, DUTY_PERCENT);     
    bmlTim3SetDutyCycle(TIM3_CH3, 0, DUTY_PERCENT);     
    bmlTim3SetDutyCycle(TIM3_CH4, 20, DUTY_PERCENT);   
}

void led_set_rot(void) {
    bmlTim3SetDutyCycle(TIM3_CH2, 20, DUTY_PERCENT);    //DAS IS hellBLAU
    bmlTim3SetDutyCycle(TIM3_CH3, 0, DUTY_PERCENT);  //DAS IS ROT (pink)
    bmlTim3SetDutyCycle(TIM3_CH4, 0, DUTY_PERCENT);    // gelbgrün
}

void led_set_gruen(void) {
    bmlTim3SetDutyCycle(TIM3_CH2, 0, DUTY_PERCENT);
    bmlTim3SetDutyCycle(TIM3_CH3, 20, DUTY_PERCENT);     //GPIOs aktivieren 
    bmlTim3SetDutyCycle(TIM3_CH4, 0, DUTY_PERCENT);
}

// === Haupt-Logikfunktion ===
void bonbonautomat_loop(void) {
    fuellstand_update();

    // RGB-LED Status setzen
    if (state == STATE_MOVING) {
        led_set_blau();
    } else if (!fuellstand_ok) {
        led_set_rot();
    } else {
        led_set_gruen();
    }

    switch (state) {
        case STATE_IDLE:
            if (!fuellstand_ok && auswurf_counter >= MAX_AUSWURF_BEI_LEER) {    //wenn Sensor low is dann gibts noch 5 Versuche frei um zu benutzen
                state = STATE_LOCKED_EMPTY;
                break;
            }

            
            if (taster_gedrueckt()) {
                state = STATE_MOVING;
            }
            break;

        case STATE_MOVING:
            bonbon_auswurf();
            if (!fuellstand_ok) {
                auswurf_counter++;
            }
            state = STATE_COOLDOWN;                //kannsch direkt in idle gehn wenn bock hast
            break;

        case STATE_COOLDOWN:
            delay_ms(500);
            state = STATE_IDLE;
            break;

        case STATE_LOCKED_EMPTY:
                          //rot erst wenn füllstand zu niedrig
            if (fuellstand_ok) {
                auswurf_counter = 0;
                state = STATE_IDLE;
            }
            break;
    }
}
