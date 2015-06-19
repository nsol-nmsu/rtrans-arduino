#ifndef _rtrans_h_
#define _rtrans_h_

#include "XBee.h"
#include "ringbuffer.h"
#include "SoftwareSerial.h"
#include <stdint.h>

/* Retransmit limit and timeout */
#define RTRANS_RETX_LIMIT       (4)
#define RTRANS_RETX_TIMEOUT     (200)

/* Packet and window sizes */
#define RTRANS_PACKET_SIZE      (100)
#define RTRANS_PAYLOAD_SIZE     (RTRANS_PACKET_SIZE - sizeof(rt_out_header) - 1)
#define RTRANS_ABBREV_SIZE      (RTRANS_PAYLOAD_SIZE + sizeof(rt_in_header))
#define RTRANS_MAX_SEGMENTS     (6)
#define RTRANS_PACKET_BUFFER    (RTRANS_MAX_SEGMENTS * RTRANS_PACKET_SIZE)
#define RTRANS_ABBREV_BUFFER    (RTRANS_ABBREV_SIZE * 2)

/* Special uuid for no master configured */
#define RTRANS_NO_MASTER        (0xffff)

/* Packet types */
#define RTRANS_TYPE_PROBE       (0)   // from master to slave only - broadcast message to detect slaves
#define RTRANS_TYPE_JOIN        (1)   // from slave to master only - response to 
#define RTRANS_TYPE_POLL        (2)   // from master to slave only - request for data
#define RTRANS_TYPE_DATA        (3)   // from slave to master only - response containing sensing data
#define RTRANS_TYPE_SET         (4)   // from master to slave only - set control params
#define RTRANS_TYPE_ERR         (5)   // from slave to master only - report high-priority hardware error
#define RTRANS_TYPE_ACK         (254) // general acknowledgment pkt - confirm join, ack data, ack set
#define RTRANS_TYPE_NAK         (255) // negative acknowledgment - refuse join, retx request, error

/* Outgoing packet header */
typedef struct __attribute__ ((__packed__)) rt_out_header_s {
    uint16_t master;    // master mac
    uint16_t slave;     // slave mac
    uint16_t pkg_no;    // package number
    uint8_t  type;      // message type
    uint8_t  seg_ct;    // number of segments in package
    uint8_t  seg_no;    // segment number
    uint8_t  len;       // payload length
} rt_out_header;

/* Incoming packet header */
typedef struct __attribute__ ((__packed__)) rt_in_header_s {
    uint16_t master;    // master mac
    uint16_t slave;     // slave mac
    uint8_t  type;      // message type
    uint8_t  len;       // payload length
} rt_in_header;

/* Callback function */
typedef void (*rt_callback)(rt_in_header *header, uint8_t payload[]);

/* Driver state and data */
class rt_state {

private:
        XBee          xbee;
        uint16_t      slave;
        uint16_t      master;
        rt_callback   rx_callback;
        uint8_t       retx_ct;
        uint8_t       tx_pkg_no;
        bool          tx_waiting;
        uint8_t       tx_wait_pkg;
        uint8_t       tx_wait_seg;
        unsigned long tx_timeout;
        ringbuffer    tx_queue;
        ringbuffer    rx_queue;
        uint8_t       rtrans_tx_buffer[RTRANS_PACKET_BUFFER];
        uint8_t       rtrans_rx_buffer[RTRANS_ABBREV_BUFFER];
        
        static unsigned long rt_time();
        void rt_fsm_event(uint8_t type, const void *data);
        void rt_queue_incoming(const rt_out_header *pkt);
        void rt_handle_incoming(const unsigned char *data);
        void rt_tx_queued();
        uint8_t rt_read_incoming(unsigned char *at_buffer, size_t at_len);
        void rt_rx_pop();
        void rt_check_timeouts();
        void rt_send_now(const rt_out_header *pkt);
        
        
public:
        rt_state(SoftwareSerial &xs, rt_callback cb_func);
        void rt_init();
        void rt_loop();
        size_t rt_send(uint8_t type, const uint8_t *payload, size_t length);
        void rt_join(uint16_t addr);
        
};

#endif
