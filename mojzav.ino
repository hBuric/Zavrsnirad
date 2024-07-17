//Definiranje koristenih biblioteka
#include <SoftwareSerial.h>
#include "ELMduino.h"
#include <Bounce2.h>
#include <TM1638plus.h>

//Instanciranje objekata
SoftwareSerial mySerial(2, 3);
#define ELM_PORT mySerial
ELM327 myELM327;

//Postavljanje pinova za 7seg pokaznik
#define  STROBE_TM 8
#define  CLOCK_TM 9
#define  DIO_TM 10

bool high_freq = false; // default false, If using a high freq CPU > ~100 MHZ set to true. 

const long intervalButton = 200; // interval to read button (milliseconds)
const long intervalDisplay = 2000; // interval at which to change display (milliseconds)

// Constructor object (GPIO STB , GPIO CLOCK , GPIO DIO, use high freq MCU)
TM1638plus tm(STROBE_TM, CLOCK_TM , DIO_TM, high_freq);
//Postavljanje debouncinga na tipkalo
Bounce debouncer = Bounce();
const int  btnMode = 6;
int btnModePushCounter = 0;           
int btnModeState = 0;                  
int previousBtnModeState = 0;             
int period = 15;                     
unsigned long time_now = 0;



//Varijable za vrijednosti parametara automobila
uint32_t rpm = 0;
int mapRPM = 0;
int kph = 0;
int mapKPH = 0;
float temp = 0;
int mapTEMP = 0;
int sensorValue = 0;
int mapSensor = 0;
int firstCounter=0;
int firstModeRPM=0;
int firstModeKPH=0;
int firstModeTEMP=0;

void setup() {

  //Postavljanje tipkala na digitalne nozice
  debouncer.attach(btnMode, INPUT);
  debouncer.interval(10);
  //povezivanje s 7seg
  tm.displayBegin();
  //povezivanje komunikacije s ELM327
  ELM_PORT.begin(38400);
  Serial.begin(9600);
  delay(3000);
  while(!myELM327.begin(ELM_PORT, true, 300, '0'))
  {
    tm.displayText("no Conn");
    
  }
  if(myELM327.begin(ELM_PORT, true, 300, '0'))
  tm.displayText("ConnDone");
  delay(1000);
}
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
    }
  }
  if (btnModePushCounter == 3) {
    btnModePushCounter = 0;
  }
  
  if(firstCounter==0){
    tm.displayText("loopdoone");
    delay(300);
    tm.displayText("");
    firstCounter++;
  }
}
void modeRPM() {
  float tempRPM = myELM327.rpm();
  if(firstModeRPM==0){
    tm.displayText("RPn");
    delay(2000);
    tm.displayText("");
    firstModeRPM++;
  }
 
  if (myELM327.nb_rx_state != ELM_SUCCESS)
  {
    tm.displayIntNum((int32_t)tempRPM);
    Serial.println(tempRPM);
         
  }
  else if (myELM327.nb_rx_state == ELM_GETTING_MSG)
  {
    tm.displayText("noRPDATA");       
  }
}
//KPH nacin rada
void modeKPH() {
   if(firstModeKPH==0){
    tm.displayText("KPH");
    delay(2000);
    tm.displayText("");
    firstModeKPH++;
  }

  float tempKPH = (int32_t)myELM327.kph();
  if (myELM327.nb_rx_state != ELM_SUCCESS)
  {
    tm.displayIntNum(tempKPH);
    Serial.println(tempKPH);
         
  }
  else if (myELM327.nb_rx_state == ELM_GETTING_MSG)
  {
   tm.displayText("noKPDATA"); 
    
  }
}
//TEMP nacin rada
void modeTEMP() {
  if(firstModeTEMP==0){
    tm.displayText("TEP");
    delay(2000);
    tm.displayText("");
    firstModeTEMP++;
  }
  float tempTemp = myELM327.engineCoolantTemp();
  if ((myELM327.nb_rx_state != ELM_SUCCESS) && tempTemp >= 0)
  {
    tm.displayIntNum((int32_t)tempTemp);
    Serial.println(tempTemp);
    
  }
  else if (myELM327.nb_rx_state == ELM_GETTING_MSG)
  {
    tm.displayText("noTPDATA");
}
}


void loop() {
  //Provjera pritiska Mode i Settings tipkala

  modeBtnCheck();
   

  //Odredivanje nacina rada s obzirom na pritisak tipkala
  if (btnModePushCounter == 0) {
    modeRPM();
  }
  else if (btnModePushCounter == 1) {
    modeKPH();
  }
  else if (btnModePushCounter == 2) {
    modeTEMP();
  }
  

}


