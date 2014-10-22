#!/usr/bin/env python
"""
ReHydrate 
Developed by Trevor Stanhope

TODO:
- Authenticate to server?
- Validate data received from Arduino
"""

__version__ = 0.2

# Libraries
import zmq
import ast
import json
import os
import sys
import cherrypy
import numpy as np
import urllib2
import cv2
from datetime import datetime
from serial import Serial, SerialException
from ctypes import *
from cherrypy.process.plugins import Monitor
from cherrypy import tools
import logging
import socket

# Constants
try:
    CONFIG_FILE = sys.argv[1]
except Exception as err:
    CONFIG_FILE = 'settings.json'

# Node
class ReHydrate:

    ## Initialize
    def __init__(self, config):
  
        print('[Setting Configuration]')
        self.hostname = socket.gethostname()
        
        print('[Loading Config File]')
        with open(config) as config_file:
            settings = json.loads(config_file.read())
            for key in settings:
                try:
                    getattr(self, key)
                except AttributeError as error:
                    print('\t' + key + ' : ' + str(settings[key]))
                    setattr(self, key, settings[key])
        
        try:
            if self.POST_ENABLED:
                print('[Initializing Post]')
                Monitor(cherrypy.engine, self.post_sample, frequency=self.POST_INTERVAL).subscribe()
                self.post_available = True
            else:
                self.post_available = False
        except Exception as error:
            print('\tERROR: %s' % str(error))
            
        try:
            if self.ARDUINO_ENABLED:
                try:
                    print('[Initializing Arduino]')
                    self.arduino = Serial(self.ARDUINO_DEV, self.ARDUINO_BAUD, timeout=self.ARDUINO_TIMEOUT)
                    self.arduino_available = True
                except Exception as error:
                    print('\tERROR: %s' % str(error))
                    self.arduino_available = False
                if self.arduino_available:
                    Monitor(cherrypy.engine, self.read_arduino, frequency=self.ARDUINO_INTERVAL).subscribe()
            else:
                self.arduino_available = False
        except Exception as error:
            print('\tERROR: %s' % str(error))
        
        try:    
            if self.CAMERA_ENABLED:
                print('[Initializing Camera]')
                Monitor(cherrypy.engine, self.read_camera, frequency=self.CAMERA_INTERVAL).subscribe()
                self.camera = cv2.VideoCapture(self.CAMERA_INDEX)
                self.camera.set(3, self.CAMERA_WIDTH)
                self.camera.set(4, self.CAMERA_HEIGHT)
                self.camera_available = True
            else:
                print('\t... camera failed.')
                self.camera_available = False
        except Exception as error:
            print('\tERROR: %s' % str(error))
        
        try:
            print('[Initializing Microphone]')
            if self.MICROPHONE_ENABLED:
                Monitor(cherrypy.engine, self.read_microphone, frequency=self.MICROPHONE_INTERVAL).subscribe()
                self.microphone_available = True
            else:
                self.microphone_available = False
        except Exception as error:
            print('\tERROR: %s' % str(error))
        
        try:
            print('[Initializing Log]')
            if self.LOG_ENABLED:
                logging.basicConfig(filename=self.LOG_FILE, level=logging.DEBUG)
                self.log_available = True
            else:
                self.log_available = False
        except Exception as error:
            print('\tERROR: %s' % str(error))
 
    ## Read Camera
    def read_camera(self):
        print('[Reading Camera]')
        try:
            for i in range(self.CAMERA_BUFFER):
                (s, bgr) = self.camera.read()
            if s:
                if self.CAMERA_FLIP:
                    bgr = cv2.flip(bgr, -1)
                cv2.imwrite('static/current.jpg', bgr)
                self.image = bgr
        except Exception as error:
            print('\tERROR: %s' % str(error))

    ## Read Arduino
    def read_arduino(self):
        print('[Reading Arduino]')
        try:
            string = self.arduino.readline()
            result = ast.literal_eval(string)
            self.sensor_data = result
        except Exception as error:
            print('\tERROR: %s' % str(error))
            self.sensor_data = 'N/A'
 
    ## Post Sample
    def post_sample(self):
        print('[Sending Sample to Server]')
        try:
            sample = {
                'time' : datetime.strftime(datetime.now(), '%Y-%m-%d %H:%M:%S'),
                'hostname' : self.hostname
            }
            if self.arduino_available:
                sensor_data = {'sensor_data' : self.sensor_data}
            if self.camera_available:
                image = {'image' : self.image}
            if self.microphone_available:
                wav = {'wav' : self.wav}
            dump = json.dumps(sample)
            req = urllib2.Request(self.POST_URL)
            req.add_header('Content-Type','application/json')
            response = urllib2.urlopen(req, dump)
            print('\tOKAY: %s' % str(response.getcode()))
        except Exception as error:
            print('\tERROR: %s' % str(error))
    
    ## Shutdown
    def shutdown(self):
        print('[Shutting Down]')
        if self.ARDUINO_ENABLED:
            try:
                self.arduino.close()
            except:
                pass
        if self.CAMERA_ENABLED:
            try:
                self.camera.release()
            except:
                pass
        os.system("sudo reboot")

    ## Render Index
    @cherrypy.expose
    def index(self):
        with open('static/index.html') as html:
            return html.read()
    
# Main
if __name__ == '__main__':
    node = ReHydrate(config=CONFIG_FILE)
    currdir = os.path.dirname(os.path.abspath(__file__))
    cherrypy.server.socket_host = node.CHERRYPY_ADDR
    cherrypy.server.socket_port = node.CHERRYPY_PORT
    conf = {
        '/': {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'static')},
    }
    cherrypy.quickstart(node, '/', config=conf)
