//#define TEL_SER Serial
#define TEL_SER TelnetStream

// Libraries Required
#include <BlynkSimpleEsp8266.h>   //Blynk magic
#include <ESP8266httpUpdate.h>    //For OTA Update
#include <Filters.h>              //For signal analysis
#include <TelnetStream.h>         //Telent Debug

// Blynk connection variables
char auth[] = "3Ces1f8NBl4PrGAqDKKDddCIrrNwWPb7";
char server[] = "lucanet.ddns.net";
char ssid[] = "WebCube42-AB50";
char pass[] = "ED248443";

// Global Variables
RunningStatistics inputStats;
BlynkTimer timer;
WidgetTerminal terminal(V0);
double integraleWattSecondo;
unsigned long tempovecchio;

float banda;
int scarto;

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V50);
  Blynk.syncVirtual(V51);
}

BLYNK_WRITE(V1)
{
  integraleWattSecondo = param.asDouble();
}

BLYNK_WRITE(V50)
{
  banda = param.asInt()/100.0;
}

BLYNK_WRITE(V51)
{
  scarto = param.asInt();
}

void fun()
{
  int sensormin = 1025;
  int sensormax = -1;
  int voltetransizioni = 0;
  int stato, statoold = false;
  unsigned long starttime, deltatime;
  int counter = 0;
  int counterold = 0;
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
    if (sensor > sensormean * (1.0-banda) + sensormax * (banda))
      stato = true;
    if (sensor < sensormean * (1.0-banda) + sensormin * (banda))
      stato = false;
    if ((stato != statoold) && (counter > counterold + scarto))
    {
      counterold = counter;
      voltetransizioni++;
      if (voltetransizioni == 5) starttime = micros();
    }
    statoold = stato;
  }

  deltatime = micros() - starttime;
  double freq = (10000000000.0 / deltatime) / 2.0;
  
  double RMS = 20.0 * inputStats.sigma() - 0.0;
  integraleWattSecondo += RMS * (millis() - tempovecchio) / 1000.0;
  tempovecchio = millis();

  TEL_SER.print("Intg: "+String(integraleWattSecondo/3600000.0,4)+"\t");

  TEL_SER.print("Freq: "+String(freq)+"\t");

  TEL_SER.print("Tran: "+String(voltetransizioni)+"\t"+String(counter)+"\t");

  TEL_SER.print("Mean: "+String(sensormean)+"\t");

  //TEL_SER.print("time: "+String(deltatime / 1000.0)+"\t");

  TEL_SER.print("sMin / SMax : "+String(sensormin)+" "+String(sensormax)+"\t");

  TEL_SER.print("Sigm: "+String(RMS));
  TEL_SER.println();
  
  Blynk.virtualWrite(V1, integraleWattSecondo);
  Blynk.virtualWrite(V2, integraleWattSecondo/3600000.0); //IntegraleKiloWattOra
  Blynk.virtualWrite(V3, sensormin);
  Blynk.virtualWrite(V4, sensormax);
  Blynk.virtualWrite(V5, RMS); //Watt
  Blynk.virtualWrite(V6, freq);
}

BLYNK_WRITE(V0)
{
  if (String("R") == param.asStr()) {
    terminal.println("Rebooting");
    terminal.flush();
    delay(100);
    ESP.restart();
  }
  else if (String("C") == param.asStr()) {
    Blynk.virtualWrite(V1,0);
    terminal.println("Clearing integraleWattSecondo");
    terminal.flush();
    delay(100);
    ESP.restart();
  }
  else {
    terminal.print("Command not found");
    terminal.flush();
  }  
}

void setup()
{
  integraleWattSecondo = 0.0;
  inputStats.setWindowSecs(40.0/50.0);
  tempovecchio = millis();
  pinMode(5, INPUT);
  pinMode(A0, INPUT);  

  TelnetStream.begin();
  Blynk.begin(auth, ssid, pass, server, 8080);
  timer.setInterval(2000L, fun);
  terminal.clear();
  terminal.println("Device started");
  terminal.println("-------------");
  terminal.flush();
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
  if (TelnetStream.read() == 'C') {
    Blynk.virtualWrite(V1,0);
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
