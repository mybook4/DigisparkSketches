
#include "I2CBitBangWriter.h"
#include <util/delay.h>

// Initialize statics
uint8_t I2CBB::I2CBB_Buffer[I2CBB_BUF_SIZE];             // Buffer to hold I2C data
uint8_t I2CBB::I2CBB_BufferIndex = 0;                    // The current index value into the buffer


I2CBB::I2CBB() {
  // nothing to construct
}


// Public interface functions

void I2CBB::initialize(){ 
  // initialize SDA and SCL lines (release them by setting DDR to 0 (input), and PORT to 0 (no pullup))
  
  DDR_I2CBB &= ~(1<<SDA_BIT); // Set SDA as an input
  DDR_I2CBB &= ~(1<<SCL_BIT); // Set SCL as an input

  PORT_I2CBB &= ~(1<<SDA_BIT); // Disable internal pullup resistor
  PORT_I2CBB &= ~(1<<SCL_BIT); // Disable internal pullup resistor
}



void I2CBB::setSlaveAddress(uint8_t slaveAddr) { 
  // setup address & write bit.  store in buffer
  I2CBB_BufferIndex = 0; 
  I2CBB_Buffer[I2CBB_BufferIndex] = (slaveAddr << 1) | I2CBB_SEND; 
}

void I2CBB::addByteForTransmission(uint8_t data) { 
  // stores data to send later
  if (I2CBB_BufferIndex >= I2CBB_BUF_SIZE) {
    return; // return to avoid exceeding buffer size
  }

  I2CBB_BufferIndex++; // increment for the next byte in buffer
  I2CBB_Buffer[I2CBB_BufferIndex] = data;
}

void I2CBB::addBytesForTransmission(uint8_t* buffer, uint8_t bufferSize) {
  for(int i=0; i < bufferSize; i++)
  {
    addByteForTransmission(buffer[i]);
  } 
}



bool I2CBB::transmitData(){ 
  // actually sends the buffer
  bool success = sendDataOverI2c(I2CBB_Buffer, I2CBB_BufferIndex + 1);
  I2CBB_BufferIndex = 0;
  return success;
}



// Private functions

void I2CBB::sendI2cStartSignal() {
  /*
    Set SDA low
    Wait 10.5us
    Set SCL low
    Wait 10.5us
    return;
  */
  
  DDR_I2CBB |= (1<<SDA_BIT); // set SDA low
  _delay_us(10);
  DDR_I2CBB |= (1<<SCL_BIT); // set SCL low
  _delay_us(10);

  return;
}


bool I2CBB::sendI2cByte(uint8_t dataByte) {
    
  /*
    Loop 8 times to send 8 bits
      Set SDA to bit
      Wait 1us
      Release SCL
      Wait 2.5us
      Set SCL low
      Wait 6us

    Relase SDA
    Wait 1us
    Make SDA an input
    Release SCL
    Read the value of SDA
    Wait 3.5us
    if(SDA was NACK) return false;
    Set SCL low
    Wait 1us
    Make SDA an output (released)
    Wait 15us
    return true;
  */
  
  // send each bit, MSB to LSB
  uint8_t mask = 0x80;
  for(int i = 0; i < 8; i++) {
    if(dataByte & mask) {
      // 1
      DDR_I2CBB &= ~(1<<SDA_BIT); // release SDA
    } else {
      // 0 
      DDR_I2CBB |= (1<<SDA_BIT); // set SDA low
    }
    _delay_us(1);
    DDR_I2CBB &= ~(1<<SCL_BIT); // release SCL
    _delay_us(3);
    DDR_I2CBB |= (1<<SCL_BIT); // set SCL low
    _delay_us(6);
    
    mask = mask >> 1;
  }
  
  DDR_I2CBB &= ~(1<<SDA_BIT); // release SDA
  _delay_us(1);
  // make SDA an input (it already is)
  DDR_I2CBB &= ~(1<<SCL_BIT); // release SCL
  // read the value of SDA
  _delay_us(3);
  
  if( PIN_I2CBB & (1<<SDA_BIT) ) { // received a NACK
    // SDA and SCL remain released
    return false;
  }
  
  // received an ACK
  DDR_I2CBB |= (1<<SCL_BIT); // set SCL low
  _delay_us(1);
  // SDA remains released
  _delay_us(15);

  return true;
}


void I2CBB::sendI2cStopSignal() {
  /*
    Set SDA low
    Wait 10.5us
    Release SCL
    Wait 11.5us
    Relase SDA
    Wait 50us
    return;
  */
  
  DDR_I2CBB |= (1<<SDA_BIT); // set SDA low
  _delay_us(10);
  DDR_I2CBB &= ~(1<<SCL_BIT);  // release SCL
  _delay_us(11);
  DDR_I2CBB &= ~(1<<SDA_BIT);  // release SDA
  _delay_us(50);

  return;
}


bool I2CBB::sendDataOverI2c(uint8_t* buffer, uint8_t bufferSize) {

  sendI2cStartSignal();

  for(uint8_t i = 0; i < bufferSize; i++) {
    if(!sendI2cByte(buffer[i])) {
      return false;
    }
  }

  sendI2cStopSignal();

  return true;
}





// Preinstantiate an object for ease of use

I2CBB I2CBitBangWriter = I2CBB();

