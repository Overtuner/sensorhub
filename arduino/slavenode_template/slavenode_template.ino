/*
A basic scetch for a slavenode
It has everything inside that it needs to operate inside the network.
Just add the specific code for your sensor(s)
Look for this lines: //****

*/
// Define a valid radiochannel here
#define RADIOCHANNEL 90
// This node: Use octal numbers starting with "0": "041" is child 4 of node 1
#define NODE 04  
// The CE Pin of the Radio module
#define RADIO_CE_PIN 10 
// The CS Pin of the Radio module
#define RADIO_CSN_PIN 9
// The pin of the statusled
#define STATUSLED A1
// The outputpin for batterycontrol for the voltagedivider
#define VMESS_OUT A4
// Zhe inputpin for batterycontrol
#define VMESS_IN A3
// the divider to get the real voltage from ADC
#define VOLTAGEDIVIDER 1600

// ------ End of configuration part ------------

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <JeeLib.h>  // Include library containing low power functions
//****
// some includes for your sensor(s)
//****

// Structure of our payload
struct payload_t
{
  uint16_t orderno;
  uint16_t seq; 
  float value;
};

payload_t payload;    

enum mode_t { sleep, listen_radio } mode;

RF24NetworkHeader rxheader;
RF24NetworkHeader txheader(0);

unsigned int sleeptime = 0;
boolean init_finished = false;
boolean radio_always_on = false;
int loopcount=0;
int loopcountmax = 9;

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup for low power waiting

// nRF24L01(+) radio attached using Getting Started board 
// Usage: radio(CE_pin, CS_pin)
RF24 radio(RADIO_CE_PIN,RADIO_CSN_PIN);

// Network uses that radio
RF24Network network(radio);


void setup(void) {
  int i = 0;
  bool mode_init_tx=true;
  pinMode(STATUSLED, OUTPUT);     
  pinMode(VMESS_OUT, OUTPUT);  
  pinMode(VMESS_IN, INPUT);
  analogReference(INTERNAL);
  SPI.begin();
  radio.begin();
  //****
  // put anything else to init here
  //****
  // end aditional init
  network.begin(RADIOCHANNEL, NODE);
  radio.setDataRate(RF24_250KBPS);
  digitalWrite(STATUSLED,HIGH); 
  // initialisation beginns: set sleeptime
  while ( ! init_finished ) {
    // Ask the master for initilisation
    if ( i > 10 ) {
      if (mode_init_tx) {
        txheader.type=119;
        payload.orderno=0;
        payload.seq=0;
        payload.value=0;
        network.write(txheader,&payload,sizeof(payload));
        i = 0;
      }
    }
    delay(20);
    network.update();
    if ( network.available() ) { 
      RF24NetworkHeader rxheader;
      network.read(rxheader,&payload,sizeof(payload));
      mode_init_tx=false;
      switch (rxheader.type) {
        case 119: {
          if ( sleeptime > 0 ) init_finished=true;
          mode = sleep;
          break; }
        case 112: {
          radio_always_on = (payload.value > 0.5);
          mode = sleep;
          break; }
        case 111: {
        // Init des Sleeptimers
          sleeptime=payload.value;
          txheader.type=111;
          network.write(txheader,&payload,sizeof(payload));
          break; }
      }
    }
    i++;
    if ( i > 100 ) { mode_init_tx = true; digitalWrite(STATUSLED,LOW); }
  }
  digitalWrite(STATUSLED,LOW); 
}


float read_battery_voltage(void) {
  float vmess;
  digitalWrite(VMESS_OUT, HIGH);
  Sleepy::loseSomeTime(100);      // Wait 100ms 
  vmess=analogRead(VMESS_IN);
  vmess=vmess+analogRead(VMESS_IN);
  vmess=vmess+analogRead(VMESS_IN);
  vmess=vmess+analogRead(VMESS_IN);
  vmess=vmess+analogRead(VMESS_IN);
  digitalWrite(VMESS_OUT, LOW);
  return vmess / VOLTAGEDIVIDER;
}

void loop(void) {
  digitalWrite(STATUSLED,HIGH); 
  Sleepy::loseSomeTime(100);
  network.update();
  if ( network.available() ) {
    RF24NetworkHeader header;    
    network.read(header,&payload,sizeof(payload));
    // stay longer awake and listen 3 seconds
    loopcountmax=30;
    switch (header.type) {
      case 1:
        txheader.type=1;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 2:
        txheader.type=2;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 3:
        txheader.type=3;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 4:
        txheader.type=4;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 5:
        txheader.type=5;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 6:
        txheader.type=6;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 7:
        txheader.type=7;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 8:
        txheader.type=8;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 9:
        txheader.type=9;
        //****
        // insert here: payload.value=[result from sensor] 
        network.write(txheader,&payload,sizeof(payload));
       break;
      case 101:  
      // battery voltage
        payload.value=read_battery_voltage();
        txheader.type=101;
        network.write(txheader,&payload,sizeof(payload));  
        break;      
      case 111:  
      // sleeptimer
        if (payload.value > 0) sleeptime=payload.value;
        mode=sleep;
        txheader.type=111;
        network.write(txheader,&payload,sizeof(payload));
        break;       
      case 112:
      // radio on (=1) or off (=0) when sleep
        txheader.type=112;
        if (payload.value > 0.5) radio_always_on = true; else radio_always_on = false;
        network.write(txheader,&payload,sizeof(payload));
        break;                
      case 117: 
      // listen radio - dont sleep
        txheader.type=117;
        mode=listen_radio;
        network.write(txheader,&payload,sizeof(payload));
        break;     
    }
//    network.write(txheader,&payload,sizeof(payload));
  }
  if (loopcount > loopcountmax) {
    digitalWrite(STATUSLED,LOW); 
    switch ( mode ) {
      case sleep:
        if ( ! radio_always_on ) radio.powerDown();
        Sleepy::loseSomeTime(sleeptime);
        radio.powerUp();
        break;
      case listen_radio:
        // nothing do do here  
        break;
    }
    loopcountmax=5;
    loopcount=0;
  }
  loopcount++;
}
