/*
  Written 2021-01-23 by waka
  
  This Library controls an ABUS door remote (ABUS HomeTec Pro Remote CFF3000).
  It uses a relay module to short out both ends of each switch electrically (necessary!).
  I did not manage to use an Opto-Coupler yet, but I suspect it won't work, as the needs to be an actual short with that switch.
  I *REALLY* would like to have a word with the circuit designer why the heck he chose to control those 2 LEDs using 1 signal only.

  The Pin "LEDsignal" monitors the point behind the Status LED, a bigger hole, which can be used to read the Status events.
  Finally, 3.3V and Ground go to the battery compartment. Be careful to not short this out!
  Use Hot Glue to fix wires straight after soldering, they tend to rip off easily. Solder from below, use the points you measure out as direct connections to each part.
  
  Simple click switches are used on 2,3,4 as inputs. 2 = open, 3 = close, 4 = status request
  9 = open-button relay
  10 = close-button relay
  
  This code relies on an Arduino Uno. If you use a different board, ADJUST THE COUNTER VALUES OF LED_blink_timer!!! Detection WILL fail otherwise!

  Here is an **approximation** of how to connect the wires. I lost the description of the other connections and used hot glue to fix the fragile soldering points.


               + Switch Open (part one, find the other half of it and connect to that
               |
+-------------------------------------------------------------------------------------------------------------+
|              |                                                                                              |
|  +-+   +-+   |                                XXXX                                                          |
|  +-+       X<+                            XXXX   XXXXX                                                      |
|                                          XX          XXX                                                    |
|                                          X              XXX                                                 |
|  +-+                                    X                  XX                                   +----+      |
|  +-+                                   XX                    XX                                 +----+ <----------+  Red LED
|                                        X                      X                                             |
|                    SW open             X       +---+           X+-+        SW close                         |
|  +-+              +-------+            X       |GND|           X| |      +-----+                            |
|  +-+              |  |-|  |            X       +---+          X +-+      | |-| |                            |
|                   +-------+            X                     X           +-----+                            |
|                                        X                    X+-+                     +-+                    |
|  +-+                                    X                   X| |  X<--+              +-+ <-+                |
|  +-+                                    XX                 XX+-+      |                    |                |
|                                          XX               X           |                    |    +----+      |
|                                           XXXX          XXX           |                    |    +----+  <--------+  Green LED
|  +-+                                          XXXXXXXXX X             |                    |                |
|  +-+ <---+                                                            |                    |                |
|          |                                                            |                    |                |
+-------------------------------------------------------------------------------------------------------------+
           |                                                            |                    |
           |                                                            |                    |
           + Positive 3.3V is fine, straight from the Arduino           |                    |
                                                                        |                    + LED signal, it's the hole slightly bigger than those tiny ones.
                                                                        + Switch Close (part one, find the other half of the switch and connect to that)




*/
#ifndef DRlib_ABUS_CFF3000_h
#define DRlib_ABUS_CFF3000_h

#include "Arduino.h"

class DRlib_ABUS_CFF3000
{
	public: 
		DRlib_ABUS_CFF3000();
		void initialize();
		uint8_t DR_status();
		uint8_t DR_status_lasttime();
		void status_output_to_serial();
		void request_get_doorlock_status();
		void request_close_the_doorlock();
		void request_open_the_doorlock();
		void run_this_continuously();
		void pin_relais_open(int pin);			//optional
		void pin_relais_close(int pin);			//optional
		void pin_status_led(int pin);			//optional
	private:
		void DR_operation();
		void DR_to_standby();
		void DR_activate();
		void tuerschloss_oeffnen();
		void tuerschloss_schliessen();
		void tuerschloss_status_abfragen();

		// Door-Remote LED Input
		int _LEDsignal = A0;
};

#endif
