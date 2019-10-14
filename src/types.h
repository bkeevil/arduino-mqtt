#ifndef MQTT_TYPES_H
#define MQTT_TYPES_H

namespace mqtt {
  
  enum ErrorCode {NONE=0,ALREADY_CONNECTED=101,NOT_CONNECTED,INSUFFICIENT_DATA,REMAINING_LENGTH_ENCODING,INVALID_PACKET_FLAGS,
                  PACKET_INVALID,PAYLOAD_INVALID,VARHEADER_INVALID,UNACCEPTABLE_PROTOCOL,CLIENTID_REJECTED,SERVER_UNAVAILABLE,
                  BAD_USERNAME_PASSWORD,NOT_AUTHORIZED,NO_CLIENTID,WILLMESSAGE_INVALID,NO_PING_RESPONSE,UNHANDLED_PACKETTYPE,
                  NO_SUBSCRIPTION_LIST,INVALID_SUBSCRIPTION_ENTRIES,INVALID_RETURN_CODES,CONNECT_TIMEOUT,NOT_IMPLEMENTED,
                  PACKET_QUEUE_FULL,PACKETID_NOT_FOUND,SEND_PUBCOMP_FAILED,SEND_PUBREL_FAILED,PACKET_QUEUE_TIMEOUT,UNKNOWN=255};

  /** Quality of Service Levels */
  enum QoS {
    AT_MOST_ONCE = 0, /**< The packet is sent once and may or may not be received by the server */
    AT_LEAST_ONCE,    /**< The packet is acknowledge by the server but may be sent by the client more than once */
    EXACTLY_ONCE,     /**< Delivery of the packet exactly once is guaranteed using multiple acknowledgements */
    MAX_VALUE         
  };

};

#endif