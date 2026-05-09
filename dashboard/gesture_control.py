import cv2
import mediapipe as mp
import serial
import time
import math

# serial connection to arduino — sleep(2) waits for arduino to reboot after connecting
ser = serial.Serial('COM3', 9600, timeout=1)
time.sleep(2)

# mediapipe and webcam setup
mp_hands = mp.solutions.hands
mp_draw  = mp.solutions.drawing_utils
cap      = cv2.VideoCapture(0)

fist_start = None
gesture    = ""
servo1     = 1500
servo2     = 1500
servo4     = 1500


#fcn to scale a value from one range to another, same as arduino's map()
def map_value(value, in_min, in_max, out_min, out_max):
    # clamp input to avoid out of range values
    value = max(in_min, min(in_max, value))
    return int((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)


# fcn to count how many fingers are extended using tip vs knuckle y positions
def count_fingers(landmarks):
    # fingertip landmark ids
    tips  = [8, 12, 16, 20]
    # base knuckle landmark ids
    bases = [6, 10, 14, 18]

    count = 0
    for tip, base in zip(tips, bases):
        # tip above knuckle = extended
        if landmarks[tip].y < landmarks[base].y:
            count += 1

    #thumb uses horizontal position instead of vertical
    if landmarks[4].x > landmarks[3].x:
        count += 1

    return count


#fcn to get the distance between two landmarks, used for gripper span
def landmark_distance(landmarks, id1, id2):
    p1 = landmarks[id1]
    p2 = landmarks[id2]
    return math.sqrt((p1.x - p2.x)**2 + (p1.y - p2.y)**2)


with mp_hands.Hands(max_num_hands=1, min_detection_confidence=0.7) as hands:
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        # mirror so movement feels natural
        frame = cv2.flip(frame, 1)

        # mediapipe requires rgb, opencv gives bgr
        rgb    = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        result = hands.process(rgb)

        if result.multi_hand_landmarks:
            for hand_landmarks in result.multi_hand_landmarks:

                #draw hand skeleton on the camera feed
                mp_draw.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

                lm = hand_landmarks.landmark

                # mapping wrist x to base servo — left/right
                servo1 = map_value(lm[0].x, 0.0, 1.0, 600, 2400)
                # mapping wrist y to shoulder servo — up/down
                servo2 = map_value(lm[0].y, 0.0, 1.0, 600, 2400)

                #mapping thumb to pinky span to gripper servo
                span   = landmark_distance(lm, 4, 20)
                servo4 = map_value(span, 0.1, 0.5, 600, 2400)

                # creating cases for 3 gestures: open palm = live, fist = record, peace = playback
                fingers = count_fingers(lm)

                if fingers == 5:
                    gesture    = "live control"
                    fist_start = None
                    ser.write(b'0')

                elif fingers == 0:
                    # fist held for 1 second triggers a record step
                    gesture = "fist — hold 1s to record"
                    if fist_start is None:
                        fist_start = time.time()
                    elif time.time() - fist_start > 1.0:
                        gesture    = "recording step"
                        fist_start = None
                        ser.write(b'r')

                elif fingers == 2:
                    gesture    = "playback"
                    fist_start = None
                    ser.write(b'2')

                else:
                    gesture    = f"{fingers} fingers"
                    fist_start = None

        #indication of current gesture and servo values for the user
        cv2.putText(frame, f"gesture: {gesture}", (10, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.putText(frame, f"s1:{servo1}  s2:{servo2}  s4:{servo4}", (10, 80),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, "q to quit", (10, frame.shape[0] - 15),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (150, 150, 150), 1)

        cv2.imshow("robotic arm control", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

#cleanup on exit
cap.release()
cv2.destroyAllWindows()
ser.close()
