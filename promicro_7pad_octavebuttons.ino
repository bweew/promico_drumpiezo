#include <MIDIUSB.h>

// -------- Piezo Settings --------
#define SENSITIVITY 80
#define DELTA 30
#define RELEASE 15
#define SAMPLES 5
#define MIN_HIT_INTERVAL 20  // ms

#define NOTE_A0 60
#define NOTE_A1 61
#define NOTE_A2 62
#define NOTE_A3 63
#define NOTE_A6 64
#define NOTE_A7 65
#define NOTE_A8 66

const int piezoPins[] = {A0, A1, A2, A3, A6, A7, A8};
const int baseNotes[] = {NOTE_A0, NOTE_A1, NOTE_A2, NOTE_A3, NOTE_A6, NOTE_A7, NOTE_A8};
const int padCount    = sizeof(piezoPins) / sizeof(piezoPins[0]);

int baselines[padCount];
bool hitActive[padCount];
unsigned long lastHitTime[padCount];

// -------- Button Settings --------
const int btnOctUp   = 14;
const int btnOctDown = 16;

bool prevUpState   = HIGH;
bool prevDownState = HIGH;

int octaveShift = 0;  // in semitones, +/- 12 steps

// -------- MIDI Helpers --------
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

int readPiezo(int pin) {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) sum += analogRead(pin);
  return sum / SAMPLES;
}

// -------- Setup --------
void setup() {
  for (int i = 0; i < padCount; i++) {
    pinMode(piezoPins[i], INPUT);
    long sum = 0;
    for (int j = 0; j < 50; j++) sum += readPiezo(piezoPins[i]);
    baselines[i] = sum / 50;
    hitActive[i] = false;
    lastHitTime[i] = 0;
  }

  pinMode(btnOctUp, INPUT_PULLUP);
  pinMode(btnOctDown, INPUT_PULLUP);
}

// -------- Loop --------
void loop() {
  unsigned long now = millis();

  // --- Piezo pads ---
  for (int i = 0; i < padCount; i++) {
    int value = readPiezo(piezoPins[i]);
    int velocityRaw = value - baselines[i];

    if (!hitActive[i] && velocityRaw > DELTA && now - lastHitTime[i] > MIN_HIT_INTERVAL) {
      bool otherPadRecentlyHit = false;
      for (int j = 0; j < padCount; j++) {
        if (i != j && now - lastHitTime[j] < MIN_HIT_INTERVAL) {
          otherPadRecentlyHit = true;
          break;
        }
      }
      if (otherPadRecentlyHit) continue;

      int vel127 = map(velocityRaw, DELTA, SENSITIVITY, 30, 127);
      vel127 = constrain(vel127, 30, 127);

      int note = baseNotes[i] + octaveShift;
      note = constrain(note, 0, 127);

      noteOn(0, note, vel127);
      MidiUSB.flush();
      noteOff(0, note, vel127);
      MidiUSB.flush();

      hitActive[i] = true;
      lastHitTime[i] = now;
    }

    if (hitActive[i] && velocityRaw < RELEASE) {
      hitActive[i] = false;
    }
  }

  // --- Octave buttons ---
  bool currUpState   = digitalRead(btnOctUp);
  bool currDownState = digitalRead(btnOctDown);

  if (prevUpState == HIGH && currUpState == LOW) {
    octaveShift += 12;
    if (octaveShift > 48) octaveShift = 48;  // limit to +4 octaves
  }

  if (prevDownState == HIGH && currDownState == LOW) {
    octaveShift -= 12;
    if (octaveShift < -48) octaveShift = -48; // limit to -4 octaves
  }

  prevUpState   = currUpState;
  prevDownState = currDownState;

  delay(5);
}
