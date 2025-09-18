#include <MIDIUSB.h>

// -------- Settings --------
#define SENSITIVITY 80   // lower = more sensitive, higher = less sensitive
#define DELTA 20          // minimum difference from baseline to trigger
#define NOTE 60           // MIDI note to send
#define PIEZO_PIN A0      // D18 on Pro Micro
#define SAMPLES 5         // moving average samples
// ---------------------------

int baseline = 0;
bool hitActive = false;

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

int readPiezo() {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(PIEZO_PIN);
    delay(1);
  }
  return sum / SAMPLES;
}

void setup() {
  pinMode(PIEZO_PIN, INPUT);
  Serial.begin(115200);

  // establish baseline at rest
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += readPiezo();
    delay(5);
  }
  baseline = sum / 50;
  Serial.print("Baseline: ");
  Serial.println(baseline);
}

void loop() {
  int value = readPiezo();
  int velocityRaw = value - baseline;

  if (!hitActive && velocityRaw > DELTA) {
    // map raw sensor to 1–80 range using SENSITIVITY as upper bound
    int vel80 = map(velocityRaw, DELTA, SENSITIVITY, 1, 80);
    vel80 = constrain(vel80, 1, 80);

    int vel127;
    if (vel80 <= 49) {
      vel127 = map(vel80, 1, 49, 1, 78);     // soft
    } else if (vel80 <= 69) {
      vel127 = map(vel80, 50, 69, 79, 109);  // medium
    } else {
      vel127 = map(vel80, 70, 80, 110, 127); // hard
    }

    Serial.print("Hit! Raw=");
    Serial.print(velocityRaw);
    Serial.print(" vel80=");
    Serial.print(vel80);
    Serial.print(" -> MIDI Velocity=");
    Serial.println(vel127);

    noteOn(0, NOTE, vel127);
    MidiUSB.flush();
    delay(10);
    noteOff(0, NOTE, vel127);
    MidiUSB.flush();

    hitActive = true;
  }

  // reset after release
  if (hitActive && velocityRaw < DELTA / 2) {
    hitActive = false;
  }
}
