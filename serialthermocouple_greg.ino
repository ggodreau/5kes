#include <SPI.h>
#include <TickTwo.h>
#include "Adafruit_MAX31855.h"

// Pins
const int MAXDO = 12; // D6 connects to 'DO' data out of MAX31855
const int MAXCS = 15; // D8 connects to 'CS' of MAX31855
const int MAXCLK = 14; // D5 connects to 'CLK' of MAX31855
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// Timers
void getTemp();
void connectAt();
void ledBlink();
TickTwo cronTemp(getTemp, 2000, 0, MILLIS); // temp read every 2s, indefinitely
TickTwo cronTempBoot(getTemp, 250, 40, MILLIS); // read temp every 1/4s, 40 times (10s for bootup)
TickTwo cronAtxBoot(connectAt, 250, 40, MILLIS); // bootstrap atx read every 1/4s, 40 times (10s for bootup)
TickTwo cronLed(ledBlink, 50, 0, MILLIS); // blink led every 1/2s, indefinitely

// Globals
const int ledEsp = 2; // by the antenna
const int ledMcu = 16; // by the mini usb connector
bool ledEspState;
bool ledMcuState;

volatile bool tempAlarmState = false; // true = in alarm state
double currentTemp; // MAX31855 will read 0.0F if it is disconnected
double tempThresh = 80.0; // temp above which alarm is triggered
double tempHyst = 3.0; // hysteresis, must be positive double that is < tempThresh

const int atsPin = 4; // D2 input pullup, triggers when connected to GND of NodeMCU
volatile bool atsState; // defaults to true or 1, meaning dry contact loop is OPEN (on-grid)
const int atsDebounceDelay = 1000; // 50ms debounce filter
volatile long lastAtsDebounceTime = 0; // last millis() time ats debounce was triggered

bool atxState = false; // autotransformer state, true = on, false = off

void ledOn(){                        // this is your callback function
//  Serial.println("led on");
  digitalWrite(ledEsp, LOW);
}

void ledOff(){                        // this is your callback function
//  Serial.println("led off");
  digitalWrite(ledEsp, HIGH);
}

void ledBlink() {
  digitalWrite(ledEsp, ledEspState);
  ledEspState = !ledEspState;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc
  Serial.println("MAX31855 1 test");
  delay(2000);
  Serial.print("Initializing 1 sensor...");
  pinMode(ledEsp, OUTPUT);
  pinMode(atsPin, INPUT_PULLUP);

  cronTemp.start();
  cronTempBoot.start();
  cronAtxBoot.start();
  
  atsState = digitalRead(atsPin);

  attachInterrupt(digitalPinToInterrupt(atsPin), atsAction, CHANGE);
  if (!thermocouple.begin()) {
    Serial.println("ERROR.");
    while (1) delay(10);
  }
  Serial.println("DONE.");
}

void loop() {
  cronTemp.update();
  cronAtxBoot.update();
  cronLed.update();
}

ICACHE_RAM_ATTR void atsAction(){
  int atsReading = digitalRead(atsPin);
//  Serial.println("atsReading: " + String(atsReading) + " atsState: " + String(atsState));
  if (atsReading == atsState) {
    return;
  }
  boolean debounce = false;
  if ((millis() - lastAtsDebounceTime) <= atsDebounceDelay) {
    debounce = true;
  }
  lastAtsDebounceTime = millis();
  if (debounce) {
    Serial.println("less than debounce time, keeping state at: " + String(atsState));
    return;
  } else {
    Serial.println("setting state to: " + String(atsReading));
    atsState = atsReading;
    if (atsState) {
      Serial.println("ON grid");
      cronLed.stop();
      ledOn();
    } else {
      Serial.println("OFF grid");
//      ledOff();
      ledOff();
      cronLed.start();
    }
  }
}

void getTemp() {
//  Serial.println(thermocouple.readInternal());
  currentTemp = thermocouple.readFahrenheit();
  if (isnan(currentTemp)) {
    Serial.println("Something wrong with thermocouple!");
  } else {
    Serial.print("F = ");
    Serial.println(currentTemp);

    if (tempAlarmState == false) {
      if (currentTemp > tempThresh) {
        handleTempAlarm();
      }
    } else {
      if (currentTemp < (tempThresh - tempHyst)) {
        handleTempDisAlarm();
      }
    }
  }
}

void handleTempAlarm() {
  Serial.println("Over temp!");
  tempAlarmState = true;
  disconnectAt();
}

void handleTempDisAlarm() {
  Serial.println("Resetting from overtemp state");
  tempAlarmState = false;
  connectAt();
}

void disconnectAt() {
  if (atxState) {
    // TODO: flip AT relay pin OFF
    Serial.println("Disconnecting AT relay");
    atxState = false;
  } else {
    Serial.println("AT already disconnected");
  }
}

void connectAt() {
  if (!currentTemp) {
    Serial.println("Cannot turn on AT without a valid temp reading");
    return;
  }
  if (atsState) {
    Serial.println("Cannot turn on AT while GW is on-grid");
    return;
  }
  if (!atxState) {
    // TODO: flip AT relay pin ON
    Serial.println("Connecting AT relay");
    atxState = true;
  }
}
