#include "ESP8266.h"
#include "SoftwareSerial.h"
#include <Wire.h>
#include "MAX30105.h"           //MAX3010x library
#include "heartRate.h"          //Heart rate calculating algorithm
#include <U8glib.h>
#define INTERVAL_LCD             20             //定义OLED刷新时间间隔  
unsigned long lcd_time = millis();                 //OLED刷新时间计时器
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);  //设置OLED型号  
#define setFont_L u8g.setFont(u8g_font_7x13)

//配置ESP8266WIFI设置
#define SSID "HUAWEI P40"    //填写2.4GHz的WIFI名称，不要使用校园网
#define PASSWORD "2020210456"//填写自己的WIFI密码
#define HOST_NAME "api.heclouds.com"  //API主机名称，连接到OneNET平台，无需修改
#define DEVICE_ID "643954257"       //填写自己的OneNet设备ID
#define HOST_PORT (80)                //API端口，连接到OneNET平台，无需修改
String APIKey = "oBpMGlpgwi0xRE7E4zrGWYR8EqQ="; //与设备绑定的APIKey

#define INTERVAL_SENSOR 400 //定义传感器采样及发送时间间隔
MAX30105 particleSensor;

float beatsPerMinute=0.00;
//定义ESP8266所连接的软串口
/*********************
 * 该实验需要使用软串口
 * Arduino上的软串口RX定义为D3,
 * 接ESP8266上的TX口,
 * Arduino上的软串口TX定义为D2,
 * 接ESP8266上的RX口.
 * D3和D2可以自定义,
 * 但接ESP8266时必须恰好相反
 *********************/
SoftwareSerial mySerial(3, 2);
ESP8266 wifi(mySerial);



static const unsigned char PROGMEM logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,              //Logo2 and Logo3 are two bmp pictures that display on the OLED if called
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,  };

static const unsigned char PROGMEM logo3_bmp[] =
{ 0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00  };

void setup()
{
  particleSensor.begin(Wire, I2C_SPEED_FAST); //Use default I2C port, 400kHz speed
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  
  mySerial.begin(115200); //初始化软串口
  Serial.begin(9600);     //初始化串口
  Serial.print("setup begin\r\n");
  pinMode(10,OUTPUT);  //设置输出口

  //以下为ESP8266初始化的代码
  Serial.print("FW Version: ");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStation()) {
    Serial.print("to station ok\r\n");
  } else {
    Serial.print("to station err\r\n");
  }

  //ESP8266接入WIFI
  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  mySerial.println("AT+UART_CUR=9600,8,1,0,0");
  mySerial.begin(9600);
  Serial.println("setup end\r\n");
}

unsigned long net_time1 = millis(); //数据上传服务器时间
void loop(){
  if (net_time1 > millis())
    net_time1 = millis();

  if (millis() - net_time1 > INTERVAL_SENSOR) //发送数据时间间隔
  {
   if (particleSensor.getIR()>= 65000)                        //If a heart beat is detected
  {digitalWrite(10,LOW);
    beatsPerMinute = (particleSensor.getIR() / 1000.0)*0.78;           //Calculating the BPM

             u8g.firstPage();  
    do {         
       setFont_L;
    u8g.drawBitmapP(10, 10, 4, 32,logo3_bmp);
    u8g.setPrintPos(60,30);
    u8g.print("BPM");
    u8g.setPrintPos(90,30);
    u8g.print(beatsPerMinute);
    }while (u8g.nextPage());

        delay(100);
        
     u8g.firstPage();  
    do {         
       setFont_L;
    u8g.drawBitmapP(15, 15, 3, 16, logo2_bmp);
    u8g.setPrintPos(60,30);
    u8g.print("BPM");
    u8g.setPrintPos(90,30);
    u8g.print(beatsPerMinute);
    }while (u8g.nextPage());
    
  }
   if (particleSensor.getIR()< 65000&&particleSensor.getIR()>= 7000)                        //If a heart beat is detected
  {digitalWrite(10,HIGH);
    beatsPerMinute = (particleSensor.getIR() / 1000.0)*0.78;           //Calculating the BPM

             u8g.firstPage();  
    do {         
       setFont_L;
    u8g.drawBitmapP(10, 10, 4, 32,logo3_bmp);
    u8g.setPrintPos(60,30);
    u8g.print("BPM");
    u8g.setPrintPos(90,30);
    u8g.print(beatsPerMinute);
    }while (u8g.nextPage());

        delay(100);
        
     u8g.firstPage();  
    do {         
       setFont_L;
    u8g.drawBitmapP(15, 15, 3, 16, logo2_bmp);
    u8g.setPrintPos(60,30);
    u8g.print("BPM");
    u8g.setPrintPos(90,30);
    u8g.print(beatsPerMinute);
    }while (u8g.nextPage());
    
  }
  if (particleSensor.getIR() < 7000){  digitalWrite(10,LOW);   
    beatsPerMinute = 0.00;
    //If no finger is detected it inform the user and put the average BPM to 0 or it will be stored for the next measure
     u8g.firstPage();  
    do {         
       setFont_L;
    u8g.setPrintPos(10,10);
    u8g.print("Please place");
    u8g.setPrintPos(30,30);
    u8g.print("your finger");
    }while (u8g.nextPage());
     delay(50);
     }

    Serial.println(beatsPerMinute);  
     
 
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) { //建立TCP连接，如果失败，不能发送该数据
      Serial.print("create tcp ok\r\n");
      char buf[10];
      //拼接发送data字段字符串
      String jsonToSend = "{\"HeartBeat\":";
      dtostrf(beatsPerMinute, 1, 2, buf);
      jsonToSend += "\"" + String(buf) + "\"";
      jsonToSend += "}";

      //拼接POST请求字符串
      String postString = "POST /devices/";
      postString += DEVICE_ID;
      postString += "/datapoints?type=3 HTTP/1.1";
      postString += "\r\n";
      postString += "api-key:";
      postString += APIKey;
      postString += "\r\n";
      postString += "Host:api.heclouds.com\r\n";
      postString += "Connection:close\r\n";
      postString += "Content-Length:";
      postString += jsonToSend.length();
      postString += "\r\n";
      postString += "\r\n";
      postString += jsonToSend;
      postString += "\r\n";
      postString += "\r\n";
      postString += "\r\n";

      const char *postArray = postString.c_str(); //将str转化为char数组

      Serial.println(postArray);
      wifi.send((const uint8_t *)postArray, strlen(postArray)); //send发送命令，参数必须是这两种格式，尤其是(const uint8_t*)
      Serial.println("send success");
      if (wifi.releaseTCP()) { //释放TCP连接
        Serial.print("release tcp ok\r\n");
      } else {
        Serial.print("release tcp err\r\n");
      }
      postArray = NULL; //清空数组，等待下次传输数据
    } else {
      Serial.print("create tcp err\r\n");
    }

    Serial.println("");

    net_time1 = millis();
  }
}
