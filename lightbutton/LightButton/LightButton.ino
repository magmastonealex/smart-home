// Enable debug prints
#define MY_DEBUG
#define MY_OTA_FIRMWARE_FEATURE
// Enable and select radio type attached
#define MY_RADIO_RF24

#include <Bounce2.h>
#include <MySensors.h>

uint32_t SLEEP_TIME = 300000; // Sleep time between reports (in milliseconds)
#define DIGITAL_INPUT_SENSOR 3
#define CHILD_ID 1   // Id of the sensor child
// Initialize motion message
MyMessage msg(CHILD_ID, V_TRIPPED);
Bounce debouncer = Bounce();

void setup()
{
	pinMode(DIGITAL_INPUT_SENSOR, INPUT_PULLUP);      // sets the motion sensor digital pin as input
  debouncer.attach(DIGITAL_INPUT_SENSOR);
  debouncer.interval(5);
  //Serial.begin(115200);
}

unsigned long sleepAt = 0;

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("LightButton", "1.2");

	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID, S_MOTION);
  
}

int value = LOW;

int countSendBattery = 10;

void loop()
{
  if(debouncer.update()) {
    value = debouncer.read();
    Serial.println("Changed!");
    send(msg.set(value == HIGH ? "1" : "0"));  // Send tripped value to gw
  }
  
  wait(1);

  if (value == LOW && ((long) (millis() - sleepAt) >= 0)) {
    Serial.println("Initiate sleep...");
    countSendBattery++;
    if(countSendBattery > 10) {
      sendBatt();
      countSendBattery = 0;
    }
    sendHeartbeat();
    wait(100);
    sleep(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), RISING, 300000);
    sleepAt = millis() + 500;
  }
}

void sendBatt() {
    long vcc = readVcc();
    Serial.println(vcc);
    vcc = vcc - 1900; // subtract 1.9V from vcc, as this is the lowest voltage we will operate at
    
    uint8_t percent = vcc / 14.0;
    Serial.println(percent);
    sendBatteryLevel(percent);
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADcdMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
 
}
