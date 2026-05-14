# Robotic Arm: Record/Playback & Hand Control

A robotic arm with four axes that is programmed using the Arduino Nano board and a shield. It offers four modes of control, namely, direct control using potentiometers, recording and playing back positions, and hand gestures using computer vision, all available and switchable over serial communication in real time.

![](<Robotic Arm Picutres/Videos/IMG_9772.jpg>)

## How It Works

The robotic arm consists of four joints that are powered by MG90S and SG90 servo motors. Four potentiometers are used as the main source of input control, with one for each joint. When turning a knob manually, it rotates its associated joint in real time. The Arduino Nano scans each potentiometer, converts the value to a corresponding pulse width in microseconds for the servo motor, and sends commands to the four servos in a timely manner. In addition to direct control, there are three other modes available, such as record, play, and gesture, all of which can be switched through serial communication.

| Command | Mode | Description |
|---------|------|-------------|
| `0` | Normal | Potentiometers control servos in real time |
| `1` | Record | Position the arm and send `r` to save each step (up to 100) |
| `2` | Playback | Smooth looping replay of recorded steps |
| `3` | Gesture | Python takes over servo control via webcam |
| `h` | Help | Prints command menu to serial monitor |

## Firmware

Smooth motion of the arm without jitters involved many factors, among them averaging of multiple sensor values, careful handling of microseconds of signal for each joint's servos.

Averaging. The readings of each potentiometer are averaged after 30 sequential readings, 100µs apart from each other. Otherwise, consecutive readings of analog signals will be affected by noise caused by capacitive effects, which manifest themselves in a permanent jittery behavior of the servos. Averaging the results over 30 times eliminates such artifacts.

Microsecond control. The servos are controlled not by write(), but by writeMicroseconds(). The former only offers 0-180 positions with an integer number between them. Using writeMicroseconds() allows for controlling the servo in a wide range of about 600-2400µs which is enough for smooth interpolation.

Timed polling loop. Reading the sensors as quickly as possible would produce jittery behavior as well as out-of-synchronicity of the joints. The firmware does its work at a predefined frequency of 25ms. Four potentiometers are read and all four servos' positions are updated in one sequence of actions: readPot(), mapping(), then move_servo(). Thus, the joints are always synchronized.

Array-based design. The values of ADC measurements of potentiometers are stored in sensVal[4]. Then, each of these numbers is converted to microseconds with map() function and stored in ist[4]. Then, move_servo() uses the numbers in this array to control the servo positions. In record mode, current ist[] array is copied to four different arrays named joint0-joint3 of 100 elements length.

Potentiometer calibration. The map() function is applied to convert sensVal[i] to ist[i] by calling map(sensVal[i], 0, 650, 600, 2400). This map uses 650 as an upper limit while 1023 is the actual maximum value. After measuring the actual rotation angle of the potentiometer, I found that only a limited rotation gives max ADC value. Therefore, the maximum value for potentiometer rotation was actually 650, and using it as the upper boundary allows for recording a full rotation without any problems.

Ease-in-out playback. When playing back the recorded data, the transitions are not abrupt. A smooth movement of the arm happens due to interpolation and cosine easing algorithm which splits each transition into 50 steps, making the whole transition take about 800ms. The cosine function generates a natural movement pattern, starting slow as the joint accelerates, reaching peak speed through the middle, then decelerating to a stop at the target position.

## Gesture Control

The gesture control script uses MediaPipe Hands to track a hand through a webcam feed and maps the number of extended fingers to servo axes on the arm. MediaPipe returns 21 landmarks per hand, one for each joint and fingertip. To determine whether a finger is extended, the script compares the y-coordinate of the fingertip landmark to the y-coordinate of the knuckle two joints down. Since y increases downward in image coordinates, a fingertip with a lower y value than its knuckle means the finger is pointing up and therefore extended.

The finger count maps directly to which servo moves: 1 finger drives servo 1, 2 fingers servo 2, and so on. Each servo has a direction variable that starts positive, incrementing the position by a fixed step each frame. When the position hits the 2400µs ceiling or 600µs floor, the direction flips and the servo starts moving back. This gives continuous bouncing motion as long as fingers are held up, which makes it intuitive to control. The number of fingers held up determines which joint is active.

The script connects to the Nano on COM3 at 9600 baud, sends a 3 to switch it into gesture mode, and then streams S s1 s2 s3 s4 commands every frame. The Nano parses each incoming S command and writes the four values directly to the servos.

Dependencies: pyserial, opencv-python, mediapipe

## Assembly

![](<Robotic Arm Picutres/Videos/First_Batch_FInished.jpg>)

![](<Robotic Arm Picutres/Videos/Partial_Assembly.jpg>)

![](<Robotic Arm Picutres/Videos/First_Full_Assembly.jpg>)

![](<Robotic Arm Picutres/Videos/Cleaned_Wiring.jpg>)

All the parts were made using PLA through 3D printing. However, there were a few modifications made before the parts were finalized. Initially, the prints came out perfect, and assembly progressed well until it was time to test the claw. The first problem was with the claw. The screw hole position in the original part file was inaccurate, resulting in improper interaction between the gears attached to the fingers of the claw. This led to a situation where only one claw out of the two was correctly closing, the other was not recieving power through the gears. To correct this problem, I uploaded the part file to Autodesk Fusion and then measured the distance from the screw hole to its required position based on the gear geometry. This information was used to relocate the hole, and then it was reprinted successfully, and the mechanism worked in the desired way. The next problem encountered was with the connector arm. The connector arm is a component that connects the claw to the main arm through a channel meant for an M3 nut that screws into the claw mechanism. The channel was small and not able to hold the nut. This problem was corrected by opening up the channel in Autodesk Fusion and then reprinting the part successfully. Both modified STEP files are in the CAD folder with full credit to the original designer.

## PCB & Schematics

The first prototype of the build made use of a breadboard to link up the Nano, potentiometers, and servo signal lines. It did work, although it was untidy and prone to failure; wires would move out of place and connections would come loose when the robot moved. In order to make things cleaner and more efficient, I came up with a design for my own PCB shield, done using Altium Designer. This allows all the components to fit onto one PCB, which is placed on top of the Nano. The Altium source files are included in the PCB + Schematic folder.

![](<PCB + Schematic Pictures/Arduino_Nano + Potentiometers.png>)

![](<PCB + Schematic Pictures/Power + Servo Schematic.png>)

![](<PCB + Schematic Pictures/Footprint.png>)

![](<PCB + Schematic Pictures/3D_PCB.png>)

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | Arduino Nano (ATmega328P) |
| Servos | 4× MG90S / SG90 |
| Potentiometers | 4× 10kΩ |
| Wiring | Jumper wires |
| PCB | Custom shield designed in Altium Designer |
| 3D Printed Parts | PLA |
| Framework | PlatformIO |

## Demo Videos

- Gesture Control: https://youtu.be/HV5-AcP3Ts8
- Record / Playback: https://youtu.be/INzKYXssj1g
- Manual Control: https://youtu.be/m88PAH4PIvc

## Build & Flash

Requires [PlatformIO](https://platformio.org/). With the Nano on COM3:

```bash
pio run --target upload
pio device monitor --baud 9600
```

Send h in the serial monitor for the full command reference.
