/*
  Power Sense — ESP32-C3 Super Mini AC energy monitor
  ---------------------------------------------------
  ZMPT101B  OUT -> GPIO3 (ADC1_CH3)
  ZMCT103C  OUT -> GPIO4 (ADC1_CH4)
  Both sensors powered from 3V3, outputs biased at about 1.65 V.

  No divider is used at 3.3 V. The LM358 inside the modules is not
  rail-to-rail and clips near 1.8 V, so keep the trimpots low and
  watch the raw min/max printed below.

  Calibrate V_CAL and I_CAL before trusting the readings.
*/

#include <Arduino.h>

// ---------- Pins ----------
constexpr uint8_t V_PIN = 3;   // ZMPT101B OUT
constexpr uint8_t I_PIN = 4;   // ZMCT103C OUT

// ---------- Sampling ----------
constexpr int      N         = 400;    // sample pairs per window
constexpr uint32_t SAMPLE_US = 100;    // 100 us -> 40 ms window (2 cycles at 50 Hz)
constexpr float    WINDOW_S  = (N * SAMPLE_US) / 1000000.0f;

// ---------- Calibration (edit after measuring) ----------
// Volts RMS and amps RMS per 12-bit ADC count. These are rough defaults.
float V_CAL = 1.60f;
float I_CAL = 0.036f;

// ---------- Limits ----------
constexpr uint16_t ADC_MAX_WARN = 2300;   // about 1.75 V at 11 dB; op-amp clips near 1.8 V
constexpr uint16_t SIGNAL_MIN   = 8;      // raw peak-to-peak below this means no signal
constexpr float    OVERCURRENT_A = 5.0f;

static uint16_t vBuf[N];
static uint16_t iBuf[N];
static float    energyKwh = 0.0f;
static uint32_t lastPrint = 0;

void setup() {
  Serial.begin(115200);
  delay(400);

  analogReadResolution(12);
  // Older cores use ADC_11db; newer cores also accept ADC_11DB / ADC_ATTEN_DB_12.
  analogSetPinAttenuation(V_PIN, ADC_11db);
  analogSetPinAttenuation(I_PIN, ADC_11db);

  Serial.println();
  Serial.println("Power Sense - ESP32-C3 energy monitor");
  Serial.printf("V_CAL=%.4f  I_CAL=%.5f\n", V_CAL, I_CAL);
  Serial.println("Calibrate against a multimeter and a known load.");
}

void loop() {
  uint16_t vMin = 4095, vMax = 0;
  uint16_t iMin = 4095, iMax = 0;
  uint32_t next = micros();

  // Sample both channels on a fixed 100 us grid.
  for (int n = 0; n < N; n++) {
    while ((int32_t)(micros() - next) < 0) {
      // wait for the next slot
    }
    next += SAMPLE_US;

    uint16_t v = (uint16_t)analogRead(V_PIN);
    uint16_t i = (uint16_t)analogRead(I_PIN);

    vBuf[n] = v;
    iBuf[n] = i;

    if (v < vMin) vMin = v;
    if (v > vMax) vMax = v;
    if (i < iMin) iMin = i;
    if (i > iMax) iMax = i;
  }

  // DC offset for this window.
  float vOff = 0, iOff = 0;
  for (int n = 0; n < N; n++) {
    vOff += vBuf[n];
    iOff += iBuf[n];
  }
  vOff /= N;
  iOff /= N;

  // RMS and real power.
  float vSq = 0, iSq = 0, vi = 0;
  for (int n = 0; n < N; n++) {
    float v = vBuf[n] - vOff;
    float i = iBuf[n] - iOff;
    vSq += v * v;
    iSq += i * i;
    vi  += v * i;
  }

  float Vrms = sqrtf(vSq / N) * V_CAL;
  float Irms = sqrtf(iSq / N) * I_CAL;
  float P    = (vi / N) * V_CAL * I_CAL;
  float S    = Vrms * Irms;
  float PF   = (S > 0.5f) ? (P / S) : 0.0f;

  energyKwh += P * WINDOW_S / 3600000.0f;

  const char* warn = "";
  if (vMax > ADC_MAX_WARN || iMax > ADC_MAX_WARN) {
    warn = "CLIP: turn trimpot down";
  } else if ((uint16_t)(vMax - vMin) < SIGNAL_MIN) {
    warn = "NO V SIGNAL";
  } else if (Irms > OVERCURRENT_A) {
    warn = "OVERCURRENT";
  }

  if (millis() - lastPrint >= 500) {
    lastPrint = millis();
    Serial.printf(
        "V=%.1f V  I=%.3f A  P=%.1f W  PF=%.2f  kWh=%.4f  "
        "raw V[%u..%u] I[%u..%u]  %s\n",
        Vrms, Irms, P, PF, energyKwh,
        (unsigned)vMin, (unsigned)vMax,
        (unsigned)iMin, (unsigned)iMax,
        warn);
  }
}
