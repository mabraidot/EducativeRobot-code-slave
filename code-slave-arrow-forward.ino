/**
Slave coding block:
-------------------
Block:    FORDWARD ARROW
Address   001
*/

#define I2C_SLAVE_ADDRESS 0x2F
#define LED_PIN 4
#include <TinyWireS.h>
// The default buffer size, Can't recall the scope of defines right now
#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

volatile uint8_t i2c_regs[] =
{
    0x0,        // Discovered
    0x0,        // Flash the LED
};
volatile byte reg_position;
const byte reg_size = sizeof(i2c_regs);


void setup()
{
  TinyWireS.begin(I2C_SLAVE_ADDRESS);   // Join i2c network
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
  
  pinMode(4, OUTPUT);                   // Status LED
}

// Gets called when the ATtiny receives an i2c command
// First byte is the reg address, next is the value
void receiveEvent(uint8_t howMany)
{
  if (howMany < 1)
  {
      // Sanity-check
      return;
  }
  if (howMany > TWI_RX_BUFFER_SIZE)
  {
      // Also insane number
      return;
  }

  reg_position = TinyWireS.receive();
  howMany--;
  if (!howMany)
  {
      // This write was only to set the buffer for next read
      return;
  }
  while(howMany--)
  {
      i2c_regs[reg_position] = TinyWireS.receive();
      reg_position++;
      if (reg_position >= reg_size)
      {
          reg_position = 0;
      }
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

  blink_led(4);
}

void blink_led(byte howMany)
{

  if(i2c_regs[1])
  {
    static byte howManyFlashes = howMany;
    static byte led_on = 1;
    static int blink_interval = 500;
    static unsigned long blink_timeout = millis() + blink_interval;
    
    if(howManyFlashes > 0)
    {
      if(blink_timeout < millis()){
        led_on = !led_on;
        blink_timeout = millis() + blink_interval;
        if(!led_on)
        {
          howManyFlashes--;
        }
      }
      digitalWrite(LED_PIN, led_on);
    }else{
      digitalWrite(LED_PIN, LOW);
      i2c_regs[1] = 0;
      howManyFlashes = howMany;
    }
  }
}

