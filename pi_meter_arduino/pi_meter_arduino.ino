#include <avr/wdt.h>  //use watch dog timer
#include <avr/sleep.h> //use sleep
#include <Wire.h> //use i2c device
#include <EEPROM.h> //use eeprom address 0~511(512)

#define LED_PIN (13) //LED_PIN
#define ACC_IN (2) //interrupt pin
#define PI_POWER (12) //raspberry pi power
#define WTMP_IN (A0) //water temp
#define OTMP_IN (A1) //oil temp 
#define OPLS_IN (A2) //oil press
#define B_LV_IN (A3) //battery level 

int read_counter=0;
int r_wtmp=0; //water temp
int r_otmp=0; //oil temp
int r_opls=0; //oil press
int r_b_lv=0; //battery lebvel
int r_out_tmp=0; //outside temp

const int SLAVE_ADDRESS = 0x04; //i2c address
const int CONFIG_END=0x00; //normal state

//GET CONFIG MODE 0x1X
const int REQUEST_CONFIG=0x10;
const int GET_INO_SLEEP=0x11; //ino_sleep_time (minutes)
const int GET_PI_SHUT=0x12; //pi shutdown wait time
const int GET_PI_ISW_TIME=0x13; //pi in sleep wakeup time

//SET CONFIG MODE 0x2X
const int CONFIG_SET=0x20;
const int SET_INO_SLEEP=0x21; //ino_sleep_time (minutes)
const int SET_PI_SHUT=0x22; //pi shutdown wait time
const int SET_PI_ISW_SLEEP=0x23; //pi in sleep wakeup time


volatile bool slp_interbal_flg;
volatile bool slp_counter_flg=false; //sleep flag
volatile bool slp_loop_flg=false; //sleep flag
volatile int interbal_counter; //sleep interbal counter
volatile int pi_me; //acc status for send pi
volatile int message_state;  //message state
volatile int ino_state;  //arduino state

//eeprom address
const int ino_sleep_addr=0;
const int pi_sleep_wakeup_addr=1;
const int pi_shut_addr=2;

const int count_max=15; //4second * 15 = 60 second 
volatile int wait_minutes=1; //ino_sleep_time (minutes)
volatile int pi_shut_time=1; //pi shutdown wait time
volatile int pi_osw_time=1; //pi in sleep wakeup time

volatile long onslp_max_time;  //on sleep wakeup max time
volatile long onslp_past_time; //on sleep wakeup past time
volatile bool counter_switch=false;

//status reset
void init_value()
{
  slp_interbal_flg=false;
  message_state=0x00;
  slp_counter_flg=false;
}

//pin init
void init_pins()
{
  pinMode(LED_PIN,OUTPUT);
  pinMode(PI_POWER,OUTPUT);
  pinMode(ACC_IN, INPUT);
}

//check first state
void init_state()
{
  if (digitalRead(ACC_IN))
  {
    ino_state=0x00;
    slp_interbal_flg=false;
  }else
  {
    ino_state=0x03;
    pi_me=0x05; //change ACC state
  }
}

//read eeprom set config
void read_rom_config()
{
  wait_minutes=EEPROM.read(ino_sleep_addr);
  if( (wait_minutes <= 0) || (wait_minutes > 250) )
  {
    wait_minutes=1;
  }
  pi_shut_time=EEPROM.read(pi_shut_addr);
  if( (pi_shut_time <= 0) || ( pi_shut_time >250) )
  {
    pi_shut_time=1;
  }
  pi_osw_time=EEPROM.read(pi_sleep_wakeup_addr);
  if( (pi_osw_time <= 0 ) || (pi_osw_time > 250) )
  {
    pi_osw_time=1;
  }
}

//write eeprom set config
void set_config(int addr, byte data)
{
  noInterrupts();
  EEPROM.write(addr,data);
  interrupts();
}

//watch dog timer setup
void wdt_set()
{
  wdt_reset();
  cli();
  MCUSR = 0;
  WDTCSR |= 0b00011000; //WDCE WDE set
  WDTCSR =  0b01000000 | 0b100000;//WDIE set  |WDIF set  scale 4 seconds
  sei();
}

//watch dog timer unset
void wdt_unset()
{
  wdt_reset();
  cli();
  MCUSR = 0;
  WDTCSR |= 0b00011000; //WDCE WDE set
  WDTCSR =  0b00000000; //status clear
  sei();
}

//watch dog timer call
ISR(WDT_vect)
{
  if(slp_counter_flg)
  {
    interbal_counter++;
    if( interbal_counter >= (count_max * wait_minutes) )
    {
      //sleep end
      slp_counter_flg = false;
    }
  }
}

//arduino wakeup interrupt
void wakeUp()
{
  if(slp_counter_flg){
    //sleep end
    slp_counter_flg = false;
  }
}

//sleep arduino
void sleep()
{
  interbal_counter = 0;
  wdt_set(); //watch dog timer set
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //set sleep mode
  attachInterrupt(0,wakeUp, RISING); //set level interrupt
  slp_counter_flg=true;
  while(slp_counter_flg)
  {
    noInterrupts();  //cli();
    sleep_enable();
    interrupts();    //sei();
    sleep_cpu();  //cpu sleep
    sleep_disable();
  }
  wdt_unset(); //watch dog timer unset
  //counter reset
  detachInterrupt(0); //unset level interrupt
}

//get message from pi
void get_message(int n){
  int cmd[4];
  int x = 0;
  while(Wire.available()) {
    cmd[x] = Wire.read();
    x++;
  }
  if ((cmd[0] >= 16) && (cmd[0] < 32)) // 0x10~0x1F get config
  {
    message_state = cmd[0];
  } 
  else if((cmd[0] >= 32) && (cmd[0] < 47)) //0x20~0x2F set config  
  {
    switch (cmd[0])
    {
      case 0x21: //ino_sleep_time (minutes)
      set_config(ino_sleep_addr, cmd[1]);
      read_rom_config(); //reload config
      break;
      case 0x22: //pi shutdown wait time
      set_config(pi_shut_addr, cmd[1]);
      read_rom_config(); //reload config
      break;
      case 0x23: //pi in sleep wakeup time
      set_config(pi_sleep_wakeup_addr, cmd[1]);
      read_rom_config(); //reload config
      break;
    }
  }else if((cmd[0] >= 48) && (cmd[0] < 79)) //0x30~0x4F return message state 
  {
  	message_state = cmd[0];
  }
  else if ((cmd[0]==0) && (cmd[3]==120))
  {
    toggle();
  }
  else
  {
  }

  if(cmd[0] == 0x50){
    message_state = cmd[0];
  } 
}

//send message to pi
void send_message(){
  //cmd switch
  switch (message_state) {
   case 0x11: //ino_sleep_time (minutes)
   Wire.write(wait_minutes);
   break;
   case 0x12: //pi shutdown wait time
   Wire.write(pi_shut_time);
   break; 
   case 0x13: //pi in sleep wakeup time 
   Wire.write(pi_osw_time);
   break; 

   //analog read
   case 0x31: //water temp upper bit
   Wire.write(r_wtmp >> 8);
   break;
   case 0x32: //water temp low bit
   Wire.write(r_wtmp & 0xFF);
   break;
   case 0x33: //oil temp upper bit
   Wire.write(r_otmp >> 8 );
   break;
   case 0x34: //oil temp low bit
   Wire.write(r_otmp & 0xFF);
   break;
   case 0x35: //oil press upper bit
   Wire.write(r_opls >> 8);
   break;
   case 0x36: //oil press low bit
   Wire.write(r_opls & 0xFF);
   break;
   case 0x37: //battery level upper bit
   Wire.write(r_b_lv >> 8);
   break;
   case 0x38: //battely level low bit
   Wire.write(r_b_lv & 0xFF);
   break;
 
   case 0x50: //
   Wire.write(pi_me); //send 
   break;  
 }
}

//wait time
void wait_time(int t)
{
  volatile unsigned long now = millis();
  volatile unsigned long out_time = (now + 1000* (unsigned long)t);
  if(now < out_time){
    while(millis()< out_time){}
  }
  else //counter over flow
  {
    while(millis() > now){}
    while(millis() < out_time){}
  }
}

//wakeup wraspberry pi
void pi_wakeup()
{
  digitalWrite(PI_POWER,HIGH); //pi power on
  digitalWrite(LED_PIN,HIGH);
}

void read_time_slp()
{
  onslp_max_time = ( millis()+ 60000 * pi_osw_time );
  onslp_past_time = millis();
  if (onslp_max_time > onslp_past_time)
  {
    counter_switch=true;
  }
  else
  {
    counter_switch=false;
  }
}

void check_input()
{
  switch (read_counter){
    case 0:
    r_wtmp=analogRead(WTMP_IN);
    read_counter++;
    break;

    case 1:
    r_otmp=analogRead(OTMP_IN);
    read_counter++;
    break;
 
    case 2:
    r_opls=analogRead(OPLS_IN);
    read_counter++;
    break;
 
    case 3:
    r_b_lv=analogRead(B_LV_IN);
    read_counter++;
    break;
 
    case 4:
    read_counter=0;
    break;
 
  }

}


//test
boolean LEDON = false; 
void toggle(){
  LEDON = !LEDON; //true and false change
  digitalWrite(LED_PIN, LEDON);  
}

//set up
void setup()
{
  init_value(); //status init
  init_pins(); //pin init
  Wire.begin(SLAVE_ADDRESS); //i2c address set
  Wire.onReceive(get_message); //i2c receive
  Wire.onRequest(send_message); //i2c requests 
  read_rom_config(); //read config from eeprom
  init_state();
  delay(1000);
}

//main loop
void loop()
{
  //toggle();
  int acc = digitalRead(ACC_IN);

  switch(ino_state)
  {
    case 0x00: //arduino first state
      pi_wakeup();
      pi_me=0x01;
      ino_state++;
      break;

    case 0x01: //arduino normal state
      if( (acc==0) && (!slp_interbal_flg) )
      {
        ino_state++; //pi shutdown state
      }
      //check sleep interbal
      else if(slp_interbal_flg)
      {
        if(counter_switch)
        {
          if(millis() > onslp_max_time)
          {
            slp_interbal_flg = false;
          }
        }
        else
        {
          if( (millis() < onslp_past_time) && (millis() > onslp_max_time) )
          {
            slp_interbal_flg = false;
          }
        }
      }else
      {
        check_input();
      }
      break;

    case 0x02: //pi shutdown state
      ino_state++;
      pi_me=0x05; //change ACC state
      digitalWrite(LED_PIN,LOW);  
      wait_time(pi_shut_time * 60);
      digitalWrite(PI_POWER,LOW);  //pi power off
      break;

    case 0x03: //arduino sleep state
      sleep(); //sleep
      ino_state++;
      break;

    case 0x04: //check set interbal
      slp_interbal_flg=true; 
      if(acc==0)
      {
        read_time_slp();
        slp_interbal_flg=true;
      }
      else
      {
        slp_interbal_flg=false;
      }
      ino_state=0x00; //arduino state reset 
      break;
  }
}