#include <Adafruit_PWMServoDriver.h>  //控制 PCA9685 16通道 PWM 驱动模块，用于舵机控制。
#include <ESP8266WiFi.h>              //让 ESP8266 能连接 WiFi 网络
#include <NTPClient.h>                //通过 NTP 协议从网络获取当前时间
#include <WiFiUdp.h>                  //配合 NTPClient 用于 UDP 通信
#include <Wire.h>                     //I2C 总线通信，控制 OLED 屏幕、PCA9685
#include <Adafruit_GFX.h>             //Adafruit 的图形库，支持基本绘图操作
#include <Adafruit_SSD1306.h>         //驱动 0.96 英寸 OLED 屏（128x64 分辨率）

#define SCREEN_WIDTH 128  //定义屏幕宽度
#define SCREEN_HEIGHT 64  //定义屏幕长度
#define OLED_ADDR 0x3C    //oled的iic地址
#define OLED_RESET -1     //屏幕没有RESET引脚设为-1

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  //创建 OLED 屏幕对象的关键语句

Adafruit_PWMServoDriver pwmH = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwmM = Adafruit_PWMServoDriver(0x41);  //初始化两个 PCA9685 舵机驱动板
//GPIO4 D2 SDA
//GPIO5 D1 SCL
const char *ssid = "HelloWorld-2.4G";    // 替换为你的 WiFi 名称
const char *password = "1234567891011";  // 替换为你的 WiFi 密码

// NTP 配置
WiFiUDP ntpUDP;
// 使用NTP服务器pool.ntp.org，时区设置为 UTC+8
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000);  //也可以用阿里云的国内服务器ntp.aliyun.com

int servoFrequency = 50;  //舵机频率

int segmentHOn[14] = { 160, 480, 360, 370, 470, 340, 500, 130, 430, 160, 400, 430, 180, 380 };  //On小时

int segmentMOn[14] = { 330, 440, 280, 310, 460, 320, 210, 360, 340, 290, 305, 420, 300, 320 };  //On分钟



int segmentHOff[14] = { 400, 250, 600, 600, 220, 560, 230, 370, 190, 410, 630, 170, 420, 130 };  //Off小时

int segmentMOff[14] = { 120, 220, 500, 110, 240, 540, 440, 130, 120, 510, 100, 190, 520, 540 };  //Off分钟

//每个舵机的角度都要慢慢一个一个调整，找到合适的位置，因为ESP8266下载程序比较慢，我建议先用arduino调整，后期再用ESP8266


int digits[10][7] = { { 1, 1, 1, 1, 1, 1, 0 },    //0
                      { 0, 1, 1, 0, 0, 0, 0 },    //1
                      { 1, 1, 0, 1, 1, 0, 1 },    //2
                      { 1, 1, 1, 1, 0, 0, 1 },    //3
                      { 0, 1, 1, 0, 0, 1, 1 },    //4
                      { 1, 0, 1, 1, 0, 1, 1 },    //5
                      { 1, 0, 1, 1, 1, 1, 1 },    //6
                      { 1, 1, 1, 0, 0, 0, 0 },    //7
                      { 1, 1, 1, 1, 1, 1, 1 },    //8
                      { 1, 1, 1, 1, 0, 1, 1 } };  //9


int hourTens = 0;  //创建变量来存储每个显示数字
int hourUnits = 0;
int minuteTens = 0;
int minuteUnits = 0;

int seconds = 0;

int prevHourTens = 8;  //创建变量以存储先前显示的数字
int prevHourUnits = 8;
int prevMinuteTens = 8;
int prevMinuteUnits = 8;

int midOffset = 150;  //中间段相邻的左右两段所移动的距离

void setup() {
  Serial.begin(115200);

  Wire.begin(4, 5);  //SDA, SCL

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {  //0x3C对应I2C地址，一般的0.96寸OLED都是这个地址，我已用程序验证过
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  //卡死在这里
  }

  //oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.println("Hello >.<");
  oled.setCursor(0, 18);
  oled.println("I'm a");
  oled.setCursor(0, 36);
  oled.println("clock");

  oled.display();
  delay(3000);  //仅展示开头动画的延时

  pwmH.begin();
  pwmM.begin();
  //pwmH.setOscillatorFrequency(27000000);
  //pwmM.setOscillatorFrequency(27000000);
  pwmH.setPWMFreq(servoFrequency);
  pwmM.setPWMFreq(servoFrequency);

  //显示WiFi连接状态
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("Connecting");

  oled.setCursor(0, 25);
  oled.print(ssid);
  oled.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    oled.print(".");
    oled.display();
  }

  //WiFi连接成功显示
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println("Connected!");
  oled.setCursor(0, 25);
  //oled.print("IP: ");
  oled.println(WiFi.localIP());
  oled.display();
  delay(2500);

  timeClient.begin();  //初始化 NTP 客户端

  initializeSegments();  //把初始都是8改为初始时中间段ON,其余都OFF，这样不会打架

  delay(1000);
}


void loop() {

  timeClient.update();  //更新 NTP 时间

  //更新时间显示到OLED
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();

    oled.clearDisplay();

    oled.setCursor(0, 0);
    //oled.print("Time: ");
    oled.print(timeClient.getFormattedTime());

    oled.setCursor(0, 25);
    //oled.print("IP: ");
    oled.println(WiFi.localIP().toString().substring(0, 13));  //显示IP

    oled.setCursor(0, 50);
    //oled.print("Name: ");
    oled.println(ssid);  //WiFi名称

    oled.display();
  }


  // 获取小时、分钟和秒
  int hour = timeClient.getHours();
  int minute = timeClient.getMinutes();

  int temp = hour;
  hourTens = temp / 10;  //把小时拆出个位和十位
  hourUnits = temp % 10;

  temp = minute;
  minuteTens = temp / 10;
  minuteUnits = temp % 10;




  if (minuteUnits != prevMinuteUnits)  //判断时间变化就舵机臂就发生变化
    updateDisplay();


  savePreviousState();

  delay(500);
}

void savePreviousState() {

  prevHourTens = hourTens;
  prevHourUnits = hourUnits;
  prevMinuteTens = minuteTens;
  prevMinuteUnits = minuteUnits;
}

void updateDisplay() {
  updateMid();                  //先将与中间段相邻的区段移开，为中间段腾出空间，然后再移动中间段
  for (int i = 0; i <= 5; i++)  //移动其余的段
  {
    if (digits[hourTens][i] == 1)  //小时十位
      pwmH.setPWM(i + 7, 0, segmentHOn[i + 7]);
    else
      pwmH.setPWM(i + 7, 0, segmentHOff[i + 7]);
    delay(50);
    if (digits[hourUnits][i] == 1)  //小时个位
      pwmH.setPWM(i, 0, segmentHOn[i]);
    else
      pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(50);
    if (digits[minuteTens][i] == 1)  //分钟十位
      pwmM.setPWM(i + 7, 0, segmentMOn[i + 7]);
    else
      pwmM.setPWM(i + 7, 0, segmentMOff[i + 7]);
    delay(50);
    if (digits[minuteUnits][i] == 1)  //分钟个位
      pwmM.setPWM(i, 0, segmentMOn[i]);
    else
      pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(50);
  }
}

void updateMid()  //优先处理中间段
{
  if (digits[minuteUnits][6] != digits[prevMinuteUnits][6])  //为了变化美观最后先判断中间段有没有变化
  {
    if (digits[prevMinuteUnits][2] == 1) {
      pwmM.setPWM(2, 0, segmentMOn[2] + midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
    if (digits[prevMinuteUnits][4] == 1) {
      pwmM.setPWM(4, 0, segmentMOn[4] - midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
  }
  if (digits[minuteUnits][6] == 1)
    pwmM.setPWM(6, 0, segmentMOn[6]);
  else
    pwmM.setPWM(6, 0, segmentMOff[6]);



  if (digits[minuteTens][6] != digits[prevMinuteTens][6]) {
    if (digits[prevMinuteTens][2] == 1) {
      pwmM.setPWM(9, 0, segmentMOn[9] + midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
    if (digits[prevMinuteTens][4] == 1) {
      pwmM.setPWM(11, 0, segmentMOn[11] - midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
  }
  if (digits[minuteTens][6] == 1)
    pwmM.setPWM(13, 0, segmentMOn[13]);
  else
    pwmM.setPWM(13, 0, segmentMOff[13]);




  if (digits[hourUnits][6] != digits[prevHourUnits][6]) {
    if (digits[prevHourUnits][2] == 1) {
      pwmH.setPWM(2, 0, segmentHOn[2] + midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
    if (digits[prevHourUnits][4] == 1) {
      pwmH.setPWM(4, 0, segmentHOn[4] - midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
  }
  if (digits[hourUnits][6] == 1)
    pwmH.setPWM(6, 0, segmentHOn[6]);
  else
    pwmH.setPWM(6, 0, segmentHOff[6]);





  if (digits[hourTens][6] != digits[prevHourTens][6]) {
    if (digits[prevHourTens][2] == 1) {
      pwmH.setPWM(9, 0, segmentHOn[9] + midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
    if (digits[prevHourTens][4] == 1) {
      pwmH.setPWM(11, 0, segmentHOn[11] - midOffset);
      delay(200);
    }  //打开一定距离让中间段收起
  }
  if (digits[hourTens][6] == 1)
    pwmH.setPWM(13, 0, segmentHOn[13]);
  else
    pwmH.setPWM(13, 0, segmentHOff[13]);
}

void initializeSegments()  //初始位置
{
  for (int i = 0; i <= 5; i++) {
    pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(15);
  }

  pwmM.setPWM(6, 0, segmentMOn[6]);
  delay(20);

  for (int i = 7; i <= 12; i++) {
    pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(15);
  }

  pwmM.setPWM(13, 0, segmentMOn[13]);
  delay(20);

  for (int i = 0; i <= 5; i++) {
    pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(15);
  }

  pwmH.setPWM(6, 0, segmentHOn[6]);
  delay(20);

  for (int i = 7; i <= 12; i++) {
    pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(15);
  }

  pwmH.setPWM(13, 0, segmentHOn[13]);
  delay(20);
  //把初始都是8改为初始时中间段ON,其余都OFF，这样不会打架
}