#include <SevSeg.h>
#include "Button.h" // Assuming this library handles debouncing and press detection
#include "Clock.h"  // Assuming this library provides time via clock.now()
#include "config.h" // Assuming this contains DISPLAY_TYPE and potentially other SevSeg configs

// --- Pin Definitions (Keep consistency with original if hardware is unchanged) ---
const int COLON_PIN = 13;
// const int SPEAKER_PIN = A3; // No longer needed for speaker

// --- Button Definitions ---
// Button 1: Study Start/Stop
Button studyButton(A0);
// Button 2: Rest Start/Stop
Button restButton(A1);
// Button 3: Review Totals
Button reviewButton(A2);

// --- Clock and Display Objects ---
Clock clock;      // For getting current time in DEFAULT mode
SevSeg sevseg;    // For controlling the 7-segment display

// --- State Machine Definition ---
enum Mode {
  MODE_DEFAULT, // Displaying current time
  MODE_STUDY,   // Timing a study session
  MODE_REST,    // Timing a rest session
  MODE_REVIEW   // Displaying total study/rest times
};
Mode currentMode = MODE_DEFAULT;
Mode previousMode = MODE_DEFAULT; // To know which timer to add to
long lastStateChange = 0;         // For timing in REVIEW mode

// --- Timer Variables ---
unsigned long timerStartTime = 0;      // Stores millis() when a timer starts
unsigned long totalStudyTimeMs = 0;    // Accumulates total study time in milliseconds
unsigned long totalRestTimeMs = 0;     // Accumulates total rest time in milliseconds

// --- Constants ---
const unsigned long REVIEW_DISPLAY_DURATION = 2000; // Display each total for 2 seconds

// === Helper Functions ===

// Function to change state and record time
void changeMode(Mode newMode) {
  if (currentMode != newMode) { // Prevent resetting timer if mode doesn't actually change
     previousMode = currentMode; // Store where we came from before changing
     currentMode = newMode;
     lastStateChange = millis();
     Serial.print("Changing mode to: "); Serial.println(newMode); // Debugging
  }
}

// Get milliseconds since last state change
long millisSinceStateChange() {
  return millis() - lastStateChange;
}

// Control the colon LED
void setColon(bool on) {
  // Assuming LOW turns the LED ON for your setup, adjust if needed
  digitalWrite(COLON_PIN, on ? LOW : HIGH);
}

// Display HH:MM from Clock
void displayCurrentTime() {
  DateTime now = clock.now();
  int displayValue = now.hour() * 100 + now.minute();
  bool blinkState = (now.second() % 2 == 0); // Blink colon every second

  sevseg.setNumber(displayValue, -1); // Display HHMM, -1 means no decimal point forced
  setColon(blinkState);
}

// Display MM:SS from elapsed milliseconds
void displayTimer(unsigned long elapsedMillis) {
  unsigned long totalSeconds = elapsedMillis / 1000;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  // Prevent display overflow if timer runs > 99 minutes 59 seconds
  minutes = constrain(minutes, 0, 99);

  int displayValue = minutes * 100 + seconds;
  bool blinkState = (totalSeconds % 2 == 0); // Blink colon while timer runs

  sevseg.setNumber(displayValue, 2); // Display MMSS, force decimal/colon at position 2 (between MM and SS) - SevSeg handles colon display if setup correctly, otherwise use setColon. Let's use setColon.
  sevseg.setNumber(displayValue, -1); // Display MMSS
  setColon(blinkState); // Manually blink colon
}

// Display total time (in minutes)
void displayTotalMinutes(unsigned long totalMillis) {
    unsigned long minutes = totalMillis / 60000; // Convert ms to minutes
    minutes = constrain(minutes, 0, 9999); // Limit to 4 digits
    sevseg.setNumber(minutes, -1); // Display total minutes
    setColon(false); // Colon off during review
}

// === State Handling Functions ===

// Handle Default Mode: Show time, check for button presses to start timers or review
void handleDefaultMode() {
  displayCurrentTime();

  if (studyButton.pressed()) {
    timerStartTime = millis(); // Reset and start timer
    changeMode(MODE_STUDY);
    return; // Exit after mode change
  }

  if (restButton.pressed()) {
    timerStartTime = millis(); // Reset and start timer
    changeMode(MODE_REST);
    return; // Exit after mode change
  }

  if (reviewButton.pressed()) {
    changeMode(MODE_REVIEW);
    return; // Exit after mode change
  }
}

// Handle Study Mode: Show timer, check for button press to stop
void handleStudyMode() {
  unsigned long elapsed = millis() - timerStartTime;
  displayTimer(elapsed);

  // Buttons 1 or 2 stop the timer
  if (studyButton.pressed() || restButton.pressed()) {
    unsigned long sessionDuration = millis() - timerStartTime;
    totalStudyTimeMs += sessionDuration;
    Serial.print("Stopped Study. Duration: "); Serial.print(sessionDuration / 1000); Serial.println("s");
    Serial.print("Total Study Time: "); Serial.print(totalStudyTimeMs / 1000); Serial.println("s");
    changeMode(MODE_DEFAULT);
    return;
  }
   // Button 3 (Review) does nothing in this mode
   if (reviewButton.pressed()) {
     // Optional: Add a small visual cue or sound that it's ignored?
   }
}

// Handle Rest Mode: Show timer, check for button press to stop
void handleRestMode() {
  unsigned long elapsed = millis() - timerStartTime;
  displayTimer(elapsed);

  // Buttons 1 or 2 stop the timer
  if (studyButton.pressed() || restButton.pressed()) {
    unsigned long sessionDuration = millis() - timerStartTime;
    totalRestTimeMs += sessionDuration;
    Serial.print("Stopped Rest. Duration: "); Serial.print(sessionDuration / 1000); Serial.println("s");
    Serial.print("Total Rest Time: "); Serial.print(totalRestTimeMs / 1000); Serial.println("s");
    changeMode(MODE_DEFAULT);
    return;
  }
   // Button 3 (Review) does nothing in this mode
   if (reviewButton.pressed()) {
     // Optional: Add a small visual cue or sound that it's ignored?
   }
}

// Handle Review Mode: Show totals sequentially then return to default
void handleReviewMode() {
  unsigned long timeInState = millisSinceStateChange();

  if (timeInState < REVIEW_DISPLAY_DURATION) {
    // Phase 1: Show "StdY" (or similar) then total study minutes
    if (timeInState < 500) { // Show label briefly
        sevseg.setChars("StdY");
        setColon(false);
    } else {
        displayTotalMinutes(totalStudyTimeMs);
    }
  } else if (timeInState < (REVIEW_DISPLAY_DURATION * 2)) {
    // Phase 2: Show "rESt" (or similar) then total rest minutes
     if (timeInState < (REVIEW_DISPLAY_DURATION + 500)) { // Show label briefly
        sevseg.setChars("rESt");
        setColon(false);
    } else {
        displayTotalMinutes(totalRestTimeMs);
    }
  } else {
    // Phase 3: Return to default mode
    changeMode(MODE_DEFAULT);
  }
}


// === Arduino Standard Functions ===

void setup() {
  Serial.begin(115200);
  Serial.println("Study/Rest Timer Starting...");

  // Initialize Clock (essential for default mode)
  clock.begin();

  // Initialize Buttons
  studyButton.begin();
  restButton.begin();
  reviewButton.begin();
  // Remove or adjust repeat/long press if not needed for new logic
  // studyButton.set_repeat(500, 200);
  // restButton.set_repeat(500, 200);
  // reviewButton.set_repeat(1000, -1); // Maybe keep long press for a reset function later? For now, disable.

  // Initialize Colon Pin
  pinMode(COLON_PIN, OUTPUT);
  digitalWrite(COLON_PIN, HIGH); // Start with colon off

  // Initialize SevSeg Display (Use settings from original code/config.h)
  byte digits = 4;
  // --- Ensure these match your hardware ---
  byte digitPins[] = {2, 3, 4, 5};
  byte segmentPins[] = {6, 7, 8, 9, 10, 11, 12};
  bool resistorsOnSegments = false; // Change if your setup differs
  //bool updateWithDelays = false; // Not recommended with frequent refreshes
  bool leadingZeros = true;      // Keep HH:MM and MM:SS looking right
  bool disableDecPoint = true;   // We use the colon pin instead
  // --- Get DISPLAY_TYPE from config.h or define directly ---
  // Example: byte displayType = COMMON_CATHODE;
  // Ensure DISPLAY_TYPE is defined correctly in config.h or replace it here.
  #ifndef DISPLAY_TYPE
    #define DISPLAY_TYPE COMMON_CATHODE // Provide a default if not in config.h
    Serial.println("Warning: DISPLAY_TYPE not defined in config.h, defaulting to COMMON_CATHODE");
  #endif

  sevseg.begin(DISPLAY_TYPE, digits, digitPins, segmentPins, resistorsOnSegments,
               false /*updateWithDelays*/, leadingZeros, disableDecPoint);
  sevseg.setBrightness(90); // Adjust brightness as needed

  Serial.println("Setup complete.");
  changeMode(MODE_DEFAULT); // Start in default mode explicitly
}

void loop() {
  // IMPORTANT: Refresh display very frequently for multiplexing
  sevseg.refreshDisplay();

  // Update button states (assuming Button library requires this)
  // If your Button library handles this internally, you might not need these calls.
  // Check the Button library documentation. Let's assume they are needed.
  studyButton.read();
  restButton.read();
  reviewButton.read();


  // Process current mode logic
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