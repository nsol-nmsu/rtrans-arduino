#include "rtrans.h"

/* Local symbols */
rt_state rtrans_state;
uint8_t rtrans_tx_buffer[RTRANS_PAYLOAD_BUFFER];
uint8_t rtrans_rx_buffer[RTRANS_PAYLOAD_BUFFER];

/** Handle an event which affects the state machine */
void rt_fsm_event(uint8_t type, const void *data){
        // TODO
}

/** Add an event to the callback queue */
void rt_queue_incoming(const rt_out_header *pkt){
        // TODO
}

/** Process incoming packet */
void rt_handle_incoming(const unsigned char *data){
        const rt_out_header *pkt = (const rt_out_header *) data;
        switch(pkt->type){
                case RTRANS_TYPE_PROBE:
                case RTRANS_TYPE_POLL:
                case RTRANS_TYPE_SET:
                        /* Queue event to be passed to callback */
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
uint8_t rt_read_incoming(rt_state *state, unsigned char *at_buffer, size_t at_len){

        state->xbee.readPacket();
        
        if(!state->xbee.getResponse().isAvailable()) {
                /* no data */
		return 0;
	}
	else{
                if(state->xbee.getResponse().getApiId() == RX_16_RESPONSE){
                        /* rx data */
                        Rx16Response rx16 = Rx16Response();
                        state->xbee.getResponse().getRx16Response(rx16);
                        
                        rt_handle_incoming(rx16.getData());          
      	                return 1;
                }
                else if(state->xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE){
                        /* AT response */
                        AtCommandResponse atResponse = AtCommandResponse();
                        state->xbee.getResponse().getAtCommandResponse(atResponse);
                        memcpy(at_buffer, atResponse.getValue(), at_len);
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
    Returns:
      A pointer to the initialized rt_state object
*/
rt_state *rt_init(SoftwareSerial xbee_serial){
        uint8_t at_response[8];
        
        /* Initialize the XBee driver on the given serial interface */
        rtrans_state.xbee.setSerial(xbee_serial);
        
        /* Initialize buffers */
        rtrans_state.tx_queue = rb_init(rtrans_tx_buffer, RTRANS_PAYLOAD_BUFFER);
        rtrans_state.rx_queue = rb_init(rtrans_rx_buffer, RTRANS_PAYLOAD_BUFFER);
        
        /* Ask the XBee for our address */
        uint8_t at_cmd[] = {'M','Y'};
        AtCommandRequest at_req = AtCommandRequest(at_cmd); 
        rtrans_state.xbee.send(at_req);
        
        /* Wait for the AT response */
        while(rt_read_incoming(&rtrans_state, at_response, 8) != 2);
        
        // TODO: process the AT response
        
        return &rtrans_state;
}


