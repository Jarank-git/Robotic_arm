#include <Arduino.h>
#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
int joint0[100];
int joint1[100];
int joint2[100];
int joint3[100];
int mode(0);
int stepCount(0);
int currentStep(0);
int sensVal[4];
float ist[4];
unsigned long previous_time(0);
unsigned long lastRecord(0);
int lastMode(-1);
const int stepDuration = 800; // ms per step transition during playback

// ease-in-out curve slow start, fast middle, slow end for natural motion, using a fcn (cos) to model the behaviour
float easeInOut(float t) {
    return -(cos(PI * t) - 1.0f) / 2.0f;
}

//fcn to smoothly move servos from one recorded position to the next over duration_ms
void smoothMove(int f0, int t0, int f1, int t1, int f2, int t2, int f3, int t3, int duration_ms) {
    const int steps = 50;
    int stepDelay = duration_ms / steps;
    for (int i = 0; i <= steps; i++) {
        //check for mode change midmove so user can interrupt
        if (Serial.available() > 0) break;
        float ease = easeInOut((float)i / steps);
        servo1.writeMicroseconds(f0 + ease * (t0 - f0));
        servo2.writeMicroseconds(f1 + ease * (t1 - f1));
        servo3.writeMicroseconds(f2 + ease * (t2 - f2));
        servo4.writeMicroseconds(f3 + ease * (t3 - f3));
        delay(stepDelay);
    }
}

//function to average the analogRead from each potentiometer to reduce noise, thus reducing jittering
unsigned int averageFcn(int pin) {
    unsigned int count(0);
    for(unsigned int i(0); i < 30; i++){
        count += analogRead(pin);
        delayMicroseconds(100);
    }
    count /= 30;
    return count;
}

void readPot() {
    sensVal[0] = averageFcn(A7);
    sensVal[1] = averageFcn(A6);
    sensVal[2] = averageFcn(A5);
    sensVal[3] = averageFcn(A1);
}


//mapping the manual potentiometer values, 0 - 650 calibrated for my setup, to 600 - 2400 for microsecond movements
void mapping() {
    ist[0] = map(sensVal[0], 0, 650, 600, 2400);
    ist[1] = map(sensVal[1], 0, 650, 600, 2400);
    ist[2] = map(sensVal[2], 0, 650, 600, 2400);
    ist[3] = map(sensVal[3], 0, 650, 600, 2400);
}

//function to merge all servo movements into 1 fcn
void move_servo() {
    servo1.writeMicroseconds(ist[0]);
    servo2.writeMicroseconds(ist[1]);
    servo3.writeMicroseconds(ist[2]);
    servo4.writeMicroseconds(ist[3]);
}

// fcn that handles recording each servo motor's movement to the respective array
void recordStep() {
    if (stepCount < 100) {
        // use the ist[] values already updated by the loop — no need to re-read pots
        joint0[stepCount] = ist[0];
        joint1[stepCount] = ist[1];
        joint2[stepCount] = ist[2];
        joint3[stepCount] = ist[3];
        stepCount++;

        Serial.print("Step ");
        Serial.print(stepCount);
        Serial.println(" recorded. Send 'r' for next step, '2' to play back.");
    } else {
        Serial.println("Memory full (100 steps). Send '2' to play back.");
    }
}

// fcn to handle the playback of the values stored in the array from the recordStep fcn
void playBack() {
    if (stepCount == 0) {
        Serial.println("No steps recorded. Record some first (send '1', then 'r').");
        mode = 0;
        return;
    }

    //on first step, snap directly to starting position then begin smooth transitions
    if (currentStep == 0) {
        servo1.writeMicroseconds(joint0[0]);
        servo2.writeMicroseconds(joint1[0]);
        servo3.writeMicroseconds(joint2[0]);
        servo4.writeMicroseconds(joint3[0]);
        currentStep = 1;
        return;
    }

    Serial.print("Playing step ");
    Serial.print(currentStep + 1);
    Serial.print(" / ");
    Serial.println(stepCount);

    smoothMove(
        joint0[currentStep - 1], joint0[currentStep],
        joint1[currentStep - 1], joint1[currentStep],
        joint2[currentStep - 1], joint2[currentStep],
        joint3[currentStep - 1], joint3[currentStep],
        stepDuration
    );

    currentStep++;
    if (currentStep >= stepCount) {
        currentStep = 0;
        Serial.println("Loop complete. Repeating... Send '0' to stop.");
    }
}

//prints the command menu to the serial monitor
void printHelp() {
    Serial.println("--- Commands ---");
    Serial.println("  0  : Normal mode (potentiometer control)");
    Serial.println("  1  : Record mode (position arm, send 'r' to save each step)");
    Serial.println("  2  : Playback mode (smooth replay of recorded steps)");
    Serial.println("  3  : Gesture mode (python controls servos)");
    Serial.println("  r  : Record current position (record mode only)");
    Serial.println("  h  : Show this help");
    Serial.println("----------------");
}


//reads and processes all incoming serial commands (mode switches, record trigger, gesture S commands)
void handleButtons() {
    while (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == '\r' || cmd == '\n') continue;

        if (cmd == '0') { mode = 0; }
        else if (cmd == '1') { mode = 1; stepCount = 0; }
        else if (cmd == '2') { mode = 2; currentStep = 0; }
        else if (cmd == '3') { mode = 3; }
        else if (cmd == 'S') {
            char buf[24];
            int len = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
            if (mode == 3) {
                buf[len] = '\0';
                int s1, s2, s3, s4;
                if (sscanf(buf, "%d %d %d %d", &s1, &s2, &s3, &s4) == 4) {
                    servo1.writeMicroseconds(s1);
                    servo2.writeMicroseconds(s2);
                    servo3.writeMicroseconds(s3);
                    servo4.writeMicroseconds(s4);
                }
            }
        }
        else if (cmd == 'r' && mode == 1 && millis() - lastRecord > 300) {
            readPot();
            mapping();
            lastRecord = millis();
            recordStep();
        }
        else if (cmd == 'h') { printHelp(); }
    }
}

//when an option is selected accurately state the correct mode in the serial monitor
void printModeChange() {
    if (mode != lastMode) {
        lastMode = mode;
        if (mode == 0) Serial.println("[MODE] Normal — move arm with potentiometers.");
        if (mode == 1) Serial.println("[MODE] Record — position arm and send 'r' to save each step.");
        if (mode == 2) Serial.println("[MODE] Playback — smooth replay of recorded steps. Send '0' to stop.");
        if (mode == 3) Serial.println("[MODE] Gesture — python is controlling servos.");
    }
}


//attaching all servos based off of my circuit, can change on your wiring
void setup() {
    servo1.attach(5);
    servo2.attach(11);
    servo3.attach(10);
    servo4.attach(6);
    pinMode(13, OUTPUT);
    Serial.begin(9600);
    Serial.setTimeout(200);
    analogReference(DEFAULT);

    //smooth opening sequence
    readPot();
    mapping();
    move_servo();

    Serial.println("Send 'h' for help.");
    printHelp();
}

void loop() {
    handleButtons();
    printModeChange();
    unsigned long current_time(millis());

    // potentiometer polling runs in modes 0 and 1; mode 2 runs playback; mode 3 is handled entirely in handleButtons
    if (mode == 0 || mode == 1) {
        if (current_time - previous_time >= 25) {
            previous_time = current_time;
            readPot();
            mapping();
            move_servo();
        }
    }

    if (mode == 2) {
        playBack();
    }
}
