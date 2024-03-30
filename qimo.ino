#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>//rfid
#include <IRremote.h>//遥控
#include <LiquidCrystal_I2C.h>
#define red A3
#define green A2
#define blue A1
#define doorlight A0//模拟开关门的RGB
#define RECV_PIN 2//红外遥控器管脚
//遥控器按键定义
#define one 12//1
#define two 24//2
#define three 94//3
#define four 8//4
#define zero 22
//RFID射频模块
#define SS_PIN 10
#define RST_PIN 9
//超声波
#define pingPin 7// 声纳触发端口
#define echoPin 6// 声纳回响端口
//红外对管
#define PIR_sensor_1 4
#define PIR_sensor_2 5
//继电器
#define jidianqi 3

//实例化
IRrecv irrecv(RECV_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);

int backlightflag;//背光是否打开flag
int permit;//是否通过flag，也可以将开门代码整合到此部分，省略此flag
int state;//红外遥控器的接收值
int state_wr = 0;
String nuid = ""; //记录id
byte nuidPICC[4];//存放新的nuid
int user_num = 0; //记录存的用户数量
int max_len_id = 12; //最大id长度
//开关门电路
int a = 0;
String char_a;
int val1 = 1,val2 = 1;
int doorState=0;//0为门关状态,1为门开状态,2为中间状态
void setup() {
  Serial.begin(9600);
  //这些初始化必须放在最前面
  irrecv.enableIRIn();//启动红外遥控接收
  lcd.init();
  lcd.noBacklight();
  Serial.print("waiting for power");
  //初始化数据总线和rfid
  SPI.begin();
  rfid.PCD_Init();
}
void loop() {
//  delay(300);
  Serial.print("state:");Serial.println(state);
  Serial.print("doorstate:");Serial.println(doorState);
  doorState=menjinkaiguan(doorState);
  if (irrecv.decode())
  {
    Serial.println("if:");
    state = irrecv.decodedIRData.command; //读取遥控器的值
    
    irrecv.resume();//继续下一次接收
    switch (state) {
      case one://开机
        power_on();
        break;
      case two://关机
        power_off();
        break;
      case three://录入用户
        add_user();
        break;
      case four://删除用户
        del_user();
        break;
      default://返回loop处，即直接执行else处
        return;
    }//end switch
  }
  else {
    //读卡模式，即正常工作状态
    light_color(0, 255, 0); //绿灯
    ispeple();//检测是否有人，有人开灯welcome，无人关灯节能
    //check card
    if ( ! rfid.PICC_IsNewCardPresent()) {
      return;
    }
    //read card
    if ( ! rfid.PICC_ReadCardSerial()) {
      return;
    }
    //match cardID
    String uid2p;
    uid2p = getuid();
    printuid(uid2p);
//    int usernum = 12; //用户数量在此定义
    if (checkuid(uid2p, 0, user_num))
    {
     doorState=1;
    }
    else doorState=0;
    
  }//end else
}//end loop

void ispeple()
{
  float duration, cm;
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  cm = microsecondsToCentimeters(duration);
  Serial.println(cm);
  if (cm <= 100)
  {
    backlightflag = 1;
    lcd.clear();
    lcd.setBacklight(backlightflag);
    lcd.setCursor(0, 0);
    lcd.print("welcome!");
  }
  else
  {
    backlightflag = 0;
    lcd.setBacklight(backlightflag);
    lcd.clear();
  }
}
//嵌套在ispeople内，声纳用的函数
float microsecondsToCentimeters(float microseconds) {
  return microseconds / 29 / 2;
}
//读出当前卡uid，这段代码可复用性很高，可以和老聂的检查混用一下
String getuid()
{
  byte nuidPICC[4];
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  String uid2p;
  uid2p = String(nuidPICC[0]) + String(nuidPICC[1]) + String(nuidPICC[2]) + String(nuidPICC[3]);
  return uid2p;
}

//显示当前读到的uid，复用性也很高，也可以通用替代混用
void printuid(String uid2p)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("your uid:");
  lcd.setCursor(0, 1);
  lcd.print(uid2p);
  lcd.setBacklight(backlightflag);
  delay(1000);
  lcd.clear();
}

//嵌套在checkuid中的函数，用来对比内存中的用户uid
bool isuser(String uid2p, int startadd, int usernum)
{
  bool y = 0;
  int len=uid2p.length();
  for (int j = 1; j <= 12 -len ; j++)
  { 
    uid2p = uid2p + " ";
  }
  Serial.print(uid2p);
  for (int i = 0; i < usernum; i++)
  { 
    y = (uid2p == get_string(12, i * 12));
    if (y == 1)
    {
      return 1;
    }
  }
  return 0;
}
//用于检查uid权限并显示是否通过
bool checkuid(String uid2p, int startadd, int usernum)
{
  if (isuser(uid2p, startadd, usernum)) {
    lcd.setCursor(0, 0);
    lcd.print("permitted!");
    delay(1000);
    return 1;
  }
  else {
    lcd.print("access denied!");
    delay(1000);
    return 0;
  }
}
//门禁开关
int menjinkaiguan (int a)
{
  int state =a;
   if (state == 0) //关门状态
  {
    analogWrite(doorlight, 255);
    digitalWrite(jidianqi, 1);
  }
  else if (state == 1)  //开门状态
  {
    digitalWrite(jidianqi, 0);  //开门，然后进中间状态
    state = 2;
    val1 = 1;
    val2 = 1;
  }
  else if (state == 2) //中间状态
  {
    delay(5000);  //开启红外检测,并且给5秒时间通过
    val1 = digitalRead(PIR_sensor_1);
    val2 = digitalRead(PIR_sensor_2);
    if (val1 == 1 && val2 == 1)   //如果没人就返回关门状态
    {
      state =0;
    }
    else if (val1 == 0 || val2 == 0) //有人就回到开门状态
    {
      state =1;
    }
  }
  return state;
}

void power_on() {
  //初始化lcd
  lcd.backlight(); lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Power on");
  Serial.println("Power on");
  //初始化rgb
  pinMode(red, OUTPUT); pinMode(green, OUTPUT); pinMode(blue, OUTPUT);
  //申请EEPROM的空间
  EEPROM.begin();//传int报错
  //开关门所需
  pinMode(doorlight, OUTPUT);
  pinMode(jidianqi, OUTPUT);
  pinMode(PIR_sensor_1, INPUT);
  pinMode(PIR_sensor_2, INPUT);
  
  lcd.setCursor(0, 0); lcd.print("System ready");
  Serial.println("System ready.");
  flash_light(blue);//蓝灯闪烁
}
void power_off() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Power off");
  flash_light(blue);//蓝灯闪烁
  lcd.noBacklight();//关闭背光
}

void add_user() {
  light_color(255, 0, 0); //红灯亮起
  lcd.backlight();
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Input state");
  while (true) {
    //收到zero退出读卡状态
    if (irrecv.decode()) {
      state_wr = irrecv.decodedIRData.command;
      Serial.print("state_wr_in_add:");
      Serial.println(state_wr);
      Serial.print("add_user"); Serial.println(nuid);
      irrecv.resume();
      if (state_wr == zero)
      {
        state_wr = 0; //刷新state_wr的值，否则下一次进入会直接break
        Serial.println("break add state");
        break;
      }
    }
    if (( ! rfid.PICC_IsNewCardPresent()) || (! rfid.PICC_ReadCardSerial()))
    {
      continue; //回到while
    }
    if (rfid.uid.uidByte[0] != nuidPICC[0] ||
        rfid.uid.uidByte[1] != nuidPICC[1] ||
        rfid.uid.uidByte[2] != nuidPICC[2] ||
        rfid.uid.uidByte[3] != nuidPICC[3] ) {
      Serial.println(F("A new card has been detected."));
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }
      //lcd显示id
      nuid = String(nuidPICC[0]) + String(nuidPICC[1]) + String(nuidPICC[2]) + String(nuidPICC[3]);
      user_num += 1; //用户数量加1
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("user"); lcd.print(user_num);
      lcd.setCursor(0, 1); lcd.print(nuid);
      int st_add = max_len_id * (user_num - 1);
      int len=nuid.length();//不满12位用空格补到12位
      for (int j = 1; j <= 12 -len ; j++)
      { 
          nuid = nuid + " ";
      }
      //写入e2prom中
      set_string(st_add, nuid);
    }
    else {
      Serial.println(F("Card read previously."));//防止重复多次写同一张卡
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("Read previously");
    }
    rfid.PICC_HaltA();//停止picc
    rfid.PCD_StopCrypto1();
  }//end while
  Serial.println("end while in add state.");
  lcd.clear(); lcd.setCursor(0, 0);
  lcd.print("End add state");
}
void del_user() {
  light_color(255, 0, 0); //红灯亮起
  //删除状态
  lcd.backlight();
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Delete state");
  //读取卡的id
  while (true) {
    if (irrecv.decode()) {
      state_wr = irrecv.decodedIRData.command;
      irrecv.resume();
      if (state_wr == zero)
      {
        state_wr = 0; //刷新state_wr的值，否则下一次进入会直接break
        break;
      }
    }
    //读卡
    if (( ! rfid.PICC_IsNewCardPresent()) || (! rfid.PICC_ReadCardSerial()))
    {
      continue; //回到while
    }
    if (rfid.uid.uidByte[0] != nuidPICC[0] ||
        rfid.uid.uidByte[1] != nuidPICC[1] ||
        rfid.uid.uidByte[2] != nuidPICC[2] ||
        rfid.uid.uidByte[3] != nuidPICC[3] ) {
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }
      nuid = String(nuidPICC[0]) + String(nuidPICC[1]) + String(nuidPICC[2]) + String(nuidPICC[3]);
    }//end if
    rfid.PICC_HaltA();//停止picc
    rfid.PCD_StopCrypto1();
    String read_id = "";
    //从E2PROM中删除用户
    for (int j = 0; j < user_num; j++) {
      read_id = get_string(max_len_id, j * max_len_id);
      Serial.print("del_user");
      if (read_id == nuid) {
        Serial.println("Delete user"); Serial.println(nuid);
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Delete user");
        lcd.setCursor(0, 1); lcd.print(nuid);
        set_string(j * max_len_id, "            ");
      }
    }
  }//end while
  lcd.clear(); lcd.setCursor(0, 0);
  lcd.print("End del state");
}

//控制状态指示灯
void light_color(int r, int g, int b) {
  analogWrite(red, r);
  analogWrite(green, g);
  analogWrite(blue, b);
}
void flash_light(int pin)
{
  light_color(0, 0, 0);
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i <= 255; i++) {
      analogWrite(pin, i);
      delay(3);
    }
    for (int i = 255; i >= 0; i--) {
      analogWrite(pin, i);
      delay(3);
    }
  }
}

//e2prom写和读
void set_string(int s, String str) {
  //s为起始地址，str为要写入的字符串
  for (int i = 0; i < max_len_id; i++)
  {
    EEPROM.write(s + i, str[i]);
  }
  //   EEPROM.commit();//执行保存到EEPROM//报错
}
String get_string(int len, int s) {
  //len为要读取的字符串长度，s为起始地址
  String data = "";
  for (int i = 0; i < len; i++)
  {
    data = data + char(EEPROM.read(s + i)); //+=报错
  }
  return data;
}
