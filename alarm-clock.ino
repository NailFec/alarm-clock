#include <SevSeg.h>
#include "Button.h"
#include "Clock.h"
#include "config.h"

const int COLON_PIN = 13;

Button studyButton(A0);
Button restButton(A1);
Button reviewButton(A2);

Clock clock;
SevSeg sevseg;

enum Mode {
    MODE_DEFAULT,
    MODE_STUDY,
    MODE_REST,
    MODE_REVIEW
};
Mode currentMode = MODE_DEFAULT;
Mode previousMode = MODE_DEFAULT;
long lastStateChange = 0;

unsigned long timerStartTime = 0;
unsigned long totalStudyTimeMs = 0;
unsigned long totalRestTimeMs = 0;

const unsigned long REVIEW_DISPLAY_DURATION = 2000;

void changeMode(Mode newMode) {
    if (currentMode != newMode) {
        previousMode = currentMode;
        currentMode = newMode;
        lastStateChange = millis();
        Serial.print("Changing mode to: ");
        Serial.println(newMode);
    }
}

long millisSinceStateChange() {
    return millis() - lastStateChange;
}

void setColon(bool on) {
    digitalWrite(COLON_PIN, on ? LOW : HIGH);
}

void displayCurrentTime() {
    DateTime now = clock.now();
    int displayValue = now.hour() * 100 + now.minute();
    bool blinkState = (now.second() % 2 == 0);

    sevseg.setNumber(displayValue, -1);
    setColon(blinkState);
}

void displayTimer(unsigned long elapsedMillis) {
    unsigned long totalSeconds = elapsedMillis / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    minutes = constrain(minutes, 0, 99);

    int displayValue = minutes * 100 + seconds;
    bool blinkState = (totalSeconds % 2 == 0);

    sevseg.setNumber(displayValue, 2);
    sevseg.setNumber(displayValue, -1);
    setColon(blinkState);
}

void displayTotalMinutes(unsigned long totalMillis) {
    unsigned long minutes = totalMillis / 60000;
    minutes = constrain(minutes, 0, 9999);
    sevseg.setNumber(minutes, -1);
    setColon(false);
}

void handleDefaultMode() {
    displayCurrentTime();

    if (studyButton.pressed()) {
        timerStartTime = millis();
        changeMode(MODE_STUDY);
        return;
    }

    if (restButton.pressed()) {
        timerStartTime = millis();
        changeMode(MODE_REST);
        return;
    }

    if (reviewButton.pressed()) {
        changeMode(MODE_REVIEW);
        return;
    }
}

void handleStudyMode() {
    unsigned long elapsed = millis() - timerStartTime;
    displayTimer(elapsed);

    if (studyButton.pressed() || restButton.pressed()) {
        unsigned long sessionDuration = millis() - timerStartTime;
        totalStudyTimeMs += sessionDuration;
        Serial.print("Stopped Study. Duration: ");
        Serial.print(sessionDuration / 1000);
        Serial.println("s");
        Serial.print("Total Study Time: ");
        Serial.print(totalStudyTimeMs / 1000);
        Serial.println("s");
        changeMode(MODE_DEFAULT);
        return;
    }

    if (reviewButton.pressed()) {
    }
}

void handleRestMode() {
    unsigned long elapsed = millis() - timerStartTime;
    displayTimer(elapsed);

    if (studyButton.pressed() || restButton.pressed()) {
        unsigned long sessionDuration = millis() - timerStartTime;
        totalRestTimeMs += sessionDuration;
        Serial.print("Stopped Rest. Duration: ");
        Serial.print(sessionDuration / 1000);
        Serial.println("s");
        Serial.print("Total Rest Time: ");
        Serial.print(totalRestTimeMs / 1000);
        Serial.println("s");
        changeMode(MODE_DEFAULT);
        return;
    }

    if (reviewButton.pressed()) {
    }
}

void handleReviewMode() {
    unsigned long timeInState = millisSinceStateChange();

    if (timeInState < REVIEW_DISPLAY_DURATION) {
        if (timeInState < 500) {
            sevseg.setChars("StdY");
            setColon(false);
        } else {
            displayTotalMinutes(totalStudyTimeMs);
        }
    } else if (timeInState < (REVIEW_DISPLAY_DURATION * 2)) {
        if (timeInState < (REVIEW_DISPLAY_DURATION + 500)) {
            sevseg.setChars("rESt");
            setColon(false);
        } else {
            displayTotalMinutes(totalRestTimeMs);
        }
    } else {
        changeMode(MODE_DEFAULT);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Study/Rest Timer Starting...");

    clock.begin();

    studyButton.begin();
    restButton.begin();
    reviewButton.begin();

    pinMode(COLON_PIN, OUTPUT);
    digitalWrite(COLON_PIN, HIGH);

    byte digits = 4;
    byte digitPins[] = {2, 3, 4, 5};
    byte segmentPins[] = {6, 7, 8, 9, 10, 11, 12};
    bool resistorsOnSegments = false;
    bool leadingZeros = true;
    bool disableDecPoint = true;

#ifndef DISPLAY_TYPE
#define DISPLAY_TYPE COMMON_CATHODE
    Serial.println("Warning: DISPLAY_TYPE not defined in config.h, defaulting to COMMON_CATHODE");
#endif

    sevseg.begin(DISPLAY_TYPE, digits, digitPins, segmentPins, resistorsOnSegments,
                 false, leadingZeros, disableDecPoint);
    sevseg.setBrightness(90);

    Serial.println("Setup complete.");
    changeMode(MODE_DEFAULT);
}

void loop() {
    sevseg.refreshDisplay();

    studyButton.read();
    restButton.read();
    reviewButton.read();

    switch (currentMode) {
    case MODE_DEFAULT:
        handleDefaultMode();
        break;
    case MODE_STUDY:
        handleStudyMode();
        break;
    case MODE_REST:
        handleRestMode();
        break;
    case MODE_REVIEW:
        handleReviewMode();
        break;
    }
}