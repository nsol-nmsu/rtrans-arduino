#include "rtrans.h"
#include "hex.h"

/** Get current timestamp, postponing overflow which will break the heap.
    Arduino will natively give us time in milliseconds, but we don't need
    that much precision. We only need tenths of a second. An unsigned long
    is 32 bits and would overflow in about 50 days. This routine extends the
    overflow period to 5000 days, or almost 14 years. After that amount of time,
    it is likely that everything will break.
*/
unsigned long rt_state::rt_time(){
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
void rt_state::rt_fsm_event(uint8_t type, const void *data){
        rt_out_header pkt;
        rt_out_header *p = (rt_out_header *) data;
        
        // check that this is the ack/nak we are waiting for
        if(p->pkg_no != this->tx_wait_pkg || p->seg_no != this->tx_wait_seg){
          return;
        }
        
        // handle the event
        switch(type){
                case RTRANS_TYPE_ACK: {
                        // we are no longer waiting for an ACK
                        this->tx_waiting = false;
                        
                        // remove the segment from the transmit queue
                        rb_get(&this->tx_queue, (uint8_t *) &pkt, sizeof(rt_out_header));
                        rb_del(&this->tx_queue, pkt.len);
                        
                        break;
                }
                case RTRANS_TYPE_NAK: {
                        // we are no longer waiting for an ACK
                        this->tx_waiting = false;
                        
                        // remove remaining segments of the package
                        do {
                          rb_get(&this->tx_queue, (uint8_t *) &pkt, sizeof(rt_out_header));
                          rb_del(&this->tx_queue, pkt.len);
                          rb_peek(&this->tx_queue, (uint8_t *) &pkt, sizeof(rt_out_header));
                        } while(pkt.pkg_no == this->tx_wait_pkg);
                        
                        break;
                }
        }
}

/** Add an event to the callback queue */
void rt_state::rt_queue_incoming(const rt_out_header *pkt){
        // Build abbreviated header
        rt_in_header  hdr_tmp = {
                .master = pkt->master,
                .slave  = pkt->slave,
                .type   = pkt->type,
                .len    = pkt->len
        };

        // Copy header to ringbuffer
        if(rb_put(&this->rx_queue, (const uint8_t *) &hdr_tmp, sizeof(rt_in_header)) != sizeof(rt_in_header)){
                // TODO: handle error (rx queue full)
        }
        
        // Copy payload to ringbuffer
        if(rb_put(&this->rx_queue, ((const uint8_t *) &hdr_tmp) + sizeof(rt_out_header), pkt->len) != pkt->len){
                // TODO: handle error (rx queue full)
        }
}

/** Process incoming packet */
void rt_state::rt_handle_incoming(const unsigned char *data){
        const rt_out_header *pkt = (const rt_out_header *) data;
        uint8_t i, acc = 0;
        
        // verify checksum
        for(i = 0; i < sizeof(rt_out_header) + pkt->len + 1; i++){
                acc += data[i];
        }
        
        if(acc != 0xff){
                // TODO: report error (bad checksum)
                return;
        }
        
        // handle packet
        switch(pkt->type){
                case RTRANS_TYPE_PROBE:
                case RTRANS_TYPE_POLL:
                case RTRANS_TYPE_SET:
                        /* Just queue the packet to be passed to the callback */
                        rt_queue_incoming(pkt);
                        break;
                        
                case RTRANS_TYPE_ACK:
                case RTRANS_TYPE_NAK:
                        /* Pass event to the FSM */
                        rt_fsm_event(pkt->type, pkt);
                        break;
        }
}

/** Transmit the segment at the front of the tx queue */
void rt_state::rt_tx_queued(){
        uint8_t pkt[RTRANS_PACKET_SIZE];
        rt_out_header *hdr = (rt_out_header *) pkt;

        // peek the message - we don't want to remove it from the buffer yet
        rb_peek(&this->tx_queue, &pkt[0], sizeof(rt_out_header));
        rb_peek(&this->tx_queue, &pkt[sizeof(rt_out_header)], hdr->len + 1);

        // some housekeeping - increment retx count and set the timeout
        ++this->retx_ct;
        this->tx_timeout = rt_time() + RTRANS_RETX_TIMEOUT / 10;
        
        // send the segment
        Tx16Request tx = Tx16Request(hdr->master, pkt, sizeof(rt_out_header) + hdr->len + 1);
        this->xbee.send(tx);
}

/** Read from the XBee serial interface
    Returns:
      0 if no packet was received
      1 for a radio packet
      2 for an AT response packet
*/
uint8_t rt_state::rt_read_incoming(unsigned char *at_buffer, size_t at_len){

        this->xbee.readPacket();
  
        if(this->xbee.getResponse().isAvailable()) {
                if(this->xbee.getResponse().getApiId() == RX_16_RESPONSE){
                        /* rx data */
                        Rx16Response rx16 = Rx16Response();
                        this->xbee.getResponse().getRx16Response(rx16);
                        rt_handle_incoming(rx16.getData());          
                        return 1;
                }
                else if(this->xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE){
                        /* AT response */
                        AtCommandResponse atResponse = AtCommandResponse();
                        this->xbee.getResponse().getAtCommandResponse(atResponse);
                        if(at_buffer != 0 && at_len != 0){
                                memcpy(at_buffer, atResponse.getValue(), at_len);
                        }
                        return 2;
                }
        }
        
        return 0;

}

/** If there is a packet waiting in the rx queue,
    pop it off and run the callback
*/
void rt_state::rt_rx_pop(){
        rt_in_header pkt;
        uint8_t cb_buffer[RTRANS_PAYLOAD_SIZE];

        /* we need at least one header in the ringbuffer */
        if(this->rx_queue.avail >= sizeof(rt_in_header)){
                /* get the header */
                rb_get(&this->rx_queue, (uint8_t *) &pkt, sizeof(rt_in_header));
                
                /* check that the payload is available */
                if(this->rx_queue.avail >= pkt.len){
                        /* copy the payload */
                        rb_get(&this->rx_queue, cb_buffer, pkt.len);
                        
                        /* run the callback*/
                        this->rx_callback(&pkt, cb_buffer);
                }
                else{
                        // TODO: handle error (ringbuffer underflow)
                }
        }
}

/** Initializes the XBee and the rtrans driver
    Params:
      rt_serial: the SoftwareSerial object to use to communicate with the XBee
      cb_func:   the callback function which will receive PROBE/POLL/SET events
    Returns:
      A pointer to the initialized rt_state object
*/
rt_state::rt_state(XBee &xbee, rt_callback cb_func) : xbee(xbee){
        this->rx_callback = cb_func;
        this->master = RTRANS_NO_MASTER;
        rb_init(&this->tx_queue, rtrans_tx_buffer, RTRANS_PACKET_BUFFER);
        rb_init(&this->rx_queue, rtrans_rx_buffer, RTRANS_PACKET_BUFFER);
}

void rt_state::rt_init(){
        uint8_t at_response[8];
        uint8_t at_cmd_sl[2] = {'S', 'L'};
        uint8_t at_cmd_my[2] = {'M', 'Y'};
        
        /* Ask the XBee for our address */
        AtCommandRequest at_cmd_sl_req = AtCommandRequest(at_cmd_sl);
        this->xbee.send(at_cmd_sl_req);
        
        /* Wait for the AT response */
        while(rt_read_incoming(at_response, 8) != 2);
        
        /* Assign the 16-bit address */
        this->slave = at_response[3] | (at_response[2] << 8);
        AtCommandRequest at_cmd_my_req = AtCommandRequest(at_cmd_my, &at_response[2], 2);
        this->xbee.send(at_cmd_my_req);
        while(rt_read_incoming(0, 0) != 2);
}

/** Checks if a timeout has occurred; if so, either retransmits the packet
    or generates a NAK event to clear the package from our buffers depending
    on whether it has met the retx count threshold.
*/
void rt_state::rt_check_timeouts(){
        rt_out_header dummy;
        if(this->tx_waiting && rt_time() > this->tx_timeout){
                if(++this->retx_ct > RTRANS_RETX_LIMIT){
                        // cancel the rest of the package (handle a dummy NAK)
                        dummy.pkg_no = this->tx_wait_pkg;
                        dummy.seg_no = this->tx_wait_seg;
                        rt_fsm_event(RTRANS_TYPE_NAK, &dummy);
                }
                else{
                        rt_tx_queued();
                }
        }
}

/** Handles all of the processing of the rtrans driver. You should be calling
    this function once in the main arduino loop() subroutine.
*/
void rt_state::rt_loop(void){
        rt_read_incoming(0, 0);
        rt_check_timeouts();
        if(!this->tx_waiting && this->tx_queue.avail > 0){
                this->retx_ct = 0;
                rt_tx_queued();
        }
        rt_rx_pop();
        
}

/** Adds a new package to the transmit queue. Returns the total number of
    segments to be transmitted, or 0 if the package can't be transmitted.
*/
size_t rt_state::rt_send(uint8_t type, const uint8_t *payload, size_t length){
        rt_out_header h;
        size_t j, i = length, expected_segments = 0;
        
        /* Calculate how many segments we will need (could do this better if the atmega had a FPU,
           probably still can do it better but I don't feel like figuring it out)
        */
        do{
            i -= (i > RTRANS_PAYLOAD_SIZE) ? RTRANS_PAYLOAD_SIZE : i;
            ++expected_segments;
        } while(i > 0);
        
        /* Calculate how much space we need in the buffer */
        i = length + expected_segments * (sizeof(rt_out_header) + 1);
        
        /* Make sure that (1) the segment count is within the maximum, and (2) there is enough free space */
        if(expected_segments > RTRANS_MAX_SEGMENTS || i > rb_free(&this->tx_queue)){
            return 0;
        }
        
        /* Prepare the header */
        h.master = this->master;
        h.slave  = this->slave;
        h.pkg_no = this->tx_pkg_no++;
        h.type   = type;
        h.seg_ct = expected_segments;
        
        /* Construct segments and put them in the buffer */
        for(i = 0; i < expected_segments; i++){
             uint8_t cs = 0xff;
          
             /* Segment-specific header fields */
             h.len = (length > RTRANS_PAYLOAD_SIZE) ? RTRANS_PAYLOAD_SIZE : length;
             h.seg_no = i;
             
             /* Calculate checksum (header part) */
             for(j = 0; j < sizeof(rt_out_header); j++){
                 cs -= ((uint8_t *) &h)[j];
             }
             
             /* Calculate checksum (payload part) */
             for(j = 0; j < h.len; j++){
                 cs -= payload[(i * RTRANS_PAYLOAD_SIZE) + j];
             }
             
             /* Put it all in the queue */
             rb_put(&this->tx_queue, (uint8_t *) &h, sizeof(rt_out_header));
             rb_put(&this->tx_queue, &payload[i * RTRANS_PAYLOAD_SIZE], h.len);
             rb_put(&this->tx_queue, &cs, sizeof(uint8_t));
             
             length -= h.len;
        }
        
        return expected_segments;
}

void rt_state::rt_join(uint16_t addr){
        this->master = addr;
        rt_send(RTRANS_TYPE_JOIN, 0, 0);
}
