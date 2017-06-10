#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <analyze_fft1024_duffs_optimization.h>

AudioAnalyzeFFT1024_Duffs_Optimization  optimized_fft;
AudioAnalyzeFFT1024                     fft;
AudioSynthWaveformSine                  sinewave;
AudioOutputAnalog                       dac;

AudioConnection patchCord1(sinewave, 0, optimized_fft, 0);
AudioConnection patchCord2(sinewave, 0, fft, 0);
AudioConnection patchCord3(sinewave, 0, dac, 0);

void setup() {
    while (!Serial);
    delay(100);
    Serial.println("FFT Usage Example...");
    AudioMemory(24);
    optimized_fft.windowFunction(AudioWindowHanning1024);
    fft.windowFunction(AudioWindowHanning1024);
    sinewave.amplitude(0.8);
    sinewave.frequency(440);
}

void loop() {
    float pu1 = optimized_fft.processorUsageMax();
    float pu2 = fft.processorUsageMax();
    Serial.printf("Optimized FFT Max Usage: %4.2f\tFFT Max Usage: %4.2f\n", pu1, pu2);
    delay(150);
}
