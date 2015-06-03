#ifndef _xbee_init_h_
#define _xbee_init_h_

#include <Arduino.h>
#include <SoftwareSerial.h>

/** Send a plain AT mode command to the xbee. Returns true
    if the OK\r response was received, false otherwise. Before
    sending AT commands, be sure to send the +++ sequence.
*/
bool xbee_command(SoftwareSerial &xs, const char command[]);

/** Sets channel, PAN ID, and API mode */
bool xbee_init(SoftwareSerial &xs);

#endif
