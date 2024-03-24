/*=====================================================================================================================================================
 EasyVario - Simple vario bluetooth pour le parapente
 Auteur : Jérôme Caloz
 Date : 24 mars 2024

Résumé du code :
 Ce programme calcul la vitesse verticale à l'aide de la pression mesurée par un BMP280.
 Transmet l'informations sour forme de phrase LK8EX1 via BLE
 Testé avec FlySkyHy
 L'IMU n'est pas utilisé pour l'instant et est commenté

Hardware (AliExpress):
 ESP32C3 SuperMini 
 IMU et pression: GY-91 BMP280+MPU9250 board

=====================================================================================================================================================
Clause de non-responsabilité :
!! Ce code est fourni à titre informatif uniquement. L'auteur ne garantit pas son exactitude ni sa fiabilité. Utilisez-le à vos propres risques. !!
!! This code is provided for information purposes only. The author does not guarantee its accuracy or reliability. Use at your own risk.         !!
=====================================================================================================================================================*/


#include <Arduino.h>
//capteurs
#include <MPU9250_asukiaaa.h>
#include <Adafruit_BMP280.h>
#include <RunningAverage.h>
// I2C
int sda = 4;
int scl = 5;

Adafruit_BMP280 BMP; 
float Press,Alti,oldAlti,Temp,Vspeed_cmps;
//MPU9250_asukiaaa IMU;
//float aX, aY, aZ, aSqrt, gX, gY, gZ, mDirection, mX, mY, mZ;

RunningAverage AVGPress(100);
RunningAverage AVGAlti(100);
RunningAverage AVGvSpeed(10);

//BLE
#include <BleSerial.h>
BleSerial ble;
int LedPin=8;


//temps de cycle pour lecture des capteurs ms
unsigned long SensorCycleTime = 10; 
unsigned long NewSensorCycleTime = 0;
//temps de cycle pour la transmission BLE ms
unsigned long CycleTime = 100; 
unsigned long NewCycleTime = 0;

String LK8EX1Sentence;

//Checksum NMEA
String NMEAcrc(String NMEAString ){
  unsigned int checksum, ai, bi; // Calculating checksum for data string
  for (checksum = 0, ai = 0; ai < NMEAString.length(); ai++)
    {
      bi = (unsigned char)NMEAString[ai];
      checksum ^= bi;
  }
  String str_out2 = '$' + NMEAString + '*' + String(checksum,HEX);
  return str_out2;
}

// calcul de la chaine de cararatère
String LK8EX1Calc(float _pressurePa,float _alti, float _VitVertical, float _temp, int _batteryPercent)// pression , altitude, température, Vitesse verticale, batterie percent
{
  //$LK8EX1,Pression,Altitude,vspeed,température,batterie,*CRC  
  int Press=round(_pressurePa);
  int Temp=round(_temp);
  int Alti=round(_alti);
  int Vspeed=round(_VitVertical);

  String str_out =  //combine all values and create part of NMEA data string output https://gitlab.com/xcontest-public/xctrack-public/-/issues/600
  String("LK8EX1,"  + String(Press,DEC) + String(",") + String(Alti,DEC)  + String(",") +
  String(Vspeed,DEC) + String(",") + String(Temp,DEC) + String(",") + String(_batteryPercent+1000,DEC) + String(","));

  str_out=NMEAcrc(str_out);
  return str_out;
};

float Vspeed(float _oldalti,float _newalti, float _dt)//Vitesse verticale
{
  float vspeed_cmps;
  vspeed_cmps=(_newalti-_oldalti)*100/(_dt/1000); //calcul en cm/s
  return vspeed_cmps;
};



//=====================================================================================
void setup() {
  //Serial
    //Serial.begin(115200);

  //Démarrage i2c
    Wire.begin(sda, scl);

  //IMU (pas utilisé pour l'instant)
    //IMU.setWire(&Wire);
    //IMU.beginAccel();
    //IMU.beginGyro();
    //IMU.beginMag();

  //BMP
    BMP.begin(0x76);

  //BLE
    ble.begin("EasyVario",true,LedPin);
    digitalWrite(LedPin,HIGH);

    AVGPress.clear();
    AVGAlti.clear();
    AVGvSpeed.clear();
}
//=====================================================================================


//=====================================================================================
void loop() {

  if (millis()>NewSensorCycleTime+SensorCycleTime) // boucle calcul et mesures
  {    
    oldAlti=AVGAlti.getAverage(); // lire l'ancienne altitude
    Alti=BMP.readAltitude(1022.0); //lire l'altitude actuelle
    AVGAlti.addValue(Alti); //ajouter à la moyenne

    Vspeed_cmps=Vspeed(oldAlti,AVGAlti.getAverage(),SensorCycleTime); //Calcul Vspeed
    AVGvSpeed.addValue(Vspeed_cmps);


    Press=BMP.readPressure(); //Lire la pression
    AVGPress.addValue(Press); // ajouter à la moyenne

    Temp=BMP.readTemperature(); // Lire la température
    NewSensorCycleTime=millis();
  };

  if (millis()>NewCycleTime+CycleTime && ble.connected()) // boucle transmission seulement si ble Actif
  {
    LK8EX1Sentence=LK8EX1Calc(AVGPress.getAverage(),AVGAlti.getAverage(),AVGvSpeed.getAverage(),Temp,100); // calcul de la phrase LK8EX1
    //Serial.println(LK8EX1Sentence);
    ble.println(LK8EX1Sentence); // transmission via BLE
    NewCycleTime=millis();
  }
  
  }
//=====================================================================================



