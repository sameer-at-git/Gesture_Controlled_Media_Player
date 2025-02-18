// --- Pin Definitions ---
const int TRIG_PIN_1 = 2, ECHO_PIN_1 = 3;
const int TRIG_PIN_2 = 4, ECHO_PIN_2 = 5;
const int TRIG_PIN_3 = 6, ECHO_PIN_3 = 7;

// --- Distance Threshold ---
const float GESTURE_THRESHOLD = 20.0;  // in centimeters

// --- Time Thresholds (milliseconds) ---
const unsigned long QUICK_SWIPE_MAX   = 1000;
const unsigned long HOLD_MIN_ACTIVATE   = 1500;  // 1.5 sec for activation
const unsigned long HOLD_MIN_DEACTIVATE = 3000;  // 3 sec for deactivation
const unsigned long ACTION_COOLDOWN     = 500;
const unsigned long MULTI_SWIPE_WINDOW  = 700;   // Time window for multi-sensor gestures

// --- Crop Modes ---
const int CROP_MODES = 3;
int currentCropMode = 0;

// --- States ---
bool isSlowMode = false;
bool isMuted = false;

// --- Tracking Variables ---
unsigned long s1StartTime = 0, s2StartTime = 0, s3StartTime = 0;
unsigned long s2DeactivationStartTime = 0;
unsigned long swipeStartTime = 0;
bool s1WasActive = false, s2WasActive = false, s3WasActive = false;
bool gestureLock = false;  // Prevents gestures during an ongoing action

// --- Multi-Sensor Mode Variable ---
bool multiSensorMode = false;

// --- Additional Variable for Swipe Timing (for contrast gestures) ---
static int swipeProgress = 0;         // 0 = no swipe, 1 = positive initiated, 2 = waiting for S3,
                                      // -1 = negative initiated, -2 = waiting for S1.
unsigned long swipeSequenceStart = 0;   // To reset sequence if gesture takes too long

// --- Function to Get Distance ---
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

// --- Perform Action ---
void performAction(const String& action) {
  Serial.println(action);
  gestureLock = true;  // Lock gestures temporarily
  delay(ACTION_COOLDOWN);
  gestureLock = false; // Unlock after cooldown
}

// --- Cycle Crop Mode ---
void cycleCropMode() {
  currentCropMode = (currentCropMode + 1) % CROP_MODES;
  performAction(currentCropMode == 0 ? "16:10 Crop" :
                (currentCropMode == 1 ? "16:9 Crop" : "4:3 Crop"));
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN_1, OUTPUT); pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT); pinMode(ECHO_PIN_2, INPUT);
  pinMode(TRIG_PIN_3, OUTPUT); pinMode(ECHO_PIN_3, INPUT);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensor distances
  float distance1 = getDistance(TRIG_PIN_1, ECHO_PIN_1);
  float distance2 = getDistance(TRIG_PIN_2, ECHO_PIN_2);
  float distance3 = getDistance(TRIG_PIN_3, ECHO_PIN_3);
  
  bool s1Active = (distance1 < GESTURE_THRESHOLD);
  bool s2Active = (distance2 < GESTURE_THRESHOLD);
  bool s3Active = (distance3 < GESTURE_THRESHOLD);

  // --- Multi-Sensor Mode Activation/Deactivation ---
  // When multiSensorMode is OFF, check for activation.
  if (!multiSensorMode) {
    if (s2Active && !s2WasActive) {
      s2StartTime = currentTime;
    }
    if (s2Active && (currentTime - s2StartTime >= HOLD_MIN_ACTIVATE)) {
      multiSensorMode = true;
      Serial.println("Multi-Sensor Mode Activated");
      gestureLock = true;   // Lock all sensors during cooldown
      delay(ACTION_COOLDOWN);
      gestureLock = false;
    }
  }
  // When multiSensorMode is ON, check for deactivation.
  else {
    if (s2Active && !s2WasActive) {
      s2DeactivationStartTime = currentTime;
    }
    if (s2Active && (currentTime - s2DeactivationStartTime >= HOLD_MIN_DEACTIVATE)) {
      multiSensorMode = false;
      Serial.println("Multi-Sensor Mode Deactivated");
      gestureLock = true;   // Lock all sensors during cooldown
      delay(ACTION_COOLDOWN);
      gestureLock = false;
    }
  }
  
  // --- Multi-Sensor Gestures (processed when no ongoing action) ---
  if (!gestureLock) {
    // 1. Crop Change Gesture: S1 & S3 simultaneously
    if (s1Active && s3Active) {
      cycleCropMode();
      gestureLock = true;
      delay(ACTION_COOLDOWN);
      gestureLock = false;
    }
  
    // 2. Swipes Across Sensors using sensor-2 as a reference
    if (s2Active && !s2WasActive) {
      swipeStartTime = currentTime;
    }
    if (!s2Active && s2WasActive) {
      unsigned long duration = currentTime - swipeStartTime;
      if (duration < MULTI_SWIPE_WINDOW) {
        if (s1Active && !s3Active) {
          performAction("Previous Video");
          return;
        }
        if (s3Active && !s1Active) {
          performAction("Next Video");
          return;
        }
      }
    }
    
    // 3. Contrast Adjustments via swipe sequence
    // Swipe order S1 -> S2 -> S3 increases contrast.
    // Swipe order S3 -> S2 -> S1 decreases contrast.
    // Reset the swipe sequence if it takes too long.
    if (swipeProgress != 0 && currentTime - swipeSequenceStart > MULTI_SWIPE_WINDOW) {
      swipeProgress = 0;
    }
    
    // Start a new sequence only if no swipe sequence is in progress.
    if (swipeProgress == 0) {
      if (s1Active && !s1WasActive) {
        swipeProgress = 1;  // positive sequence initiated
        swipeSequenceStart = currentTime;
      } else if (s3Active && !s3WasActive) {
        swipeProgress = -1; // negative sequence initiated
        swipeSequenceStart = currentTime;
      }
    }
    
    // Process positive sequence: expecting S1 then S2 then S3.
    if (swipeProgress == 1) {
      if (s2Active) {
        swipeProgress = 2;
      }
    }
    if (swipeProgress == 2 && s3Active) {
      performAction("Increase Contrast");
      swipeProgress = 0;
      return;
    }
    
    // Process negative sequence: expecting S3 then S2 then S1.
    if (swipeProgress == -1) {
      if (s2Active) {
        swipeProgress = -2;
      }
    }
    if (swipeProgress == -2 && s1Active) {
      performAction("Decrease Contrast");
      swipeProgress = 0;
      return;
    }
  }
  
  // --- Single Sensor Gestures (only processed if multiSensorMode is OFF) ---
  if (!gestureLock && !multiSensorMode) {
    // Sensor 1 gestures
    if (s1Active && !s1WasActive) {
      s1StartTime = currentTime;
    }
    if (!s1Active && s1WasActive) {
      unsigned long duration = currentTime - s1StartTime;
      if (duration < QUICK_SWIPE_MAX) {
        performAction("Rotate Left");
      } else if (duration >= HOLD_MIN_ACTIVATE) {  // using HOLD_MIN_ACTIVATE for single sensor hold
        isSlowMode = !isSlowMode;
        performAction(isSlowMode ? "Slow Speed" : "Normal Speed");
      }
    }
    
    // Sensor 3 gestures
    if (s3Active && !s3WasActive) {
      s3StartTime = currentTime;
    }
    if (!s3Active && s3WasActive) {
      unsigned long duration = currentTime - s3StartTime;
      if (duration < QUICK_SWIPE_MAX) {
        performAction("Rotate Right");
      } else if (duration >= HOLD_MIN_ACTIVATE) {
        isMuted = !isMuted;
        performAction(isMuted ? "Mute" : "Unmute");
      }
    }
    
    // Sensor 2 quick swipe gesture for snapshot
    if (s2Active && !s2WasActive) {
      s2StartTime = currentTime;
    }
    if (!s2Active && s2WasActive) {
      unsigned long duration = currentTime - s2StartTime;
      if (duration < QUICK_SWIPE_MAX) {
        performAction("Take Snapshot");
      }
      // The hold gesture on sensor-2 is now reserved for mode activation/deactivation.
    }
  }
  
  // --- Store Previous States ---
  s1WasActive = s1Active;
  s2WasActive = s2Active;
  s3WasActive = s3Active;
  
  delay(50);
}
