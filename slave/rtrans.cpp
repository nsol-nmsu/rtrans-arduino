#include "rtrans.h"
#include "ringbuffer.h"

/* Local symbols */
rt_state rtrans_state;
uint8_t rtrans_tx_buffer[RTRANS_PACKET_BUFFER];
uint8_t rtrans_rx_buffer[RTRANS_PACKET_BUFFER];

/** Get current timestamp, postponing overflow which will break the heap.
    Arduino will natively give us time in milliseconds, but we don't need
    that much precision. We only need tenths of a second. An unsigned long
    is 32 bits and would overflow in about 50 days. This routine extends the
    overflow period to 5000 days, or almost 14 years. After that amount of time,
    it is likely that everything will break.
*/
unsigned long rt_time(){
        static unsigned long last_time = 0;
        static unsigned long offset = 0;
        unsigned long cur_time = millis() / 100;
        if(cur_time < last_time){
                offset += ((unsigned long) -1) / 100;
        }
        last_time = cur_time;
        return cur_time + offset;
}

/** Handle an event which affects the state machine */
void rt_fsm_event(uint8_t type, const void *data){
        switch(type){
                case RTRANS_TYPE_ACK:
                        // TODO: clear the tx_waiting flag and advance the ringbuffer
                        break;
                case RTRANS_TYPE_NAK:
                        // TODO: clear tx_waiting and advance the ringbuffer to the end of the package
                        break;
        }
}

/** Send an ACK (immediate) */
void rt_ack_packet(const rt_out_header *pkt){
        // TODO
}

/** Add an event to the callback queue */
void rt_queue_incoming(const rt_out_header *pkt){
        // Build abbreviated header
        rt_in_header  hdr_tmp = {
                .master = pkt->master,
                .slave  = pkt->slave,
                .type   = pkt->type,
                .len    = pkt->len
        };

        // Copy header to ringbuffer
        if(rb_put(&rtrans_state.rx_queue, (const uint8_t *) &hdr_tmp, sizeof(rt_in_header)) != sizeof(rt_in_header)){
                // TODO: handle error (rx queue full)
        }
        
        // Copy payload to ringbuffer
        if(rb_put(&rtrans_state.rx_queue, ((const uint8_t *) &hdr_tmp) + sizeof(rt_out_header), pkt->len) != pkt->len){
                // TODO: handle error (rx queue full)
        }
}

/** Process incoming packet */
void rt_handle_incoming(const unsigned char *data){
        const rt_out_header *pkt = (const rt_out_header *) data;
        switch(pkt->type){
                case RTRANS_TYPE_PROBE:
                case RTRANS_TYPE_POLL:
                case RTRANS_TYPE_SET:
                        /* Send an ACK then queue event to be passed to callback */
                        rt_ack_packet(pkt);
                        rt_queue_incoming(pkt);
                        break;
                        
                case RTRANS_TYPE_ACK:
                case RTRANS_TYPE_NAK:
                        /* Pass event to the FSM */
                        rt_fsm_event(pkt->type, pkt);
                        break;
        }
}

/** Read from the XBee serial interface
    Returns:
      0 if no packet was received
      1 for a radio packet
      2 for an AT response packet
*/
uint8_t rt_read_incoming(unsigned char *at_buffer, size_t at_len){

        rtrans_state.xbee.readPacket();
        
        if(!rtrans_state.xbee.getResponse().isAvailable()) {
                /* no data */
		return 0;
	}
	else{
                if(rtrans_state.xbee.getResponse().getApiId() == RX_16_RESPONSE){
                        /* rx data */
                        Rx16Response rx16 = Rx16Response();
                        rtrans_state.xbee.getResponse().getRx16Response(rx16);
                        rt_handle_incoming(rx16.getData());          
      	                return 1;
                }
                else if(rtrans_state.xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE){
                        /* AT response */
                        AtCommandResponse atResponse = AtCommandResponse();
                        rtrans_state.xbee.getResponse().getAtCommandResponse(atResponse);
                        if(at_buffer != 0 && at_len != 0){
                                memcpy(at_buffer, atResponse.getValue(), at_len);
                        }
                        return 2;
                }
                else {
                        return 0; 
                }
        }
        
        
        return 0;

}

/** Initializes the XBee and the rtrans driver
    Params:
      rt_serial: the SoftwareSerial object to use to communicate with the XBee
      cb_func:   the callback function which will receive PROBE/POLL/SET events
    Returns:
      A pointer to the initialized rt_state object
*/
void rt_init(SoftwareSerial xbee_serial, rt_callback cb_func){
        uint8_t at_response[8];
        
        /* Nullify the struct */
        memset(&rtrans_state, 0, sizeof(rt_state));
        
        /* Initialize the XBee driver on the given serial interface */
        rtrans_state.xbee.setSerial(xbee_serial);
        
        /* Initialize buffers */
        rb_init(&rtrans_state.tx_queue, rtrans_tx_buffer, RTRANS_PACKET_BUFFER);
        rb_init(&rtrans_state.rx_queue, rtrans_rx_buffer, RTRANS_PACKET_BUFFER);
        
        /* Ask the XBee for our address */
        uint8_t at_cmd[] = {'M','Y'};
        AtCommandRequest at_req = AtCommandRequest(at_cmd); 
        rtrans_state.xbee.send(at_req);
        
        /* Wait for the AT response */
        while(rt_read_incoming(at_response, 8) != 2);
        
        // TODO: process the AT response

}

/** Checks if a timeout has occurred; if so, either retransmits the packet
    or generates a NAK event to clear the package from our buffers depending
    on whether it has met the retx count threshold.
*/
void rt_check_timeouts(){
        rt_out_header dummy;
        unsigned long cur_time = rt_time();
        if(rtrans_state.tx_waiting && cur_time > rtrans_state.tx_timeout){
                if(++rtrans_state.retx_ct > RTRANS_RETX_LIMIT){
                        // cancel the rest of the package (handle a dummy NAK)
                        dummy.pkg_no = rtrans_state.tx_wait_pkg;
                        dummy.seg_no = rtrans_state.tx_wait_seg;
                        rt_fsm_event(RTRANS_TYPE_NAK, &dummy);
                }
                else{
                        // TODO: retransmit packet
                }
        }
}

/** Handles all of the processing of the rtrans driver. You should be calling
    this function once in the main arduino loop() subroutine.
*/
void rt_loop(void){
        rt_read_incoming(0, 0);
        rt_check_timeouts();
        if(!rtrans_state.tx_waiting){
                // TODO: transmit next segment
        }
}
