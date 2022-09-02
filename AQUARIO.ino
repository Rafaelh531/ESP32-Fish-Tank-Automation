#ifdef ESP32
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#else
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>S
#endif
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SSD1306.h>

// Pinos do display (comunicação i2c)
const int DISPLAY_ADDRESS_PIN = 0x3c;
const int DISPLAY_SDA_PIN = 4;
const int DISPLAY_SCL_PIN = 15;
const int DISPLAY_RST_PIN = 16;

// Altura da fonte (correspondente a fonte ArialMT_Plain_16)
const int fontHeight = 8   ;

// Objeto do display
SSD1306 display(DISPLAY_ADDRESS_PIN, DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);
// Data wire is connected to GPIO 4
#define ONE_WIRE_BUS 17
#define LED 12
#define RESISTENCIA 13

float ATUAL;
float DESEJADA = 30;
bool displayBegin()
{
  // Reiniciamos o display
  pinMode(DISPLAY_RST_PIN, OUTPUT);
  digitalWrite(DISPLAY_RST_PIN, LOW);
  delay(1);
  digitalWrite(DISPLAY_RST_PIN, HIGH);
  delay(1);

  return display.init();
}
void displayConfig()
{
  // Invertemos o display verticalmente
  display.flipScreenVertically();
  // Setamos a fonte
  display.setFont(ArialMT_Plain_10);
  // Alinhamos a fonta à esquerda
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}


// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Variables to store temperature values
String temperatureC = "";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readDSTemperatureC() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == -127.00) {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC);
    ATUAL = tempC;
    // Limpamos o display
    display.clear();
      display.setFont(ArialMT_Plain_10);
    // Escrevemos a mensagem "Sending packet: " na primeira linha
    display.drawString(0, 0, "TEMPERATURA ATUAL: ");
      display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10,String(ATUAL) + "°C");
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 26, "TEMPERATURA DESEJADA: ");
     display.setFont(ArialMT_Plain_16);
     display.drawString(0, 36,String(DESEJADA) + "°C");
    // Exibimos as alterações no display
    display.display();

    
    if (digitalRead(LED) == LOW) {
      if (ATUAL < (DESEJADA - 0.5)) {
        digitalWrite(LED, HIGH);
        digitalWrite(RESISTENCIA, LOW);
      }

    }
    else if (digitalRead(LED) == HIGH) {
      if (ATUAL > DESEJADA) {
        digitalWrite(LED, LOW);
        digitalWrite(RESISTENCIA, HIGH);
      }
    }
  }
  return String(tempC);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <script src="https://kit.fontawesome.com/2b3183bb95.js" crossorigin="anonymous"></script>
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>  <i class="fa-solid fa-fish-fins" style="color:#059e8a;"></i>  AQUÁRIO  <i class="fa-solid fa-fish-fins" style="color:#059e8a;"></i>  </h2>

  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperatura Atual:</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
    <p>
     <span class="ds-labels">Temperatura Desejada:</span> 
    <span class="ds-labels" id="valBox"> 30.0</span>
      </p>
    <input type="range" min="20.0" max="40.0" value="30.0" step="0.25" onchange="showVal(this.value)">
</body>
<script language = "JavaScript">
function showVal(newVal){
    document.getElementById("valBox").innerHTML=newVal;
    var xhttp = new XMLHttpRequest();
    xhttp.open("POST", "/post", true);
    xhttp.send(newVal);
}
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 1000) ;

</script>
</html>)rawliteral";

// Replaces placeholder with DS18B20 values
String processor(const String& var) {
  //Serial.println(var);
  if (var == "TEMPERATUREC") {
    return temperatureC;
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();
  pinMode(LED, OUTPUT);
  pinMode(RESISTENCIA, OUTPUT);
  digitalWrite(LED, LOW);
  digitalWrite(RESISTENCIA, LOW);

  // Iniciamos o display
  if (!displayBegin())
  {
    // Se não deu certo, exibimos falha de display na serial
    Serial.println("Display failed!");
    // E deixamos em loop infinito
    while (1);
  }

  // Configuramos o posicionamento da tela, fonte e o alinhamento do texto
  displayConfig();
  // Start up the DS18B20 library
  sensors.begin();

  temperatureC = readDSTemperatureC();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", temperatureC.c_str());
  });
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest * request) {},
  NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    String tempor = "";
    for (size_t i = 0; i < len; i++) {
      tempor += char(data[i]);
      //Serial.write(data[i]);
    }
    DESEJADA = tempor.toFloat();
    Serial.println(DESEJADA);
        display.clear();
      display.setFont(ArialMT_Plain_10);
    // Escrevemos a mensagem "Sending packet: " na primeira linha
    display.drawString(0, 0, "TEMPERATURA ATUAL: ");
      display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10,String(ATUAL) + "°C");
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 26, "TEMPERATURA DESEJADA: ");
     display.setFont(ArialMT_Plain_16);
     display.drawString(0, 36,String(DESEJADA) + "°C");
    // Exibimos as alterações no display
    display.display();
    //Serial.println("aqui " + String(DESEJADA));
    request->send(200);
  });

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    temperatureC = readDSTemperatureC();
    lastTime = millis();
  }
}
