#ifndef MQTT_NETWORK_H
#define MQTT_NETWORK_H

#include "Arduino.h"

namespace mqtt {

  /** @brief Provides utility methods for reading/writing MQTT protocol data to/from a Client object */
  class Network {
    public:
      /** @brief Creates a network wrapper around a client connection */
      Network(Client& client): client(client) {} 
    protected:
        /** @brief The network stream to read/write from */
      Client& client; 

      /** @brief  Reads a word from the stream in big endian order 
       *  @param  A pointer to a variable that will receive the outgoing word
       *  @return True iff two bytes were read from the stream */
      bool readWord(word* value);

      /** @brief  Writes a word to the stream in big endian order 
       *  @param  word  The word to write to the Stream
       *  @return bool  Returns true iff both bytes were successfully written to the stream */
      bool writeWord(const word value);

      /** @brief  Reads the remaining length field of an MQTT packet 
       *  @param  value Receives the remaining length
       *  @return True if successful, false otherwise
       *  @remark See the MQTT 3.1.1 specifications for the format of the remaining length field */
      bool readRemainingLength(long* value);

      /** @brief  Writes the remaining length field of an MQTT packet
       *  @param  value The remaining length to write
       *  @return True if successful, false otherwise 
       *  @remark See the MQTT 3.1.1 specification for the format of the remaining length field */
      bool writeRemainingLength(const long value);

      /** @brief    Writes a UTF8 string to the stream in the format required by the MQTT protocol */
      bool writeStr(const String& str);

      /** @brief    Reads a UTF8 string from the stream in the format required by the MQTT protocol */
      bool readStr(String& str);    

      friend class Packet;
  };

}

#endif