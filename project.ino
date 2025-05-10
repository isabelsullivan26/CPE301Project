//CPE301 Final Project
//Isabel Sullivan, Ashton Hayes, Nik Sunga, Rakiyah Jackson
//Includes
#include <Arduino.h>
#include <Stepper.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <RTClib.h>


//VENTS
#define VENT_LEFT 22
#define VENT_RIGHT 23

//UART
 #define RDA 0x80 // receives data
 #define TBE 0x20 
 volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
 volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
 volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
 volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
 volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//Millis delay
unsigned long millisPrev = 0;
unsigned const long interval = 60000;

#define WATER_SENSOR_CHANNEL 0
#define WATER_THRESHOLD 200
#define TEMP_THRESHOLD_HIGH 30
#define TEMP_THRESHOLD_LOW 25
#define DHT_PIN 2
#define DHT_TYPE DHT11

#define YELLOW_LED 10
#define GREEN_LED 11
#define BLUE_LED 12
#define RED_LED 13
#define FAN_PIN 14
#define START_BUTTON 7
#define STOP_BUTTON 8
#define RESET_BUTTON 9

Stepper ventStepper(2048, 3, 4, 5, 6);
LiquidCrystal lcd(A2,A3,A4,A5,A6,A7);
DHT dht(DHT_PIN, DHT_TYPE);
#define SDA_PIN 20
#define SCL_PIN 21
RTC_DS1307 rtc;

//ADC and UART
void adc_init();
uint16_t adc_read(uint8_t channel);

enum State { DISABLED, RUNNING, ERROR, IDLE };
volatile State current = DISABLED;
State prevState = DISABLED;

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
   U0init(9600);
    DDRB |= (1<<YELLOW_LED)|(1<<GREEN_LED)|(1<<BLUE_LED)|(1<<RED_LED)|(1<<FAN_PIN);
    DDRD &= ~((1<<START_BUTTON) | (1<<STOP_BUTTON) | (1<<RESET_BUTTON));
    //attachinterrupt
    attachInterrupt(digitalPinToInterrupt(START_BUTTON), startISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(STOP_BUTTON), stopISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetISR, FALLING);
    adc_init();
    lcd.begin(16,2);
    dht.begin();
    rtc.begin();
    ventStepper.setSpeed(10);
    DDRD &= ~((1<<VENT_LEFT) | (1<<VENT_RIGHT));
    PORTD |= (1<<VENT_LEFT) | (1<<VENT_RIGHT); 
}

void loop(){
    switch(current){
        case DISABLED:
          PORTB |= (1<<YELLOW_LED);
          PORTB &= ~((1<<RED_LED)|(1<<GREEN_LED)|(1<<BLUE_LED)|(1<<FAN_PIN));
          lcd.clear();
          lcd.print("Disabled");
          break;
        case IDLE:
          PORTB |= (1<<GREEN_LED);
          PORTB &= ~((1<<RED_LED)|(1<<BLUE_LED)|(1<<YELLOW_LED));
          water_level_check();
          if (millis() - millisPrev >= interval) {
            millisPrev = millis();
            updateTemp();
          }
          break;
        case RUNNING:
          PORTB |= (1<<BLUE_LED);
          PORTB &= ~((1<<RED_LED)|(1<<GREEN_LED)|(1<<YELLOW_LED));
          water_level_check();
          if (millis() - millisPrev >= interval) {
            millisPrev = millis();
            updateTemp();
          }
          break;
        case ERROR:
          PORTB |= (1<<RED_LED);
          PORTB &= ~((1<<GREEN_LED)|(1<<BLUE_LED)|(1<<YELLOW_LED)|(1<<FAN_PIN));
          lcd.clear();
          lcd.print("System ERROR");
          break;
    }
    logState();
    if (current != DISABLED) {
      if (!(PIND & (1 << VENT_LEFT))) {
        ventStepper.step(-10);
        event("Vent left");
      }
      if (!(PIND & (1 << VENT_RIGHT))) {
        ventStepper.step(10);
        event("Vent right");
      }
  }
}

void U0init(unsigned long U0baud)
{
  
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
//CODE FROM UART LAB
// Read USART0 RDA status bit and return non-zero true if set
//
unsigned char U0kbhit(void)
{
  return (*myUCSR0A & RDA) ? 1:0;
}
//
// Read input character from USART0 input buffer
//
unsigned char U0getchar(void)
{
  return *myUDR0;
}
//
// Wait for USART0 (myUCSR0A) TBE to be set then write character to
// transmit buffer
//
void U0putchar(unsigned char U0pdata)
{
  while(!(*myUCSR0A & TBE));
  *myUDR0 = U0pdata;
}
void adc_init(){
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

uint16_t adc_read(uint8_t point){
    ADMUX = (ADMUX&0xF0) | (point & 0x0F);
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
  static bool prevOn = false;
  bool fanOn = temp>=TEMP_THRESHOLD_HIGH;
  if(fanOn && !prevOn){
    PORTB |= (1<<FAN_PIN);
    event("FAN ON");
  } else if (!fanOn && prevOn && temp <= TEMP_THRESHOLD_LOW){
    PORTB &= ~(1<<FAN_PIN);
    event("FAN OFF");
  }
  prevOn = fanOn;
}

void updateTemp(){
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    if((isnan(temp)) || isnan(humidity)){
      lcd.clear();
      lcd.print("Error reading temp");
      return;
    }

  lcd.clear();
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.setCursor(0,1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  manageFan(temp);
}

void event(const char* msg){
  DateTime now = rtc.now();
  U0putchar('[');
  U0putchar('0' + now.hour() / 10);  // tens digit
  U0putchar('0' + now.hour() % 10);  // ones digit
  U0putchar(':');
  U0putchar('0' + now.minute() / 10);
  U0putchar('0' + now.minute() % 10);
  U0putchar(':');
  U0putchar('0' + now.second() / 10);
  U0putchar('0' + now.second() % 10);
  U0putchar(']');
  U0putchar(' ');
  while (*msg) {
    U0putchar(*msg++);
    }
    U0putchar('\n');
}

void logState(){
  if(current != prevState){
    if(current == IDLE){
      event("STATE: IDLE");
    } else if (current == RUNNING){
      event("STATE: RUNNING");
    } else if (current == ERROR){
      event("STATE: ERROR");
    } else if (current == DISABLED){
      event("STATE: DISABLED");
    }
    prevState = current;
  }
}
