
#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_switch_on;

static uint32_t next_heap_millis = 0;

#define PIN_RELAY 2

int KEY_DOWN_COUNT    = 0; //按键按下计数
int KEY_STATE_NOW     = 0; //按键目前状态
int KEY_STATE         = 0; //按键即将判断的状态 1：短按，2：长按


//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value) {
	bool on = value.bool_value;
	cha_switch_on.value.bool_value = on;	//sync the value
	LOG_D("Switch: %s", on ? "ON" : "OFF");
	digitalWrite(PIN_RELAY, on ? LOW : HIGH);
}

void my_homekit_setup() {
	pinMode(PIN_RELAY, OUTPUT);
  pinMode(3, INPUT_PULLUP);
	digitalWrite(PIN_RELAY, HIGH);

	//Add the .setter function to get the switch-event sent from iOS Home APP.
	//The .setter should be added before arduino_homekit_setup.
	//HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
	//Maybe this is a legacy design issue in the original esp-homekit library,
	//and I have no reason to modify this "feature".
	cha_switch_on.setter = cha_switch_on_setter;
	arduino_homekit_setup(&config);

	//report the switch value to HomeKit if it is changed (e.g. by a physical button)
	//bool switch_is_on = true/false;
	//cha_switch_on.value.bool_value = switch_is_on;
	//homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}

/* 按键扫描以及功能 */
void keyScan() {
  if(digitalRead(3)==0){
    KEY_DOWN_COUNT++;
    KEY_STATE_NOW = 1;
    if (KEY_DOWN_COUNT >= 10 && KEY_DOWN_COUNT < 100) { //按键1计数
      KEY_STATE = 1;
    }
    if (KEY_DOWN_COUNT >= 100) {
      KEY_STATE = 2;
    }
  } else {
    KEY_DOWN_COUNT = 0;
    KEY_STATE_NOW = 0;
  }
  if (KEY_STATE_NOW == 0 && KEY_STATE == 1){ //按键短按
    // DO SOME
    if(digitalRead(PIN_RELAY) == HIGH){
      digitalWrite(PIN_RELAY, LOW); // 这个led是低电平出发
      bool switch_is_on = true;
	    cha_switch_on.value.bool_value = switch_is_on;
	    homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
    } else {
      digitalWrite(PIN_RELAY, HIGH);
      bool switch_is_on = false;
	    cha_switch_on.value.bool_value = switch_is_on;
	    homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
    }
    
    KEY_STATE = 0;
  }
  if (KEY_STATE_NOW == 0 && KEY_STATE == 2){ //按键长按
  
    KEY_STATE = 0;
  }
 
}

void setup() {
	Serial.begin(115200);
	wifi_connect(); // in wifi_info.h
	//homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
	my_homekit_setup();
}

void loop() {
  keyScan();
	my_homekit_loop();
	delay(10);
}


