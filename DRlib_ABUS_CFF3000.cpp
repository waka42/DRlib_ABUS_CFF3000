/*
  Written 2021-01-23 by waka42 (on reddit)
  
  This Library controls the ABUS door remote actuator. It uses a relay module to short out both ends of each switch electrically (necessary!).
  A third Pib "LEDsignal" monitors the point behind the Status LED, a bigger hole, which can be used to read the Status events.
  Finally, 3.3V and Ground go to the battery compartment. Be careful to not short this out!
  Use Hot Glue to fix wires straight after soldering, they tend to rip off easily. Solder from below, use the points you measure out as direct connections to each part.
  
  Simple click switches are used on 2,3,4 as inputs. 2 = open, 3 = close, 4 = status request
  9 = open-button relay
  10 = close-button relay
  
  This code relies on an Arduino Uno. If you use a different board, ADJUST THE COUNTER VALUES OF LED_blink_timer!!! Detection WILL fail otherwise!
*/
#include "DRlib_ABUS_CFF3000.h"

#include "Arduino.h"


// DR LED Status Variables
uint16_t _sensorValue = 0;
//bool  LED_activating = false;
unsigned long _LED_signal_active_since = 0;
//bool  LED_detection_active = true;
bool  _LED_green = false;
bool  _LED_red = false;
bool  _LED_error = false;
bool  _flag_req_open_the_doorlock = false;
bool  _flag_req_close_the_doorlock = false;
bool  _flag_req_doorlock_status = false;
bool  _DR_active = false;
uint8_t _LED_blink_counter = 0;
unsigned long _LED_blink_timer = 0;
bool _LED_light_status = false;
bool _LED_light_status_old = false;
uint8_t _LED_phase = 0;
uint8_t _LED_phase_old = 0;
uint16_t  _reset_delay = 10000; // After 12 seconds, become ready for inputs again

// Open-Close Switch Relay Outputs
int _open_doorlock_relay = 9;
int _close_doorlock_relay = 10;

// DR Relais Control Variables
unsigned long _doorlock_input_timer = 0;
String _hr_status = "Started.";

// Serial Output Variables
int _statuscode = 0;
int _statuscode_old = 0;

// Debugging Output, set to true to activate it
bool _DEBUG = false;

DRlib_ABUS_CFF3000::DRlib_ABUS_CFF3000()
{
	Serial.println("DRlib_ABUS_CFF3000: Library loaded");
}

void DRlib_ABUS_CFF3000::pin_relais_open(int pin)
{
  _open_doorlock_relay = pin;
}
void DRlib_ABUS_CFF3000::pin_relais_close(int pin)
{
  _close_doorlock_relay = pin;
}
void DRlib_ABUS_CFF3000::pin_status_led(int pin)
{
  _LEDsignal = pin;
}


void DRlib_ABUS_CFF3000::initialize()
{
  // DR Relais Control
  pinMode(_open_doorlock_relay,OUTPUT);
  pinMode(_close_doorlock_relay,OUTPUT);
  digitalWrite(_open_doorlock_relay,HIGH);   // Relay is inverted!
  digitalWrite(_close_doorlock_relay,HIGH);  // Relay is inverted!

  // Serial Output
  Serial.begin(9600);

  // Start Delay
  delay(2000); // DR needs to calm down on power up, takes about 2 seconds. DO NOT SKIP THIS!
  Serial.println("DRlib_ABUS_CFF3000: Ready");
}



// LED signal detection and computation
void DRlib_ABUS_CFF3000::DR_operation()
{
  // Read block. Continuously reads the current voltage
  _sensorValue = analogRead(_LEDsignal);
  // Recalculate from RAW values to Voltage on a Scale of 0-5V: 0-1023 = 0-5V
  float voltage = _sensorValue * (5.0 / 1023.0);

  // Analyse and set flags

  // DEBUGGING
  //Serial.println(voltage);

  /*
   * This Logic detects changes in voltage automatically. This needs to run closely, as an LED activation signal jumps from about 0V to above 2V,
   * then within 200ms to above 3V, then 200ms later to 0V. I call this a Trigger Signal. This signals the Remote becoming active.
   * This Logic is not used, since it's more of a dead-end logic-wise. This library gets active on request and does not wait for a Trigger.
   * Checking for a Trigger is therefore senseless.
   *
  // Half-Activation Flag
  if ( LED_detection_active && (LED_phase <= 0 || LED_phase > 404) && voltage > 2.0 && voltage < 2.8 ) LED_activating = true;
  // Full-Activation Flag
  if ( LED_detection_active && (LED_phase <= 0 || LED_phase > 404) && LED_activating && voltage > 3.0 && voltage < 3.6 ) {
    if (DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: Activation detected");
  	DR_LED_reset(); // Reset variables, this is a code artifact I don't care about
  	LED_phase = 1;
  	LED_active_since = millis();
    LED_detection_active = false;
  }
  */

  // Wait Phase, wait for 2 seconds, then check the LEDsignal status.
  if ( _LED_phase == 1 ){ // Schonung der CPU
	  //if (DEBUG) Serial.println("DEBUG: DR Phase-Check active");
	  if (millis() - _LED_signal_active_since > 2000) _LED_phase = 2; // Switch to Phase 2 after 2 seconds
  }
  
  // Reaction Phase
  // Read signal after 2200ms, 200ms extra so variations in the Remotes Microcontroller do not give us false inputs.
  //
  // Note: Now either you have Green = doorlock closed, Red = doorlock open, or Green-Red blinking. Here's the trick:
  // 1. Read signal and change to phase 3
  if ( _LED_phase == 2 ){ // save CPU time
	  if ( millis() - _LED_signal_active_since > 2200) {
	    if (voltage > 3.0) {
        // Vermutung: Gruene LED
        if(_DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: Green detected, Voltage: " + String(voltage));
  	    _LED_green = true;
        _LED_phase = 3;
	    } else {
        // Vermutung: Rote LED
        if(_DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: Red detected, Voltage: " + String(voltage));
        _LED_red = true;
        _LED_phase = 3;
	    }
	  }
  }

  // Phase 3: Connection Error detection
  // 2. Check for blinking via a counter. Each time the Voltage switches, we count up or down, but just once!
  if ( _LED_phase == 3 ){ // save CPU time
    if ( millis() - _LED_signal_active_since > 3500){
      // Blinking-Phase

      // Start Timer
      if ( _LED_blink_timer == 0) {
        _LED_blink_timer = millis(); // Reset Timer for blink detection
        if ( voltage > 3.0 ) { _LED_light_status = true; _LED_light_status_old = true; }
        if ( voltage < 0.8 ) { _LED_light_status = false; _LED_light_status_old = false; }
      }

      // Check Voltage after delay, read signal, count or not count up to detect blinking
      if ( millis() - _LED_blink_timer > 220 ){
    	  if(_DEBUG) Serial.println("Blink-Counter running, Voltage: " + String(voltage));
    	  if ( voltage > 3.0 ) _LED_light_status = true;
    	  if ( voltage < 0.8 ) _LED_light_status = false;
    	  if ( _LED_light_status != _LED_light_status_old ) {
    		  _LED_blink_counter = _LED_blink_counter + 1;
    		  _LED_light_status_old = _LED_light_status;
    	  }
    	  _LED_blink_timer = millis(); //Reset Timer
      }
    }

    // After 6 seconds, detect how many changes were detected. It's a bit wonky, but works.
    if ( millis() - _LED_signal_active_since > 8000 ) {
      if(_DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: LED Blink Counter: " + String(_LED_blink_counter));
      if (_LED_blink_counter > 2 ) {
    	  // More than 2 changes detected, must be an error. 3 might be a safer approach, but 2 worked fine so far.
    	  // Note: We are setting green and red to false and error to true, which correctly transfers the status.
    	  // One could save variable space by using a numbered counter, but I am unsure how much would be saved compared to understandability of this logic.
    	  _LED_error = true;
    	  _LED_green = false;
    	  _LED_red = false;
      }
      _LED_phase = 4; // No blinking detected. Continue without changing our original reading.
    }
  }
  
  // Evaluate Results
  // Set LED_phase to our result, which also stops this function from running.
  if ( _LED_phase == 4 ){ // save CPU time
	  // If Status was requested
	  if(_flag_req_doorlock_status){
		  if (_LED_error) _LED_phase = 40;
		  else if (_LED_green) _LED_phase = 10;
		  else if (_LED_red)   _LED_phase = 20;
		  else _LED_phase = 50; // Just in case nothing matches. This means a major error in the code must have appeared!
	  }

	  // If close was requested
	  if(_flag_req_close_the_doorlock){
		  if (_LED_error) _LED_phase = 40;
		  else if (_LED_green) _LED_phase = 10;
		  else _LED_phase = 50;
	  }

	  // If open was requested
	  if(_flag_req_open_the_doorlock){
		  if (_LED_error) _LED_phase = 40;
		  else if (_LED_red) _LED_phase = 20;
		  else _LED_phase = 50;
	  }
  }
}

// Reset-Function for DR_operation
void DRlib_ABUS_CFF3000::DR_to_standby()
{
	_LED_phase_old = _LED_phase; // Save old status
	_LED_phase = 0;
	_LED_green = false;
	_LED_red = false;
	_LED_error = false;
	_LED_signal_active_since = 0;
	// LED_detection_active = true; // obsolete
	_LED_blink_counter = 0;
	_flag_req_doorlock_status = false;
	_flag_req_open_the_doorlock = false;
	_flag_req_close_the_doorlock = false;
	_doorlock_input_timer = 0;
	_LED_blink_counter = 0;
	_DR_active = false;
}

// DR_operation: Activation sequence, setting it all up
void DRlib_ABUS_CFF3000::DR_activate()
{
	_doorlock_input_timer = millis();
	_LED_phase = 1;
	_LED_signal_active_since = millis();
	//LED_detection_active = false; // obsolete
	_DR_active = true;
}

// API: Return the current library Status code
uint8_t DRlib_ABUS_CFF3000::DR_status()
{
  uint8_t returncode = 0;
	if (_LED_phase == 0) returncode = 0;	// Sleeps
	if (_LED_phase > 0 && _LED_phase < 10) returncode = 1;	// Door Remote actively checking stuff
	if (_LED_phase == 10 ) returncode = 10; 	// Doorlock closed
	if (_LED_phase == 20 ) returncode = 20; 	// Doorlock open
	if (_LED_phase == 40) returncode = 40;	// Connection to Doorlock failed
	if (_LED_phase > 40) returncode = 50; // Code error! Something went wrong inside this librarys code!
  return returncode;
}

uint8_t DRlib_ABUS_CFF3000::DR_status_lasttime(){
	return _LED_phase_old;
}

// Translate status code to human-readable code
void DRlib_ABUS_CFF3000::status_output_to_serial()
{
  if ( _statuscode == 0 ) _hr_status = "Standby";
  if ( _statuscode == 1 ) _hr_status = "Remote active, checking Status...";
  if ( _statuscode == 10 ) _hr_status = "Doorlock closed";
  if ( _statuscode == 20 ) _hr_status = "Doorlock open";
  if ( _statuscode == 40 ) _hr_status = "Connection to Doorlock failed";
  if ( _statuscode == 50 ) _hr_status = "Read Error! Could not correctly detected Door Remote Status Code. Tweak the timing.";
  if ( _statuscode < 0 || _statuscode > 50 ) _hr_status = "Invalid Status Code! Something went really REALLY wrong in DRlib_ABUS_CFF3000.cpp!";
  Serial.println( "DRlib_ABUS_CFF3000: Status: " + _hr_status );
}

// API: Request to open the doorlock
void DRlib_ABUS_CFF3000::request_open_the_doorlock(){
	if( _LED_phase == 0 ) _flag_req_open_the_doorlock = true;
}
void DRlib_ABUS_CFF3000::open_doorlock()
{
  if ( _LED_phase == 0 && _doorlock_input_timer == 0) {
	  if(_DEBUG) Serial.println( "DRlib_ABUS_CFF3000__DEBUG: Opening doorlock..." );
	  DR_activate();
	  digitalWrite(_open_doorlock_relay,LOW);
  }
  if (millis() - _doorlock_input_timer >=500) {
	  digitalWrite(_open_doorlock_relay,HIGH);
	  _doorlock_input_timer = 0;
  }
}


// API: Request to close the doorlock
void DRlib_ABUS_CFF3000::request_close_the_doorlock(){
	if( _LED_phase == 0 ) _flag_req_close_the_doorlock = true;
}
void DRlib_ABUS_CFF3000::close_doorlock()
{
  if ( _LED_phase == 0 && _doorlock_input_timer == 0) {
	  if(_DEBUG) Serial.println( "DRlib_ABUS_CFF3000__DEBUG: Closing doorlock..." );
	  DR_activate();
	  digitalWrite(_close_doorlock_relay,LOW);
  }
  if (millis() - _doorlock_input_timer >=500) {
	  digitalWrite(_close_doorlock_relay,HIGH);
	  _doorlock_input_timer = 0;
  }
}


// API: Request the doorlock status
void DRlib_ABUS_CFF3000::request_get_doorlock_status(){
	if ( _LED_phase == 0 ) _flag_req_doorlock_status = true;
}
void DRlib_ABUS_CFF3000::status_doorlock()
{
  if ( _LED_phase == 0 && _doorlock_input_timer == 0 ){
    if(_DEBUG) Serial.println( "DRlib_ABUS_CFF3000__DEBUG: Request status..." );
    DR_activate();
    digitalWrite(_close_doorlock_relay,LOW);
    digitalWrite(_open_doorlock_relay,LOW);
  }
  if ( millis() - _doorlock_input_timer >= 500) {
    digitalWrite(_close_doorlock_relay,HIGH);
    digitalWrite(_open_doorlock_relay,HIGH);
    _doorlock_input_timer = 0;
  }
}


void DRlib_ABUS_CFF3000::run_this_continuously()
{
  _statuscode = DR_status();
  if(_DEBUG) if ( _statuscode_old != _statuscode) { _statuscode_old = _statuscode; status_output_to_serial(); }  // Print new status
  if ( _flag_req_doorlock_status ) status_doorlock();
  if ( _flag_req_open_the_doorlock ) open_doorlock();
  if ( _flag_req_close_the_doorlock ) close_doorlock();
  if (_DR_active) {
	  if ( _LED_phase > 0 && _LED_phase < 10 ) DR_operation();  // Run Checks if active Check Phase running
	  if ( millis() - _LED_signal_active_since > _reset_delay ) DR_to_standby(); // Standby after 10 Seconds
  }

}
