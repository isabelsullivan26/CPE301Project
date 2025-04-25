//CPE301 Final Project

//Includes
#include <Arduino.h>
#include <Stepper.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <RTClib.h>

#define WATER_SENSOR_CHANNEL 0
#define WATER_THRESHOLD 200
#define TEMP_THRESHOLD_HIGH 30
#define TEMP_THRESHOLD_LOW 25
#define DHT_PIN 22
#define DHT_TYPE DHT11

#define YELLOW_LED PB0
#define GREEN_LED PB1
#define BLUE_LED PB2
#define RED_LED PB3
#define FAN_PIN PB4
#define START_BUTTON PD3
#define STOP_BUTTON PD2
#define RESET_BUTTON PD1

Stepper ventStepper(2048, 8, 10, 9, 11);
LiquidCrystal lcd(7,6,5,4,3,2);
DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS1307 rtc;

//ADC and UART
void adc_init();
void adc_read(uint8_t channel);
void uart_init();
void uart_print(const char* out);


enum State { DISABLED, RUNNING, ERROR, IDLE }
volatile State current = DISABLED;


//ISR
void startISR(){
    if(current == DISABLED){
        current = IDLE;
    }
}

void stopISR(){
    current = DISABLED;
}

void resetISR(){
    if(current == ERROR && adc_read(WATER_SENSOR_CHANNEL)>=WATER_THRESHOLD){
        current = IDLE;
    }
}

void setup(){
    DDRB |= (1<<YELLOW_LED)|(1<<GREEN_LED)|(1<<BLUE_LED)|(1<<RED_LED)|(1<<FAN_PIN);
    DDRD &= ((1<<START_BUTTON) | (1<<STOP_BUTTON) | (1<<RESET_BUTTON));
    //attachinterrupt
    attachInterrupt(digitalPinToInterrupt(START_BUTTON), startISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(STOP_BUTTON), stopISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetISR, FALLING);
    adc_init();
    uart_init();
    lcd.begin(16,2);
    dht.begin();
    rtc.begin();
    ventStepper.setSpeed(10);

}

void loop(){
    switch(current){
        case DISABLED:
            //
        case IDLE:
           //
        case RUNNING:
           //
        case ERROR:
           //
    }
}

void adc_init(){
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

uint16_t adc_read(uint8_t point){
    ADMUX = (ADMUX&0x50) | (point & 0x0F);
    ADCSRA |= (1<<ADSC);

    while(ADCSRA & (1 << ADSC));

    return ADC;
}

void water_level_check(){
    uint16_t waterLevel = adc_read(WATER_SENSOR_CHANNEL);
    if(waterLevel < WATER_THRESHOLD){
        current = ERROR;
        PORTB |= (1<<RED_LED);
        PORTB &= ~((1<<GREEN_LED)|(1<<BLUE_LED)|(1<<YELLOW_LED));
        lcd.clear();
        lcd.print("Error: Water level low");
    }
}

void manageFan(float temp){
    if(temp >= TEMP_THRESHOLD_HIGH){
        PORTB |= (1<<FAN_PIN);
    } else if (temp <= TEMP_THRESHOLD_LOW){
        PORTB &= ~(1<<FAN_PIN);
    }
}

void updateTemp(){
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    //if(isnan)
}