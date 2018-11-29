/**
Slave coding block:
-------------------
Block:    FORDWARD ARROW
Address   001
*/

#define I2C_SLAVE_ADDRESS 0x2F
#define LED_PIN 4
#define GATE_PIN 3
#include <TinyWireS.h>
// The default buffer size
#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

volatile uint8_t i2c_regs[] =
{
    0,        // Active/Discovered
    0,        // Open gate
    0         // Flash the LED
};
volatile byte reg_position = 0;
const byte reg_size = sizeof(i2c_regs);
volatile char discovered = 0;

void setup()
{
  TinyWireS.begin(I2C_SLAVE_ADDRESS);   // Join i2c network
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
  
  pinMode(LED_PIN, OUTPUT);             // Status LED
  pinMode(GATE_PIN, OUTPUT);            // Status GATE
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

  set_discovered();
  blink_led();
  open_gate();
}

void blink_led()
{
  if(discovered && i2c_regs[2])
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

void open_gate()
{
  if(discovered)         // Only if the block was already discovered
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
}


void set_discovered()
{
  if(i2c_regs[0])
  {
    discovered = 1;
  }
  else
  {
    discovered = 0;
    memset(i2c_regs,0,sizeof(i2c_regs));
  }
}