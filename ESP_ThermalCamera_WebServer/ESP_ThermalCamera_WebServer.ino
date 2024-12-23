//--------------------------------------------------------------------------------------------------------//
// Thermal Image Camera - Project using ESP8266 or ESP32 and MLX90640 sensor (32x24 px)                   //
// Design with 0.95' OLED (SD1331) and WebServer to see images (with interpolation) on any other device   //
// Project based on MLX data sheet and examples                                                           //
// Author: Szymon Baczyński                                                                               //
// Date: April 2019                                                                                       //
// Version: Ver 1.1                                                                                       //
// GitHub: https://github.com/Samox1/ESP_Thermal_Camera_WebServer                                         //
//--------------------------------------------------------------------------------------------------------//

#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "ESPAsyncWebSrv.h"
#include "SPIFFS.h"
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1331.h>
#include "MLX90640_I2C_Driver.h"
#include "MLX90640_API.h"
#include <ArduinoJson.h>

const byte MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640


// Initialize JSON buffer
const size_t capacity = JSON_OBJECT_SIZE(5) + JSON_ARRAY_SIZE(768); // Adjusted for five key-value pairs plus the raw array
StaticJsonDocument<capacity> jsonDoc;

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

float mlx90640To[768];
paramsMLX90640 mlx90640;

// You can use any (4 or) 5 pins
#define sclk 18
#define mosi 23
#define cs   17
#define rst  5
#define dc   16


// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define PART_BOUNDARY "123456789000000000000987654321"

//the colors we will be using
const uint16_t camColors[] = {0x480F,
                              0x400F, 0x400F, 0x400F, 0x4010, 0x3810, 0x3810, 0x3810, 0x3810, 0x3010, 0x3010,
                              0x3010, 0x2810, 0x2810, 0x2810, 0x2810, 0x2010, 0x2010, 0x2010, 0x1810, 0x1810,
                              0x1811, 0x1811, 0x1011, 0x1011, 0x1011, 0x0811, 0x0811, 0x0811, 0x0011, 0x0011,
                              0x0011, 0x0011, 0x0011, 0x0031, 0x0031, 0x0051, 0x0072, 0x0072, 0x0092, 0x00B2,
                              0x00B2, 0x00D2, 0x00F2, 0x00F2, 0x0112, 0x0132, 0x0152, 0x0152, 0x0172, 0x0192,
                              0x0192, 0x01B2, 0x01D2, 0x01F3, 0x01F3, 0x0213, 0x0233, 0x0253, 0x0253, 0x0273,
                              0x0293, 0x02B3, 0x02D3, 0x02D3, 0x02F3, 0x0313, 0x0333, 0x0333, 0x0353, 0x0373,
                              0x0394, 0x03B4, 0x03D4, 0x03D4, 0x03F4, 0x0414, 0x0434, 0x0454, 0x0474, 0x0474,
                              0x0494, 0x04B4, 0x04D4, 0x04F4, 0x0514, 0x0534, 0x0534, 0x0554, 0x0554, 0x0574,
                              0x0574, 0x0573, 0x0573, 0x0573, 0x0572, 0x0572, 0x0572, 0x0571, 0x0591, 0x0591,
                              0x0590, 0x0590, 0x058F, 0x058F, 0x058F, 0x058E, 0x05AE, 0x05AE, 0x05AD, 0x05AD,
                              0x05AD, 0x05AC, 0x05AC, 0x05AB, 0x05CB, 0x05CB, 0x05CA, 0x05CA, 0x05CA, 0x05C9,
                              0x05C9, 0x05C8, 0x05E8, 0x05E8, 0x05E7, 0x05E7, 0x05E6, 0x05E6, 0x05E6, 0x05E5,
                              0x05E5, 0x0604, 0x0604, 0x0604, 0x0603, 0x0603, 0x0602, 0x0602, 0x0601, 0x0621,
                              0x0621, 0x0620, 0x0620, 0x0620, 0x0620, 0x0E20, 0x0E20, 0x0E40, 0x1640, 0x1640,
                              0x1E40, 0x1E40, 0x2640, 0x2640, 0x2E40, 0x2E60, 0x3660, 0x3660, 0x3E60, 0x3E60,
                              0x3E60, 0x4660, 0x4660, 0x4E60, 0x4E80, 0x5680, 0x5680, 0x5E80, 0x5E80, 0x6680,
                              0x6680, 0x6E80, 0x6EA0, 0x76A0, 0x76A0, 0x7EA0, 0x7EA0, 0x86A0, 0x86A0, 0x8EA0,
                              0x8EC0, 0x96C0, 0x96C0, 0x9EC0, 0x9EC0, 0xA6C0, 0xAEC0, 0xAEC0, 0xB6E0, 0xB6E0,
                              0xBEE0, 0xBEE0, 0xC6E0, 0xC6E0, 0xCEE0, 0xCEE0, 0xD6E0, 0xD700, 0xDF00, 0xDEE0,
                              0xDEC0, 0xDEA0, 0xDE80, 0xDE80, 0xE660, 0xE640, 0xE620, 0xE600, 0xE5E0, 0xE5C0,
                              0xE5A0, 0xE580, 0xE560, 0xE540, 0xE520, 0xE500, 0xE4E0, 0xE4C0, 0xE4A0, 0xE480,
                              0xE460, 0xEC40, 0xEC20, 0xEC00, 0xEBE0, 0xEBC0, 0xEBA0, 0xEB80, 0xEB60, 0xEB40,
                              0xEB20, 0xEB00, 0xEAE0, 0xEAC0, 0xEAA0, 0xEA80, 0xEA60, 0xEA40, 0xF220, 0xF200,
                              0xF1E0, 0xF1C0, 0xF1A0, 0xF180, 0xF160, 0xF140, 0xF100, 0xF0E0, 0xF0C0, 0xF0A0,
                              0xF080, 0xF060, 0xF040, 0xF020, 0xF800,
                             };

//WebServer server(80);
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//Enter your SSID and PASSWORD
const char* ssid = "ESP";
const char* password = "camera";

float p = 3.1415926;
// Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);

float MaxTemp = 0;
float MinTemp = 0;
float CenterTemp = 0;


String getCenterTemp(){
  extern float CenterTemp;
  return String(CenterTemp);
}
String getMaxTemp(){
  extern float MaxTemp;
  return String(MaxTemp);
}
String getMinTemp(){
  extern float MinTemp;
  return String(MinTemp);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
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
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    img { 
      display: inline-block;
      vertical-align: middle;
      max-height: 100%;
      max-width: 100%;
    }
  </style>
</head>
<body>
  <h2>ESP32 Thermal Camera</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature: </span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup><br>
    
    <span class="dht-labels">Temp Max: </span> 
    <span id="tempmax">%TEMPMAX%</span>
    <sup class="units">&deg;C</sup><br>
    
    <span class="dht-labels">Temp Min: </span> 
    <span id="tempmin">%TEMPMIN%</span>
    <sup class="units">&deg;C</sup>

    <img src="thermal" id="thermalimage" style="width: 100%">
  </p>
</body>
<script>

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 500 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("tempmax").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/tempmax", true);
  xhttp.send();
}, 500 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("tempmin").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/tempmin", true);
  xhttp.send();
}, 500 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    //if (this.readyState == 4 && this.status == 200) {
      var image = document.getElementById("thermalimage");
      //image.src = this.response;
      image.src = "thermal";
      //document.getElementById("thermal") = this.response;
    //}
  };
  xhttp.open("GET", "/thermal", true);
  xhttp.send();
}, 500 ) ;

</script>
</html>)rawliteral";


// Replaces placeholder with values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return getCenterTemp();
  }
  if(var == "TEMPMAX"){
    return getMaxTemp();
  }
  if(var == "TEMPMIN"){
    return getMinTemp();
  }
  
  return String();
}


//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected()
{
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}


//void getThermalImage(){ }


std::vector<uint8_t> ThermalImageToWeb(float mlx90640To[], float MinTemp, float MaxTemp) {
  std::vector<uint8_t> bmpData;
  
  uint8_t colorIndex = 0;
  uint16_t color = 0;
  unsigned int headers[13];
  int extrabytes;
  int paddedsize;
  int x = 0;
  int y = 0;
  int n = 0;
  int red = 0;
  int green = 0;
  int blue = 0;

  int WIDTH = 32;
  int HEIGHT = 24;

  extrabytes = 4 - ((WIDTH * 3) % 4); // Padding to align each row to a multiple of 4 bytes
  if (extrabytes == 4)
    extrabytes = 0;

  paddedsize = ((WIDTH * 3) + extrabytes) * HEIGHT;

  // BMP Headers
  headers[0]  = paddedsize + 54; // bfSize (whole file size)
  headers[1]  = 0;               // bfReserved (both)
  headers[2]  = 54;              // bfOffbits
  headers[3]  = 40;              // biSize
  headers[4]  = WIDTH;           // biWidth
  headers[5]  = HEIGHT;          // biHeight
  headers[7]  = 0;               // biCompression
  headers[8]  = paddedsize;      // biSizeImage
  headers[9]  = 0;               // biXPelsPerMeter
  headers[10] = 0;               // biYPelsPerMeter
  headers[11] = 0;               // biClrUsed
  headers[12] = 0;               // biClrImportant

  // Append BMP header to bmpData
  bmpData.push_back('B');
  bmpData.push_back('M');
  for (n = 0; n <= 5; n++) {
    bmpData.push_back(headers[n] & 0x000000FF);
    bmpData.push_back((headers[n] & 0x0000FF00) >> 8);
    bmpData.push_back((headers[n] & 0x00FF0000) >> 16);
    bmpData.push_back((headers[n] & 0xFF000000) >> 24);
  }

  bmpData.push_back(1);
  bmpData.push_back(0);
  bmpData.push_back(24);
  bmpData.push_back(0);

  for (n = 7; n <= 12; n++) {
    bmpData.push_back(headers[n] & 0x000000FF);
    bmpData.push_back((headers[n] & 0x0000FF00) >> 8);
    bmpData.push_back((headers[n] & 0x00FF0000) >> 16);
    bmpData.push_back((headers[n] & 0xFF000000) >> 24);
  }

  // Write pixel data
  for (y = 0; y < HEIGHT; y++) {  // BMP image format written top to bottom
    for (x = 0; x < WIDTH; x++) {
        // Map temperature to color
        colorIndex = map(mlx90640To[x + (32 * y)], MinTemp - 5.0, MaxTemp + 5.0, 0, 255);
        colorIndex = constrain(colorIndex, 0, 255);
        color = camColors[colorIndex];

        // Convert color to RGB
        red = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
        green = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
        blue = (((color & 0x1F) * 527) + 23) >> 6;

        // Append BGR to bmpData
        bmpData.push_back(blue);
        bmpData.push_back(green);
        bmpData.push_back(red);
    }

    // Add padding if needed
    for (n = 0; n < extrabytes; n++) {
        bmpData.push_back(0);
    }
  }

  return bmpData;
}




void lcdThermalImage(float mlx90640To[], float MinTemp, float MaxTemp)
{
  return;
}


void lcdTestThermalImage(void)
{
  return;
}

void MLX_to_Serial(float mlx90640To[])
{
  for (int x = 0 ; x < 768 ; x++)
  {
    //Serial.print("Pixel ");
    Serial.print(x);
    Serial.print(": ");
    Serial.print(mlx90640To[x], 2);
    //Serial.print("C");
    Serial.println();
  }
}


// SETUP
//==========================================================================

void setup()
{
  int sda_pin = 0;
  int scl_pin = 1;
  Wire.setPins(sda_pin, scl_pin);
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz
  Serial.begin(115200);//while (!Serial); //Wait for user to open terminal

  if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
}
  //ESP32 As access point
  WiFi.mode(WIFI_AP); //Access Point mode
  WiFi.softAP(ssid, password);
  
  Serial.print("SDA pin: "); Serial.println(sda_pin);
  Serial.print("SCL pin: ");Serial.println(scl_pin);
  
  Serial.println("MLX90640 IR Array Example");

  if (isConnected() == false)
  {
    Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
    while (1);
  }
  Serial.println("MLX90640 online!");

  //Get device parameters - We only have to do this once
  int status=0;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
    Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
    Serial.println("Parameter extraction failed");
  
  int SetRefreshRate = 0;
  SetRefreshRate = MLX90640_SetRefreshRate(0x33,0x03);
  //int SetInterleavedMode = MLX90640_SetInterleavedMode(MLX90640_address);
  int SetChessMode = MLX90640_SetChessMode(MLX90640_address);
  

// --- Part Display OLED --- //


  Serial.println("init");
  uint16_t time = millis();
  time = millis() - time;
  Serial.println(time, DEC);
  delay(500);


 // display.fillScreen(BLACK);
  lcdTestThermalImage();
  delay(1000);
  //Once params are extracted, we can release eeMLX90640 array

// --- Part WebServer ESP --- //
  
  IPAddress ServerIP = WiFi.softAPIP(); // Obtain the IP of the Serve
  Serial.print("IP address: ");
  Serial.println(ServerIP);   //IP address assigned to your ESP
  //display.setCursor(0,49);
  //display.print(ServerIP);    // IP address on Display
//----------------------------------------------------------------

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
    //request->send_P(200, "text/html", index_html);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getCenterTemp().c_str());
  });
  server.on("/tempmax", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getMaxTemp().c_str());
  });
  server.on("/tempmin", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getMinTemp().c_str());
  });
  //server.on("/thermal", HTTP_GET, [](AsyncWebServerRequest *request){
//    request->send(SPIFFS, "/thermal.bmp", "image/bmp", false);
//  });
server.on("/thermal", HTTP_GET, [](AsyncWebServerRequest *request) {
    std::vector<uint8_t> bmpData = ThermalImageToWeb(mlx90640To, MinTemp, MaxTemp);
    request->send_P(200, "image/bmp", reinterpret_cast<const uint8_t*>(bmpData.data()), bmpData.size());
});


  server.begin();                  //Start server
  //Serial.println("HTTP server started");
}


// LOOP
//===========================================================================

void loop()
{
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  float vdd, Ta, tr, emissivity;
  float centerTempSum = 0;
  int centerPixelCount = 0;

  // Read Thermal Image from MLX90640
  for (byte x = 0 ; x < 2 ; x++) //Read both subpages
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0)
    {
      Serial.print("GetFrame Error: ");
      Serial.println(status);
    }

     vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
     Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

     tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
     emissivity = 0.95;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);

  }

    CenterTemp = (mlx90640To[367]+mlx90640To[368]+mlx90640To[399]+mlx90640To[400]) / 4.0;  // Temp in Center - based on 4 pixels

    MaxTemp = mlx90640To[0];            // Get first data to find Max and Min Temperature
    MinTemp = mlx90640To[0];
    
    for (int x = 0 ; x < 768 ; x++)     // Find Maximum and Minimum Temperature
    {
      if (mlx90640To[x] > MaxTemp){
        MaxTemp = mlx90640To[x];
      }
      if (mlx90640To[x] < MinTemp){
        MinTemp = mlx90640To[x];
      }
    }


    for (int y = 8; y < 24; y++) { // Calculate the average of the center 16x16 pixels
      for (int x = 8; x < 24; x++) {
        int idx = x + (y * 32);
        centerTempSum += mlx90640To[idx];
        centerPixelCount++;

      }
    }
    float avgCenterTemp = centerTempSum / centerPixelCount;

    // Print data in JSON format once per second
    if (currentTime - lastPrintTime >= 1000) {
      lastPrintTime = currentTime;

      // Populate JSON document
      jsonDoc["avg_center_temp"] = avgCenterTemp;
      jsonDoc["max_temp"] = MaxTemp;
      jsonDoc["min_temp"] = MinTemp;
      jsonDoc["vdd"] = vdd;
      jsonDoc["Ta"] = Ta;
      JsonArray rawArray = jsonDoc.createNestedArray("raw_temps");
      for (int i = 0; i < 768; i++) {
        rawArray.add(mlx90640To[i]);
      }

      // Serialize JSON and print to Serial
      serializeJson(jsonDoc, Serial);
      Serial.println(); // Newline for readability

      // Clear the document to avoid overflow in the next loop
      jsonDoc.clear();

    }
    //MLX_to_Serial(mlx90640To);
    
  //ThermalImageToWeb(mlx90640To, MinTemp, MaxTemp);
  //delay(50);
  //void ThermalImageToWeb(float mlx90640To[], float MinTemp, float MaxTemp)


}

