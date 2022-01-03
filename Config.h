
//其他配置信息
//=================端口定义=====================//需要根据自己的硬件进行调整

#define KEY             0
#define SG90_PWM       13
#define SG90_POWER     12

#define OLED_POWER     14

#define OLED_SDA        4
#define OLED_SCL        5
//=================显示参数宏定义=====================//
#define DIS_TIME           0     //显示时间、日期、星期
#define TODAY_WEATHER      1     //显示今天的天气信息
#define TOMORROW_WEATHER   2     //显示明天的天气信息
#define AFTERDAY_WEATHER   3     //显示后天的天气信息
#define WIFI_INFO          4     //显示网络信息

#define LAN_LIGHT_ON       5     //远程开灯
#define LAN_LIGHT_OFF      6     //远程关灯

#define NULLINFO           100     //显示空信息，延时后显示时间
//=================变量=====================//
typedef struct  
{
   int Hour;
   int Minute;
   int Second;
   int Year;
   int Month;
   int Day;
   int Week;
}TimeInfo;       //时间日期结构体

typedef struct
{
  int Max_Temperature;
  int Min_Temperature;
  int Log_Num;
}WeatherInfo; //最高温度、最低温度和天气信息的结构体

WeatherInfo TodayWeathe,TomorroWeathe,AfterDayWeathe;

TimeInfo Time;

String inputString = "";  //接收到的数据

String IP;

unsigned char TimerNum;
unsigned char SecondNum;
unsigned long LNum;
unsigned char DisplayFlag,DisplayTime;

//==========心知天气目标服务器网址和端口==============//
const char* WeatherHost = "api.seniverse.com";
const uint16_t WeatherPort = 80;


//==========获取时间的目标服务器网址和端口==============//
const char* Timehost = "quan.suning.com";
const uint16_t Timeport = 80;

//==========获取时间的URL和信息==============//
const String TimeUrl = "/getSysTime.do";
const String TimeUrlDat = "getSysTime.do";
