// RPM measurement tool for Jan Ridders
//     2017 Aug  21  Maarten Pennings  v4  Enabled low power (wifi off), added version
//     2017 Aug  20  Maarten Pennings  v3  sen_isr improved (no longer ignores spike-times)
//     2017 July 16  Maarten Pennings  v2  Removed 'String', fixed spelling errors, fixed error in hz100, added but_init(), added #defines
//     2017 July 10  Maarten Pennings  v1  Created
// Back links: 
//   - The root of this project
//     https://github.com/maarten-pennings/mRPM
//   - This file
//     https://github.com/maarten-pennings/mRPM/blob/master/arduino/README.md
// Hardware used:
//   - RobotDyn NodeMCU V3 ESP8266 32M flash USB-serial CH340G
//     https://www.aliexpress.com/item/NodeMCU-WIFI-module-integration-of-ESP8266-extra-memory-32M-flash-USB-serial-CH340G/32739832131.html?
//   - MAX7219 Dot Matrix Module 4 time 8x8 LEDs
//     https://www.aliexpress.com/item/1Pcs-MAX7219-Dot-Matrix-Module-For-arduino-Microcontroller-4-In-One-Display-with-5P-Line/32624431446.html
//   - RobotDyn Line tracking sensor, digital out
//     https://www.aliexpress.com/item/Line-tracking-Sensor-For-robotic-and-car-DIY-Arduino-projects-Digital-Out/32654587628.html
#define VERSION "v4"


#include <limits.h>
#include <ESP8266WiFi.h>
extern "C" {
#include <user_interface.h>
}
#include "MAX7219_Dot_Matrix.h"


// ESP8266 power control
// ============================================================

#define ESP8266_FREQ   160 // CPU frequency in MHz (valid: 80, 160)
#define ESP8266_WIFI   0   // WiFi radio enabled (valid: 0, 1)

// Configures WiFi power and CPU speed
void esp8266_init(void) {
  #if !ESP8266_WIFI
    // Turn off ESP8266 radio
    WiFi.forceSleepBegin();
    delay(1);       
  #endif                         
  // Set CPU clock frequency
  system_update_cpu_freq(ESP8266_FREQ);
}


// Led Matrix display
// ============================================================

#define DSP_COUNT      4    // The display has 4 elements of 8x8 LEDs (VCC on 3V3, GND on GND).
#define DSP_DIN        D7   // DIN pin of display is connected to D7 of ESP8266.
#define DSP_CLK        D5   // CLK pin of display is connected to D5 of ESP8266.
#define DSP_CS         D2   // CS  pin of display is connected to D2 of ESP8266.
#define DSP_INTENSITY  8    // Brightness level of display: 0 (min) to 15 (max).

MAX7219_Dot_Matrix dsp(DSP_COUNT,DSP_CS,DSP_DIN,DSP_CLK);

// Send setup commands to the matrix display (clear all LEDs, sets brightness).
void dsp_init(void) {
  dsp.begin();
  dsp.setIntensity(DSP_INTENSITY);
}

// Puts 'msg' (flush right) on the display ('msg' should be max 4 chars, rest is truncated).
void dsp_set(char * msg) {
  int offset = (strlen(msg)-DSP_COUNT)*8 - 1; // The -1 shifts one column - looks better.
  dsp.sendSmooth(msg,offset);
}

// Scrolls 'msg' on the display; returns when scrolling done ('msg' can have any length)).
void dsp_scroll(char * msg) {
  int len = strlen(msg);
  for(int offset=-DSP_COUNT*8; offset<len*8; offset++ ) {
    dsp.sendSmooth(msg,offset);
    delay(40);
  }
}


// Sensor
// ============================================================

// Each time the sensor detects the marker on the rotating wheel, its output pin is pulled up.
// This triggers an interrupt, causing sen_isr() to be called.
// The ISR looks at the system clock - micros() - to determine the time stamp (sen_time) of the interrupt.
// Two consecutive interrupts allow the computation of their difference (delta).
// Note: the term 'delta' refers to a time in micro seconds for one full rotation.

// In reality there is a problem: there are spurious interrupts ("spikes" on the sensor output pin).
// They are caused by the "rough" edges of the marker on the wheel (see accompanying README.md for a picture).

// To mitigate this problem, the software keeps track of a moving average (sen_movav1) of the (raw) revolution times.
// Any new revolution time that is far below the average (a factor SEN_STEP1 lower) is ignored, otherwise it is accepted.
// The software also keeps track of a moving average (sen_movav2) of the (accepted) revolution times.
// This makes the display less jumpy. 
// See https://github.com/maarten-pennings/mRPM/blob/master/arduino/implnotes.md for more details.

// To trace the measurement process, measurements are printed over the serial line.
// With the following macros this can be enabled or disabled.
#define SEN_TRACE(...)      Serial.printf(__VA_ARGS__)
//#define SEN_TRACE(...)      // nothing

#define SEN_PIN        D6          // D0 pin of line tracking sensor connected to this pin of ESP8266 (VCC on VIN, GND on GND).
#define SEN_IDLE       2000000     // If there is no interrupt within 2 000 000 us (2 seconds), assume rotation stopped.
#define SEN_COUNT1     4           // Number of interrupts after a restart ("stabilization") before reporting measurements (must be >=2).
#define SEN_SIZE1      8           // Window size for first moving average (sen_movav1) - this is used to filter out spikes.
#define SEN_STEP1      2           // Any delta smaller than the moving average (sen_movav1) divided by SEN_STEP1 is rejected.
#define SEN_SIZE2      32          // Window size for second moving average (sen_movav2) - this is to smooth output (not too fast changes).
volatile int           sen_count1; // Number of all interrupts.
volatile unsigned long sen_time1;  // Time stamp of the last interrupt from the sensor.
volatile unsigned long sen_movav1; // The moving average of all delta's.
volatile int           sen_count2; // Number of interrupts belonging to accepted delta's.
volatile unsigned long sen_time2;  // Time stamp of the last accepted delta.
volatile unsigned long sen_movav2; // The moving average of all accepted delta's.

// The sen_count1 and sen_count2 determine the overall state:
//   0              / 0   = IDLE  = not rotating
//   1              / 0   = STAB1 = stabilizing (first interrupt received)
//   2..SEN_COUNT1  / 0   = STAB  = stabilizing; delta's available
//   SEN_COUNT1+1.. / 0   = SPIKE = measuring, but no accepted interrupt yet
//   SEN_COUNT1+1.. / 1   = MEAS1 = measuring, but only one accepted interrupt yet (hence no delta)
//   SEN_COUNT1+1.. / 2.. = MEAS  = measuring; accepted delta's available

// Prepare sensor (setup pin, enable interrupt, initialize signal processing).
void sen_init(void) {
  // Set state to idle
  sen_count1= 0;
  sen_count2= 0;
  // Configure hardware
  pinMode(SEN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SEN_PIN), sen_isr, RISING); // ensures sen_isr() is called a level change is detected on pin SEN_PIN
}

// The interrupts service routine called for every level change on SEN_PIN (registered in sen_init).
void sen_isr(void) {
  // Record time stamp of the interrupt.
  unsigned long now= micros();
  // Compute difference with previous interrupt.
  unsigned long delta1= now - sen_time1;
  sen_time1= now;
  // Increment interrupt count, but clip to MAX_INT.
  if( sen_count1<INT_MAX ) sen_count1++;
  // If this is the first after idle, bail out (for a delta we need two interrupts).
  if( sen_count1==1 ) {
    SEN_TRACE("                                         All times in us\n");
    SEN_TRACE("------- ----- ---------- ------- ------- ------- -------\n");
    SEN_TRACE("%7s %5s %10s %7s %7s %7s %7s\n","statenr","state","time","delta1","movav1","delta2","movav2");
    SEN_TRACE("------- ----- ---------- ------- ------- ------- -------\n");
    SEN_TRACE("%03d/%03d IDLE\n",0,0);
    SEN_TRACE("%03d/%03d STAB1 %10lu\n",sen_count1,sen_count2,sen_time1);
    return;
  }
  // Maintain (approximation of) the moving average of all delta's (but initialize moving average after a restart).
  sen_movav1= (sen_count1==2) ? delta1 : (sen_movav1*SEN_SIZE1 + delta1 - sen_movav1)/SEN_SIZE1;
  // Bail out if still stabilizing.
  if( sen_count1<=SEN_COUNT1 ) {
    SEN_TRACE("%03d/%03d STAB  %10lu %7lu %7lu\n",sen_count1,sen_count2,sen_time1,delta1,sen_movav1);
    return;
  }
  // Bail out when there seems to be a spike (delta is a factor SEN_STEP1 smaller than moving average).
  if( delta1*SEN_STEP1<sen_movav1 ) {
    SEN_TRACE("%03d/%03d SPIKE %10lu %7lu %7lu\n",sen_count1,sen_count2,sen_time1,delta1,sen_movav1);
    return;
  }
  // Stabilization is over: increment accepted interrupt count, but clip to MAX_INT.
  if( sen_count2<INT_MAX ) sen_count2++;
  // Compute difference with previous accepted interrupt.
  unsigned long delta2= now - sen_time2;
  sen_time2= now;
  // If this is the first after stabilization, bail out (for a delta we need two interrupts).
  if( sen_count2==1 ) {
    SEN_TRACE("%03d/%03d MEAS1 %10lu %7lu %7lu\n",sen_count1,sen_count2,sen_time1,delta1,sen_movav1);
    return;
  }
  // Compute the (approximation of) the moving average of all accepted delta's (but initialize moving average after stabilization).
  sen_movav2= (sen_count2==2) ? delta2 : (sen_movav2*SEN_SIZE2 + delta2 - sen_movav2)/SEN_SIZE2;
  SEN_TRACE("%03d/%03d MEAS  %10lu %7lu %7lu %7lu %7lu\n",sen_count1,sen_count2,sen_time1,delta1,sen_movav1,delta2,sen_movav2);
}

// Returns the duration of one rotation in us (smoothed).
// Returns 0 when there is no rotation (detected no rotation during the last SEN_IDLE us).
// Returns 1 when there is rotation, but no stable data yet.
unsigned long sen_get(void) {
  int count1;
  int count2;
  unsigned long delta;
  noInterrupts();
    // Check if the last interrupt was too old; go back to idle.
    if( micros()-sen_time1 > SEN_IDLE ) {
      if( sen_count1>0 ) {
        SEN_TRACE("%03d/%03d IDLE\n",0,0);
        SEN_TRACE("------- ----- ---------- ------- ------- ------- -------\n");
        SEN_TRACE("\n");
      }
      // Flag to ISR that state is idle (so it can restart).
      sen_count1= 0;
      sen_count2= 0;
  }
    // Get the state and delta from the ISR.
    count1= sen_count1;
    count2= sen_count2;
    delta= sen_movav2;
  interrupts();
  // Report result
  if( count1==0 ) return 0;
  if( count2<2 ) return 1;
  return delta;
}


// Button
// ============================================================

#define BUT_PIN        D3          // D3 pin is the 'flash' button

int but_oldstate;
int but_curstate;

// Initialize the button hardware (pin) and software module.
void but_init(void) {
  pinMode(BUT_PIN, INPUT);
  but_scan();
  but_scan();
}

// Checks the status of the pins; to be called periodically.
void but_scan(void) {
  int but_newstate= digitalRead(BUT_PIN)==0;
  but_oldstate = but_curstate;
  but_curstate = but_newstate;
}

// Returns current state of the button (1=is-down, 0=is-up).
int but_isdown(void) {
  return but_curstate;
}

// Returns the current state with respect to the previous state - transition  (1=went-down, 0=is-down/is-up/went-up).
int but_wentdown(void) {
  return (but_curstate ^ but_oldstate) & but_curstate;
}


// Helper functions
// ============================================================

// Helper function that formats an integer to a string.
// It puts a dot at position 'dotpos'.
// Note that the font is patched (to have digits with a leading decimal point).
char * tostr(int val,int dotpos) {
  static char buf[]="xxxx"; // Local buffer - only works for single threaded - ok here.
  for(int i=0; i<4; i++ ) {
    int digit=val%10; // Extract LSB
    buf[3-i]=digit+'0'; // Put at rightmost position
    if( i==dotpos && i!=0 ) buf[3-i]=digit+'\x10'; // Overlay decimal point (font is patched to have '0.', '1.' etc at 0x10 and up).
    val=val/10;
  }
  // Erase leading 0s
  for( int i=0; buf[i]=='0' && i<3-dotpos; i++ ) buf[i]=' ';
  return buf;
}

// Shows the current units on the display.
void show_units( int units ) {
  switch( units ) {
    case 0: dsp_set("rpm "); break;
    case 1: dsp_set("ms  "); break;
    case 2: dsp_set("Hz  "); break;
  }
}

// Shows the current delta on the display (using current units).
void show_delta( unsigned long delta, int units ) {
  if( delta<2 ) { // idle or stabilizing, show dashes
    switch( units ) { // Font is patched to have '-.' at 0x1A.
      case 0: dsp_set("-"); break;
      case 1: dsp_set("\x1A-"); break;
      case 2: dsp_set("\x1A--"); break;
    }
  } else { // delta is rotation time in us
    // Compute derived values - the additions below are for rounding.
    int rpm= (60*1000*1000+delta/2)/delta;    // units: rotations per minute
    int ms10= (delta+50)/100;                 // units: 1/10 milliseconds
    int hz100= (100*1000*1000+delta/2)/delta; // units: 1/100 hertz
    switch( units ) {
      case 0: dsp_set(tostr(rpm  ,0)); break;
      case 1: dsp_set(tostr(ms10 ,1)); break;
      case 2: dsp_set(tostr(hz100,2)); break;
    }
  }
}


// Arduino entry points
// ============================================================

#define BAUD       115200  // Baud rate for serial connection.
#define REFRESH_MS 100     // Time (in ms) between display and button refreshes.
#define TOGGLE_MS  1000    // Time (in ms) between displaying units and delta (when not rotating).
int units;                 // With the 'flash' button, the units can be chosen. This variable holds the current mode.
int olddelta;              // Last valid measurement.
unsigned long oldtime;     // Last time the display was refreshed (when not rotating).

// Arduino callback at startup
void setup() {
  // Setup serial port (over USB) for tracing.
  Serial.begin(BAUD);
  Serial.printf("\n\nWelcome at mRPM\n");
  Serial.printf("Version " VERSION "\n");
  // Initialize matrix, sensor en button.
  esp8266_init();
  dsp_init();
  sen_init();
  but_init();
  // Initialize program variables.
  units= 0; // 0=rpm, 1=ms, 2=Hz
  olddelta= 0;
  // Give welcome message.
  dsp_scroll("mRPM " VERSION);
}

// Arduino callback, repeated called after setup()
void loop(){
  // If 'flash' button is pressed, cycle through unit modes.
  but_scan();
  if( but_wentdown() ) {
    units=(units+1)%3;
  }
  // Update display
  unsigned long delta = sen_get();
  if( delta==0 ) {
    // Not rotating: toggle display between units and last known delta.
    unsigned long now = millis();
    if( now-oldtime<1*TOGGLE_MS ) show_delta(olddelta,units);
    else if( now-oldtime<2*TOGGLE_MS ) show_units(units);
    else oldtime= now;
  } else {
    // Rotating: show current delta.
    show_delta(delta,units);
    olddelta= delta;
    oldtime= millis();
  }
  // Display and buttons are refreshed every 100ms.
  delay(REFRESH_MS);
}

