#Connect to COM7 at 9600,8,N,1
#Needs the serial monitor loaded onto the AVR board

#Commands are sent as follows:
#
# LC  - clear LCD
# LP1 - print to line1 of LCD
# LP2 - print to line2 of LCD
#
#\r\n marks the end of a command.
#
#Commands received will be:
#
# P	  - Pause Spotify
#
#Layout
#
#	Line1 - Song
#	Line2 - Artist

import wmi
import win32gui
import win32api
import urllib
import serial
import xml
import datetime, time
import time
from elementtree import ElementTree as ET

import winGuiAuto

import sys
reload(sys)
sys.setdefaultencoding('utf-8')

VK_MEDIA_PLAY_PAUSE = 0xB3 #windows key code for media play/pause button
COM_PORT = 6 #COM7, zero-indexed

class AVR:
	def __init__(self, port):
		self.connection = serial.Serial(int(port)) #open a serial connection on the specified port

	def send_command(self, command):
		self.connection.write(command) #send string over the connection
			
	def close_connection(self):
		self.connection.close()
		
def main():
	hwcode = win32api.MapVirtualKey(VK_MEDIA_PLAY_PAUSE, 0)

	old_lcd_text = {'line1': "", 'line2': ""}
	artist_pos = {'begin':0, 'end':39}
	song_pos = {'begin':0, 'end':39}
	
	spotify = winGuiAuto.findTopWindow("Spotify") #get the handle of the Spotify window
	if not spotify:
		raise ValueError("Spotify window not found. Is it running?")
		
	print "Opening serial connection to AVR."
	avr = AVR(port=COM_PORT) #COM7 - port is zero-indexed
	
	print "Connection open on %s." % avr.connection.name
	print "Waiting for reset..."
	
	time.sleep(3) 
	avr.send_command("LC\r\n")
	
	song = "Waiting for"
	artist = "Spotify data"
	
	while True:
		title = win32gui.GetWindowText(spotify) #get the window title
		title = title.decode('ascii', 'ignore') #Ignore the stupid unicode dash
		title = title.split('  ') #split it so we can get the playing song
		
		try:
			artist = title[0].split('Spotify - ')[1]
			song = title[1]
		except IndexError:
			song = song #do not update if Spotify is paused
			artist = artist #do not update if Spotify is paused
			
		cur_lcd_text = {'line1':str((song[song_pos['begin']:song_pos['end']]).ljust(16, ' ')), 'line2':str((artist[artist_pos['begin']:artist_pos['end']]).ljust(16, ' '))}
		
		time.sleep(1) #There needs to be a short delay
		
		if cur_lcd_text['line1'] != old_lcd_text['line1']:
			avr.send_command("LP1%s\r\n" % cur_lcd_text['line1'])
			old_lcd_text['line1'] = cur_lcd_text['line1']
		
		time.sleep(1) #There needs to be a short delay
		
		if cur_lcd_text['line2'] != old_lcd_text['line2']:
			avr.send_command("LP2%s\r\n" % cur_lcd_text['line2'])
			old_lcd_text['line2'] = cur_lcd_text['line2']
			
		if avr.connection.inWaiting(): #there's a command in the buffer
			r = avr.connection.read() #read the char in the buffer
			if r == "P": #if it's P for pause...
				print "Pause!" #log this
				win32api.keybd_event(VK_MEDIA_PLAY_PAUSE, hwcode) #and send the scancode
			if r == "T":
				print "Tick"
	
		time.sleep(.5)
		
	avr.close_connection()
	
	print "Connection closed."
	
main()