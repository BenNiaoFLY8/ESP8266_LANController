
/**********************************************************************
项目名称/Project          : 局域网开关灯神器
程序名称/Program name     : LAN_Controller V1.0
作者/Author               : 笨鸟FLY8
日期/Date（YYYYMMDD）     : 20211206
程序目的/Purpose          : 使用ESP8266建立基本服务器,用户可通过浏览器访问8266所建立的网页,并通过该页面控制舵机，打开、关闭灯的开关。另外增加了OLED可以显示程序的运行状态和实时时间已经近三天的天气信息。
***********************************************************************/

#include <ESP8266WiFi.h>        // 本程序使用 ESP8266WiFi库
#include <ESP8266WebServer.h>   //  ESP8266WebServer库
#include <FS.h>

#include <ArduinoJson.h>
#include <Wire.h>  
#include "SSD1306Wire.h"  

#include "Chinese.h"//汉字信息库
#include "Images.h"//天气图像库
#include "Config.h"//其他配置信息

//===============获取天气信息的地区===================//
String City = "xian";//城市

//===============心知天气私钥===================//
String My_Key = "XXXXXXXXXXXXXXXX";//需要自己注册

//心知天气请求URL
String url = "/v3/weather/daily.json?key=" + My_Key +"&location=" + City + "&language=zh-Hans&unit=c&start=0&days=3";

//心知天气请求数据
String urlDat = "key=" + My_Key +"&location=" + City + "&language=zh-Hans&unit=c&start=0&days=3";

WiFiClient client;

//=================配置OLED端口信息=====================//

SSD1306Wire display(0x3c,OLED_SDA,OLED_SCL);

//=================配置网络信息=====================//

const char* ssid     = "笨鸟FLY8";      // 连接WiFi名需要连接的WiFi名填入引号中
const char* password = "benniaoFLY8";   // 连接WiFi密码需要连接的WiFi密码填入引号中

//===============为了方便访问，配置为固定IP，需要根据自己路由器调整===================//

IPAddress local_IP(192,168,31,31);    // 设置IP
IPAddress gateway(192,168,31, 1);     // 设置网关
IPAddress subnet(255, 255, 255, 0);   // 设置子网掩码
IPAddress dns(61,134,1,4);            // 设置局域网DNS

ESP8266WebServer esp8266_server(80);// 建立网络服务器对象，该对象用于响应HTTP请求。监听端口（80）

void setup(void)
{
  Serial.begin(115200);   // 启动串口通讯波特率115200

  InitPort();

  delay(100);//延时100ms启动OLED

  display.init();//OLED初始化
  display.flipScreenVertically();//屏幕垂直翻转

  // 设置开发板网络环境
  if (!WiFi.config(local_IP, gateway, subnet, dns)) {
    Serial.println("Failed to Config ESP8266 IP"); 
    display.clear();//清屏
    display.setFont(ArialMT_Plain_16);
    display.drawStringMaxWidth(0, 10, 128, "Failed to Config ESP8266 IP");
    display.display();
  } 

  WiFi.begin(ssid, password);                  // 启动网络连接
  Serial.print("Connecting to ");              // 串口监视器输出网络连接信息
  Serial.print(ssid); 
  Serial.println(" ...");  // 告知用户控制器正在尝试WiFi连接

  display.clear();//清屏
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 0, 128, "Connecting to :");
//  display.drawStringMaxWidth(0, 36, 128, ssid);//英文的SSID可以直接显示
  display.drawXbm(0 , 25, 16, 16, ben);//显示“笨”//汉字的SSID需要处理后显示
  display.drawXbm(16, 25, 16, 16, niao);//显示“鸟”
  display.drawStringMaxWidth(32, 25, 128, "FLY8");
  display.drawStringMaxWidth(0, 48, 128, "Please wait...");

  display.display();
  
  int i = 0;                                   // 这一段程序语句用于检查WiFi是否连接成功
  while (WiFi.status() != WL_CONNECTED)        // WiFi.status()函数的返回值是由WiFi连接状态所决定的。 
  {
    delay(1000);                               // 如果WiFi连接成功则返回值为WL_CONNECTED                       
    Serial.print(i++); 
    Serial.print(' ');                         // 此处通过While循环每秒检查一次WiFi.status()函数返回值
  }                                            // 同时通过串口监视器输出连接时长读秒。
                                               // 这个读秒是通过变量i每隔一秒自加1来实现的。
  
  // WiFi连接成功后将通过串口监视器输出连接成功信息 
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // 通过串口监视器输出连接的WiFi名称
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // 通过串口监视器输出控制板的IP

  display.clear();//清屏
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 0, 128, "Connected to :");
//  display.drawStringMaxWidth(0, 36, 128, ssid);//字符的SSID可以直接显示
  display.drawXbm(0 , 16, 16, 16, ben);//显示“笨”//汉字的SSID需要处理后显示
  display.drawXbm(16, 16, 16, 16, niao);//显示“鸟”

  display.drawStringMaxWidth(32, 16, 128, "FLY8");
  display.drawStringMaxWidth(0, 32, 128, "IP : ");
  
  IP = String(WiFi.localIP().toString());

  display.drawStringMaxWidth(0, 48, 128, IP);
  display.display();
 
  if(SPIFFS.begin())                       // 启动闪存文件系统
  {
    Serial.println("SPIFFS Started");
  } 
  else 
  {
    Serial.println("SPIFFS Failed to Start");
  }     
 
  esp8266_server.on("/setLED", HandleSwitch);
  
  esp8266_server.onNotFound(HandleUserRequest); // 处理其它网络请求

  esp8266_server.begin();         // 启动网站服务
  Serial.println("Server Started");

  delay(2000);//延时3S

  display.clear();//清屏
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 24, 128, "Server Started");
  display.display();

  delay(2000);//延时3S

  esp8266_server.handleClient();
  GET_Weather();

  display.clear();//清屏
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 24, 128, "Getting Time...");
  display.display();
  GET_Time();
}
 
// 设置处理404情况的函数'handleNotFound'
void handleNotFound()
{
  esp8266_server.send(404, "text/plain", "404: Not found"); // 发送"404: Not found"
}

void loop(void) 
{
  esp8266_server.handleClient();
  Display();

  if(LOW == digitalRead(KEY))
  {
    delay(100);//防抖动
    while(LOW == digitalRead(KEY));//等待释放

    DisplayTime = 10;
    
    if(AFTERDAY_WEATHER == DisplayFlag)
    {
      DisplayTime = 0;
      DisplayFlag = WIFI_INFO;
    }

//    Serial.println(DisplayFlag);
//    Serial.println(DisplayTime);
  }

  if(LNum != millis()/1000)//秒计时器
  {
    LNum = millis()/1000;
    TimerNum++;
    tick();  // 计时函数
  }
}

void HandleSwitch(void) 
{
 String ledState = "OFF";
 String LED_State = esp8266_server.arg("LEDstate"); //获取参数
 Serial.println(LED_State);

 if(LED_State == "1")
 {
  digitalWrite(LED_BUILTIN,LOW); //LED ON

  OledDisplay(LAN_LIGHT_ON);

  DisplayTime = 4;

  SG_90_Ctr(true);//打开开关
 } 
 else 
 {
  digitalWrite(LED_BUILTIN,HIGH); //LED OFF

  OledDisplay(LAN_LIGHT_OFF);

  DisplayTime = 4;

  SG_90_Ctr(false);//关闭开关
 }
}

// 处理用户浏览器的HTTP访问
void HandleUserRequest(void) 
{         
  String reqResource = esp8266_server.uri();  // 获取用户请求资源(Request Resource）
  Serial.print("reqResource: ");
  Serial.println(reqResource);
  
  bool fileReadOK = handleFileRead(reqResource);  // 通过handleFileRead函数处处理用户请求资源

  // 如果在SPIFFS无法找到用户访问的资源，则回复404 (Not Found)
  if(!fileReadOK)
  {                                                 
    esp8266_server.send(404, "text/plain", "404 Not Found"); 
  }
}

bool handleFileRead(String resource)              //处理浏览器HTTP访问
{
  if(resource.endsWith("/"))                      // 如果访问地址以"/"为结尾
  {
    resource = "/index.html";                     // 则将访问地址修改为/index.html便于SPIFFS访问
  } 
  
  String contentType = getContentType(resource);  // 获取文件类型
  
  if (SPIFFS.exists(resource))                    // 如果访问的文件可以在SPIFFS中找到 
  {
    File file = SPIFFS.open(resource, "r");       // 则尝试打开该文件
    esp8266_server.streamFile(file, contentType); // 并且将该文件返回给浏览器
    file.close();                                 // 并且关闭文件
    return true;                                  // 返回true
  }
  return false;                                   // 如果文件未找到，则返回false
}

/**************************************************
 * 函数名称：SG_90_Ctr
 * 函数功能：控制舵机开/关灯
 * 参数说明：Flag：true  打开开关。Flag：false 关闭开关。  
**************************************************/
void SG_90_Ctr(bool Flag)
{
  int k;  
  if(true == Flag)//打开开关
  {
    for(k=0;k<20;k++)
    {
        digitalWrite(SG90_PWM, HIGH);// 输出高
        delayMicroseconds(1800);
        digitalWrite(SG90_PWM, LOW);// 输出低
        delayMicroseconds(19200);
      }
    }
    else//关闭开关
    {
      for(k=0;k<20;k++)
      {
        digitalWrite(SG90_PWM, HIGH);// 输出高
        delayMicroseconds(1200);
        digitalWrite(SG90_PWM, LOW);// 输出低
        delayMicroseconds(18800);
        }
    }

    for(k=0;k<20;k++)//舵机复位
    {
      digitalWrite(SG90_PWM, HIGH);// 输出高
      delayMicroseconds(1500);
      digitalWrite(SG90_PWM, LOW);// 输出低
      delayMicroseconds(18500);
      }
  }

/**************************************************
 * 函数名称：GET_Time
 * 函数功能：获取时间数据
 * 参数说明：无
**************************************************/
void GET_Time(void)
{
  String TimeinputString = "";  //接收到的数据
  
  if (!client.connect(Timehost,Timeport)) 
  {
    Serial.println("服务器连接失败");
    return;
  }    

  client.print(String("GET ") + TimeUrl + " HTTP/1.1\r\n" +  //请求行  请求方法 ＋ 请求地址 + 协议版本
               "Host: " + Timehost + "\r\n" +                //请求头部
               "Connection: close\r\n" +                     //处理完成后断开连接
               "\r\n" +                                      //空行
               TimeUrlDat);                                  //请求数据     
                
  delay(100);  
  while(client.available()) //接收数据
  {          
      String Timeline = client.readStringUntil('\r');
      TimeinputString += Timeline;
  }

  int jsonBeginAt = TimeinputString.indexOf("{");   //判断json数据完整性
  int jsonEndAt = TimeinputString.lastIndexOf("}");
  if (jsonBeginAt != -1 && jsonEndAt != -1) 
  {
      //净化json数据
      TimeinputString = TimeinputString.substring(jsonBeginAt, jsonEndAt + 1);//取得一个完整的JSON字符串

      const size_t capacity = JSON_OBJECT_SIZE(2) + 60;
      
      DynamicJsonDocument doc(capacity);
      deserializeJson(doc, TimeinputString);
    
      const char* sysTime2 = doc["sysTime2"]; // "2021-12-17 17:20:31"
      const char* sysTime1 = doc["sysTime1"]; // "20211217172031"
    
      String temptime = doc["sysTime1"].as<String>();
    
      Time.Year   = (temptime.substring(0,4)).toInt();
      Time.Month  = (temptime.substring(4,6)).toInt();
      Time.Day    = (temptime.substring(6,8)).toInt();
    
      Time.Hour   = (temptime.substring(8,10)).toInt();
      Time.Minute = (temptime.substring(10,12)).toInt();
      Time.Second = (temptime.substring(12)).toInt();
    
      //基姆拉尔森计算公式
      Time.Week=(Time.Day + 2*Time.Month + 3*(Time.Month+1)/5 + Time.Year + Time.Year/4 - Time.Year/100 + Time.Year/400) % 7;

      TimeinputString="";
  }

  if(0 == Time.Year)//如果获取时间失败，递归调用
  {
    GET_Time();
  }
}

/**************************************************
 * 函数名称：GET_Weather
 * 函数功能：获取天气数据
 * 参数说明：无
**************************************************/
void GET_Weather(void)
{
  display.clear();//清屏
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 24, 128, "Getting Weather");//显示Getting Weather
  display.display();

  if(!client.connect(WeatherHost,WeatherPort)) 
  {
      Serial.println("服务器连接失败");
      return;
  }    
    
  // 发送请求报文
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +  //请求行  请求方法 ＋ 请求地址 + 协议版本
               "Host: " + WeatherHost + "\r\n" +         //请求头部
               "Connection: close\r\n" +                 //处理完成后断开连接
               "\r\n" +                                  //空行
               urlDat);                                  //请求数据            
  delay(100);  
  while(client.available()) //接收数据
  {          
    String line = client.readStringUntil('\r');
    inputString += line;
  }
  client.stop();      //断开与服务器连接以节约资源

  ProcessInfo();      //处理接收到的信息
}

/**************************************************
 * 函数名称：ProcessInfo
 * 函数功能：处理接收到的json数据
 * 参数说明：无
**************************************************/
void ProcessInfo(void)
{
  int jsonBeginAt = inputString.indexOf("{");   //判断json数据完整性
  int jsonEndAt = inputString.lastIndexOf("}");
  
  if(jsonBeginAt != -1 && jsonEndAt != -1) 
  {
    //净化json数据
    inputString = inputString.substring(jsonBeginAt, jsonEndAt + 1);//取得一个完整的JSON字符串
    
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 3*JSON_OBJECT_SIZE(14) + 810;

    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, inputString);
    JsonObject results_0 = doc["results"][0];

    JsonObject results_0_location = results_0["location"];
    const char* results_0_location_id = results_0_location["id"]; // "WQJ6YY8MHZP0"
    const char* results_0_location_name = results_0_location["name"]; // "西安"
    const char* results_0_location_country = results_0_location["country"]; // "CN"
    const char* results_0_location_path = results_0_location["path"]; // "西安,西安,陕西,中国"
    const char* results_0_location_timezone = results_0_location["timezone"]; // "Asia/Shanghai"
    const char* results_0_location_timezone_offset = results_0_location["timezone_offset"]; // "+08:00"
    
    JsonArray results_0_daily = results_0["daily"];
    
    JsonObject  results_0_daily_0 = results_0_daily[0];
    const char* results_0_daily_0_date = results_0_daily_0["date"]; // "2021-12-16"
    const char* results_0_daily_0_text_day = results_0_daily_0["text_day"]; // "多云"
    const char* results_0_daily_0_code_day = results_0_daily_0["code_day"]; // "4"
    const char* results_0_daily_0_text_night = results_0_daily_0["text_night"]; // "多云"
    const char* results_0_daily_0_code_night = results_0_daily_0["code_night"]; // "4"
    const char* results_0_daily_0_high = results_0_daily_0["high"]; // "8"
    const char* results_0_daily_0_low = results_0_daily_0["low"]; // "-3"
    const char* results_0_daily_0_rainfall = results_0_daily_0["rainfall"]; // "0.00"
    const char* results_0_daily_0_precip = results_0_daily_0["precip"]; // "0.00"
    const char* results_0_daily_0_wind_direction = results_0_daily_0["wind_direction"]; // "南"
    const char* results_0_daily_0_wind_direction_degree = results_0_daily_0["wind_direction_degree"]; // "180"
    const char* results_0_daily_0_wind_speed = results_0_daily_0["wind_speed"]; // "8.4"
    const char* results_0_daily_0_wind_scale = results_0_daily_0["wind_scale"]; // "2"
    const char* results_0_daily_0_humidity = results_0_daily_0["humidity"]; // "54"
    
    JsonObject  results_0_daily_1 = results_0_daily[1];
    const char* results_0_daily_1_date = results_0_daily_1["date"]; // "2021-12-17"
    const char* results_0_daily_1_text_day = results_0_daily_1["text_day"]; // "阴"
    const char* results_0_daily_1_code_day = results_0_daily_1["code_day"]; // "9"
    const char* results_0_daily_1_text_night = results_0_daily_1["text_night"]; // "多云"
    const char* results_0_daily_1_code_night = results_0_daily_1["code_night"]; // "4"
    const char* results_0_daily_1_high = results_0_daily_1["high"]; // "4"
    const char* results_0_daily_1_low = results_0_daily_1["low"]; // "-3"
    const char* results_0_daily_1_rainfall = results_0_daily_1["rainfall"]; // "0.00"
    const char* results_0_daily_1_precip = results_0_daily_1["precip"]; // "0.00"
    const char* results_0_daily_1_wind_direction = results_0_daily_1["wind_direction"]; // "东北"
    const char* results_0_daily_1_wind_direction_degree = results_0_daily_1["wind_direction_degree"]; // "45"
    const char* results_0_daily_1_wind_speed = results_0_daily_1["wind_speed"]; // "23.4"
    const char* results_0_daily_1_wind_scale = results_0_daily_1["wind_scale"]; // "4"
    const char* results_0_daily_1_humidity = results_0_daily_1["humidity"]; // "54"
    
    JsonObject  results_0_daily_2 = results_0_daily[2];
    const char* results_0_daily_2_date = results_0_daily_2["date"]; // "2021-12-18"
    const char* results_0_daily_2_text_day = results_0_daily_2["text_day"]; // "阴"
    const char* results_0_daily_2_code_day = results_0_daily_2["code_day"]; // "9"
    const char* results_0_daily_2_text_night = results_0_daily_2["text_night"]; // "多云"
    const char* results_0_daily_2_code_night = results_0_daily_2["code_night"]; // "4"
    const char* results_0_daily_2_high = results_0_daily_2["high"]; // "9"
    const char* results_0_daily_2_low = results_0_daily_2["low"]; // "-2"
    const char* results_0_daily_2_rainfall = results_0_daily_2["rainfall"]; // "0.00"
    const char* results_0_daily_2_precip = results_0_daily_2["precip"]; // "0.00"
    const char* results_0_daily_2_wind_direction = results_0_daily_2["wind_direction"]; // "南"
    const char* results_0_daily_2_wind_direction_degree = results_0_daily_2["wind_direction_degree"]; // "180"
    const char* results_0_daily_2_wind_speed = results_0_daily_2["wind_speed"]; // "3.0"
    const char* results_0_daily_2_wind_scale = results_0_daily_2["wind_scale"]; // "1"
    const char* results_0_daily_2_humidity = results_0_daily_2["humidity"]; // "53"
    
    const char* results_0_last_update = results_0["last_update"]; // "2021-12-16T08:00:00+08:00"  

    TodayWeathe.Log_Num = atoi(results_0_daily_0_code_day);//获取今天天气信息
    TodayWeathe.Max_Temperature = atoi(results_0_daily_0_high);
    TodayWeathe.Min_Temperature = atoi(results_0_daily_0_low);

    TomorroWeathe.Log_Num = atoi(results_0_daily_1_code_day);
    TomorroWeathe.Max_Temperature = atoi(results_0_daily_1_high);
    TomorroWeathe.Min_Temperature = atoi(results_0_daily_1_low);  
      
    AfterDayWeathe.Log_Num = atoi(results_0_daily_2_code_day);
    AfterDayWeathe.Max_Temperature = atoi(results_0_daily_2_high);
    AfterDayWeathe.Min_Temperature = atoi(results_0_daily_2_low);

    inputString="";
  }
}
/**************************************************
 * 函数名称：Display
 * 函数功能：循环显示实时时间和最近三天的天气信息
 * 参数说明：无
**************************************************/
void Display(void)
{
  if(((AFTERDAY_WEATHER == DisplayFlag)&&(5 < DisplayTime))||((NULLINFO == DisplayFlag)&&(5 < DisplayTime)))
  {
    DisplayTime = 0;
    DisplayFlag = DIS_TIME;
  }
  else if((DIS_TIME == DisplayFlag)&&(5 < DisplayTime))
  {
    DisplayTime = 0;
    DisplayFlag = TODAY_WEATHER;
    OledDisplay(TODAY_WEATHER);
  }
  else if((TODAY_WEATHER == DisplayFlag)&&(5 < DisplayTime))
  {
    DisplayTime = 0;
    DisplayFlag = TOMORROW_WEATHER;
    OledDisplay(TOMORROW_WEATHER);
  }
  else if((TOMORROW_WEATHER == DisplayFlag)&&(5 < DisplayTime))
  {
    DisplayTime = 0;
    DisplayFlag = AFTERDAY_WEATHER;
    OledDisplay(AFTERDAY_WEATHER);
  }
  else if(WIFI_INFO == DisplayFlag)
  {
    DisplayFlag = NULLINFO;
    OledDisplay(WIFI_INFO);
  }
}


/**************************************************
 * 函数名称：DisplayTime
 * 函数功能：显示时间、日期、星期
 * 参数说明：Flag：传递需要显示的内容
**************************************************/
void OledDisplay(char Flag)
{
   char displaystr[20];
    
   display.clear();//清屏

   switch(Flag)
   {
     case DIS_TIME:
                  display.setFont(ArialMT_Plain_24);//2021-12-15 时间放大显示
                  
                  sprintf(displaystr,"%02d:%02d:%02d",Time.Hour,Time.Minute,Time.Second);
                  display.drawStringMaxWidth(20, 0, 128, displaystr);
                
                  display.setFont(ArialMT_Plain_16);
                
                  sprintf(displaystr,"%04d-%02d-%02d",Time.Year,Time.Month,Time.Day);
                  display.drawStringMaxWidth(24, 28, 128, displaystr);
                
                  display.drawXbm(40, 48, 16, 16, xing);//星
                  display.drawXbm(56, 48, 16, 16, qi);//期
                
                  switch(Time.Week)
                  {
                      case 0:
                             display.drawXbm(72, 48, 16, 16, yi);//一
                             break;  
                      case 1:
                             display.drawXbm(72, 48, 16, 16, er);//二
                             break;
                      case 2:
                             display.drawXbm(72, 48, 16, 16, san);//三
                             break;
                
                      case 3:
                             display.drawXbm(72, 48, 16, 16, si);//四
                             break;  
                      case 4:
                             display.drawXbm(72, 48, 16, 16, wu);//五
                             break;
                      case 5:
                             display.drawXbm(72, 48, 16, 16, liu);//六
                             break;
                
                       case 6:
                             display.drawXbm(72, 48, 16, 16, tian);//天
                             break;
                      default:
                             display.drawXbm(72, 48, 16, 16, tian);//天
                             break;
                  }  
                  break;
                  
     case TODAY_WEATHER:

                  DisplayWeather(TODAY_WEATHER,TodayWeathe.Log_Num,TodayWeathe.Max_Temperature,TodayWeathe.Min_Temperature);

                  break;

     case TOMORROW_WEATHER:
     
                  DisplayWeather(TOMORROW_WEATHER,TomorroWeathe.Log_Num,TomorroWeathe.Max_Temperature,TomorroWeathe.Min_Temperature);
                 
                  break;
                  
     case AFTERDAY_WEATHER:
     
                  DisplayWeather(AFTERDAY_WEATHER,AfterDayWeathe.Log_Num,AfterDayWeathe.Max_Temperature,AfterDayWeathe.Min_Temperature);
                 
                  break;

     case WIFI_INFO:

                  display.setFont(ArialMT_Plain_16);
                  display.drawStringMaxWidth(0, 0, 128, "SSID : ");
                //  display.drawStringMaxWidth(0, 36, 128, ssid);//字符的SSID可以直接显示
                  display.drawXbm(0 , 16, 16, 16, ben);//显示“笨”//汉字的SSID需要处理后显示
                  display.drawXbm(16, 16, 16, 16, niao);//显示“鸟”
                
                  display.drawStringMaxWidth(32, 16, 128, "FLY8");
                  display.drawStringMaxWidth(0, 32, 128, "IP : ");
                  
                  IP = String(WiFi.localIP().toString());
                
                  display.drawStringMaxWidth(0, 48, 128, IP);

                  break;

     case LAN_LIGHT_ON:

                  display.drawXbm(32 , 24, 16, 16, yuan); //显示“远”
                  display.drawXbm(48 , 24, 16, 16, cheng);//显示“程”
                  display.drawXbm(64 , 24, 16, 16, kai);  //显示“开”
                  display.drawXbm(80 , 24, 16, 16, deng); //显示“灯”

                  break;

     case LAN_LIGHT_OFF:

                  display.drawXbm(32 , 24, 16, 16, yuan); //显示“远”
                  display.drawXbm(48 , 24, 16, 16, cheng);//显示“程”
                  display.drawXbm(64 , 24, 16, 16, guan); //显示“关”
                  display.drawXbm(80 , 24, 16, 16, deng); //显示“灯”
                  
                  break;      
    }

  display.display();
}


/**************************************************
 * 函数名称：DisplayWeather
 * 函数功能：显示天气信息
 * 参数说明：dayinfo：1今天，2明天，3后天；weatherinfo：天气代码；maxtemperatureinfo：最高温度；mintemperatureinfo：最低温度。
**************************************************/
void DisplayWeather(int dayinfo,int weatherinfo,int maxtemperatureinfo,int mintemperatureinfo)
{
  char displaystr[20];
  
  display.drawXbm(64, 0, 16, 16, xi);//西
  display.drawXbm(80, 0, 16, 16, an);//安

  switch(dayinfo)
  {
      case 1:
             display.drawXbm(96, 0, 16, 16, jin);//今
             break;  
      case 2:
             display.drawXbm(96, 0, 16, 16, ming);//明
             break;
      case 3:
             display.drawXbm(96, 0, 16, 16, hou);//后
             break;
      default:
             display.drawXbm(96, 0, 16, 16, hou);//后
             break;
  }  

  display.drawXbm(112, 0, 16, 16, tian);//天

  display.setFont(ArialMT_Plain_16);

  sprintf(displaystr,"%d~%d°C",mintemperatureinfo,maxtemperatureinfo);
  display.drawStringMaxWidth(68, 48, 128, displaystr);

  switch(weatherinfo)
  {
    case 0:
    case 1:
    case 2:
    case 3:
           display.drawXbm(88, 25, 16, 16, qing);//晴
           display.drawXbm(0, 4, 60, 60,   qing_log);//晴图标
           break;  
           
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
           display.drawXbm(80, 25, 16, 16, duo);//多
           display.drawXbm(96, 25, 16, 16, yun);//云
           display.drawXbm(0, 4, 60, 60,   duoyun_log);//多云图标
           break;  

    case 9:
           display.drawXbm(88, 25, 16, 16, yin);//阴
           display.drawXbm(0, 4, 60, 60,   yin_log);//阴图标
           break; 
            
    case 10:
    case 11:
    case 12:
           display.drawXbm(80, 25, 16, 16, zhen);//阵
           display.drawXbm(96, 25, 16, 16, yu);//雨
           display.drawXbm(0, 4, 60, 60,   zhenyu_log);//阵雨图标
           break;  

    case 13:
           display.drawXbm(80, 25, 16, 16, xiao);//小
           display.drawXbm(96, 25, 16, 16, yu);//雨
           display.drawXbm(0, 4, 60, 60,   xiaoyu_log);//小雨图标
           break;  
           
    case 14:
           display.drawXbm(80, 25, 16, 16, zhong);//中
           display.drawXbm(96, 25, 16, 16, yu);//雨
           display.drawXbm(0, 4, 60, 60,   zhongyu_log);//中雨图标
           break;  

    case 15:
    case 16:
    case 17:
    case 18:
           display.drawXbm(80, 25, 16, 16, da);//大
           display.drawXbm(96, 25, 16, 16, yu);//雨
           display.drawXbm(0, 4, 60, 60,   dayu_log);//大雨图标
           break;  

    case 19:
    case 20:
    case 21:
    case 22:
           display.drawXbm(80, 25, 16, 16, xiao);//小
           display.drawXbm(96, 25, 16, 16, xue);//雪
           display.drawXbm(0, 4, 60, 60,   xiaoxue_log);//小雪图标
           break;  

    case 23:
           display.drawXbm(80, 25, 16, 16, zhong);//中
           display.drawXbm(96, 25, 16, 16, xue);//雪
           display.drawXbm(0, 4, 60, 60,   zhongxue_log);//中雪图标
           break;  

    case 24:
    case 25:
           display.drawXbm(80, 25, 16, 16, da);//大
           display.drawXbm(96, 25, 16, 16, yu);//雨
           display.drawXbm(0, 4, 60, 60,   dayu_log);//大雨图标
           break;  

    default:
           display.drawXbm(88, 25, 16, 16, yin);//阴
           display.drawXbm(0, 4, 60, 60,   yin_log);//阴图标
           break; 
  }
}

/**************************************************
 * 函数名称：tick
 * 函数功能：计时器函数，每秒调用一次，处理时间相关功能 
 * 参数说明：无
**************************************************/
void tick(void)
{
  Time.Second++;
  
   if(59 < Time.Second)
   {
     Time.Second = 0;
     Time.Minute++;
     if(59 < Time.Minute)//每小时更新一次天气
     {
      GET_Weather();
      GET_Time();
     }
   }

   if(100 > DisplayTime)
   {
    DisplayTime++;
   }

   if(DIS_TIME == DisplayFlag) 
   {
     OledDisplay(DIS_TIME); // 每秒更新时钟显示      
   }
}
//--- 定时调用计时器函数结束 --- 

// 获取文件类型
String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

/**************************************************
 * 函数名称：tInitPortick
 * 函数功能：初始化端口 
 * 参数说明：无
**************************************************/

void InitPort(void)
{
  pinMode(LED_BUILTIN, OUTPUT);   // 初始化板载LED引脚为OUTPUT
  digitalWrite(LED_BUILTIN, HIGH);// 初始化LED引脚状态

  pinMode(KEY, INPUT);   // 初始按键为INPUT

  pinMode(SG90_PWM, OUTPUT);   // 初始化GPIO13引脚为OUTPUT,SG90的PWM控制端口
  digitalWrite(SG90_PWM, LOW); // 初始化为低电平

  pinMode(SG90_POWER, OUTPUT);   // 初始化GPIO12引脚为OUTPUT,SG90的电源控制端口
  digitalWrite(SG90_POWER, HIGH);// 初始化LED引脚状态

  pinMode(OLED_POWER, OUTPUT);   // 初始化GPIO14引脚为OUTPUT,OLED的电源控制端口
  digitalWrite(OLED_POWER, LOW);//  初始化为低电平
}
