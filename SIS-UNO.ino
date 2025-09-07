// Code for Arduino UNO to manage a rocket launch countdown with buzzer and firework fuse trigger

const int buzzerPin = 13;
const int triggerPin = 7; // Signal pin for external trigger (button/IoT module)
const int fusePin = 9;    // PWM pin for firework fuse trigger

void setup() {
  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);
  pinMode(triggerPin, INPUT_PULLUP); // Internal pull-up (LOW = triggered)
  pinMode(fusePin, OUTPUT);          // PWM output for fuse trigger
  
  digitalWrite(fusePin, LOW);        // Ensure fuse pin is off initially

  Serial.println("========================================");
  Serial.println("         LAUNCH COUNTDOWN SYSTEM");
  Serial.println("========================================");
  Serial.println();
  Serial.println("System Ready. Waiting for launch signal on pin 7...");
  Serial.println("Fuse trigger on pin 9 - READY");
}

void loop() {
  // Wait until signal received
  if (digitalRead(triggerPin) == LOW) {
    Serial.println("Launch signal received!");
    delay(200); // debounce
    rocketCountdown();

    Serial.println("System on standby mode.");
    Serial.println("Waiting for next launch signal on pin 7...");
    Serial.println();
    delay(500); // small pause before rearming
  }
}

void rocketCountdown() {
  Serial.println("INITIATING LAUNCH SEQUENCE...");
  Serial.println();
  delay(1000);

  // Phase 1: Normal countdown (10 to 6)
  Serial.println("=== PHASE 1: INITIAL COUNTDOWN ===");
  for (int i = 10; i >= 6; i--) {
    Serial.print("T-");
    Serial.println(i);

    tone(buzzerPin, 800, 500); // 800Hz for 500ms
    delay(1000);
  }

  Serial.println();

  // Phase 2: Accelerating countdown (5 to 1)
  Serial.println("=== PHASE 2: ACCELERATION PHASE ===");
  for (int i = 5; i >= 1; i--) {
    Serial.print("T-");
    Serial.println(i);

    tone(buzzerPin, 1200, 300); // 1200Hz for 300ms
    delay(600);
  }

  Serial.println();

  // Phase 3: Final rapid beeps
  Serial.println("=== PHASE 3: FINAL SEQUENCE ===");
  for (int i = 0; i < 5; i++) {
    Serial.print("⚠️   ");
    tone(buzzerPin, 1500, 150);
    delay(200);
  }

  Serial.println();
  Serial.println("LAUNCH!🚀");
  Serial.println("Deployment Successful!");

  // FIREWORK LAUNCH SEQUENCE
  // Activates both buzzer and fuse trigger simultaneously for 3 seconds
  Serial.println("*** ACTIVATING FUSE TRIGGER ***");
  
  tone(buzzerPin, 2000);        // Start continuous 2000Hz tone
  analogWrite(fusePin, 255);    // Full PWM signal (100% duty cycle) to fuse

  for (int i = 1; i <= 3; i++) {
    delay(1000);
    Serial.print("Launch +");
    Serial.print(i);
    Serial.println(" seconds - FUSE ACTIVE");
  }
  
  noTone(buzzerPin);            // Stop buzzer
  analogWrite(fusePin, 0);      // Stop PWM signal to fuse
  Serial.println("*** FUSE TRIGGER DEACTIVATED ***");

  Serial.println();
  Serial.println("=== LAUNCH SEQUENCE COMPLETED ===");
  Serial.println("System Status: Normal");
  Serial.println("launch successful!");
  Serial.println("========================================");
  Serial.println();
}