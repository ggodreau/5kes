#include <SPI.h>
//#include <Ticker.h>
#include <TickTwo.h>
#include "Adafruit_MAX31855.h"

//Ticker everyOn;
//Ticker everyOff;

void getTemp();
TickTwo cronTemp(getTemp, 2000, 0, MILLIS); // read every 0.5s, indefinitely

// Pins
const int MAXDO = 12; // D6 connects to 'DO' data out of MAX31855
const int MAXCS = 15; // D8 connects to 'CS' of MAX31855
const int MAXCLK = 14; // D5 connects to 'CLK' of MAX31855
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// Globals
bool tempAlarmState = false; // true = in alarm state
double tempThresh = 80.0; // temp above which alarm is triggered
double tempHyst = 3.0; // hysteresis, must be positive double that is < tempThresh

const int atsPin = 4; // D2 input pullup, triggers when connected to GND of NodeMCU
bool atsState = false;

// Timer: Auxiliary variables
//unsigned long now = millis();
//unsigned long lastTrigger = 0;
//boolean startTimer = false;

void ledOn(){                        // this is your callback function
//  Serial.println("led on");
  digitalWrite(LED_BUILTIN, LOW);
}

void ledOff(){                        // this is your callback function
//  Serial.println("led off");
  digitalWrite(LED_BUILTIN, HIGH);
}

ICACHE_RAM_ATTR void atsAction(){                        // this is your callback function
  Serial.println("------ ats called!");
  int atsReading = digitalRead(atsPin);
  if (atsReading == 1) {
    atsState = true;
    Serial.println("ATS on, state: " + String(atsState) + " pin read: " + String(atsReading));
  } else if (atsReading == 0) {
    atsState = false;
    Serial.println("ATS on, state: " + String(atsState) + " pin read: " + String(atsReading));
  }
//  startTimer = true;
//  lastTrigger = millis();
}


void setup() {
  Serial.begin(9600);
  //SPI.begin();

  while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc

  Serial.println("MAX31855 1 test");
  // wait for MAX chip to stabilize
  delay(2000);
  Serial.print("Initializing 1 sensor...");

  pinMode(LED_BUILTIN, OUTPUT);
  
  cronTemp.start();
  
//  everyOn.attach_ms(500,ledOn); // "register" your callback
//  everyOff.attach_ms(800,ledOff); // "register" your callback
  
  pinMode(atsPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(atsPin), atsAction, RISING);
  
  if (!thermocouple.begin()) {
    Serial.println("ERROR.");
    while (1) delay(10);
  }
  Serial.println("DONE.");
}

void loop() {
  cronTemp.update();
}

void getTemp() {
//  Serial.println(thermocouple.readInternal());
  double f = thermocouple.readFahrenheit();
  if (isnan(f)) {
    Serial.println("Something wrong with thermocouple!");
  } else {
    Serial.print("F = ");
    Serial.println(f);

    if (tempAlarmState == false) {
      if (f > tempThresh) {
        handleTempAlarm();
      }
    } else {
      if (f < (tempThresh - tempHyst)) {
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
  // TODO
  Serial.println("Disconnecting AT relay");
}

void connectAt() {
  // TODO
  Serial.println("Connecting AT relay");
}
