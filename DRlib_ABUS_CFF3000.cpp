/*
  Written 2021-01-23 by waka4200 (on reddit)
  
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


// FB LED Status Variablen
uint16_t _sensorValue = 0;
//bool  LED_aktiviert_sich = false;
unsigned long _LED_ist_aktiv_seit = 0;
//bool  LED_erkennung_aktiv = true;
bool  _LED_gruen = false;
bool  _LED_rot = false;
bool  _LED_error = false;
bool  _flag_req_open_the_doorlock = false;
bool  _flag_req_close_the_doorlock = false;
bool  _flag_req_doorlock_status = false;
bool  _DR_active = false;
uint8_t _LED_blink_counter = 0;
unsigned long _LED_blink_timer = 0;
bool _LED_leuchtstatus = false;
bool _LED_leuchtstatus_old = false;
uint8_t _LED_phase = 0;
uint8_t _LED_phase_old = 0;
uint16_t  _reset_delay = 10000; // After 12 seconds, become ready for inputs again

// Open-Close Switch Relay Outputs
int _open_doorlock_relay = 9;
int _close_doorlock_relay = 10;

// FB-Relay-Steuerung Variablen
unsigned long _tuerschloss_input_timer = 0;
String _hr_status = "Gestartet.";

// Serial-Output Variablen
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
  // FB Relay-Steuerung
  pinMode(_open_doorlock_relay,OUTPUT);
  pinMode(_close_doorlock_relay,OUTPUT);
  digitalWrite(_open_doorlock_relay,HIGH); //Relais is inverted
  digitalWrite(_close_doorlock_relay,HIGH);  //Relais is inverted

  //Serial-Output
  Serial.begin(9600);

  //Start-Delay
  delay(2000); //FB muss sich erst beruhigen <-- PFLICHT!
  Serial.println("DRlib_ABUS_CFF3000: Ready");
}



// LED Auslese-Funktion
void DRlib_ABUS_CFF3000::DR_operation()
{
  // Ablese-Block, daueraktiv pro Runde
  _sensorValue = analogRead(_LEDsignal);
  // Wert von 0-1023 = 0-5V, umrechnen.
  float voltage = _sensorValue * (5.0 / 1023.0);

  // Auswerten und damit Flags setzen

  // DEBUGGING
  //Serial.println(voltage);

  /*
  // Diese Logik erkennt automatisch Veraenderungen. Dazu muss sie zeitnah laufen. Ist nicht garantiert, daher Ersatz. FB_LED_activate setzt LED_phase = 1
  //
  // Halbaktivierungs-Flag
  if ( LED_erkennung_aktiv && (LED_phase <= 0 || LED_phase > 404) && voltage > 2.0 && voltage < 2.8 ) LED_aktiviert_sich = true;
  // Vollaktivierungs-Flag
  if ( LED_erkennung_aktiv && (LED_phase <= 0 || LED_phase > 404) && LED_aktiviert_sich && voltage > 3.0 && voltage < 3.6 ) {
    if (DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: Aktivierung erkannt");
  	FB_LED_reset(); // Alles zurueck auf Anfang
  	LED_phase = 1;
  	LED_ist_aktiv_seit = millis();
    LED_erkennung_aktiv = false;
  }
  */

  // Wartephase
  // Reaktionsphase nach 2 Sekunden
  if ( _LED_phase == 1 ){ // Schonung der CPU
	  //if (DEBUG) Serial.println("DEBUG: FB-Phasencheck ist aktiv.");
	  if (millis() - _LED_ist_aktiv_seit > 2000) _LED_phase = 2; // Zeit die Reaktion zu pruefen
  }
  
  // Reaktionsphase
  // Ablesen des Status bei 2200ms
  if ( _LED_phase == 2 ){ // Schonung der CPU
	  if ( millis() - _LED_ist_aktiv_seit > 2200) {
	    if (voltage > 3.0) {
        // Vermutung: Gruene LED
        if(_DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: Gruen vermutet, Volt: " + String(voltage));
  	    _LED_gruen = true;
        _LED_phase = 3;
	    } else {
        // Vermutung: Rote LED
        if(_DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: Rot vermutet, Volt: " + String(voltage));
        _LED_rot = true;
        _LED_phase = 3;
	    }
	  }
  }

  // Phase: Fehlererkennung
  if ( _LED_phase == 3 ){ // Schonung der CPU
    if ( millis() - _LED_ist_aktiv_seit > 3500){
      // Blink-Phase

      // Starte Timer neu
      if ( _LED_blink_timer == 0) {
        _LED_blink_timer = millis(); // Starte Timer fuer Blink-Erkennung
        if ( voltage > 3.0 ) { _LED_leuchtstatus = true; _LED_leuchtstatus_old = true; }
        if ( voltage < 0.8 ) { _LED_leuchtstatus = false; _LED_leuchtstatus_old = false; }
      }

      // Teste Status nach 210ms, zaehle Blinkerei
      if ( millis() - _LED_blink_timer > 220 ){
    	  if(_DEBUG) Serial.println("Blinkzaehli laeuft, Volt: " + String(voltage));
    	  if ( voltage > 3.0 ) _LED_leuchtstatus = true;
    	  if ( voltage < 0.8 ) _LED_leuchtstatus = false;
    	  if ( _LED_leuchtstatus != _LED_leuchtstatus_old ) {
    		  _LED_blink_counter = _LED_blink_counter + 1;
    		  _LED_leuchtstatus_old = _LED_leuchtstatus;
    	  }
    	  _LED_blink_timer = millis(); //Reset Timer
      }
    }

    // Auswertung der Counterzaehlung. Etwas ungenau, scheint zu funktionieren.
    if ( millis() - _LED_ist_aktiv_seit > 8000 ) {
      if(_DEBUG) Serial.println("DRlib_ABUS_CFF3000__DEBUG: LED Blink Counter: " + String(_LED_blink_counter));
      if (_LED_blink_counter > 2 ) {
        // Blinken und damit Fehler erkannt, resette Variablen entsprechend. Ansonsten Status wie vermutet.
        _LED_error = true;
        _LED_gruen = false;
        _LED_rot = false;
      }
      _LED_phase = 4; // Kein Blinken erkannt, weiter ohne Aenderung vom Erkennen beim ersten Auslesen
    }
  }
  
  // Auswertungsphase
  // Setze LED_phase entsprechend. Analysiere dabei moegliche Fehler
  if ( _LED_phase == 4 ){ // Schonung der CPU
	  // If Status was requested
	  if(_flag_req_doorlock_status){
		  if (_LED_error) _LED_phase = 40;
		  else if (_LED_gruen) _LED_phase = 10;
		  else if (_LED_rot)   _LED_phase = 20;
		  else _LED_phase = 50; // Falls nichts davon passt, sollte nicht vorkommen
	  }

	  // If close was requested
	  if(_flag_req_close_the_doorlock){
		  if (_LED_error) _LED_phase = 40;
		  else if (_LED_gruen) _LED_phase = 10;
		  else _LED_phase = 50;
	  }

	  // If open was requested
	  if(_flag_req_open_the_doorlock){
		  if (_LED_error) _LED_phase = 40;
		  else if (_LED_rot) _LED_phase = 20;
		  else _LED_phase = 50;
	  }
  }
}

// Reset-Funktion fuer readLED
void DRlib_ABUS_CFF3000::DR_to_standby()
{
	_LED_phase_old = _LED_phase; // Save old status
	_LED_phase = 0;
	_LED_gruen = false;
	_LED_rot = false;
	_LED_error = false;
	_LED_ist_aktiv_seit = 0;
	// LED_erkennung_aktiv = true;
	_LED_blink_counter = 0;
	_flag_req_doorlock_status = false;
	_flag_req_open_the_doorlock = false;
	_flag_req_close_the_doorlock = false;
	_tuerschloss_input_timer = 0;
	_LED_blink_counter = 0;
	_DR_active = false;
}

// Aktivierungs-Funktion fuer readLED
void DRlib_ABUS_CFF3000::DR_activate()
{
	_tuerschloss_input_timer = millis();
	_LED_phase = 1;
	_LED_ist_aktiv_seit = millis();
	//LED_erkennung_aktiv = false;
	_DR_active = true;
}

// API-Abruf: Frage aktuellen Status vom Tuerschloss ab
uint8_t DRlib_ABUS_CFF3000::DR_status()
{
  uint8_t returncode = 0;
	if (_LED_phase == 0) returncode = 0;	// Schlaeft
	if (_LED_phase > 0 && _LED_phase < 10) returncode = 1;	// FB aktiv, Inputs werden gesammelt
	if (_LED_phase == 10 ) returncode = 10; 	// Tuerschloss zu
	if (_LED_phase == 20 ) returncode = 20; 	// Tuerschloss offen
	if (_LED_phase == 40) returncode = 40;	// Verbindungsfehler
	if (_LED_phase > 40) returncode = 50; // Fehler im Code! Sollte nicht zurueckkommen!
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

// API: Tuerschloss oeffnen
void DRlib_ABUS_CFF3000::request_open_the_doorlock(){
	if( _LED_phase == 0 ) _flag_req_open_the_doorlock = true;
}
void DRlib_ABUS_CFF3000::tuerschloss_oeffnen()
{
  if ( _LED_phase == 0 && _tuerschloss_input_timer == 0) {
	  if(_DEBUG) Serial.println( "DRlib_ABUS_CFF3000__DEBUG: Tuer oeffnen..." );
	  DR_activate();
	  digitalWrite(_open_doorlock_relay,LOW);
  }
  if (millis() - _tuerschloss_input_timer >=500) {
	  digitalWrite(_open_doorlock_relay,HIGH);
	  _tuerschloss_input_timer = 0;
  }
}


// API: Tuerschloss schliessen
void DRlib_ABUS_CFF3000::request_close_the_doorlock(){
	if( _LED_phase == 0 ) _flag_req_close_the_doorlock = true;
}
void DRlib_ABUS_CFF3000::tuerschloss_schliessen()
{
  if ( _LED_phase == 0 && _tuerschloss_input_timer == 0) {
	  if(_DEBUG) Serial.println( "DRlib_ABUS_CFF3000__DEBUG: Tuer schliessen..." );
	  DR_activate();
	  digitalWrite(_close_doorlock_relay,LOW);
  }
  if (millis() - _tuerschloss_input_timer >=500) {
	  digitalWrite(_close_doorlock_relay,HIGH);
	  _tuerschloss_input_timer = 0;
  }
}


// API: Tuerschlossstatus abfragen
void DRlib_ABUS_CFF3000::request_get_doorlock_status(){
	if ( _LED_phase == 0 ) _flag_req_doorlock_status = true;
}
void DRlib_ABUS_CFF3000::tuerschloss_status_abfragen()
{
  if ( _LED_phase == 0 && _tuerschloss_input_timer == 0 ){
    if(_DEBUG) Serial.println( "DRlib_ABUS_CFF3000__DEBUG: Status abfragen..." );
    DR_activate();
    digitalWrite(_close_doorlock_relay,LOW);
    digitalWrite(_open_doorlock_relay,LOW);
  }
  if ( millis() - _tuerschloss_input_timer >= 500) {
    digitalWrite(_close_doorlock_relay,HIGH);
    digitalWrite(_open_doorlock_relay,HIGH);
    _tuerschloss_input_timer = 0;
  }
}


void DRlib_ABUS_CFF3000::run_this_continuously()
{
  _statuscode = DR_status();
  if(_DEBUG) if ( _statuscode_old != _statuscode) { _statuscode_old = _statuscode; status_output_to_serial(); }  // Veraenderten Status ausgeben
  if ( _flag_req_doorlock_status ) tuerschloss_status_abfragen();
  if ( _flag_req_open_the_doorlock ) tuerschloss_oeffnen();
  if ( _flag_req_close_the_doorlock ) tuerschloss_schliessen();
  if (_DR_active) {
	  if ( _LED_phase > 0 && _LED_phase < 10 ) DR_operation();  // Run Checks if active Check Phase running
	  if ( millis() - _LED_ist_aktiv_seit > _reset_delay ) DR_to_standby(); // Standby after 10 Seconds
  }

}
