//Definiranje koristenih biblioteka
#include <SoftwareSerial.h>
#include "ELMduino.h"
#include <FastLED.h>
#include <U8x8lib.h>
#include <Bounce2.h>
#include <EEPROM.h>

//Instanciranje objekata
#define NUM_LEDS 20
CRGBArray<NUM_LEDS> leds;
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(SCL, SDA, U8X8_PIN_NONE);
SoftwareSerial mySerial(2, 3);
#define ELM_PORT mySerial
ELM327 myELM327;

//Postavljanje debouncinga na tipkalo
Bounce debouncer = Bounce();
const int  btnMode = 7;
int btnModePushCounter = 0;           
int btnModeState = 0;                  
int previousBtnModeState = 0;             
int period = 15;                     
unsigned long time_now = 0;

//Deklariranje varijabli za koristenje tipkala
static const int btnSettings = 6;
int btnSettingsPreviousState = LOW;
unsigned long minBtnSettingsLongPressDuration = 4000;
unsigned long minBtnSettingsLongPressMillis;
bool btnModeStateLongPress = false;
const int intervalBtnSettings = 50;
unsigned long previousBtnSettingsMillis;
unsigned long btnSettingsPressDuration;
unsigned long currentMillis;
bool settingsMode = false;
int btnModeForSettingsPushCounter = 0;

//Deklariranje varijabli za koristenje potenciometra
int potentioValue = 0;
int potentioMaped;
int ledsNumb;
int minBrightness;
int maxBrightness;

//Varijable za vrijednosti parametara automobila
uint32_t rpm = 0;
int mapRPM = 0;
int kph = 0;
int mapKPH = 0;
float temp = 0;
int mapTEMP = 0;
int sensorValue = 0;
int mapSensor = 0;

/*------------------------------------------------------------------------ SETUP ------------------------------------------------------------------------------------------*/

void setup()
{
  //Postavljanje tipkala na digitalne nozice
  debouncer.attach(btnMode, INPUT);
  debouncer.interval(10);
  pinMode(btnSettings, INPUT);
  //Postavljanje maksimalnog broja ledica LED trake
  FastLED.addLeds<NEOPIXEL, 10>(leds, NUM_LEDS);
  //Pokretanje komunikacije na objektima
  u8x8.begin();
  Serial.begin(38400);
  //Spajanje s ELM327 adapterom
  ELM_PORT.begin(38400);
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_victoriabold8_r);
  u8x8.setCursor(2, 1);
  u8x8.print("Connecting");
  u8x8.setCursor(6, 3);
  u8x8.print("to");
  u8x8.setCursor(4, 5);
  u8x8.print("OBDII...");
  delay(6000);
  if (!myELM327.begin(ELM_PORT, true, 300, '0'))
  {
    //Neuspjesno povezivanje
    u8x8.clearDisplay();
    u8x8.setCursor(4, 1);
    u8x8.print("Failed to");
    u8x8.setCursor(5, 3);
    u8x8.print("connect");
    u8x8.setCursor(4, 5);
    u8x8.print("with OBDII");
    while(!myELM327.begin(ELM_PORT, true, 300, '0'));
  }
  delay(300);
  //Spajanje uspjesno
  u8x8.clearDisplay();
  u8x8.setCursor(4, 1);
  u8x8.print("Connected");
  u8x8.setCursor(6, 3);
  u8x8.print("to");
  u8x8.setCursor(5, 5);
  u8x8.print("OBDII");
  delay(1500);
  u8x8.clearDisplay();
  delay(2000);
  //Dohvacanje postavki iz EEPROM memorije
  EEPROM.get(0, minBrightness);
  EEPROM.get(2, maxBrightness);
  EEPROM.get(4, ledsNumb);
}

/*-------------------------------------------------------------------------- MAIN LOOP ------------------------------------------------------------------------------------*/

void loop()
{
  //Provjera pritiska Mode i Settings tipkala
  settingsBtnCheck();
  modeBtnCheck();

  //Odredivanje nacina rada s obzirom na pritisak tipkala
  if (btnModePushCounter == 0 && settingsMode == false) {
    modeRPM();
  }
  else if (btnModePushCounter == 1 && settingsMode == false) {
    modeKPH();
  }
  else if (btnModePushCounter == 2 && settingsMode == false) {
    modeTEMP();
  }
  else if (btnModePushCounter == 3 && settingsMode == false) {
    modeLedsOFF();
  }
  //Postavljanje fonta za OLED zaslon
  u8x8.setFont(u8x8_font_victoriabold8_r  );

  //Promjena vrijednosti u postavkama
  if (btnModeForSettingsPushCounter == 0 && settingsMode == true) {
    EEPROM.get(0, minBrightness);
    EEPROM.get(2, maxBrightness);
    EEPROM.get(4, ledsNumb);
    drawSettings();
  }  
  if (btnModeForSettingsPushCounter == 1 && settingsMode == true) {
    potentioValue = analogRead(A1);
    potentioMaped = map(potentioValue, 0, 1023, 0, 255);
    EEPROM.put(0, potentioMaped);
    EEPROM.get(0, minBrightness);
    drawSettings();
  }
  else if (btnModeForSettingsPushCounter == 2 && settingsMode == true) {
    EEPROM.get(0, minBrightness);
    potentioValue = analogRead(A1);
    potentioMaped = map(potentioValue, 0, 1023, minBrightness, 255);
    EEPROM.put(2, potentioMaped);
    EEPROM.get(2, maxBrightness);
    drawSettings();
  }
  else if (btnModeForSettingsPushCounter == 3 && settingsMode == true) {
    potentioValue = analogRead(A1);
    potentioMaped = map(potentioValue, 0, 1023, 0, NUM_LEDS);
    EEPROM.put(4, potentioMaped);
    EEPROM.get(4, ledsNumb);
    drawSettings();
  }
  else if (btnModeForSettingsPushCounter == 4 && settingsMode == true) {
    drawSettings();
  }
  previousBtnModeState = btnModeState;
}

/*----------------------------------------------------------------------- OBD AND LIGHTING FUNCTIONS ------------------------------------------------------------------------------------*/
//RPM nacin rada
void modeRPM() {
  float tempRPM = myELM327.rpm();
  if (myELM327.nb_rx_state == ELM_SUCCESS)
  {
    rpm = (uint32_t)tempRPM;
    mapRPM = map(rpm, 0, 2000, minBrightness, maxBrightness);
    sensorValue = analogRead(A1);
    mapSensor = map(sensorValue, 0, 1023, 0, 255);
    for (int i = 0; i < ledsNumb; i++) {
      leds[i] = CHSV(mapSensor, 255, mapRPM);
      FastLED.show();
    }
    draw(rpm);
    temp = 0;
  }
  else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
  {
    for (int i = 0; i < ledsNumb; i++) {
      leds[i].fadeToBlackBy(255);
    }
    u8x8.setFont(u8x8_font_victoriabold8_r);
    u8x8.setCursor(4, 1);
    u8x8.print("RPM data");
    u8x8.setCursor(6, 3);
    u8x8.print("not");
    u8x8.setCursor(4, 5);
    u8x8.print("available!");
    u8x8.clearDisplay();
  }
}
//KPH nacin rada
void modeKPH() {
  float tempKPH = (int32_t)myELM327.kph();
  if ((myELM327.nb_rx_state == ELM_SUCCESS) && (int)tempKPH != 12 && (int)tempKPH != 13)
  {
    kph = tempKPH;
    mapKPH = map(kph, 0, 70, 0, ledsNumb);
    sensorValue = analogRead(A1);
    mapSensor = map(sensorValue, 0, 1023, 0, 255);
    for (int i = 0; i < mapKPH; i++) {
      leds[i] = CHSV(mapSensor, 255, maxBrightness);
      FastLED.show();
    }
    for (int i = mapKPH; i < ledsNumb; i++) {
      leds[i].fadeToBlackBy(255);
    }
    drawKPH(kph);
    rpm = 0;
  }
  else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
  {
    for (int i = 0; i < ledsNumb; i++) {
      leds[i].fadeToBlackBy(255);
    }
    u8x8.setFont(u8x8_font_victoriabold8_r);
    u8x8.setCursor(4, 1);
    u8x8.print("KPH data");
    u8x8.setCursor(6, 3);
    u8x8.print("not");
    u8x8.setCursor(4, 5);
    u8x8.print("available!");
    u8x8.clearDisplay();
  }
}
//TEMP nacin rada
void modeTEMP() {
  float tempTemp = myELM327.engineCoolantTemp();
  if ((myELM327.nb_rx_state == ELM_SUCCESS) && tempTemp >= 0)
  {
    temp = tempTemp;
    mapTEMP = map(temp, 10, 90, 40, 0);
    for (int i = 0; i < ledsNumb; i++) {
      leds[i] = CHSV(mapTEMP, 255, 255);
      FastLED.show();
    }
    drawTEMP(temp);
    kph = 0;
  }
  else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
  {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(255);
    }
    u8x8.setFont(u8x8_font_victoriabold8_r);
    u8x8.setCursor(4, 1);
    u8x8.print("TEMP data");
    u8x8.setCursor(6, 3);
    u8x8.print("not");
    u8x8.setCursor(4, 5);
    u8x8.print("available!");
    u8x8.clearDisplay();
  }
}
//LedsOFF nacin rada
void modeLedsOFF() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(255);
  }
  FastLED.show();
  u8x8.setFont(u8x8_font_victoriabold8_r);
  u8x8.setCursor(4, 4);
  u8x8.print("Leds off!");
}

/*------------------------------------------------------------------- BUTTON FUNCTIONS -----------------------------------------------------------------------------*/
//Provjera pritiska tipkala Mode, brojanje broja pritiska, debouncing
void modeBtnCheck() {         
  time_now = millis();
  while (millis() < time_now + period) {
    debouncer.update();
  }
  int value = debouncer.read();
  btnModeState = value;
  if (btnModeState != previousBtnModeState) {
    if (btnModeState == HIGH) {
      btnModePushCounter++;
      btnModeForSettingsPushCounter++;
      u8x8.clearDisplay();
      for (int i = 0; i < 20; i++) {
        leds[i] = CHSV(0, 0, 0);
      }
      FastLED.show();
    }
  }
  if (btnModePushCounter == 4) {
    btnModePushCounter = 0;
  }
  if (btnModeForSettingsPushCounter == 5) {
    btnModeForSettingsPushCounter = 0;
  }
}
//Provjera pritiska tipkala Settings minimalno 4 sekunde
void settingsBtnCheck(){
  currentMillis = millis();
  int btnModeState = digitalRead(btnSettings);
  if (btnModeState == HIGH && btnSettingsPreviousState == LOW && !btnModeStateLongPress) {
    minBtnSettingsLongPressMillis = currentMillis;
    btnSettingsPreviousState = HIGH;
    Serial.println("Button pressed");
  }
  btnSettingsPressDuration = currentMillis - minBtnSettingsLongPressMillis;
  if (btnModeState == HIGH && !btnModeStateLongPress && btnSettingsPressDuration >= minBtnSettingsLongPressDuration) {
    btnModeStateLongPress = true;
    btnModeForSettingsPushCounter = 0;
    u8x8.clearDisplay();
    settingsMode = true;
    Serial.println("Button long pressed");
  }  
  if (btnModeState == LOW && btnSettingsPreviousState == HIGH) {
    btnSettingsPreviousState = LOW;
    btnModeStateLongPress = false;
    Serial.println("Button released");
  }
  previousBtnSettingsMillis = currentMillis;
}

/*----------------------------------------------------------------- DRAWING FUNCTIONS -------------------------------------------------------------------------------*/
//Funkcija za prikaz vrijednosti broja okretaja motora na zaslonu
void draw(uint32_t rpm)
{
  u8x8.setFont(u8x8_font_courB18_2x3_r);
  u8x8.setCursor(3, 0);
  u8x8.print("RPM:");
  u8x8.setCursor(3, 4);
  u8x8.print(rpm);
  u8x8.print(" ");
}
//Funkcija za prikaz vrijednosti brzine vozila na zaslonu
void drawKPH(int32_t kph)
{
  u8x8.setFont(u8x8_font_courB18_2x3_r);
  u8x8.setCursor(3, 0);
  u8x8.print("KPH:");
  u8x8.setCursor(3, 4);
  u8x8.print(kph);
  u8x8.print("  ");
}
//Funkcija za prikaz vrijednosti temp rashladne tekucine na zaslonu
void drawTEMP(float temp)
{
  u8x8.setFont(u8x8_font_courB18_2x3_r);
  u8x8.setCursor(3, 0);
  u8x8.print("TEMP:");
  u8x8.setCursor(3, 4);
  u8x8.print((int)temp);
  u8x8.setFont(u8x8_font_victoriabold8_r);
  u8x8.print("o");
  u8x8.setFont(u8x8_font_courB18_2x3_r);
  u8x8.print("C");
}
//Funkcija za prikaz izbornika za postavke na zaslonu
void drawSettings() {
  if (btnModeForSettingsPushCounter == 4)
  {
    u8x8.setCursor(4, 0);
    u8x8.print("SAVED!");
    u8x8.setCursor(2, 2);
    u8x8.print("Min Bright:");
    u8x8.print(minBrightness);
    u8x8.setCursor(2, 4);
    u8x8.print("Max Bright:");
    u8x8.print(maxBrightness);
    u8x8.setCursor(2, 6);
    u8x8.print("Leds: ");
    u8x8.print(ledsNumb);
    btnModePushCounter = 0;
    delay(4000);
    u8x8.clearDisplay();
    settingsMode = false;
  }
  else
  {
    u8x8.setCursor(4, 0);
    u8x8.print("Settings");
    if (btnModeForSettingsPushCounter == 1)
    {
      u8x8.setCursor(0, 2);
      u8x8.print("> ");
    }
    u8x8.setCursor(2, 2);
    u8x8.print("Min Bright:");
    u8x8.print(minBrightness);
    u8x8.print("  ");
    if (btnModeForSettingsPushCounter == 2)
    {
      u8x8.setCursor(0, 4);
      u8x8.print("> ");
    }
    u8x8.setCursor(2, 4);
    u8x8.print("Max Bright:");
    u8x8.print(maxBrightness);
    u8x8.print("  ");
    if (btnModeForSettingsPushCounter == 3)
    {
      u8x8.setCursor(0, 6);
      u8x8.print("> ");
    }
    u8x8.setCursor(2, 6);
    u8x8.print("Leds: ");
    u8x8.print(ledsNumb);
    u8x8.print("  ");
  }
}
