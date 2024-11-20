import shutil
import serial
import time

src = "./build/gamepadi2c.uf2"
dst = "D:\\"
SERIAL = "COM29"
print("Resetting " + str(SERIAL))
try:
 ser = serial.Serial()
 ser.port = SERIAL
 ser.open()
 ser.baudrate = 9600
 ser.dtr = True
 time.sleep(0.1)
 ser.dtr = False
 ser.baudrate = 1200
 ser.close()
except Exception as e:
 print("Failed to reset " + str(SERIAL))
 print(e)
time.sleep(1)
shutil.copy(src, dst)