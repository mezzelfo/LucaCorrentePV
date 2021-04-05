#define BLYNK_PRINT Serial
#define BLYNK_TIMEOUT_MS 500
#define LEN 512 //Lunghezza vettori readings

#include <WiFi.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BlynkSimpleEsp32.h>
#include "LowPassFilter.h"
#include <HTTPUpdate.h>
#include <Filters.h>

#include <FS.h> //SPIFFS
#include <SPIFFS.h> //SPIFFS

WiFiClient client; //OTA


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
int toggle = 0;
int counter = 0;

byte Tri, Tri_old;




bool online = 0;

AsyncWebServer server(80);

RunningStatistics inputStats;

static const char BlynkServer[] = "lucanet.ddns.net";
static const char auth[] = "3Ces1f8NBl4PrGAqDKKDddCIrrNwWPb7";
static const char ssid[] = "WebCube42-AB50";
static const char pass[] = "ED248443";

void fun()
{
  int sensormin = 1025;
  int sensormax = -1;
  int voltetransizioni = 0;
  int stato, statoold = +1;
  unsigned long starttime, deltatime;
  //int counter = 0;
  int counterold = 0;
  double sensormean = 0;
  float high = 545;
  float low = 545;
  float FiltMean = 0, FiltMin = 1025, FiltMax = -1;

  digitalWrite(2, LOW);
  delay(200);
  digitalWrite(2, HIGH);

  LowPassFilter lpf = LowPassFilter(545.0);

  //if (toggle == 1)
  //{
  //  toggle = 0;
  //}
  //else
  //{
  //  toggle = 1;
  //}
  //Serial.print("TOGGLE  ");
  //Serial.println(toggle);
  counter = 0;
  while (voltetransizioni < 100 + 5)
  {
    counter++;
    //yield();
    if (counter >= 4000)
      break;
    
    sensorValue = analogRead(A0);
    
    //Serial.println(sensorValue);
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
      filter[counter - FILTERLEN / 2] = filterValue;
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

  for (int i = 17; i < 250; i = i + 1) //LEN 17
  {
    Serial.print(80 + 20 * toggle);
    Serial.print(",");
    Serial.print(counter / 10.0);
    Serial.print(",");
    Serial.println(sensor[i]);

    yield();
  }

  //Serial.print("80,");
  //Serial.println("100");


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
  Blynk.virtualWrite(V30, counter);
  Blynk.virtualWrite(V98, toggle);

}


void CheckConnection(){    // check every 11s if connected to Blynk server
  if(!Blynk.connected()){
    online = 0;
    yield();
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Not connected to Wifi! Connect...");
      Blynk.connectWiFi(ssid, pass); // used with Blynk.connect() in place of Blynk.begin(auth, ssid, pass, server, port);
      //WiFi.config(arduino_ip, gateway_ip, subnet_mask);
      //WiFi.begin(ssid, pass);
      delay(400); //give it some time to connect
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Cannot connect to WIFI!");
        online = 0;
      }
      else
      {
        Serial.println("Connected to wifi!");
      }
    }
    
    if ( WiFi.status() == WL_CONNECTED && !Blynk.connected() )
    {
      Serial.println("Not connected to Blynk Server! Connecting..."); 
      Blynk.connect();  // // It has 3 attempts of the defined BLYNK_TIMEOUT_MS to connect to the server, otherwise it goes to the enxt line 
      if(!Blynk.connected()){
        Serial.println("Connection failed!"); 
        online = 0;
      }
      else
      {
        online = 1;
      }
    }
  }
  else{
    Serial.println("Connected to Blynk server!"); 
    online = 1;    
  }
}

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(GPIO_NUM_2, HIGH);

  integraleWattSecondo = 0.0;
  //inputStats.setWindowSecs(40.0 / 50.0);
  tempovecchio = millis();
  pinMode(5, INPUT);
  pinMode(A0, INPUT);

  SPIFFS.begin();  
  
  Serial.begin(115200);
  Serial.println();

  Blynk.config(auth, BlynkServer, 8080);
  CheckConnection();
  timer.setInterval(5000L, CheckConnection);
  timer.setInterval(4000L, fun);

  terminal.clear();
  terminal.println("Device started");
  terminal.println("-------------");
  terminal.flush();


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Hello world! Visit /graph");
  });
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "wizardry not found :(");
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->addHeader("Server","ESP Async Web Server");
    for (int i = 0; i < LEN; i++)
    {
      response->printf("%d,%d\n",i,sensor[i]);
    }
    request->send(response);
  });
  server.on("/graph", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/graph.html");
  });
  server.begin();
}

void loop() {
  if(Blynk.connected()){
    Blynk.run();
  }
  timer.run();
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
  Serial.println(String("Firmware update URL: ") + overTheAirURL);

  // Disconnect, not to interfere with OTA process
  Blynk.disconnect();

  HTTPClient httpClient;
  httpClient.begin( client, overTheAirURL );
  // Start OTA
  switch (httpUpdate.update(client, overTheAirURL)) {
    case HTTP_UPDATE_FAILED:
      Serial.println(String("Firmware update failed (error ") + httpUpdate.getLastError() + "): " + httpUpdate.getLastErrorString());
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
BLYNK_WRITE(V99) // V5 is the number of Virtual Pin
{
  int pinValue = param.asInt();
  Serial.print("Il valore letto dal pin virtuale V99: ");
  Serial.println(pinValue);
  toggle = pinValue;

}
