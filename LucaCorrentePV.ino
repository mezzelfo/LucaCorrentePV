//#define TEL_SER Serial
#define TEL_SER TelnetStream

// Blynk defines
#define BLYNk_HEARTBEAT 50
#define BLYNK_PRINT Serial

// Libraries Required
#include <BlynkSimpleEsp8266.h>   //Blynk magic
#include <ESP8266httpUpdate.h>    //For OTA Update
#include <Filters.h>              //Analyze signal
#include <TelnetStream.h>         //Telent Debug

// Blynk connection variables
char auth[] = "OUbg10Z2bMiggS-7ZkJtvSEg7I1FMqJi";
char server[] = "lucanet.ddns.net";
char ssid[] = "WebCube42-AB50";
char pass[] = "ED248443";


float Gain = 20.0; //19.5;
float  Offset = 40.0; // Watt
double RMS;
BlynkTimer timer;
float testFrequency = 50;                     // test signal frequency (Hz)
float windowLength = 40.0 / testFrequency;   // how long to average the signal, for statistist
RunningStatistics inputStats;                //Easy life lines, actual calculation of the RMS requires a load of coding


float integrale = 0.0;
unsigned long tempovecchio;

// This function will run every time Blynk connection is established
BLYNK_CONNECTED() {
  //get data stored in virtual pin V0 from server
  Blynk.syncVirtual(V30);
}

// restoring counter from server
BLYNK_WRITE(V30)
{
  //restoring int value
  integrale = param.asFloat();
}

void fun()
{
  int sensormin = 1025;
  int sensormax = -1;

  int voltetransizioni = 0;

  int stato, statoold = false;

  long starttime, deltatime;
  int counter = 0;
  double sensormean = 0;

  while (voltetransizioni < 100 + 5)
  {

    counter++;
    if (counter >= 4000)
      break;
    int sensor = analogRead(A0);

    if (sensor < sensormin) sensormin = sensor;
    if (sensor > sensormax) sensormax = sensor;
    inputStats.input(sensor);

    sensormean = sensormean * (counter - 1) / counter + sensor * 1.0 / counter;

    if (sensor > sensormean * (2.0 / 3.0) + sensormax * (1.0 / 3.0))
      stato = true;
    if (sensor < sensormean * (2.0 / 3.0) + sensormin * (1.0 / 3.0))
      stato = false;

    if (stato != statoold)
    {
      //yield();
      voltetransizioni++;
      if (voltetransizioni == 5) starttime = micros();
    }

    statoold = stato;


  }

  deltatime = micros() - starttime;
  float freq = (10000000000.0 / deltatime) / 2;
  RMS = Gain * inputStats.sigma() - Offset;
  integrale += RMS * float(millis() - tempovecchio);
  tempovecchio = millis();

  TEL_SER.print("Intg: ");
  TEL_SER.print(integrale * 1.0 / 3600000000.0, 4);
  TEL_SER.print("\t");

  TEL_SER.print("Freq: ");
  TEL_SER.print(freq);
  TEL_SER.print("\t");

  TEL_SER.print("Tran: ");
  TEL_SER.print(voltetransizioni);
  TEL_SER.print("\t");
  TEL_SER.print(counter);
  TEL_SER.print("\t");

  TEL_SER.print("Mean: ");
  TEL_SER.print(sensormean);
  TEL_SER.print("\t");

  //TEL_SER.print("time: ");
  //TEL_SER.print(deltatime * 1.0 / 1000.0);
  //TEL_SER.print("\t");

  TEL_SER.print("sMin / SMax : ");
  TEL_SER.print(sensormin);
  TEL_SER.print(" ");
  TEL_SER.print(sensormax);
  TEL_SER.print("\t");

  TEL_SER.print("Sigm: ");
  TEL_SER.print(RMS);
  TEL_SER.println();

  if (counter < 4000) Blynk.virtualWrite(V10, freq);
  Blynk.virtualWrite(V11, RMS);
  Blynk.virtualWrite(V20, sensormin);
  Blynk.virtualWrite(V21, sensormax);
  Blynk.virtualWrite(V30, integrale * 1.0 / 3600000000.0);

}

void setup()
{
  pinMode(5, INPUT);
  pinMode(A0, INPUT);

  inputStats.setWindowSecs(windowLength);
  tempovecchio = millis();

  Serial.begin(9600);
  TelnetStream.begin();

  Blynk.begin(auth, ssid, pass, server, 8080);
  timer.setInterval(2000L, fun);
  //Blynk.syncAll();
  Blynk.syncVirtual(V30);

}

void loop()
{
  Blynk.run();
  timer.run();

  if (TelnetStream.read() == 'R') {
    TelnetStream.stop();
    delay(100);
    ESP.restart();
  }
  
}

BLYNK_WRITE(InternalPinOTA) {
  String overTheAirURL = param.asString();
  String OTAoutcome;
  //terminal.println(String("Firmware update URL: ") + overTheAirURL);
  
  // Disconnect, not to interfere with OTA process
  Blynk.disconnect();

  // Start OTA
  switch (ESPhttpUpdate.update(overTheAirURL)) {
  case HTTP_UPDATE_FAILED:
    Serial.println(String("Firmware update failed (error ") + ESPhttpUpdate.getLastError() + "): " + ESPhttpUpdate.getLastErrorString());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("No firmware updates available.");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("Firmware update: OK.");
    delay(10);
    ESP.restart();
    break;
  }
}
