// ----- config -----
const int TOUCH_PIN = T9;   // GPIO32
const int LED_PIN   = 2;

// lower threshold; we’ll also use a step trigger
const float THRESH_N = 0.03f;   // 3% drop
const int   STEP_MIN = 10;      // counts step
const int   DEBOUNCE = 30;      // ms

float base=0, filt=0;

void setup(){
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(300);

  // Longer integration -> higher sensitivity/SNR (2-arg in ESP32 core 2.x)
  touchSetCycles(0x7FFF, 0x7FFF);   // max practical
  // If available in your core, uncomment (else ignore):
  // touchSetVoltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

  // 1s baseline after everything is still/open
  for(int i=0;i<100;i++){ base = 0.9*base + 0.1*touchRead(TOUCH_PIN); delay(10); }
  filt = base;
}

void flash3(){ for(int i=0;i<3;i++){ digitalWrite(LED_PIN,1); delay(70); digitalWrite(LED_PIN,0); delay(70);} }

void loop(){
  static uint32_t okSince=0;
  static int prev=0;

  int raw = touchRead(TOUCH_PIN);           // lower = closer
  filt = 0.7f*filt + 0.3f*raw;
  float d = (base - filt) / (base > 1 ? base : 1);

  bool near = d > THRESH_N;                 // normalized drop
  bool jab  = (prev - raw) > STEP_MIN;      // fast approach
  prev = raw;

  uint32_t now = millis();
  if (near || jab){
    if (!okSince) okSince = now;
    if (now - okSince >= DEBOUNCE){ flash3(); okSince=0; delay(300); }
  } else okSince = 0;

  delay(10); // ~100 Hz
}