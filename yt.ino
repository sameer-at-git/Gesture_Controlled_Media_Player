// Define pins for ultrasonic sensors
const int TRIG_PIN_1 = 2;
const int ECHO_PIN_1 = 3;
const int TRIG_PIN_2 = 4;
const int ECHO_PIN_2 = 5;

// Extended states for gesture detection
enum State {
  IDLE,
  SINGLE_SENSOR_HOLD,
  BOTH_ACTIVE,      // Both hands in gesture zone (used for reset or next/prev)
  SEQUENCE_COMPLETE,
  ROTATION_WAIT_FOR_SENSOR1_FAR,
  ROTATION_WAIT_FOR_SENSOR1_NEAR
};

// Timing constants (all in milliseconds)
const unsigned long SINGLE_HOLD_TIME    = 1500;   // 1.5 sec hold for speed changes
const unsigned long RESET_HOLD_TIME     = 3000;   // 3 sec for reset gesture
const unsigned long SEQUENCE_GAP        = 2000;   // 2 sec hold for next/prev gesture
const unsigned long ROTATION_WINDOW     = 1000;   // Time window for the second part of rotation
const unsigned long RESET_COOLDOWN      = 1000;   // Cooldown after reset
const unsigned long SPEED_COOLDOWN      = 500;    // Cooldown after speed change
const unsigned long SEQUENCE_COOLDOWN   = 500;    // Cooldown after next/prev
const unsigned long ROTATION_COOLDOWN   = 800;    // Cooldown after rotation

// Global state variables
State currentState = IDLE;
unsigned long stateStartTime = 0;
unsigned long cooldownTime   = 0;

// For single-hand gestures
bool sensor1WasActive = false;
bool sensor2WasActive = false;

// For the rotation gesture from sensor2
bool rotationCandidate = false;
float sensor2InitialDistance = 0.0;

// For dynamic brightness/contrast control
// (For sensor2: brightness; for sensor1: contrast)
int brightness = 50;    // brightness level (adjust as needed)
int contrast   = 50;    // contrast level (adjust as needed)

// To track previous distance for dynamic adjustments; a negative value means "not set yet"
float lastSensor1Distance = -1.0;
float lastSensor2Distance = -1.0;
// Flags to indicate that a dynamic adjustment has been made (to cancel the speed action)
bool dynamicSensor1 = false;
bool dynamicSensor2 = false;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
}

// Returns distance (in cm) from an ultrasonic sensor
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); // timeout after 30ms to avoid hang
  return duration * 0.034 / 2;
}

// Reset state and dynamic variables back to IDLE.
void resetToIdle() {
  currentState = IDLE;
  stateStartTime = millis();
  sensor1WasActive = false;
  sensor2WasActive = false;
  rotationCandidate = false;
  sensor2InitialDistance = 0.0;
  
  lastSensor1Distance = -1.0;
  lastSensor2Distance = -1.0;
  dynamicSensor1 = false;
  dynamicSensor2 = false;
}

void loop() {
  unsigned long currentTime = millis();
  
  // Get current distances from both sensors
  float distance1 = getDistance(TRIG_PIN_1, ECHO_PIN_1);
  float distance2 = getDistance(TRIG_PIN_2, ECHO_PIN_2);
  
  // --- Zone Thresholds ---
  // Gesture zone: hand closer than 20 cm
  // Dynamic zone: hand at or beyond 20 cm (for brightness/contrast adjustments)
  const float ZONE_THRESHOLD = 20.0;  // 20 cm boundary
  
  // "Active" threshold for any function (we use 30 cm as before)
  const float ACTIVE_THRESHOLD = 30.0;  

  // For rotation/dynamic gestures thresholds:
  const float NEAR_THRESHOLD = 10.0;  
  const float FAR_THRESHOLD  = 30.0;
  
  // Determine if each sensor sees an object within the "active" zone.
  bool newSensor1Active = (distance1 < ACTIVE_THRESHOLD);
  bool newSensor2Active = (distance2 < ACTIVE_THRESHOLD);
  
  // Check overall cooldown period
  if (currentTime - cooldownTime < SPEED_COOLDOWN) {
    delay(50);
    return;
  }
  
  // --- State Machine ---
  switch (currentState) {
    case IDLE:
      // If both sensors are active and in the gesture zone (<20 cm), go to BOTH_ACTIVE state.
      if (newSensor1Active && newSensor2Active && (distance1 < ZONE_THRESHOLD) && (distance2 < ZONE_THRESHOLD)) {
        currentState = BOTH_ACTIVE;
        stateStartTime = currentTime;
      }
      // Otherwise, if only one sensor is active, go to SINGLE_SENSOR_HOLD.
      else if (newSensor1Active && !newSensor2Active) {
        currentState = SINGLE_SENSOR_HOLD;
        stateStartTime = currentTime;
        sensor1WasActive = true;
        sensor2WasActive = false;
        // If in dynamic zone, set baseline for contrast adjustments.
        if(distance1 >= ZONE_THRESHOLD) {
          lastSensor1Distance = distance1;
          dynamicSensor1 = false;
        }
      }
      else if (newSensor2Active && !newSensor1Active) {
        currentState = SINGLE_SENSOR_HOLD;
        stateStartTime = currentTime;
        sensor2WasActive = true;
        sensor1WasActive = false;
        if(distance2 >= ZONE_THRESHOLD) {
          lastSensor2Distance = distance2;
          dynamicSensor2 = false;
        }
        // For sensor2, mark as rotation candidate if starting very near (in gesture zone)
        if (distance2 < NEAR_THRESHOLD) {
          rotationCandidate = true;
          sensor2InitialDistance = distance2;
        } else {
          rotationCandidate = false;
        }
      }
      break;
      
    case SINGLE_SENSOR_HOLD:
      // SINGLE_SENSOR_HOLD is used for single-hand actions:
      // - In the dynamic zone (>=20 cm): adjust brightness (sensor2) or contrast (sensor1)
      // - In the gesture zone (<20 cm): a hold for speed change or (if released early) rotation.
      
      // For sensor1 (contrast control)
      if (sensor1WasActive && newSensor1Active) {
        if (distance1 >= ZONE_THRESHOLD) {  // dynamic region: adjust contrast only
          if (lastSensor1Distance < 0) {
            lastSensor1Distance = distance1;
          }
          float diff = distance1 - lastSensor1Distance;
          if(diff >= 4) {
            contrast++;
            Serial.print("Contrast increased to: ");
            Serial.println(contrast);
            lastSensor1Distance = distance1;
            dynamicSensor1 = true;
          }
          else if(diff <= -4) {
            contrast--;
            Serial.print("Contrast decreased to: ");
            Serial.println(contrast);
            lastSensor1Distance = distance1;
            dynamicSensor1 = true;
          }
        } 
        else {  // gesture region: if held long enough, trigger speed action
          if (currentTime - stateStartTime >= SINGLE_HOLD_TIME) {
            if (!dynamicSensor1) {  // only trigger if no dynamic adjustments occurred
              Serial.println("Slower Speed");
              cooldownTime = currentTime;
              resetToIdle();
            }
          }
        }
      }
      
      // For sensor2 (brightness control)
      if (sensor2WasActive && newSensor2Active) {
        if (distance2 >= ZONE_THRESHOLD) {  // dynamic region: adjust brightness only
          if (lastSensor2Distance < 0) {
            lastSensor2Distance = distance2;
          }
          float diff = distance2 - lastSensor2Distance;
          if(diff >= 4) {
            brightness++;
            Serial.print("Brightness increased to: ");
            Serial.println(brightness);
            lastSensor2Distance = distance2;
            dynamicSensor2 = true;
          }
          else if(diff <= -4) {
            brightness--;
            Serial.print("Brightness decreased to: ");
            Serial.println(brightness);
            lastSensor2Distance = distance2;
            dynamicSensor2 = true;
          }
        } 
        else {  // gesture region: if held long enough, trigger speed action
          if (currentTime - stateStartTime >= SINGLE_HOLD_TIME) {
            if (!dynamicSensor2) {  // only trigger if no dynamic adjustments occurred
              Serial.println("Faster Speed");
              cooldownTime = currentTime;
              resetToIdle();
            }
          }
        }
      }
      
      // If the sensor is released early, then for gestures (in the gesture zone) try rotation.
      if (sensor1WasActive && !newSensor1Active) {
        if(distance1 < ZONE_THRESHOLD) {
          // For sensor1 (left hand) no special decision here—simply reset.
          resetToIdle();
        } else {
          resetToIdle();
        }
      }
      else if (sensor2WasActive && !newSensor2Active) {
        if(distance2 < ZONE_THRESHOLD) {
          // For sensor2, decide between rotation and simply resetting.
          if(rotationCandidate && (distance2 >= FAR_THRESHOLD)) {
            currentState = ROTATION_WAIT_FOR_SENSOR1_FAR;
            stateStartTime = currentTime;
          } else {
            resetToIdle();
          }
        } else {
          resetToIdle();
        }
      }
      
      // Cancel if the other sensor becomes active unexpectedly.
      if ((sensor1WasActive && newSensor2Active) || (sensor2WasActive && newSensor1Active)) {
        resetToIdle();
      }
      break;
      
    case BOTH_ACTIVE:
      // BOTH_ACTIVE state is entered when both sensors are active in the gesture zone (<20 cm).
      // Here the gesture works as follows:
      //   - If both hands remain active for 3 seconds, trigger "Reset All Effects".
      //   - If both are held for at least 2 seconds and then one hand is released, trigger next/prev:
      //       * If sensor2 (right hand) remains, trigger "Next Video".
      //       * If sensor1 (left hand) remains, trigger "Previous Video".
      
      if (newSensor1Active && newSensor2Active) {
        // Both still active.
        if (currentTime - stateStartTime >= RESET_HOLD_TIME) {
          Serial.println("Action: Reset All Effects");
          cooldownTime = currentTime + RESET_COOLDOWN;
          resetToIdle();
        }
      } 
      else {
        // One sensor has been released.
        if (currentTime - stateStartTime >= SEQUENCE_GAP) {
          if (newSensor2Active) {  // right hand remains → Next Video
            Serial.println("Next Video");
          } 
          else if (newSensor1Active) {  // left hand remains → Previous Video
            Serial.println("Previous Video");
          }
          cooldownTime = currentTime;
          currentState = SEQUENCE_COMPLETE;
          stateStartTime = currentTime;
        } 
        else {
          // Released too early; cancel the gesture.
          resetToIdle();
        }
      }
      break;
      
    case SEQUENCE_COMPLETE:
      // Brief pause after the sequence gesture.
      if (currentTime - stateStartTime <= ROTATION_WINDOW) {
        if (!newSensor1Active && !newSensor2Active) {
          cooldownTime = currentTime + SEQUENCE_COOLDOWN;
          resetToIdle();
        }
      } else {
        resetToIdle();
      }
      break;
      
    case ROTATION_WAIT_FOR_SENSOR1_FAR:
      // For rotation: after a sensor2 initiation in gesture zone,
      // wait for sensor1 to see the hand in the far region.
      if (currentTime - stateStartTime <= ROTATION_WINDOW) {
        if (distance1 >= FAR_THRESHOLD) {
          currentState = ROTATION_WAIT_FOR_SENSOR1_NEAR;
          stateStartTime = currentTime;
        }
      } else {
        resetToIdle();
      }
      break;
      
    case ROTATION_WAIT_FOR_SENSOR1_NEAR:
      // Now wait for sensor1 to detect the hand coming "near" (<NEAR_THRESHOLD)
      if (currentTime - stateStartTime <= ROTATION_WINDOW) {
        if (distance1 < NEAR_THRESHOLD) {
          Serial.println("Action: Rotate");
          cooldownTime = currentTime + ROTATION_COOLDOWN;
          resetToIdle();
        }
      } else {
        resetToIdle();
      }
      break;
  } // end switch
  
  delay(50);
}
