/*
 This sketch tests if the outputs from the standard fft and my optimized version are giving
 the same results. It uses the Frequency Sweep Object fed to both fft's. You can switch back
 and forth between the two by Serial Commands like so..
 
 Example1: Sending the string -> "opt" with a NewLine appended by the serial monitor.
 
 Example2: Sending the string -> "std" with a NewLine appended by the serial monitor.
 
 'opt' is the optimized fft output.
 'std' is the stock fft output.
 
 This sketch also prints the Max Processor Usage for whatever version of the fft you have
 selected to print to the serial monitor. Also you don't need any special hardware just a
 teensy to see the results.
 */
#include <SerialFlash.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <analyze_fft1024_duffs_optimization.h>
//---------------------------------------------------------------------------------------
AudioAnalyzeFFT1024_Duffs_Optimization  optfft;
AudioAnalyzeFFT1024                     stdfft;
AudioSynthToneSweep                     sweep;
AudioOutputAnalog                       dac;
AudioMixer4                             mixer;
//---------------------------------------------------------------------------------------
AudioConnection patchCord0(sweep, 0, mixer, 0);
AudioConnection patchCord1(mixer, 0, optfft, 0);
AudioConnection patchCord2(mixer, 0, stdfft, 0);
AudioConnection patchCord3(mixer, 0, dac, 0);
//---------------------------------------------------------------------------------------
IntervalTimer sweepTimer;
// Serial input
String input;
// what version of fft is being displayed
enum fft_version_t { OPTIMIZED, STANDARD } FFT_VERSION;
volatile bool up_or_down;
const float t_ampx = 0.8;
const int t_lox = 40;
const int t_hix = 1200;
// Length of time for the sweep in seconds
const float t_timex = 5;
// number fft bins to print
const uint16_t numOfBins = 30;
//---------------------------------------------------------------------------------------
void sweepISR(void) {
    if (!sweep.isPlaying()) {
        if (up_or_down == false) {
            sweep.play(t_ampx, t_lox, t_hix, t_timex);
            //Serial.println("AudioSynthToneSweep up - begin");
            up_or_down = true;
        }
        // and now reverse the sweep
        else if (up_or_down == true) {
            sweep.play(t_ampx, t_hix, t_lox, t_timex);
            //Serial.println("AudioSynthToneSweep down - begin");
            up_or_down = false;
        }
    }
    digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
}
//---------------------------------------------------------------------------------------
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    while (!Serial);
    delay(100);
    up_or_down = false;
    AudioMemory(30);
    optfft.windowFunction(AudioWindowHanning1024);
    stdfft.windowFunction(AudioWindowHanning1024);
    // Audio library isr allways gets priority
    sweepTimer.priority(144);
    sweepTimer.begin(sweepISR, 100000);
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        input += c;
        if (input.endsWith("\n")) {
            if (input.startsWith("opt")) {
                FFT_VERSION = OPTIMIZED;
            }
            
            if (input.startsWith("std")) {
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
