// RPM measurement tool for Jan Ridders
//     2017 july 10 by Maarten Pennings
// Hardware used:
//   - RobotDyn NodeMCU V3 ESP8266 32M flash USB-serial CH340G
//     https://www.aliexpress.com/item/NodeMCU-WIFI-module-integration-of-ESP8266-extra-memory-32M-flash-USB-serial-CH340G/32739832131.html?
//   - MAX7219 Dot Matrix Module 4 time 8x8 LEDs
//     https://www.aliexpress.com/item/1Pcs-MAX7219-Dot-Matrix-Module-For-arduino-Microcontroller-4-In-One-Display-with-5P-Line/32624431446.html
//   - RobotDyn Line tracking sensor, digital out
//     https://www.aliexpress.com/item/Line-tracking-Sensor-For-robotic-and-car-DIY-Arduino-projects-Digital-Out/32654587628.html

#include "MAX7219_Dot_Matrix.h"


// Led Matrix display
// ============================================================

#define DSP_COUNT 4    // The display has 4 elements of 8x8 LEDs (VCC on 3V3, GND on GND)
#define DSP_DIN   D7   // DIN pin of display is connected to D7 of ESP8266
#define DSP_CLK   D5   // CLK pin of display is connected to D5 of ESP8266
#define DSP_CS    D2   // CS  pin of display is connected to D2 of ESP8266 

MAX7219_Dot_Matrix dsp(DSP_COUNT,DSP_CS,DSP_DIN,DSP_CLK);

// Send commands to the matrix display (clear all leds, sets brightness)
void dsp_init(void) {
  dsp.begin();
  dsp.setIntensity(8);
}

// Puts 'msg' (flush right) on the display (msg should be max 4 chars, rest is truncated)
void dsp_set(String msg) {
  int offset = (msg.length()-DSP_COUNT)*8-1;
  dsp.sendSmooth(msg.c_str(),offset);
}


// Sensor
// ============================================================

// Each time the sensor detects the marker on the rotating wheel, a pulse is generated.
// The pulse fires an interrupt, causing sen_isr() to be called.
// The isr looks at the system clock - micros() - to determine the time stamp (sen_time) of the pulse.
// Two consecutive pulses allow the computation of their difference (delta).
// Note: the term 'delta' refers to a time in micro seconds for one full retation.

// In reality there are some problems to solve
//  - We need at least two pulses for a delta, so the first pulse must be largly ignored.
//  - There are spurious pulses ("spikes"), probably caused when seeing the "rough" edges of the marker.
//    It seems that the edges at the start of the marker as well as at the end of the marker cause spikes
//  - A moving average (sen_movav1) is maintained to detect spikes (window size is SEN_SIZE1).
//  - A delta is considered a spike when it is much smaller than the moving average.
//  - A counter is maintained (sen_count) to know when moving average has seen enough pulses to be considered stable.
//  - The delta's fluctuate so a moving average (sen_movav2) is maintained to smoothen the measurements (window size is SEN_SIZE2).

// To trace the measurement process, measurements are printed over the serial line.
// With the following macros this can be enabled or disabled.
#define SEN_TRACE(...)      Serial.printf(__VA_ARGS__)
//#define SEN_TRACE(...)      // nothing

#define SEN_PIN        D6          // D0 pin of line tracking sensor connected to this pin of ESP8266 (VCC on VIN, GND on GND)
#define SEN_IDLE       2000000     // If there is no pulse within 2 000 000 us (2 seconds), assume no rotation
#define SEN_COUNT      4           // Number of pulses to wait after a restart before reporting measurements
#define SEN_SIZE1      8           // Window size for first moving average (sen_movav1) - this is used to filter out spikes
#define SEN_STEP       2           // Any measured delta smaller than the moving average (sen_movav1) divided by SEN_STEP is ignored
#define SEN_SIZE2      16          // Window size for first moving average (sen_movav1) - this is used to filter out spikes
volatile unsigned long sen_time;   // Timestamp of the last pulse fromn the sensor
unsigned long          sen_count1; // Number of samples (delta's) in sen_movav1
unsigned long          sen_movav1; // The moving average of all measured delta's
unsigned long          sen_count2; // Number of samples (delta's) in sen_movav2
volatile unsigned long sen_movav2; // The moving average of all accepted delta's


// Prepare pin SEN_PIN for generating interrupts
void sen_init(void) {
  sen_time = micros()-SEN_IDLE; // Pretend not sensor was idle for SEN_IDLE
  pinMode(SEN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SEN_PIN), sen_isr, RISING); // ensures sen_isr() is called when a pulse is detected on pin SEN
}

// The interrupts serivice routine called for every pulse
void sen_isr(void) {
  // Record stamp of the incoming pulse, and compute diff with previous pulse
  unsigned long now= micros();
  unsigned long delta= now - sen_time;
  sen_time= now;
  // Check if this is the first pulse after a long time; if so, bail out (for a delta we need two pulses).
  if( delta >= SEN_IDLE ) { 
    // Restating; clear the moving averages
    sen_count1= 0; 
    sen_count2= 0; 
    SEN_TRACE("isr: %12s %12s %-12s\n","delta(us)","movav(us)","status");   
    SEN_TRACE("isr: %12lu %12lu start\n",delta,sen_movav1);   
    return; 
  }
  // Compute the moving average (approximation) of all delta's (but reset moving average after a restart)
  sen_movav1= (sen_count1==0) ? delta : (sen_movav1*SEN_SIZE1 + delta - sen_movav1)/SEN_SIZE1;
  // Bail out if restarting
  if( sen_count1<SEN_COUNT ) {
    sen_count1++; 
    SEN_TRACE("isr: %12lu %12lu wait\n",delta,sen_movav1);   
    return;
  }
  // Bail out when the pulse seems a spike (delta much smaller than moving average)
  if( delta*SEN_STEP<sen_movav1 ) {
    SEN_TRACE("isr: %12lu %12lu spike\n",delta,sen_movav1);   
    return;
  }
  // Compute the moving average (approximation) of all delta's (but reset moving average after a restart)
  sen_movav2= (sen_count2==0) ? delta : (sen_movav2*SEN_SIZE2 + delta - sen_movav2)/SEN_SIZE2;
  SEN_TRACE("isr: %12lu %12lu av=%lu\n",delta,sen_movav1,sen_movav2);  
  // Step counter (when warpping, skip 0)
  sen_count2++; 
  if( sen_count2==0 ) sen_count2++; 
}

// Returns the duration of one rotation in us.
// Returns 0 when there is no rotation (detected the last SEN_IDLE us).
unsigned long sen_get(void) {
  unsigned long delta;
  noInterrupts();
    // Get the moving average of the delta's from the isr
    delta= sen_movav2;
    // Check if the last pulse was too old, flag
    if( micros()- sen_time >= SEN_IDLE ) { 
      if( sen_count1>0 ) { SEN_TRACE("isr: end\n"); sen_count1=0; } // sen_count1 is misused to only trace 'end' once
      sen_time = micros()-SEN_IDLE; // Reset sen_time (needed to prevent wrap-around artefacts)
      delta=0;
    }
  interrupts();
  return delta;
}

  
// Button
// ============================================================

#define BUT_PIN        D3          // D3 pin is the 'flash' button

int but_oldstate;
int but_curstate;

void but_init(void) {
  pinMode(BUT_PIN, INPUT);   
  but_scan();
  but_scan();
}

void but_scan(void) {
  int but_newstate= digitalRead(BUT_PIN)==0;
  but_oldstate = but_curstate;
  but_curstate = but_newstate;
}

int but_isdown(void) {
  return but_curstate;
}

int but_wentdown(void) {
  return (but_curstate ^ but_oldstate) & but_curstate;
}


// Helper functions
// ============================================================

// Helper function that formats an integer to a string.
// It puts an dot at position dotpos.
// Note that the font is patched (to have digits with a leading decimal point)
char * tostr(int val,int dotpos) {
  static char buf[]="xxxx"; 
  for(int i=0; i<4; i++ ) {
    int digit=val%10;
    buf[3-i]=digit+'0';
    if( i==dotpos && i!=0 ) buf[3-i]=digit+'\x10'; // Font is patched to have 0. 1. etc at 0x10 and up
    val=val/10;
  }
  // Erase trailing 0s
  for( int i=0; buf[i]=='0' && i<3-dotpos; i++ ) buf[i]=' ';
  return buf;
}

// Shows the current units on the display
void show_units( int units ) {
  switch( units ) {
    case 0: dsp_set("rpm "); break;
    case 1: dsp_set("ms  "); break;
    case 2: dsp_set("Hz  "); break;
  }  
}

// Shows the current delta on the display (using current units)
void show_delta( unsigned long delta, int units ) {
  if( delta==0 ) { 
    switch( units ) { // Font is patched to have -. at 0x1A
      case 0: dsp_set("-"); break;
      case 1: dsp_set("\x1A-"); break;
      case 2: dsp_set("\x1A--"); break;
    }
  } else {
    int rpm= (60*1000*1000+delta/2)/delta;  // units: rotations per minute
    int ms10= (delta+50)/100;               // units: 1/10 microseconds
    int hz100= (100*1000*1000+delta)/delta; // units: 1/100 herz
    switch( units ) {
      case 0: dsp_set(tostr(rpm,0)); break;
      case 1: dsp_set(tostr(ms10,1)); break;
      case 2: dsp_set(tostr(hz100,2)); break;
    }
  }
}


// Arduino entry points
// ============================================================

#define TOGGLE 1000    // Time (in ms) between displaying units and delta (when not rotating)
int units;             // With the 'flash' buttons, the units can be chosen. This variable hold the current mode.
int olddelta;          // Last valid measurement
unsigned long oldtime; // Last time the display was refreshed (when not rotating)

void setup() {
  // Init serial port (over USB), matrix and sensor
  Serial.begin(115200);
  dsp_init();
  sen_init();
  // Init program variables
  units= 0; // 0=rpm, 1=ms, 2=Hz
  olddelta= 0;
  // Welcome messages
  Serial.printf("\n\nWelcome at mRPM\n");
  dsp_set("mRPM");
  delay(3000);
}

void loop(){
  // If 'flash' button is pressed, cycle through unit modes
  but_scan();
  if( but_wentdown() ) {
    units=(units+1)%3;
  }
  // Update display
  unsigned long delta = sen_get();
  if( delta==0 ) {
    // Not rotating: toggle display between units and last known delta
    unsigned long now = millis();
    if( now-oldtime<1*TOGGLE ) show_units(units);
    else if( now-oldtime<2*TOGGLE ) show_delta(olddelta,units);
    else oldtime= now;
  } else {
    // Rotating: show current delta
    show_delta(delta,units);
    olddelta= delta;
    oldtime= millis();
  }
  // Display and buttons are refreshed every 100ms
  delay(100); 
}



