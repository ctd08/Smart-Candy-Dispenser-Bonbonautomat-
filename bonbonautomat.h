#ifndef BONBONAUTOMAT_H
#define BONBONAUTOMAT_H

#include "Lib/bml_gpio.h"
#include "Lib/bml_timer3.h"
#include "Lib/bml_systick.h"
#include "Lib/bml_adc.h"

// Servo-Parameter
#define SERVO_CHANNEL     TIM3_CH1
#define SERVO_MIN_US      1225   // eig 1220
#define SERVO_MAX_US      1725   // eig 1730
#define SERVO_STEPS       100
#define SERVO_DELAY_MS    10

// Taster
#define TASTER_LOW_ACTIVE 1

// ADC
#define ADC_CHANNEL       1
#define FUELL_SCHWELLE    1700
#define MAX_AUSWURF_BEI_LEER 3

// Initialisierung und Hauptlogik
void bonbonautomat_init(void);
void bonbonautomat_loop(void);

// Aktionen
void bonbon_auswurf(void);
void servo_smooth_to(uint16_t from, uint16_t to, uint16_t steps, uint16_t delay);
void delay_ms(unsigned int ms);

// LED-Anzeige (falls du's noch brauchst)
void led_set_blau(void);
void led_set_rot(void);
void led_set_gruen(void);

#endif // BONBONAUTOMAT_H