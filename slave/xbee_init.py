#Import and init an XBee device
from xbee import XBee, ZigBee
import serial, time

xs = serial.Serial('/dev/ttyUSB0',9600)

def xbee_wait_for_ok(xs):
   okcr = 'OK\r'
   err = False

   c = xs.read(3)
   #print ('RESPONSE: %s' %c)
   if c != okcr:
      err = True

   return not err

def xbee_command(xs, command):
   #Clear garbage from the buffer
   while xs.inWaiting():
      xs.read()

   #print ('SEND: %s' % command)
   xs.write(command)
   return xbee_wait_for_ok(xs)

def xbee_init(xs):
   ok = True
   command = [
      '+++',
      'ATRE\r',
      'ATFR\r',
      '+++',
      'ATCH0C\r',
      'ATID3332\r',
      'ATAP2\r',
      'ATAC\r',
      'ATCN\r'
   ]

   for i in range(0,3):
      ok &= xbee_command(xs, command[i])

   time.sleep(1)

   for i in range(3,9):
      ok &= xbee_command(xs, command[i])

   time.sleep(1)
   return ok

#xbee_init(xs)
