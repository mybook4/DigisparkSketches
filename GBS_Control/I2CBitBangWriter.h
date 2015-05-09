/*
  mybook4's I2C bit banging library for the Digispark Pro
  Only supports writing at the moment
  Developed specifically to write register values to the GBS 8220
  Timing is based on the timing observed with the GBS 8220's onboard microcontroller

  This is (somewhat) based off of the TinyWireM and USI_TWI_Master interface
*/

#ifndef I2CBitBangWriter_h
#define I2CBitBangWriter_h

#include <inttypes.h>
#include <Arduino.h>

#define I2CBB_SEND        0              // indicates sending
#define I2CBB_BUF_SIZE    33             // bytes in message buffer (holds a slave address and 32 bytes

#define DDR_I2CBB             DDRB
#define PORT_I2CBB            PORTB
#define PIN_I2CBB             PINB
#define SDA_BIT        0
#define SCL_BIT        2

#define I2C_ACK 0
#define I2C_NACK 1

class I2CBB
{	
  public:
    I2CBB();
    
    /*
      Normal interface usage pseudo code is as follows:
      initialize();
      setSlaveAddress(<7-bit address>);
      addByteForTransmission(<byte to send>);
      if(!transmitData())
        failed to send data
      else
        success
    
    */
    
    void initialize();
    void setSlaveAddress(uint8_t address); // 7-bit address
    
    // Adds a byte to an internally managed transmission buffer
    void addByteForTransmission(uint8_t data);
    
    // Adds a buffer to an internally managed transmission buffer
    void addBytesForTransmission(uint8_t* buffer, uint8_t bufferSize);
    
    // Once the 
    bool transmitData();

  private:
    static uint8_t I2CBB_Buffer[];           // holds I2C send data
    static uint8_t I2CBB_BufferIndex;        // current number of bytes in the send buff

    bool sendDataOverI2c(uint8_t* buffer, uint8_t bufferSize);
    void sendI2cStartSignal();
    bool sendI2cByte(uint8_t dataByte);
    void sendI2cStopSignal();
};

extern I2CBB I2CBitBangWriter;

#endif

