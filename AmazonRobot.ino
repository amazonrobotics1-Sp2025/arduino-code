#include <Arduino.h>
#include <deque>
#include <AsyncTCP.h>

#include <WiFi.h>
char ssid[] = "AmazonRobotics1";  //  your network SSID (name)
char pass[] = "test-password";      // your network password
int status = WL_IDLE_STATUS;    // the Wifi radio's status

#include <ESPAsyncWebServer.h>
static AsyncWebServer web_server(80);


#include <ArduinoWebsockets.h>
// const char *websockets_server_host = "192.168.4.100";
const char *websockets_server_host = "192.168.8.100"; //router subnet
const uint16_t websockets_server_port = 81;
using namespace websockets;
WebsocketsClient ws_client;
std::deque<WSInterfaceString> messages;

#include <LittleFS.h>


void strjoin(const std::deque<String> &v, char c, uint32_t offset, String &s) {
  if (offset >= v.size())
    return;
  s.clear();
  for (std::deque<String>::const_iterator p = v.begin() + offset;
       p != v.end(); ++p) {
    s += *p;
    if (p != v.end() - 1)
      s += c;
  }
}


void onMessageCallback(WebsocketsMessage message) {
  Serial.print("Got Message: ");
  messages.push_back(message.data());
  Serial.println(messages.at(messages.size() - 1));
}
void onEventsCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connnection Opened");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connnection Closed");
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Got a Pong!");
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }


  web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });
  web_server.serveStatic("/index.html", LittleFS, "/index.html");
  web_server.serveStatic("/store.svg", LittleFS, "/store.svg");
  web_server.serveStatic("/grid.json", LittleFS, "/grid.json");

  web_server.serveStatic("/assets", LittleFS, "/assets");


  web_server.on("/dumpstatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint32_t offset = 0;
    if(request->hasParam("offset")) offset = request->getParam("offset")->value().toInt();
    String res;
    strjoin(messages, offset, '\n', res);
    request->send(200, "text/plain", res);
  });
  web_server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    ws_client.send("$S\n");
    request->send(200, "text/plain", "Good /config");
  });
  web_server.on("/cmd", HTTP_GET, [](AsyncWebServerRequest *request) {
    ws_client.send("$CMD\n");
    request->send(200, "text/plain", "Good /cmd");
  });



  ws_client.onMessage(onMessageCallback);
  ws_client.onEvent(onEventsCallback);
  ws_client.connect(websockets_server_host, websockets_server_port, "/");
  ws_client.ping();

  ws_client.send("$GS\n");
  ws_client.send("$SS\n");
  web_server.begin();
}

// not needed
void loop() {
  delay(100);
  ws_client.poll();
}
