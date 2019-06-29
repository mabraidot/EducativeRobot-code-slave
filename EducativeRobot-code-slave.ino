/**
Slave coding block:
-------------------
*/
#include <Arduino.h>
#include <EEPROM.h>
#include <TinyWireS.h>


#define LED_PIN     3
#define GATE_PIN    4
#define RESET_PIN   0

// The default buffer size
#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

byte i2c_slave_address = 0x08;
/*
Slave function modes

MODE_FUNCTION               1
MODE_MODIFIER_LOOP          2
MODE_SLAVE_FORWARD_ARROW    3
MODE_SLAVE_BACKWARD_ARROW   4
MODE_SLAVE_LEFT_ARROW       5
MODE_SLAVE_RIGHT_ARROW      6
MODE_SLAVE_WAIT_LIGHT       7
MODE_SLAVE_WAIT_SOUND       8
*/
byte slave_function     = 3;

volatile uint8_t i2c_regs[] =
{
    0,              // 0: Set new I2C address
    0,              // 1: Activate to any child slave
    0,              // 2: Flash the LED
    0,              // 3: Activated block
    slave_function, // 4: Slave function
    0               // 5: Reserved feature
};
volatile byte reg_position = 0;
const byte reg_size = sizeof(i2c_regs);

// Needed for software reset
void(* resetFunc) (void) = 0;


void clear_eeprom(void){
  //EEPROM.write(0x00, 0);
  for(int i = 0; i < EEPROM.length(); i++){
    EEPROM.write(i, 0);
  }
}


/*
Rotate function to wear out the eeprom records evenly
*/
void write_eeprom(byte value){
  /*
  values    |0|1|x|0|0|0|0|
  records   |0|1|2|3|4|5|6|
  */
  int addr = 0;
  for(int i=0; i < EEPROM.length(); i++){
    if(EEPROM.read(i) > 0){
      addr = i;
      break;
    }
  }
  if(addr+1 >= EEPROM.length()){  // End of EEPROM, move to the begining
    EEPROM.write(0x01, value);
    EEPROM.write(0x00, 1);
    EEPROM.write(addr+1, 0);
  }else{  // Not end of EEPROM, so move to the next  set of addresses
    EEPROM.write(addr+2, value);
    EEPROM.write(addr+1, 1);
  }
  EEPROM.write(addr, 0);

}


byte read_eeprom(){
  /*
  values    |0|0|1|x|0|0|0|
  records   |0|1|2|3|4|5|6|
  */
  int addr = 0;
  for(int i=0; i < EEPROM.length(); i++){
    if(EEPROM.read(i) > 0){
      addr = i;
      break;
    }
  }
  return EEPROM.read(addr+1);

}


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
  //byte i2c_new_address = EEPROM.read(0x00);
  byte i2c_new_address = read_eeprom();
  if (i2c_new_address == 0x00) { 
    i2c_new_address = i2c_slave_address; 
  }else if(i2c_new_address != i2c_slave_address){
    // Write back the original placeholder address, in case of an unplug
    //EEPROM.write(0x00, i2c_slave_address);
    write_eeprom(i2c_slave_address);
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
  if(reg_position == 255){
    clear_eeprom();
  }
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
  if(i2c_regs[3]){
    TinyWireS.send(i2c_regs[reg_position]);
    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size)
    {
      reg_position = 0;
    }
  }
}



void loop()
{
  ////////////////////////////////////
  // This needs to be here
  TinyWireS_stop_check();
  ////////////////////////////////////

  set_new_address();
  led();
  activate_child();

  readReset();
}


void led()
{
  if(i2c_regs[3])
  {
    // Blink
    if(i2c_regs[2] == 2){
      static byte led_on = 1;
      static int blink_interval = 500;
      static unsigned long blink_timeout = millis() + blink_interval;
      
      if(blink_timeout < millis()){
        led_on = !led_on;
        blink_timeout = millis() + blink_interval;
      }
      digitalWrite(LED_PIN, led_on);
    }else if(i2c_regs[2] == 1){
      digitalWrite(LED_PIN, 1);
    }else{
      digitalWrite(LED_PIN, 0);
    }
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
    i2c_regs[2] = 0;
  }

}


void activate_child()
{
  if(i2c_regs[3] && i2c_regs[1])
  {
    digitalWrite(GATE_PIN, HIGH);
  }
  else
  {
    digitalWrite(GATE_PIN, LOW);
    i2c_regs[1] = 0;
  }
}


void set_new_address()
{
  if(i2c_regs[3] && i2c_regs[0])
  {
    //write EEPROM and reset
    //EEPROM.write(0x00, i2c_regs[0]);
    write_eeprom(i2c_regs[0]);
    i2c_regs[0] = 0;
    resetFunc();  
  }else{
    i2c_regs[0] = 0;
  }
}

void readReset(){
  static const unsigned int REFRESH_INTERVAL = 50; // ms 
  static unsigned long lastRefreshTime = 0;
  if(millis() - lastRefreshTime >= REFRESH_INTERVAL){
    lastRefreshTime = millis();
    if (analogRead(RESET_PIN) > 1000 ) {  // reset pin is near Vcc
      if(i2c_regs[3]){      // If it is soft resetting for the first time, reset it for real
        resetFunc();
      }
      i2c_regs[1] = 0;                    // disable slave
      i2c_regs[3] = 0;   // Set itself as an inactive block
    } else {                              // reset pin is less than 1000/1024 * 5 vcc
      i2c_regs[3] = 1;   // Set itself as an active block
    }
  }
}