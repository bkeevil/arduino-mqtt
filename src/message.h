#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#include "Arduino.h"
#include "Print.h"
#include "Printable.h"

namespace mqtt {

  using MessageHandlerFunc = bool (*)(const Subscription& sub, const Message& msg);
  
  using DefaultMessageHandlerFunc = void (*)(const Message& msg);

  /** @brief    Represents an MQTT message that is sent or received 
   *  @details  Use the methods of the Print and Printable ancestor classes to access the message 
   *            data buffer. If the size of the data buffer is known, call reserve() to reserve 
   *            a specific memory size. Otherwise memory will be allocated in chunks of 
   *            clientConfiguration.msgAllocBlockSize bytes. Optionally, call pack() when done to free any 
   *            unused bytes.
   * @remark    The Print interface is used to write directly to the data buffer. 
   */
  class Message: public Printable, public Print {
    public:
      Message() = default;
      Message(const String& topic, QoS qos = QoS::AT_MOST_ONCE): topic(topic) {}
      Message(const String& topic, const String& data, QoS qos = QoS::AT_MOST_ONCE): topic(topic) { print(data); }
      Message(const String& topic, const byte* data, const size_t data_len, QoS qos = QoS::AT_MOST_ONCE): topic(topic) { write(data,data_len); }
      Message(const char* topic, QoS qos = QoS::AT_MOST_ONCE): topic(topic) {}
      Message(const char* topic, const char* data, QoS qos = QoS::AT_MOST_ONCE): topic(topic) { print(data); }
      Message(const char* topic, const byte* data, const size_t data_len, QoS qos = QoS::AT_MOST_ONCE): topic(topic) { write(data,data_len); }
      Message(const Message& msg);
      Message(Message&& msg);   

      /** @brief Prints the data buffer to any object descended from the Print class
       *  @param p The object to print to
       *  @return size_t The number of bytes printed */
      size_t printTo(Print& p) const override;

      /** @brief Reads a byte from the data buffer and advance to the next character
       *  @return int The byte read, or -1 if there is no more data */
      int read();
      
      /** @brief Read a byte from the data buffer without advancing to the next character
       *  @return int The byte read, or -1 if there is no more data */    
      int peek() const;
      
      /** @brief Writes a byte to the end of the data buffer
       *  @param c The byte to be written
       *  @return size_t The number of bytes written */
      size_t write(byte b) override;
      
      /** @brief Writes a block of data to the data buffer
       *  @param buffer The data to be written
       *  @param size The size of the data to be written
       *  @return size_t The number of bytes actually written to the buffer */
      size_t write(const byte* buffer, size_t size) override;
      
      /** @brief The number of bytes remaining to be read from the buffer */
      int available() const { return data_len - data_pos_; }
      
      /** @brief The number of bytes allocated but not written to */
      int availableForWrite() const /* override */ { return data_size_ - data_pos_; }  
      
      /** @brief Pre-allocate bytes in the data buffer to prevent reallocation and fragmentation
       *  @param size Number of bytes to reserve */
      void reserve(size_t size);
      
      /** @brief Call after writing the data buffer is done to free any extra reserved memory */
      void pack();
      
      /** @brief Set the index from which the next character will be read/written */
      void seek(int pos) { data_pos_ = pos; }

      /** @brief Returns true if data matches str **/
      bool dataEquals(const char* str) const;
      
      /** @brief Returns true if data matches str **/
      bool dataEquals(const String& str) const;
      
      /** @brief Returns true if data matches str **/
      bool dataEqualsIgnoreCase(const char* str) const;
      
      /** @brief Returns true if data matches str **/
      bool dataEqualsIgnoreCase(const String& str) const;

      byte* data() { return data_; }
      size_t data_len() { return data_len_; }
      Topic topic;                  /**< The message topic */
      QoS qos {QoS::AT_MOST_ONCE};  /**< The message QoS level */
      bool duplicate {false}; /**< Set to true if this message is a duplicate copy of a previous message because it has been resent */
      bool retain { false };  /**< For incoming messages, whether it is being sent because it is a retained message. For outgoing messages, tells the server to retain the message. */
      static const byte msgAllocBlockSize {8}; /**< Minimum additional RAM to allocate when the buffer needs to be enlarged. Reduces the number of calls to realloc() */
    private: 
      byte* data_;             /**< The data buffer */           
      size_t data_len_;        /**< The number of valid bytes in the data buffer. Might by < data_size*/
      size_t data_size_;       /**< The number of bytes allocated in the data buffer */
      size_t data_pos_;        /**< Index of the next byte to be written */
  };

  /** @brief Used for will message. Could also be used for connect message, or disconnect message */

  class SystemMessage : public Message {
    public:
      bool enabled {false};
  };

  class MessageList {

  }

}

#endif