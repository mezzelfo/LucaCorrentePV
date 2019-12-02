// Libraries Required
#include <BlynkSimpleEsp8266.h>   //Blynk magic
#include <ESP8266httpUpdate.h>    //For OTA Update
#include <Filters.h>
#include <ESP8266WebServer.h>
#include "LowPassFilter.h"

static const char auth[] = "3Ces1f8NBl4PrGAqDKKDddCIrrNwWPb7";
static const char BlynkServer[] = "lucanet.ddns.net";
static const char ssid[] = "WebCube42-AB50";
static const char pass[] = "ED248443";

ESP8266WebServer server(80);

#define LEN 2048
#define CHUCNKSIZE 16

RunningStatistics inputStats;
BlynkTimer timer;
WidgetTerminal terminal(V0);

double integraleWattSecondo;
unsigned long tempovecchio;

float banda, K_high, K_low;
int scarto;
int sensorValue;
int sensor[LEN];
int analisi[LEN];
double filterValue;
double filter[LEN];

void fun()
{
  int sensormin = 1025;
  int sensormax = -1;
  int voltetransizioni = 0;
  int stato, statoold = +1;
  unsigned long starttime, deltatime;
  int counter = 0;
  int counterold = 0;
  double sensormean = 0;
  float high = 545;
  float low = 545;
  float FiltMean = 0, FiltMin = 1025, FiltMax = -1;

  LowPassFilter lpf = LowPassFilter(545.0);


  while (voltetransizioni < 100 + 5)
  {
    counter++;
    if (counter >= 4000)
      break;
    sensorValue = analogRead(A0);
    if ( sensorValue < sensormin) sensormin =  sensorValue;
    if ( sensorValue > sensormax) sensormax =  sensorValue;

    inputStats.input(sensorValue);
    sensormean = sensormean * (counter - 1) / counter +  sensorValue * 1.0 / counter;
    
    
    /*high = K_high * sensorValue + (1 - K_high) * high;
    low = K_low * sensorValue + (1 - K_low) * low;
    filterValue = high;*/

    filterValue = lpf.filter(sensorValue);
    
    if ( filterValue < FiltMin) FiltMin = filterValue;
    if ( filterValue > FiltMax) FiltMax = filterValue;
    FiltMean = FiltMean * (counter - 1) / counter +  filterValue * 1.0 / counter;
    

    if ( (filterValue) > FiltMean * (1.0 - banda) + FiltMax * (banda))
      stato = +1 ;
    else if ( (filterValue) < FiltMean * (1.0 - banda) + FiltMin * (banda))
      stato = -1 ;
    else
      stato = 0;

    if ((counter < LEN) && (counter > FILTERLEN))
    {
      sensor[counter] = sensorValue;
      analisi[counter] = stato;
      filter[counter-FILTERLEN/2] = filterValue;
    }

    if (((stato == +1) | (stato == -1)) && (stato != statoold) && (counter > counterold + scarto))
      //if ((stato != statoold) && (counter > counterold + scarto))
    {
      counterold = counter;
      voltetransizioni++;
      if (voltetransizioni == 5) starttime = micros();
    }
    statoold = stato;
  }

  deltatime = micros() - starttime;
  double freq = (10000000000.0 / deltatime) / 2.0;

  for (int i = 0; i < LEN; i++)
  {
    sensor[i] = round((600.0 * (sensor[i] - sensormin)) / (1.0 * (sensormax - sensormin)));
    filter[i] = (600.0 * (filter[i] - FiltMin)) / (1.0 * (FiltMax - FiltMin));
    analisi[i] = round(600.0 * (analisi[i] + 2.0) / 4.0);
  }

  double RMS = 20.0 * inputStats.sigma() - 0.0;
  integraleWattSecondo += RMS * (millis() - tempovecchio) / 1000.0;
  tempovecchio = millis();
  Blynk.virtualWrite(V1, integraleWattSecondo);
  Blynk.virtualWrite(V2, integraleWattSecondo / 3600000.0); //IntegraleKiloWattOra
  Blynk.virtualWrite(V3, sensormin);
  Blynk.virtualWrite(V4, sensormax);
  Blynk.virtualWrite(V5, RMS); //Watt
  Blynk.virtualWrite(V6, freq);
  Blynk.virtualWrite(V7, FiltMin);
  Blynk.virtualWrite(V8, FiltMax);
}



void handleGraph()
{
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Transfer-Encoding", "chunked");
  server.send(200, "image/svg+xml");
  server.sendContent("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 4140 600\"> preserveAspectRatio=\"none\"\n");
  server.sendContent("<rect width=\"100%\" height=\"100%\" fill=\"#3d3d35\"/>\n");

  //for(int i=0; i<LEN/CHUCNKSIZE; i++) {
  for (int i = 1; i < 400 / CHUCNKSIZE; i++) {
    char chunk[800];
    sprintf(chunk, "<polyline points=\"%d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d\" fill=\"none\" stroke=\"white\"/>\n)",
            10 * (CHUCNKSIZE * i - 1),   sensor[CHUCNKSIZE * i - 1],
            10 * (CHUCNKSIZE * i + 0),   sensor[CHUCNKSIZE * i + 0],
            10 * (CHUCNKSIZE * i + 1),   sensor[CHUCNKSIZE * i + 1],
            10 * (CHUCNKSIZE * i + 2),   sensor[CHUCNKSIZE * i + 2],
            10 * (CHUCNKSIZE * i + 3),   sensor[CHUCNKSIZE * i + 3],
            10 * (CHUCNKSIZE * i + 4),   sensor[CHUCNKSIZE * i + 4],
            10 * (CHUCNKSIZE * i + 5),   sensor[CHUCNKSIZE * i + 5],
            10 * (CHUCNKSIZE * i + 6),   sensor[CHUCNKSIZE * i + 6],
            10 * (CHUCNKSIZE * i + 7),   sensor[CHUCNKSIZE * i + 7],
            10 * (CHUCNKSIZE * i + 8),   sensor[CHUCNKSIZE * i + 8],
            10 * (CHUCNKSIZE * i + 9),   sensor[CHUCNKSIZE * i + 9],
            10 * (CHUCNKSIZE * i + 10),  sensor[CHUCNKSIZE * i + 10],
            10 * (CHUCNKSIZE * i + 11),  sensor[CHUCNKSIZE * i + 11],
            10 * (CHUCNKSIZE * i + 12),  sensor[CHUCNKSIZE * i + 12],
            10 * (CHUCNKSIZE * i + 13),  sensor[CHUCNKSIZE * i + 13],
            10 * (CHUCNKSIZE * i + 14),  sensor[CHUCNKSIZE * i + 14],
            10 * (CHUCNKSIZE * i + 15),  sensor[CHUCNKSIZE * i + 15]
           );
    server.sendContent(chunk);

    sprintf(chunk, "<polyline points=\"%d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f %d,%f\" fill=\"none\" stroke=\"#42f700\"/>\n)",
            10 * (CHUCNKSIZE * i - 1),   filter[CHUCNKSIZE * i - 1],
            10 * (CHUCNKSIZE * i + 0),   filter[CHUCNKSIZE * i + 0],
            10 * (CHUCNKSIZE * i + 1),   filter[CHUCNKSIZE * i + 1],
            10 * (CHUCNKSIZE * i + 2),   filter[CHUCNKSIZE * i + 2],
            10 * (CHUCNKSIZE * i + 3),   filter[CHUCNKSIZE * i + 3],
            10 * (CHUCNKSIZE * i + 4),   filter[CHUCNKSIZE * i + 4],
            10 * (CHUCNKSIZE * i + 5),   filter[CHUCNKSIZE * i + 5],
            10 * (CHUCNKSIZE * i + 6),   filter[CHUCNKSIZE * i + 6],
            10 * (CHUCNKSIZE * i + 7),   filter[CHUCNKSIZE * i + 7],
            10 * (CHUCNKSIZE * i + 8),   filter[CHUCNKSIZE * i + 8],
            10 * (CHUCNKSIZE * i + 9),   filter[CHUCNKSIZE * i + 9],
            10 * (CHUCNKSIZE * i + 10),  filter[CHUCNKSIZE * i + 10],
            10 * (CHUCNKSIZE * i + 11),  filter[CHUCNKSIZE * i + 11],
            10 * (CHUCNKSIZE * i + 12),  filter[CHUCNKSIZE * i + 12],
            10 * (CHUCNKSIZE * i + 13),  filter[CHUCNKSIZE * i + 13],
            10 * (CHUCNKSIZE * i + 14),  filter[CHUCNKSIZE * i + 14],
            10 * (CHUCNKSIZE * i + 15),  filter[CHUCNKSIZE * i + 15]
           );
    server.sendContent(chunk);

    sprintf(chunk, "<polyline points=\"%d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d\" fill=\"none\" stroke=\"red\"/>\n)",
            10 * (CHUCNKSIZE * i - 1),   analisi[CHUCNKSIZE * i - 1],
            10 * (CHUCNKSIZE * i + 0),   analisi[CHUCNKSIZE * i + 0],
            10 * (CHUCNKSIZE * i + 1),   analisi[CHUCNKSIZE * i + 1],
            10 * (CHUCNKSIZE * i + 2),   analisi[CHUCNKSIZE * i + 2],
            10 * (CHUCNKSIZE * i + 3),   analisi[CHUCNKSIZE * i + 3],
            10 * (CHUCNKSIZE * i + 4),   analisi[CHUCNKSIZE * i + 4],
            10 * (CHUCNKSIZE * i + 5),   analisi[CHUCNKSIZE * i + 5],
            10 * (CHUCNKSIZE * i + 6),   analisi[CHUCNKSIZE * i + 6],
            10 * (CHUCNKSIZE * i + 7),   analisi[CHUCNKSIZE * i + 7],
            10 * (CHUCNKSIZE * i + 8),   analisi[CHUCNKSIZE * i + 8],
            10 * (CHUCNKSIZE * i + 9),   analisi[CHUCNKSIZE * i + 9],
            10 * (CHUCNKSIZE * i + 10),  analisi[CHUCNKSIZE * i + 10],
            10 * (CHUCNKSIZE * i + 11),  analisi[CHUCNKSIZE * i + 11],
            10 * (CHUCNKSIZE * i + 12),  analisi[CHUCNKSIZE * i + 12],
            10 * (CHUCNKSIZE * i + 13),  analisi[CHUCNKSIZE * i + 13],
            10 * (CHUCNKSIZE * i + 14),  analisi[CHUCNKSIZE * i + 14],
            10 * (CHUCNKSIZE * i + 15),  analisi[CHUCNKSIZE * i + 15]
           );
    server.sendContent(chunk);
  }
  server.sendContent("</svg>\n");
  server.sendContent("");
}

void setup(void) {
  integraleWattSecondo = 0.0;
  inputStats.setWindowSecs(40.0 / 50.0);
  tempovecchio = millis();
  pinMode(5, INPUT);
  pinMode(A0, INPUT);

  Blynk.begin(auth, ssid, pass, BlynkServer, 8080);
  timer.setInterval(8000L, fun);
  terminal.clear();
  terminal.println("Device started");
  terminal.println("-------------");
  terminal.flush();

  server.on("/", []() {
    server.send(200, "text/plain", "Hello world! Visit /graph");
  });
  server.onNotFound([]() {
    server.send(404, "text/plain", "wizardry not found :(");
  });
  server.on("/graph", handleGraph);
  server.begin();
}

void loop(void) {
  Blynk.run();
  timer.run();
  server.handleClient();
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1, V50, V51, V52, V53);
}

BLYNK_WRITE(V1)  {
  integraleWattSecondo = param.asDouble();
}
BLYNK_WRITE(V50) {
  banda = param.asInt() / 100.0;
}
BLYNK_WRITE(V51) {
  scarto = param.asInt();
}
BLYNK_WRITE(V52) {
  K_low = param.asInt() / 1000.0;
}
BLYNK_WRITE(V53) {
  K_high = param.asInt() / 1000.0;
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
    Blynk.virtualWrite(V1, 0);
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
