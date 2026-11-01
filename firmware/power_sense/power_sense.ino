// Power Sense — ESP32-C3 Super Mini AC power monitor
// Repository: https://github.com/pradeshwaster-design/Power-Sense
// License: MIT
//
// Hardware: ESP32-C3 Super Mini, ZMPT101B voltage sensor,
//           passive 5 A CT module with a 1.65 V bias network.
//
// Connect to WiFi "PowerSense" (password 12345678), then open:
//   Dashboard:   http://192.168.4.1
//   Calibration: http://192.168.4.1/cal   (not linked from the dashboard)

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define V_PIN 3             // ZMPT101B OUT
#define I_PIN 4             // CT module S (via 1k)

// ---- Calibration (defaults; overwritten by saved values from the /cal page) ----
float V_CAL   = 0.305;      // volts per raw count
float I_CAL   = 0.005;      // amps per raw count
float V_NOISE = 2.0;        // raw counts RMS with the mains off
float I_NOISE = 1.3;        // raw counts RMS with nothing in the CT
const float I_MIN_A = 0.03; // readings below this (amps) are shown as 0
const float V_MIN_V = 20.0; // below this voltage, show 0 V
const uint32_t WARMUP_MS = 5000;

const uint32_t WINDOW_US = 200000;   // 200 ms = 10 cycles at 50 Hz
const int MAX_SAMPLES = 3000;
static uint16_t vBuf[MAX_SAMPLES];
static uint16_t iBuf[MAX_SAMPLES];

float Vrms = 0, Irms = 0, P = 0;     // calibrated, smoothed outputs
float vRawS = 0, iRawS = 0;          // smoothed raw RMS (counts)
float vCs = 0, iCs = 0;              // smoothed raw RMS with idle noise removed (counts)
bool primed = false;

WebServer server(80);
Preferences prefs;

// ---------- Dashboard page ----------
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PowerSense</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;text-align:center;margin:0;padding:16px}
h1{font-size:22px}
.c{display:inline-block;width:140px;margin:8px;padding:16px;background:#222;border-radius:12px}
.v{font-size:30px;font-weight:bold}
.l{font-size:13px;color:#999}
</style></head><body>
<h1>PowerSense</h1>
<div class="c"><div class="v" id="v">--</div><div class="l">Voltage (V)</div></div>
<div class="c"><div class="v" id="i">--</div><div class="l">Current (A)</div></div>
<div class="c"><div class="v" id="p">--</div><div class="l">Power (W)</div></div>
<script>
function u(){fetch('/data').then(r=>r.json()).then(d=>{
v.textContent=d.v.toFixed(1);i.textContent=d.i.toFixed(3);p.textContent=d.p.toFixed(1);}).catch(()=>{});}
u();setInterval(u,1000);
</script></body></html>
)rawliteral";

// ---------- Hidden calibration page ----------
const char CALPAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Calibration</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:16px;max-width:420px;margin:auto}
h1{font-size:20px;text-align:center}
.b{background:#222;border-radius:12px;padding:14px;margin:12px 0}
.l{font-size:13px;color:#999}
input{width:100%;box-sizing:border-box;padding:10px;margin:6px 0;border-radius:8px;border:1px solid #444;background:#111;color:#eee;font-size:16px}
button{width:100%;padding:12px;margin-top:6px;border:0;border-radius:8px;background:#2a7;color:#fff;font-size:16px}
button.g{background:#555}
#m{text-align:center;min-height:20px;color:#7d7}
</style></head><body>
<h1>Calibration</h1>
<div class="b">
<div class="l">Live readings</div>
<div>Voltage: <b id="dv">--</b> V &nbsp; Current: <b id="di">--</b> A</div>
<div class="l">Signal (counts): V <span id="rv">--</span> | I <span id="ri">--</span></div>
<div class="l">Factors: V_CAL <span id="vc">--</span> | I_CAL <span id="ic">--</span></div>
</div>
<div class="b">
<div class="l">Real voltage (V) from a multimeter</div>
<input id="inv" type="number" step="any" placeholder="e.g. 230">
<div class="l">Real current (A) from a clamp meter or known load (W / V)</div>
<input id="ini" type="number" step="any" placeholder="e.g. 0.44">
<button onclick="save()">Save calibration</button>
</div>
<div class="b">
<div class="l">With the CT empty and the mains off, tap this to measure the idle noise</div>
<button class="g" onclick="act('/zero')">Zero idle noise</button>
<button class="g" onclick="act('/reset')">Reset to defaults</button>
</div>
<div id="m"></div>
<script>
function g(x){return document.getElementById(x)}
function u(){fetch('/data').then(r=>r.json()).then(d=>{
g('dv').textContent=d.v.toFixed(1);g('di').textContent=d.i.toFixed(3);
g('rv').textContent=d.rv.toFixed(1);g('ri').textContent=d.ri.toFixed(1);
g('vc').textContent=d.vc.toFixed(5);g('ic').textContent=d.ic.toFixed(5);}).catch(()=>{});}
function show(t){g('m').textContent=t;u();}
function save(){
 var q='/setcal?v='+(g('inv').value||0)+'&i='+(g('ini').value||0);
 fetch(q).then(r=>r.text()).then(show);}
function act(p){fetch(p).then(r=>r.text()).then(show);}
u();setInterval(u,1000);
</script></body></html>
)rawliteral";

// ---------- Saved settings ----------
void loadCal() {
  prefs.begin("ps", true);
  V_CAL   = prefs.getFloat("vcal", V_CAL);
  I_CAL   = prefs.getFloat("ical", I_CAL);
  V_NOISE = prefs.getFloat("vn",   V_NOISE);
  I_NOISE = prefs.getFloat("in",   I_NOISE);
  prefs.end();
}

void saveCal() {
  prefs.begin("ps", false);
  prefs.putFloat("vcal", V_CAL);
  prefs.putFloat("ical", I_CAL);
  prefs.putFloat("vn",   V_NOISE);
  prefs.putFloat("in",   I_NOISE);
  prefs.end();
}

// ---------- Measurement ----------
void measure() {
  // 1) Capture both channels over a whole number of AC cycles
  int n = 0;
  uint32_t t0 = micros();
  while ((micros() - t0) < WINDOW_US && n < MAX_SAMPLES) {
    vBuf[n] = analogRead(V_PIN);
    iBuf[n] = analogRead(I_PIN);
    n++;
  }
  if (n < 2) return;   // not enough samples; keep the previous values

  // 2) DC offset (bias point) = mean of each channel
  double vSum = 0, iSum = 0;
  for (int k = 0; k < n; k++) { vSum += vBuf[k]; iSum += iBuf[k]; }
  double vOff = vSum / n, iOff = iSum / n;

  // 3) RMS and average instantaneous power (raw counts)
  double vSq = 0, iSq = 0, pSum = 0;
  for (int k = 0; k < n; k++) {
    double v = vBuf[k] - vOff;
    double i = iBuf[k] - iOff;
    vSq += v * v; iSq += i * i; pSum += v * i;
  }
  float vRaw = sqrt(vSq / n);
  float iRaw = sqrt(iSq / n);

  // 4) Remove idle noise (subtract in quadrature)
  float vC = sqrtf(fmaxf(0.0f, vRaw * vRaw - V_NOISE * V_NOISE));
  float iC = sqrtf(fmaxf(0.0f, iRaw * iRaw - I_NOISE * I_NOISE));

  // 5) Apply calibration and dead-bands
  float v = vC * V_CAL;
  float i = iC * I_CAL;
  float p = fabs((pSum / n) * V_CAL * I_CAL);
  if (v < V_MIN_V) { v = 0; i = 0; p = 0; }
  if (i < I_MIN_A) { i = 0; p = 0; }

  // 6) Smoothing (starts from the first real reading, not from zero)
  if (!primed) {
    Vrms = v; Irms = i; P = p;
    vRawS = vRaw; iRawS = iRaw; vCs = vC; iCs = iC;
    primed = true;
  } else {
    const float a = 0.3;
    Vrms  += a * (v - Vrms);
    Irms  += a * (i - Irms);
    P     += a * (p - P);
    vRawS += a * (vRaw - vRawS);
    iRawS += a * (iRaw - iRawS);
    vCs   += a * (vC - vCs);
    iCs   += a * (iC - iCs);
  }
}

// ---------- Web handlers ----------
void handleRoot() { server.send_P(200, "text/html", PAGE); }
void handleCal()  { server.send_P(200, "text/html", CALPAGE); }

void handleData() {
  char buf[200];
  snprintf(buf, sizeof(buf),
           "{\"v\":%.1f,\"i\":%.3f,\"p\":%.1f,\"rv\":%.1f,\"ri\":%.1f,\"vc\":%.6f,\"ic\":%.6f}",
           Vrms, Irms, P, vCs, iCs, V_CAL, I_CAL);
  server.send(200, "application/json", buf);
}

// Enter the real values; the factors are computed from the current signal level
void handleSetCal() {
  float rv = server.arg("v").toFloat();
  float ri = server.arg("i").toFloat();
  String msg = "";
  if (rv > 0) {
    if (vCs > 50) { V_CAL = rv / vCs; msg += "Voltage saved. "; }
    else msg += "Voltage signal too low. ";
  }
  if (ri > 0) {
    if (iCs > 5) { I_CAL = ri / iCs; msg += "Current saved. "; }
    else msg += "Current signal too low: connect a load of about 0.1 A or more. ";
  }
  if (msg.length() == 0) msg = "Enter a value first.";
  saveCal();
  primed = false;
  server.send(200, "text/plain", msg);
}

// Record the idle noise (CT empty, mains off) so it is subtracted
void handleZero() {
  V_NOISE = vRawS;
  I_NOISE = iRawS;
  saveCal();
  primed = false;
  server.send(200, "text/plain", "Idle noise saved.");
}

void handleReset() {
  V_CAL = 0.305; I_CAL = 0.005; V_NOISE = 2.0; I_NOISE = 1.3;
  saveCal();
  primed = false;
  server.send(200, "text/plain", "Defaults restored.");
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(V_PIN, ADC_11db);
  analogSetPinAttenuation(I_PIN, ADC_11db);
  loadCal();

  WiFi.mode(WIFI_AP);
  WiFi.softAP("PowerSense", "12345678");
  WiFi.setTxPower(WIFI_POWER_8_5dBm);   // helps stability on SuperMini boards

  server.on("/", handleRoot);
  server.on("/cal", handleCal);
  server.on("/data", handleData);
  server.on("/setcal", handleSetCal);
  server.on("/zero", handleZero);
  server.on("/reset", handleReset);
  server.begin();

  Serial.print("Open http://");
  Serial.println(WiFi.softAPIP());

  // Warm-up: let the bias capacitor charge and the ADC settle
  uint32_t w0 = millis();
  while (millis() - w0 < WARMUP_MS) {
    measure();
    server.handleClient();
  }
  primed = false;
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > 800) {
    measure();
    last = millis();
    Serial.printf("V: %.1f V | I: %.3f A | P: %.1f W\n", Vrms, Irms, P);
  }
  server.handleClient();
}
