#include <Wire.h>
#include <VL53L0X.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PulseSensorPlayground.h>
#include <LiquidCrystal_I2C.h>
#include <HX711_ADC.h>
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>

// -------- Distance Sensor --------
VL53L0X sensor;

// -------- Temperature Sensor --------
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
bool heightmeasurementDone = false;
bool weightmeasurementDone = false;
bool tempmeasurementDone = false;
bool pulsemeasurementDone = false;
const char* bmiCategory;
float bmiValue;
// -------- Pulse Sensor --------
const int PulseWire = A0;
const int LED = LED_BUILTIN;
int Threshold = 550;
PulseSensorPlayground pulseSensor;

#define MODE_BUTTON 6
byte mode = 0;   // 0=Height,1=Weight,2=Temp,3=Pulse

unsigned long measureStartTime;
const unsigned long measureDuration = 30000;  // 30 seconds

float last10[10];
int indexVal = 0;
int countVal = 0;
bool buttonPressed = false;


// -------- LCD --------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------- HX711 --------
const int HX711_dout = 4;
const int HX711_sck = 5;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

float calibrationValue = 21.15;  
int height = 0;
float avgHeight;
float avgWeight;
bool bmicalculate=false;

void addValue(float val) {
  last10[indexVal] = val;
  indexVal++;
  if (indexVal >= 10) indexVal = 0;

  if (countVal < 10) countVal++;
}

float getAverage() {
  float sum = 0;
  for (int i = 0; i < countVal; i++) {
    sum += last10[i];
  }
  if (countVal == 0) return 0;
  return sum / countVal;
}
// -------- MP3 Speaker --------
SoftwareSerial mp3Serial(8,7); // RX,TX

class Mp3Notify
{
public:

  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source == DfMp3_PlaySources_Sd)
      Serial.print("SD Card ");
    else if (source == DfMp3_PlaySources_Usb)
      Serial.print("USB ");
    else
      Serial.print("Flash ");

    Serial.println(action);
  }

  static void OnError(DFMiniMp3<SoftwareSerial, Mp3Notify>& mp3, uint16_t errorCode)
  {
    Serial.print("MP3 Error:");
    Serial.println(errorCode);
  }

  static void OnPlayFinished(DFMiniMp3<SoftwareSerial, Mp3Notify>& mp3,
                             DfMp3_PlaySources source,
                             uint16_t track)
  {
    Serial.print("Track finished:");
    Serial.println(track);
  }

  static void OnPlaySourceOnline(DFMiniMp3<SoftwareSerial, Mp3Notify>& mp3,
                                 DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }

  static void OnPlaySourceInserted(DFMiniMp3<SoftwareSerial, Mp3Notify>& mp3,
                                   DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }

  static void OnPlaySourceRemoved(DFMiniMp3<SoftwareSerial, Mp3Notify>& mp3,
                                  DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};

typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
DfMp3 mp3(mp3Serial);

// -----------------------------------
void bmifunction(){
  if (bmicalculate == true) {

  if (avgHeight > 0 && avgWeight > 0) {

    float heightM = avgHeight / 100.0;
    bmiValue = (avgWeight/1000) / (heightM * heightM);

    if (bmiValue < 18.5) {
    bmiCategory = "Underweight";
    mp3.playMp3FolderTrack(2);   // 0002.mp3
  }
  else if (bmiValue < 25) {
    bmiCategory = "Normal";
    mp3.playMp3FolderTrack(1);   // 0001.mp3
  }
  else {
    bmiCategory = "Overweight";
    mp3.playMp3FolderTrack(3);   // 0003.mp3
  }

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("BMI:");
    lcd.print(bmiValue,1);

    lcd.setCursor(0,1);
    lcd.print(bmiCategory);

    Serial.print("BMI: ");
    Serial.println(bmiValue);
    Serial.print("Category: ");
    Serial.println(bmiCategory);

    delay(3000);
  }
  else {
    lcd.clear();
    lcd.print("Invalid Data");
    delay(2000);
  }

  bmicalculate = false;
}
}



void setup() {
  Serial.begin(115200);

  pinMode(MODE_BUTTON, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Starting");
  delay(2000);
  lcd.clear();

  Wire.begin();
sensor.setTimeout(500);

if (!sensor.init()) {
  Serial.println("VL53L0X not detected!");
  while (1);
}

sensor.startContinuous();
  sensors.begin();

  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED);
  pulseSensor.setThreshold(Threshold);
  pulseSensor.begin();

  LoadCell.begin();
  LoadCell.start(2000, true);
  LoadCell.setCalFactor(calibrationValue);

  Serial.println("System Ready...");

  mp3Serial.begin(9600);
  mp3.begin();
  mp3.setVolume(20);   // volume 0–30
}

// -----------------------------------

void loop() {

  mp3.loop();
  // -------- Button Handling --------
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(MODE_BUTTON);

  if (currentButtonState == LOW) {
    mode++;
    if (mode > 3) 
    {
      mode = 0;
      heightmeasurementDone = false;
      weightmeasurementDone = false;
      tempmeasurementDone = false;
      pulsemeasurementDone = false;
      bmicalculate = false;
    }
    
    lcd.clear();
      // restart full measurement cycle
        // reset for new mode
    //delay(300); // debounce
  }

  lastButtonState = currentButtonState;

  // =====================================
  // ============ MODE 0 =================
  // ============ HEIGHT =================
  // =====================================
if (mode == 0 && heightmeasurementDone == false) {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Measuring...");
  lcd.setCursor(0,1);
  lcd.print("Stand Straight");

  measureStartTime = millis();
  indexVal = 0;
  countVal = 0;

  while (millis() - measureStartTime < measureDuration) {

  yield();  // important

  int distanceMM = sensor.readRangeContinuousMillimeters();

  if (!sensor.timeoutOccurred()) {

    int distanceCM = distanceMM / 10;
    float heightVal = 206 - distanceCM;

    addValue(heightVal);
  }

  delay(300);
}

  avgHeight = getAverage();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Height:");
  lcd.setCursor(0,1);
  lcd.print(avgHeight,1);
  lcd.print(" cm");

  Serial.print("Final Height: ");
  Serial.println(avgHeight);

  heightmeasurementDone = true;
}
  // =====================================
  // ============ MODE 1 =================
  // ============ WEIGHT =================
  // =====================================
  if (mode == 1 && weightmeasurementDone == false && heightmeasurementDone == true) {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Measuring...");
  lcd.setCursor(0,1);
  lcd.print("Stand Still");

  measureStartTime = millis();
  indexVal = 0;
  countVal = 0;

  while (millis() - measureStartTime < measureDuration) {

    if (LoadCell.update()) {
      float weightg = LoadCell.getData();
      addValue(weightg);
    }

    delay(300);
  }

  avgWeight = getAverage();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Weight:");
  lcd.setCursor(0,1);
  lcd.print(avgWeight/1000,2);
  lcd.print("kg");

  Serial.print("Final Weight: ");
  Serial.println(avgWeight/1000);

  weightmeasurementDone = true;
}
  // =====================================
  // ============ MODE 2 =================
  // ============ TEMP ===================
  // =====================================
  if (mode == 2 && tempmeasurementDone == false && weightmeasurementDone ==true) {

  lcd.clear();
  lcd.print("Measuring Temp");

  measureStartTime = millis();
  indexVal = 0;
  countVal = 0;

  while (millis() - measureStartTime < measureDuration) {

    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    addValue(tempC);

    delay(500);
  }

  float avgTemp = getAverage();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temperature:");
  lcd.setCursor(0,1);
  lcd.print(avgTemp,1);
  lcd.print(" C");

  Serial.print("Final Temp: ");
  Serial.println(avgTemp);

  tempmeasurementDone = true;
}

  // =====================================
  // ============ MODE 3 =================
  // ============ PULSE ==================
  // =====================================
 if (mode == 3 && pulsemeasurementDone == false && tempmeasurementDone == true) {

  lcd.clear();
  lcd.print("Measuring Pulse");

  measureStartTime = millis();
  indexVal = 0;
  countVal = 0;

  while (millis() - measureStartTime < measureDuration) {

    int bpm = pulseSensor.getBeatsPerMinute();
    if (pulseSensor.sawStartOfBeat()) {
      addValue(bpm);
    }

    delay(200);
  }

  float avgPulse = getAverage();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pulse Rate:");
  lcd.setCursor(0,1);
  lcd.print(avgPulse,0);
  lcd.print(" BPM");

  Serial.print("Final Pulse: ");
  Serial.println(avgPulse);

  pulsemeasurementDone = true;
  bmicalculate=true;
}
if(pulsemeasurementDone ==true && bmicalculate==true )
bmifunction();
}