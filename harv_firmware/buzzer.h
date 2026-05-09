#pragma once

// ── Passive buzzer on GPIO 25 ─────────────────────────────────
// Uses ESP32 ledc PWM. Non-blocking: play() starts a sequence,
// update() advances it — call update() every loop iteration.

#define BUZZER_PIN 25

namespace Buzzer {

  struct Note { uint16_t freq; uint16_t dur; }; // freq Hz (0 = rest), dur ms

  static Note      _seq[8];
  static uint8_t   _seqLen = 0;
  static uint8_t   _seqIdx = 0;
  static unsigned long _noteEnd = 0;
  static bool      _playing = false;

  void off() {
    ledcWrite(BUZZER_PIN, 0);
    _playing = false;
  }

  void _startNote(uint8_t i) {
    if (_seq[i].freq == 0) {
      ledcWrite(BUZZER_PIN, 0);        // rest
    } else {
      ledcWriteTone(BUZZER_PIN, _seq[i].freq);
    }
    _noteEnd = millis() + _seq[i].dur;
    _seqIdx  = i;
    _playing = true;
  }

  void play(const Note* notes, uint8_t len) {
    if (len == 0) return;
    _seqLen = min(len, (uint8_t)8);
    memcpy(_seq, notes, _seqLen * sizeof(Note));
    _startNote(0);
  }

  void begin() {
    ledcAttach(BUZZER_PIN, 2000, 8);   // 2 kHz default, 8-bit resolution
    ledcWrite(BUZZER_PIN, 0);
  }

  void update() {
    if (!_playing) return;
    if (millis() >= _noteEnd) {
      uint8_t next = _seqIdx + 1;
      if (next < _seqLen) _startNote(next);
      else                off();
    }
  }

  // ── Named sounds ─────────────────────────────────────────────

  void boot() {
    static const Note s[] = {{523,80},{659,80},{784,80},{1047,140}};
    play(s, 4);
  }

  void chirp() {                       // touch feedback
    static const Note s[] = {{1200,35},{1600,55}};
    play(s, 2);
  }

  void happy() {
    static const Note s[] = {{523,60},{659,60},{784,60},{1047,100}};
    play(s, 4);
  }

  void excited() {
    static const Note s[] = {{784,50},{1047,50},{0,30},{1047,50},{1319,100}};
    play(s, 5);
  }

  void sad() {
    static const Note s[] = {{523,120},{440,120},{392,180}};
    play(s, 3);
  }

  void scared() {
    static const Note s[] = {{800,45},{600,45},{400,90}};
    play(s, 3);
  }

  void alert() {
    static const Note s[] = {{880,90},{0,40},{880,90}};
    play(s, 3);
  }

  void tap() {
    static const Note s[] = {{1000,25}};
    play(s, 1);
  }

  void curious() {
    static const Note s[] = {{880,60},{1047,90}};
    play(s, 2);
  }

  // Play by name — used by harv/buzzer MQTT topic
  void playNamed(const String& name) {
    if      (name == "boot")    boot();
    else if (name == "chirp")   chirp();
    else if (name == "happy")   happy();
    else if (name == "excited") excited();
    else if (name == "sad")     sad();
    else if (name == "scared")  scared();
    else if (name == "alert")   alert();
    else if (name == "tap")     tap();
    else if (name == "curious") curious();
    else if (name == "off")     off();
  }

}
