/*
   This sketch tests if the outputs from the standard fft and my optimized version are giving the same 
   results. It uses the AudioSynthKarplusStrong Object fed to both fft's. You can switch back and forth 
   between the two and change the AudioSynthKarplusStrong frequency by Serial Commands like so..

  Example1: Sending the string -> "opt 440.0" with a NewLine appended by the serial monitor will show 
            the optimized fft output and set AudioSynthKarplusStrong frequency to 440.0 Hz.

  Example2: Sending the string -> "std 440.0" with a NewLine appended by the serial monitor will show 
            the standard fft output and set AudioSynthKarplusStrong frequency to 440.0 Hz.

  This sketch also prints the Max Processor Usage for whatever version of the fft you have selected 
  to print to the serial monitor. Also you don't need any special hardware just a teensy to see the 
  results.
*/
#include <SerialFlash.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
// include the library
#include <analyze_fft1024_duffs_optimization.h>
//---------------------------------------------------------------------------------------
// add objects
AudioAnalyzeFFT1024_Duffs_Optimization  optfft;
AudioAnalyzeFFT1024                     stdfft;
AudioSynthKarplusStrong                 string;
AudioOutputAnalog                       dac;
AudioMixer4                             mixer;
//---------------------------------------------------------------------------------------
AudioConnection patchCord0(string,  0, mixer,  0);
AudioConnection patchCord1(mixer,   0, optfft, 0);
AudioConnection patchCord2(mixer,   0, stdfft, 0);
AudioConnection patchCord3(mixer,   0, dac,    0);
//---------------------------------------------------------------------------------------
IntervalTimer playNoteTimer;
// Karplus Strong freq
volatile float frequency;
// Serial input
String input;
// what version of fft is being displayed
enum fft_version_t { OPTIMIZED, STANDARD } FFT_VERSION;
// number fft bins to print
const uint16_t numOfBins = 30;
//---------------------------------------------------------------------------------------
void playNote(void) {
  string.noteOn(frequency, .8);
  digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
}
//---------------------------------------------------------------------------------------
void setup() {
  FFT_VERSION = OPTIMIZED;
  frequency = 196.0;
  AudioMemory(30);
  while (!Serial);
  delay(120);
  // which window
  optfft.windowFunction(AudioWindowHanning1024);
  stdfft.windowFunction(AudioWindowHanning1024);
  pinMode(LED_BUILTIN, OUTPUT);
  // Audio library isr allways gets priority
  playNoteTimer.priority(144);
  playNoteTimer.begin(playNote, 3000000);
}

void loop() {

  if (Serial.available()) {
    char c = Serial.read();
    input += c;
    if (input.endsWith("\n")) {

      if (input.startsWith("opt")) {
        input = input.substring(3);
        float f = input.toFloat();
        if (f < 30.00) f = 30.00;
        if (f > 800.00) f = 800.00;
        Serial.print("opt ");
        Serial.println(f);
        frequency = f;
        FFT_VERSION = OPTIMIZED;
      }

      if (input.startsWith("std")) {
        input = input.substring(3);
        float f = input.toFloat();
        if (f < 30.00) f = 30.00;
        if (f > 800.00) f = 800.00;
        Serial.print("std ");
        Serial.println(f);
        frequency = f;
        FFT_VERSION = STANDARD;
      }
      input = "";
    }
  }

  float n;
  int i;

  float pusageOpt = optfft.processorUsageMax();
  float pusageStd = stdfft.processorUsageMax();

  if (optfft.available() && FFT_VERSION == OPTIMIZED) {
    Serial.printf("OPT FFT USAGE: %.2f |\t\t:", pusageOpt);
    for (i = 0; i < numOfBins; i++) {
      n = optfft.read(i);
      if (n >= 0.01) {
        Serial.print(n);
        Serial.print(" ");
      } else Serial.print("  -  "); // don't print "0.00"
    }
    Serial.println(":");
  }


  if (stdfft.available() && FFT_VERSION == STANDARD) {
    Serial.printf("STD FFT USAGE: %.2f |\t\t:", pusageStd);
    for (i = 0; i < numOfBins; i++) {
      n = stdfft.read(i);
      if (n >= 0.01) {
        Serial.print(n);
        Serial.print(" ");
      } else Serial.print("  -  "); // don't print "0.00"
    }
    Serial.println(":");
  }
}
