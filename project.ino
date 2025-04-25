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

#define YELLOW_LED 2
#define GREEN_LED 3
#define BLUE_LED 4
#define RED_LED 6
#define FAN_PIN 6
#define START_BUTTON 18
#define STOP_BUTTON 20
#define RESET_BUTTON 19

