// Example for using DRlib_ABUS_CFF3000 using a breadboard with switches. You can use the functions to your likings.
//
// ############################### IMPORTANT WARNING ###############################################
// ## NEVER EVER USE delay(XX) in your code! It ### WILL BREAK ### DRlib_ABUS_CFF3000s function!  ##
// #################################################################################################

#include "Arduino.h"
#include <DRlib_ABUS_CFF3000.h>


// Create lib object to use here
DRlib_ABUS_CFF3000 DRlib;

// Define Relay Outputs and LED signal input
int open_doorlock_relay = 9;
int close_doorlock_relay = 10;
int _LEDsignal = A0;

// Define Switch inputs
int sw_open = 2;
int sw_close = 3;
int sw_status = 4;

// Status Request Timer
unsigned long status_timer = 0;
unsigned long count_delay = 1000;

void setup() {
	// Pin Setup
	DRlib.pin_relais_open(open_doorlock_relay);
	DRlib.pin_relais_close(close_doorlock_relay);
	DRlib.pin_status_led(_LEDsignal);
	DRlib.initialize();
}

// Open-Close-Buttons Input from Breadboard or something. Just use Switches with Pull-Up Resistors.
void sw_input()
{
	if ( digitalRead(sw_open) == HIGH) {
		DRlib.request_open_the_doorlock();
		if ( status_timer == 0) status_timer = millis();
	}
	if ( digitalRead(sw_close) == HIGH) {
		DRlib.request_close_the_doorlock();
		if ( status_timer == 0) status_timer = millis();
	}
	if ( digitalRead(sw_status) == HIGH) {
		DRlib.request_get_doorlock_status();
		if ( status_timer == 0) status_timer = millis();
	}
}

void loop() {
	// Check if the Timer is running
	if(status_timer != 0) {

		// Important! Run this every loop if Remote is active!
		DRlib.run_this_continuously();

		if (millis() - status_timer > count_delay ) {

			// Print current status every second
			Serial.println("Statuscode: " + String( DRlib.DR_status() ) );

			// Print the raw status code
			DRlib.status_output_to_serial();

			// Print the status from the last run, always works
			Serial.println("Status last time was: " + String( DRlib.DR_status_lasttime() ) );

			// Delay this output by another second
			count_delay = count_delay + 1000;
		}
		if(DRlib.DR_status() == 0) { status_timer = 0; count_delay = 0;} // Reset if Door Remote is on Standby
	}
	else sw_input();	// Read Switch Input every loop if the remote is not in use

	//delay(10); 	// Delays up to 50ms work if you do not use this example with any other code!
				// Note that running other code takes up CPU time, increasing the delays.
				// DRlib_ABUS_CFF3000 has been written without using delay()
				// DRlib_ABUS_CFF3000 highly depends on timers, which have to be checked while the Remote is active!
}
 
