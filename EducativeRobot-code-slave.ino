/**
Slave coding block:
-------------------
*/

#include <EEPROM.h>
#include <TinyWireS.h>


#define LED_PIN 3
#define GATE_PIN 4


// The default buffer size
#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

byte i2c_slave_address = 0x01;

volatile uint8_t i2c_regs[] =
{
    0,        // Set new I2C address
    0,        // Activate to any child slave
    0         // Flash the LED
};
volatile byte reg_position = 0;
const byte reg_size = sizeof(i2c_regs);

// Needed for software reset
void(* resetFunc) (void) = 0;

void setup()
{

  pinMode(LED_PIN, OUTPUT);             // Status LED
  pinMode(GATE_PIN, OUTPUT);            // Status GATE for child slave

  TinyWireS.begin(getAddress());        // Join i2c network
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
  
}

// Slave address is stored at EEPROM address 0x00
byte getAddress() {
  byte i2c_new_address = EEPROM.read(0x00);
  if (i2c_new_address == 0x00) { 
    i2c_new_address = i2c_slave_address; 
  }else if(i2c_new_address != i2c_slave_address){
    // Write back the original placeholder address, in case of an unplug
    EEPROM.write(0x00, i2c_slave_address);
    activate_child();
  }

  return i2c_new_address;
}

// Gets called when the ATtiny receives an i2c command
// First byte is the starting reg address, next is the value
void receiveEvent(uint8_t howMany)
{
  if (howMany < 1)
  {
      // Sanity-check
      reg_position = 0;
      return;
  }
  if (howMany > TWI_RX_BUFFER_SIZE)
  {
      // Also insane number
      reg_position = 0;
      return;
  }

  reg_position = TinyWireS.receive();
  howMany--;
  if (!howMany)
  {
      // This write was only to set the buffer for next read
      reg_position = 0;
      return;
  }
  while(howMany--)
  {
      i2c_regs[reg_position] = TinyWireS.receive();
      // reset position
      reg_position = 0;
      /*reg_position++;
      if (reg_position >= reg_size)
      {
          reg_position = 0;
      }*/
  }
}


// Gets called when the ATtiny receives an i2c request
void requestEvent()
{
  TinyWireS.send(i2c_regs[reg_position]);
  // Increment the reg position on each read, and loop back to zero
  reg_position++;
  if (reg_position >= reg_size)
  {
    reg_position = 0;
  }
}



void loop()
{
  ////////////////////////////////////
  // This needs to be here
  TinyWireS_stop_check();
  ////////////////////////////////////

  set_new_address();
  blink_led();
  activate_child();
}

void blink_led()
{
  if(i2c_regs[2])
  {
    static byte led_on = 1;
    static int blink_interval = 500;
    static unsigned long blink_timeout = millis() + blink_interval;
    
    if(blink_timeout < millis()){
      led_on = !led_on;
      blink_timeout = millis() + blink_interval;
      if(!led_on)
      {
        i2c_regs[2]--;
      }
    }
    digitalWrite(LED_PIN, led_on);
    
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
  }

}

void activate_child()
{
  if(i2c_regs[1])
  {
    digitalWrite(GATE_PIN, HIGH);
  }
  else
  {
    digitalWrite(GATE_PIN, LOW);
  }
}


void set_new_address()
{
  if(i2c_regs[0])
  {
    //write EEPROM and reset
    EEPROM.write(0x00, i2c_regs[0]);
    i2c_regs[0] = 0;
    resetFunc();  
  }
}