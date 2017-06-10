// FFT Test
//
// Compute a 1024 point Fast Fourier Transform (spectrum analysis)
// on audio connected to the Left Line-In pin.  By changing code,
// a synthetic sine wave can be input instead.
//
// The first 40 (of 512) frequency analysis bins are printed to
// the Arduino Serial Monitor.  Viewing the raw data can help you
// understand how the FFT works and what results to expect when
// using the data to control LEDs, motors, or other fun things!
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <analyze_fft1024_duffs_optimization.h>

AudioSynthWaveformSine                  sinewave;
AudioAnalyzeFFT1024_Duffs_Optimization  optimized_fft;
AudioOutputAnalog                       dac;

AudioConnection patchCord1(sinewave, 0, optimized_fft, 0);
AudioConnection patchCord2(sinewave, 0, dac, 0);

void setup() {
    AudioMemory(12);
    optimized_fft.windowFunction(AudioWindowHanning1024);
    sinewave.amplitude(0.8);
    sinewave.frequency(1034.007);
}

void loop() {
    float n;
    int i;
    
    if (optimized_fft.available()) {
        // each time new FFT data is available
        // print it all to the Arduino Serial Monitor
        Serial.print("FFT: ");
        for (i=0; i<40; i++) {
            n = optimized_fft.read(i);
            if (n >= 0.01) {
                Serial.print(n);
                Serial.print(" ");
            } else {
                Serial.print("  -  "); // don't print "0.00"
            }
        }
        Serial.println();
    }
}

