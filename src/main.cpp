#include <Arduino.h>
#include "playlist.h"

// --- Hardware Pins (RT-Spark Setup) ---
const int PIN_RED     = PA8;   // 1st LED (Red)
const int PIN_GREEN   = PA1;   // 2nd LED (Green)
const int PIN_YELLOW  = PB14;  // 3rd LED (Yellow)
const int PIN_BUTTON  = PA0;   // Control Button
const int PIN_POT     = PA3;   // Volume Potentiometer
const int PIN_AUDIO   = PB0;   // Speaker Pin

// --- State Variables ---
int currentTrack = 0;
unsigned int noteIdx = 0;
bool isPlaying = false;

// --- Timing & Controls ---
unsigned long pressStartTime = 0;
bool lastBtnState = HIGH;
bool isHolding = false;

unsigned long lastNoteTime = 0;
unsigned long lastSquareToggle = 0;
bool speakerState = LOW;
int ledSeqStep = 0;

// Volume Tracking
int currentVolPercent = 0;
int lastVolPercent = -1;
unsigned long lastVolPrintTime = 0;

void setup() {
  Serial1.begin(9600);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_AUDIO, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // Initial State: Paused (Red ON, Green & Yellow OFF)
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, LOW);
  digitalWrite(PIN_YELLOW, LOW);

  int rawPot = analogRead(PIN_POT);
  currentVolPercent = (int)((rawPot / 1023.0f) * 100.0f);
  if (currentVolPercent > 100) currentVolPercent = 100;
  
  updateScreen(currentTrack, isPlaying, currentVolPercent);
}

void loop() {
  // --- 1. Button Logic (Tap = Play/Pause | Hold >800ms = Track Switch) ---
  bool btnState = digitalRead(PIN_BUTTON);

  if (btnState == LOW && lastBtnState == HIGH) {
    pressStartTime = millis();
    isHolding = false;
  }

  // Hold >800ms -> Switch Track
  if (btnState == LOW && !isHolding) {
    if (millis() - pressStartTime >= 800) {
      isHolding = true;
      currentTrack = (currentTrack + 1) % NUM_TRACKS;
      noteIdx = 0;

      // Synchronous 3-LED strobe on track change
      for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_RED, HIGH); digitalWrite(PIN_GREEN, HIGH); digitalWrite(PIN_YELLOW, HIGH); delay(100);
        digitalWrite(PIN_RED, LOW);  digitalWrite(PIN_GREEN, LOW);  digitalWrite(PIN_YELLOW, LOW);  delay(100);
      }

      updateScreen(currentTrack, isPlaying, currentVolPercent);
    }
  }

  // Short Tap Release (Play / Pause Toggle)
  if (btnState == HIGH && lastBtnState == LOW) {
    if (!isHolding && (millis() - pressStartTime > 40)) {
      isPlaying = !isPlaying;

      if (!isPlaying) {
        digitalWrite(PIN_RED, HIGH);
        digitalWrite(PIN_GREEN, LOW);
        digitalWrite(PIN_YELLOW, LOW);
        digitalWrite(PIN_AUDIO, LOW);
      }
      updateScreen(currentTrack, isPlaying, currentVolPercent);
    }
  }
  lastBtnState = btnState;

  // Read volume potentiometer
  int rawPot = analogRead(PIN_POT);
  currentVolPercent = (int)((rawPot / 1023.0f) * 100.0f);
  if (currentVolPercent > 100) currentVolPercent = 100;

  unsigned long currentMillis = millis();

  // Print volume updates when rotated
  if (abs(currentVolPercent - lastVolPercent) >= 3 && (currentMillis - lastVolPrintTime > 300)) {
    lastVolPercent = currentVolPercent;
    lastVolPrintTime = currentMillis;
    updateScreen(currentTrack, isPlaying, currentVolPercent);
  }

  // --- 2. Non-Blocking Audio Generator & LED Chase ---
  if (isPlaying) {
    if (currentMillis - lastNoteTime >= 200) {
      lastNoteTime = currentMillis;
      noteIdx++;
      ledSeqStep = (ledSeqStep + 1) % 3;

      unsigned int activeLength = getActiveTrackLength(currentTrack);
      if (noteIdx >= activeLength) {
        noteIdx = 0;
      }

      // Rotating LED Sequence
      if (ledSeqStep == 0) {
        digitalWrite(PIN_RED, HIGH);  digitalWrite(PIN_GREEN, LOW);   digitalWrite(PIN_YELLOW, LOW);
      } else if (ledSeqStep == 1) {
        digitalWrite(PIN_RED, LOW);   digitalWrite(PIN_GREEN, HIGH);  digitalWrite(PIN_YELLOW, LOW);
      } else {
        digitalWrite(PIN_RED, LOW);   digitalWrite(PIN_GREEN, LOW);   digitalWrite(PIN_YELLOW, HIGH);
      }
    }

    // Square-wave output
    unsigned int currentFreq = getActiveFrequency(currentTrack, noteIdx);

    if (currentFreq > 0) {
      unsigned long halfPeriod = 1000000UL / (currentFreq * 2);

      if (micros() - lastSquareToggle >= halfPeriod) {
        lastSquareToggle = micros();
        speakerState = !speakerState;

        if (speakerState && currentVolPercent > 2) {
          digitalWrite(PIN_AUDIO, HIGH);
        } else {
          digitalWrite(PIN_AUDIO, LOW);
        }
      }
    } else {
      digitalWrite(PIN_AUDIO, LOW);
    }
  } else {
    digitalWrite(PIN_AUDIO, LOW);
  }
}