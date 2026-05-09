#pragma once

// ── HarvSound: passive PWM buzzer on GPIO 25 ─────────────────
// LEDC channel 0, 8-bit resolution.
// Non-blocking: play queues a note sequence; update() advances it.
// Call begin() in setup(), update() every loop iteration.

#define HARVSOUND_PIN     25
#define HARVSOUND_CHANNEL 0

namespace HarvSound {

  struct Note { uint16_t freq; uint16_t dur; }; // freq Hz (0 = rest), dur ms

  static Note          _seq[12];
  static uint8_t       _seqLen  = 0;
  static uint8_t       _seqIdx  = 0;
  static unsigned long _noteEnd = 0;
  static bool          _playing = false;

  // ── Core ─────────────────────────────────────────────────────

  void silence() {
    ledcWrite(HARVSOUND_CHANNEL, 0);
    _playing = false;
  }

  void _startNote(uint8_t i) {
    if (_seq[i].freq == 0) {
      ledcWrite(HARVSOUND_CHANNEL, 0);          // rest
    } else {
      ledcWriteTone(HARVSOUND_CHANNEL, _seq[i].freq);
    }
    _noteEnd = millis() + _seq[i].dur;
    _seqIdx  = i;
    _playing = true;
  }

  void _playSeq(const Note* notes, uint8_t len) {
    if (len == 0) return;
    _seqLen = min(len, (uint8_t)12);
    memcpy(_seq, notes, _seqLen * sizeof(Note));
    _startNote(0);
  }

  // Play a single tone for durationMs, then silence (non-blocking).
  void playTone(uint16_t frequency, uint16_t durationMs) {
    static Note n;
    n = { frequency, durationMs };
    _playSeq(&n, 1);
  }

  void begin() {
    ledcSetup(HARVSOUND_CHANNEL, 2000, 8);
    ledcAttachPin(HARVSOUND_PIN, HARVSOUND_CHANNEL);
    ledcWrite(HARVSOUND_CHANNEL, 0);
  }

  void update() {
    if (!_playing) return;
    if (millis() >= _noteEnd) {
      uint8_t next = _seqIdx + 1;
      if (next < _seqLen) _startNote(next);
      else                silence();
    }
  }

  // ── Emotional sounds ──────────────────────────────────────────

  // Ascending three-note startup sequence
  void soundBoot() {
    static const Note s[] = {
      {523, 100}, {659, 100}, {784, 180}
    };
    _playSeq(s, 3);
  }

  // Quick soft chirp — touch feedback
  void soundTouch() {
    static const Note s[] = {
      {1200, 30}, {1600, 50}
    };
    _playSeq(s, 2);
  }

  // Warm rising two-note — someone came home
  void soundHomecoming() {
    static const Note s[] = {
      {392, 200}, {523, 300}
    };
    _playSeq(s, 2);
  }

  // Fast ascending chirp sequence — joy
  void soundHappy() {
    static const Note s[] = {
      {523, 60}, {659, 60}, {784, 60}, {1047, 110}
    };
    _playSeq(s, 4);
  }

  // Rapid irregular short beeps — on edge
  void soundAnxious() {
    static const Note s[] = {
      {880, 40}, {0, 20}, {1100, 30}, {0, 35},
      {880, 40}, {0, 15}, {1100, 55}
    };
    _playSeq(s, 7);
  }

  // Slow descending tone — low mood
  void soundSad() {
    static const Note s[] = {
      {523, 200}, {440, 200}, {392, 280}
    };
    _playSeq(s, 3);
  }

  // Harsh rapid beeps — overwhelmed
  void soundStressed() {
    static const Note s[] = {
      {1200, 35}, {0, 20}, {1200, 35}, {0, 20},
      {1200, 35}, {0, 20}, {1200, 70}
    };
    _playSeq(s, 7);
  }

  // Rising questioning tone — what's that?
  void soundCurious() {
    static const Note s[] = {
      {784, 80}, {0, 30}, {1047, 130}
    };
    _playSeq(s, 3);
  }

  // Slow descending fade — running low
  void soundSleepy() {
    static const Note s[] = {
      {440, 220}, {392, 240}, {349, 320}
    };
    _playSeq(s, 3);
  }

  // Groggy ascending — slow start, picks up
  void soundWakeup() {
    static const Note s[] = {
      {392, 220}, {0, 100}, {440, 180},
      {0,   60},  {523, 150}, {659, 130}
    };
    _playSeq(s, 6);
  }

  // Play by name — used by harv/buzzer MQTT topic
  void playNamed(const String& name) {
    if      (name == "boot")       soundBoot();
    else if (name == "touch")      soundTouch();
    else if (name == "homecoming") soundHomecoming();
    else if (name == "happy")      soundHappy();
    else if (name == "anxious")    soundAnxious();
    else if (name == "sad")        soundSad();
    else if (name == "stressed")   soundStressed();
    else if (name == "curious")    soundCurious();
    else if (name == "sleepy")     soundSleepy();
    else if (name == "wakeup")     soundWakeup();
    else if (name == "off")        silence();
  }

}
