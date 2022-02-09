import adafruit_dotstar
import board
import time
import pwmio
from adafruit_motor import servo
import pulseio
last_update = time.monotonic()
pulse_low = 986
pwm = pwmio.PWMOut(board.D13, duty_cycle=2 ** 15, frequency = 50)
pwm2 = pwmio.PWMOut(board.D5, duty_cycle=2 ** 15, frequency = 50)
esc = servo.ContinuousServo(pwm, min_pulse=1000, max_pulse=2000)
steering = servo.Servo(pwm2, min_pulse=1000, max_pulse=2000)

# pulseio counts both rising and falling edges so for 8 channels + delay we need 18 values
remote_input = pulseio.PulseIn(board.SDA, maxlen=18)
channels = [0.5 for i in range(8)]
last_received = time.monotonic()
led = adafruit_dotstar.DotStar(board.APA102_SCK, board.APA102_MOSI, 1)
while True:
    # remove first value if the delay is not a long pause (found between frames of ppm signal)
    if remote_input and remote_input[0] < 3000:
        remote_input.popleft()

    # if all the expected pulses are received
    if len(remote_input) == remote_input.maxlen:
        remote_input.pause()
        s_out = []
        # copy values from remote_input but exclude values representing long pause
        values = [remote_input[i] for i in range(2, remote_input.maxlen)]

        # convert values from pairs of integers into floats from 0 to 1
        for channel in range(0, len(values)/2):
            channel_nums = channel*2
            value = values[channel_nums] + values[channel_nums + 1]
            adjusted_value = value - pulse_low
            normalized_value = adjusted_value / 1024
            channels[channel] = min(1, max(0, normalized_value))
            s_out.append(f'{channels[channel]:4.2}')
        #print(' '.join(s_out), end=' ')
        if channels[2] < 0.33:
            steering.angle = channels[1] * 180
            esc.throttle = (channels[0] - 0.5) * 2
            #print('raw mode')
        elif channels[2] < 0.66:
            steering.angle = channels[1] * 180
            turn_reduction_strength = 0.7*(2 * abs(channels[1]-0.5))
            turn_reduction_factor = 1 - turn_reduction_strength
            esc.throttle = ((channels[0] - 0.5) * 2) * turn_reduction_factor
            #print('steer assist mode')
        print((steering.angle / 180, esc.throttle))
        last_received = time.monotonic()
        led[0] = (0, 255, 0)
        led.show()

        #resume reading pulses
        remote_input.clear()
        remote_input.resume()



    # if it has been 0.5s since the last valid input was received, stop the car
    if time.monotonic() - last_received > 0.5:
        esc.throttle = 0
        steering.angle = 90
        print("killed")
        led[0] = (255, 0, 0)
        led.show()
