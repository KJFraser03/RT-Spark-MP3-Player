#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <Arduino.h>

const int NUM_TRACKS = 4;

// --- Track Data Structure ---
struct Track {
  const char* title;
  const unsigned int* frequencies;
  const unsigned int length;
};

// --- Track Frequencies ---
const unsigned int track1_Frequencies[] = { 1046, 1175, 1318, 1396, 1568, 1760, 1975, 2093, 2093, 1975, 1760, 1568, 1396, 1318, 1175, 1046 };
const unsigned int track2_Frequencies[] = { 440, 0, 440, 0, 440, 0, 349, 0, 523, 0, 440, 0, 349, 0, 523, 0, 440, 0 };
const unsigned int track3_Frequencies[] = { 660, 0, 660, 0, 0, 660, 0, 0, 510, 0, 660, 0, 0, 770, 0, 0, 0, 0, 380, 0 };
const unsigned int track4_Frequencies[] = { 466, 0, 349, 0, 0, 466, 0, 523, 0, 587, 0, 622, 0, 698, 0, 0, 0, 698 };

// --- Named Playlist ---
const Track playlist[NUM_TRACKS] = {
  { "1. Pitch Scale",       track1_Frequencies, sizeof(track1_Frequencies) / sizeof(track1_Frequencies[0]) },
  { "2. Imperial March",    track2_Frequencies, sizeof(track2_Frequencies) / sizeof(track2_Frequencies[0]) },
  { "3. Super Mario Bros", track3_Frequencies, sizeof(track3_Frequencies) / sizeof(track3_Frequencies[0]) },
  { "4. Zelda Theme",       track4_Frequencies, sizeof(track4_Frequencies) / sizeof(track4_Frequencies[0]) }
};

// --- Modular Display Hook (Serial Telemetry + Ready for Hardware OLED) ---
inline void updateScreen(int trackIndex, bool playing, int volPercent) {
  // Serial Telemetry Log
  Serial1.print("[DISPLAY UPDATE] Track: ");
  Serial1.print(playlist[trackIndex].title);
  Serial1.print(playing ? " | Status: PLAYING" : " | Status: PAUSED");
  Serial1.print(" | Vol: ");
  Serial1.print(volPercent);
  Serial1.println("%");
}

inline unsigned int getActiveFrequency(int track, unsigned int index) {
  return playlist[track].frequencies[index];
}

inline unsigned int getActiveTrackLength(int track) {
  return playlist[track].length;
}

#endif