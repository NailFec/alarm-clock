#include <SevSeg.h> // 包含 SevSeg 库，用于显示数字。
#include "Button.h" // 包含 Button 库，用于处理按键输入。
#include "AlarmTone.h" // 包含 AlarmTone 库，用于播放警报音效。
#include "Clock.h" // 包含 Clock 库，用于管理时间、闹钟等功能。
#include "config.h" // 包含配置信息，例如显示类型、按键引脚等。

const int COLON_PIN = 13; // 定义冒号的引脚为 13 号引脚。
const int SPEAKER_PIN = A3; // 定义扬声器的引脚为 A3 号引脚。

Button hourButton(A0); // 创建一个小时按键对象，连接到 A0 引脚。
Button minuteButton(A1); // 创建一个分钟按键对象，连接到 A1 引脚。
Button alarmButton(A2); // 创建一个闹钟按键对象，连接到 A2 引脚。

AlarmTone alarmTone; // 创建 AlarmTone 对象，用于播放警报音效。
Clock clock; // 创建 Clock 对象，用于管理时间、闹钟等功能。
SevSeg sevseg; // 创建 SevSeg 对象，用于显示数字。

enum DisplayState {
  DisplayClock, // 显示当前时间
  DisplayAlarmStatus, // 显示闹钟状态（开启/关闭）
  DisplayAlarmTime, // 显示闹钟时间
  DisplayAlarmActive, // 闹钟正在响
  DisplaySnooze, // 显示睡眠模式
};

DisplayState displayState = DisplayClock; // 初始化显示状态为当前时间显示。
long lastStateChange = 0; // 保存上次状态改变的时间戳，用于计算时间间隔。

// 函数：changeDisplayState
// 功能：更改显示状态并记录下状态改变的时间。
void changeDisplayState(DisplayState newValue) {
  displayState = newValue; // 设置当前显示状态为新的状态值。
  lastStateChange = millis(); // 记录下状态改变的时间戳。
}

// 函数：millisSinceStateChange
// 功能：计算自上次状态改变以来经过的毫秒数。
long millisSinceStateChange() {
  return millis() - lastStateChange; // 返回当前时间与上次状态改变时间的差值，即经过的时间。
}

// 函数：setColon
// 功能：设置冒号引脚的状态（高电平或低电平），用于显示冒号。
void setColon(bool value) {
  digitalWrite(COLON_PIN, value ? LOW : HIGH); // 根据value的值，将COLON_PIN设置为LOW或HIGH。
}

// 函数：displayTime
// 功能：显示当前时间（小时和分钟）。
void displayTime() {
  DateTime now = clock.now(); // 获取当前的 DateTime 对象。
  bool blinkState = now.second() % 2 == 0; // 判断秒针是否闪烁，如果秒数是偶数则闪烁，否则不闪烁。
  sevseg.setNumber(now.hour() * 100 + now.minute()); // 设置 SevSeg 显示的数字为小时乘以100加上分钟。
  setColon(blinkState); // 设置冒号的状态，根据秒针是否闪烁来决定。
}

// 函数：clockState
// 功能：处理时间调整和闹钟控制逻辑。
void clockState() {
  displayTime(); // 显示当前时间。

  if (alarmButton.read() == Button::RELEASED && clock.alarmActive()) {
    // 如果按下闹钟按键且闹钟正在响，则执行以下操作：
    alarmButton.has_changed(); // 清除按键的状态，确保下次读取时状态正确。
    changeDisplayState(DisplayAlarmActive); // 更改显示状态为闹钟正在响状态。
    return; // 返回函数，防止继续执行后续代码。
  }

  if (hourButton.pressed()) {
    clock.incrementHour(); // 如果按下小时按键，则增加小时值。
  }
  if (minuteButton.pressed()) {
    clock.incrementMinute(); // 如果按下分钟按键，则增加分钟值。
  }
  if (alarmButton.pressed()) {
    clock.toggleAlarm(); // 如果按下闹钟按键，则切换闹钟状态（开启/关闭）。
    changeDisplayState(DisplayAlarmStatus); // 更改显示状态为闹钟状态显示。
  }
}

// 函数：alarmStatusState
// 功能：显示闹钟的开启或关闭状态。
void alarmStatusState() {
  setColon(false); // 关闭冒号。
  sevseg.setChars(clock.alarmEnabled() ? " on" : " off"); // 根据闹钟是否开启，设置 SevSeg 显示的字符为 "on" 或 "off"。
  if (millisSinceStateChange() > ALARM_STATUS_DISPLAY_TIME) {
    changeDisplayState(clock.alarmEnabled() ? DisplayAlarmTime : DisplayClock); // 如果自上次状态改变以来经过的时间大于 ALARM_STATUS_DISPLAY_TIME，则更改显示状态为闹钟时间或当前时间。
    return; // 返回函数，防止继续执行后续代码。
  }
}

// 函数：alarmTimeState
// 功能：显示闹钟时间（小时和分钟）。
void alarmTimeState() {
  DateTime alarm = clock.alarmTime(); // 获取闹钟时间。
  sevseg.setNumber(alarm.hour() * 100 + alarm.minute(), -1); // 设置 SevSeg 显示的数字为闹钟小时乘以100加上闹钟分钟。

  if (millisSinceStateChange() > ALARM_HOUR_DISPLAY_TIME || alarmButton.pressed()) {
    changeDisplayState(DisplayClock); // 如果自上次状态改变以来经过的时间大于 ALARM_HOUR_DISPLAY_TIME 或者按下闹钟按键，则更改显示状态为当前时间。
    return; // 返回函数，防止继续执行后续代码。
  }

  if (hourButton.pressed()) {
    clock.incrementAlarmHour(); // 如果按下小时按键，则增加闹钟小时值。
    lastStateChange = millis(); // 更新上次状态改变的时间戳。
  }
  if (minuteButton.pressed()) {
    clock.incrementAlarmMinute(); // 如果按下分钟按键，则增加闹钟分钟值。
    lastStateChange = millis(); // 更新上次状态改变的时间戳。
  }
  if (alarmButton.pressed()) {
    changeDisplayState(DisplayClock); // 如果按下闹钟按键，则更改显示状态为当前时间。
  }
}

// 函数：alarmState
// 功能：处理闹钟响铃和睡眠模式逻辑。
void alarmState() {
  displayTime(); // 显示当前时间。

  if (alarmButton.read() == Button::RELEASED) {
    alarmTone.play(); // 如果按下闹钟按键且松开，则播放警报音效。
  }
  if (alarmButton.pressed()) {
    alarmTone.stop(); // 如果按下闹钟按键，则停止警报音效。
  }
  if (alarmButton.released()) {
    alarmTone.stop(); // 确保警报音效已停止。
    bool longPress = alarmButton.repeat_count() > 0; // 检查是否长时间按下闹钟按键。
    if (longPress) {
      clock.stopAlarm(); // 如果长时间按下，则停止闹钟。
      changeDisplayState(DisplayClock); // 更改显示状态为当前时间。
    } else {
      clock.snooze(); // 否则进入睡眠模式。
      changeDisplayState(DisplaySnooze); // 更改显示状态为睡眠模式。
    }
  }
}

// 函数：snoozeState
// 功能：显示睡眠模式状态。
void snoozeState() {
  sevseg.setChars("****"); // 设置 SevSeg 显示的字符为 "****"。
  if (millisSinceStateChange() > SNOOZE_DISPLAY_TIME) {
    changeDisplayState(DisplayClock); // 如果自上次状态改变以来经过的时间大于 SNOOZE_DISPLAY_TIME，则更改显示状态为当前时间。
    return; // 返回函数，防止继续执行后续代码。
  }
}

// 函数：setup
// 功能：初始化硬件和设置参数。
void setup() {
  Serial.begin(115200); // 初始化串口通信。

  clock.begin(); // 初始化 Clock 对象。

  hourButton.begin(); // 初始化小时按键对象，并设置重复间隔。
  hourButton.set_repeat(500, 200); // 设置小时按键的重复间隔为 500ms (触发) 和 200ms (恢复)。

  minuteButton.begin(); // 初始化分钟按键对象，并设置重复间隔。
  minuteButton.set_repeat(500, 200); // 设置分钟按键的重复间隔为 500ms (触发) 和 200ms (恢复)。

  alarmButton.begin(); // 初始化闹钟按键对象，并设置重复间隔。
  alarmButton.set_repeat(1000, -1); // 设置闹钟按键的重复间隔为 1000ms (触发)，-1 表示不恢复。

  alarmTone.begin(SPEAKER_PIN); // 初始化 AlarmTone 对象，并指定扬声器引脚。

  pinMode(COLON_PIN, OUTPUT); // 将冒号引脚设置为输出模式。

  byte digits = 4; // 设置数字位数
  byte digitPins[] = {2, 3, 4, 5}; // 数字显示管的引脚连接
  byte segmentPins[] = {6, 7, 8, 9, 10, 11, 12}; // 数字显示管的段线连接
  bool resistorsOnSegments = false; // 是否使用段线电阻
  bool updateWithDelays = false; // 是否使用延时更新
  bool leadingZeros = true; // 是否在数字前面添加零
  bool disableDecPoint = true; // 是否禁用小数点显示
  sevseg.begin(DISPLAY_TYPE, digits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint); // 初始化 SevSeg 对象。
  sevseg.setBrightness(90); // 设置显示亮度。
}

// 函数：loop
// 功能：循环执行程序逻辑。
void loop() {
  sevseg.refreshDisplay(); // 刷新数字显示管的显示内容。

  switch (displayState) {
    case DisplayClock: // 如果当前显示状态为当前时间显示，则调用 clockState 函数。
      clockState();
      break;

    case DisplayAlarmStatus: // 如果当前显示状态为闹钟状态显示，则调用 alarmStatusState 函数。
      alarmStatusState();
      break;

    case DisplayAlarmTime: // 如果当前显示状态为闹钟时间显示，则调用 alarmTimeState 函数。
      alarmTimeState();
      break;

    case DisplayAlarmActive: // 如果当前显示状态为闹钟正在响显示，则调用 alarmState 函数。
      alarmState();
      break;

    case DisplaySnooze: // 如果当前显示状态为睡眠模式显示，则调用 snoozeState 函数。
      snoozeState();
      break;
  }
}
