#include <SoftwareSerial.h>
#include <XBee.h>
#include "rtrans.h"
#include "ringbuffer.h"
#include "xbee_init.h"

void callback(rt_in_header *header, uint8_t payload[]);
SoftwareSerial xs(6,7);
rt_state rtrans_state(xs, callback);

void callback(rt_in_header *header, uint8_t payload[]){
  static boolean joined = false;
  if(header->type == RTRANS_TYPE_POLL){
   Serial.print("POLL from ");
   Serial.print(header->master, HEX);
   Serial.println();
   rtrans_state.rt_send(RTRANS_TYPE_DATA, (const unsigned char *) "Hello world!", 12);
  }
  else if(header->type == RTRANS_TYPE_PROBE && !joined){
   Serial.print("PROBE from ");
   Serial.print(header->master, HEX);
   Serial.println();
   rtrans_state.rt_join(header->master);
   joined = true;
  }
}

void setup(){
  Serial.begin(9600);
  
  Serial.print("Initializing serial... ");
  xs.begin(9600);
  
  delay(1000);
  Serial.println("ok.");
  
  Serial.print("Initializing xbee... ");
  if(xbee_init(xs)){
    Serial.println("ok.");
  }
  else{
    Serial.println("fail."); 
  }
    
  Serial.print("Initializing rt... ");
  rtrans_state.rt_init();
  Serial.println("ok");
}

void loop(){
  rtrans_state.rt_loop();
}
