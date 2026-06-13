# Read sysfs entry related to the discovery test code
# and display a circular counter (0-9) or enable LEDs

import sys
import time
from sense_hat import SenseHat

sense = SenseHat()

def read_discovery():
        # fp1 = open("/sys/soo/soolink/discovery/stream_count", "r")
        fp1 = open("/sys/soo/soolink/discovery/neighbours", "r")
        #value = [fp1.read(), fp2.read()]

        value = [fp1.read()]

        # fp2.close()
        fp1.close()
        return value

on = 1;

while True:
        if on == 1:
            sense.set_pixel(7, 7, (0, 0, 255))
            on = 0
        else:
            sense.set_pixel(7, 7, (0, 0, 0))
            on = 1

        val = read_discovery()
        # print("value " + val[0] + " neigh " + val[1])
        # count = int(val[0]) % 10

        # Display the number of successful downloads
        # sense.show_letter(str(count))

        # Display the number of neighbours
        for pix in range(0, 7):
            sense.set_pixel(0, pix, (0, 0, 0))

        for pix in range (0, int(val[0])):
            sense.set_pixel(0, pix, (0, 255, 0))

        time.sleep(1)
