#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40); // 第一个板
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41); // 第二个板

void setup() {
  Serial.begin(9600);
  pwm1.begin();
  pwm2.begin();
  pwm1.setPWMFreq(50);  // 设置舵机频率
  pwm2.setPWMFreq(50);
  Serial.println("双PCA9685 PWM调试器已启动");
  Serial.println("输入格式：板号 通道号 PWM值（例如：1 5 300）");
}

void loop() {
  static String inputString = "";
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      inputString.trim();
      if (inputString.length() > 0) {
        processInput(inputString);
        inputString = "";
      }
    } else {
      inputString += inChar;
    }
  }
}

void processInput(String input) {
  int firstSpace = input.indexOf(' ');
  int secondSpace = input.indexOf(' ', firstSpace + 1);

  if (firstSpace == -1 || secondSpace == -1) {
    Serial.println("格式错误：请输入 板号 通道号 PWM值");
    return;
  }

  int board = input.substring(0, firstSpace).toInt();
  int channel = input.substring(firstSpace + 1, secondSpace).toInt();
  int pwmValue = input.substring(secondSpace + 1).toInt();

  if (board != 1 && board != 2) {
    Serial.println("板号无效：只能是 1 或 2");
    return;
  }
  if (channel < 0 || channel > 15 || pwmValue < 0 || pwmValue > 4095) {
    Serial.println("通道或PWM值无效：通道应为 0~15，PWM为 0~4095");
    return;
  }

  if (board == 1) {
    pwm1.setPWM(channel, 0, pwmValue);
  } else {
    pwm2.setPWM(channel, 0, pwmValue);
  }

  Serial.print("板 ");
  Serial.print(board);
  Serial.print(" 通道 ");
  Serial.print(channel);
  Serial.print(" 设置为 PWM = ");
  Serial.println(pwmValue);
}
