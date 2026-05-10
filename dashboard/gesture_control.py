import serial
import time
import cv2
import mediapipe as mp

port = "COM3"
baud = 9600

#neutral position for gesture control saved in array for each servo 1 - 4
positions = [1500, 1500, 1500,1500 ]
#keep track of directions 1 = rotating clockwise, -1 = rotating anti-clockwise
directions = [1, 1, 1, 1]
#how much to move each motor by, ensuring smooth movement
step = 20
min_Pos = 600
max_Pos = 2400

# let DTR reset happen naturally (same approach as the working version),
# then wait for Arduino to finish booting before sending anything
arm = serial.Serial(port, baud, timeout=1)
time.sleep(2)
arm.reset_input_buffer()
arm.write(b'3')
arm.flush()
time.sleep(0.3)
response = arm.read_all().decode('utf-8', errors='ignore')
if '[MODE] Gesture' in response:
    print("Gesture mode confirmed!")
else:
    print(f"WARNING: mode 3 not confirmed. Got: {response!r}")

#limit # of hands to 1, and setup video
mp_drawing = mp.solutions.drawing_utils
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(max_num_hands=1)
cap = cv2.VideoCapture(0)

tip_ids = [4, 8, 12, 16, 20]

while True:
    ret,frame = cap.read()
    if not ret:
        break
    #reflect the camera
    frame = cv2.flip(frame, 1)
    #openCV gives bgr, mediapipe requires rgb convert
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    #result of the analysis through mediapipeline
    result = hands.process(rgb)

    fingers_up = 0

    #check if a hand is visible then from there if it is create the landmarks
    if result.multi_hand_landmarks:
        #draw the 21 hand landmarks and skeleton connections onto the frame
        for hand_landmarks in result.multi_hand_landmarks:
            mp_drawing.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)
        landmarks = result.multi_hand_landmarks[0].landmark
        for i in range(1, 5):
            #fingertip y < knuckle y means the finger is extended (y increases downward in image coordinates)
            if landmarks[tip_ids[i]].y < landmarks[tip_ids[i] - 2].y:
                fingers_up +=1

        if fingers_up > 0:
            #fingers_up - 1 converts the count to a 0-based index (1 finger → servo 0, 2 → servo 1, etc.)
            idx = fingers_up - 1
            positions[idx] += step * directions[idx]


            if positions[idx] >= max_Pos or positions[idx] <= min_Pos:
                directions[idx] *= -1
            positions[idx] = max(min_Pos, min(max_Pos, positions[idx]))

    command = f"S {positions[0]} {positions[1]} {positions[2]} {positions[3]}\n"
    arm.write(command.encode())
    arm.flush()

    cv2.imshow("Gesture Control", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
arm.close()
