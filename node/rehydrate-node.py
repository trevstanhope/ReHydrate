#!/usr/bin/env python
"""
ReHydrate 

TODO:
- Authenticate to server?
- Validate data received from Arduino
"""

__version__ = 0.2
__author__ = 'Trevor Stanhope'

# Libraries
import zmq
import ast
import json
import os
import sys
import cherrypy
import numpy as np
import urllib2
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

    def add_log_entry(self, origin, msg):
        date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
        print('[%s] %s %s' % (date, origin, msg))

    ## Initialize
    def __init__(self, config):
    
        self.add_log_entry('INIT', 'Setting Configuration')
        self.hostname = socket.gethostname()
        self.add_log_entry('INIT', 'Loading Config File')
        with open(config) as config_file:
            settings = json.loads(config_file.read())
            for key in settings:
                try:
                    getattr(self, key)
                except AttributeError as error:
                    self.add_log_entry('INIT', '%s : %s' % (key, settings[key]))
                    setattr(self, key, settings[key])
        self.init_arduino()
        self.init_log()
        self.init_monitors()

    def init_monitors(self):
        try:
            self.add_log_entry('INIT', 'Initializing Monitors')
            Monitor(cherrypy.engine, self.new_sample, frequency=self.SAMPLE_INTERVAL).subscribe()
        except Exception as error:
            self.add_log_entry('ERROR', str(error))

    def init_arduino(self):
        try:
            self.add_log_entry('INIT', 'Initializing Arduino')
            self.sensor_data = {}
            self.arduino = Serial(self.ARDUINO_DEV, self.ARDUINO_BAUD, timeout=self.ARDUINO_TIMEOUT)
        except Exception as error:
            self.add_log_entry('ERROR', str(error))

    def init_log(self):
        try:
            self.add_log_entry('INIT', 'Initializing Log')
            self.log_path = '%s/%s' % (self.LOG_DIR, datetime.strftime(datetime.now(), self.LOG_FILE))
            logging.basicConfig(filename=self.log_path, level=logging.DEBUG)
        except Exception as error:
            self.add_log_entry('ERROR', str(error))

    ## Check Data
    def check_data(self, data):
        keys = ['EC', 'TEMP', 'N', 'CA', 'K']        
        for k in keys:
            if len(data[k]) != 5:
                self.add_log_entry('ERROR', 'key-values failed check')                
                return {}
        return data

    ## Read Arduino
    def read_arduino(self):
        self.add_log_entry('NODE', 'Reading Arduino')
        try:
            string = self.arduino.readline()
            result = ast.literal_eval(string)
            self.sensor_data = self.check_data(result)
        except Exception as error:
            self.add_log_entry('ERROR', str(error))
            self.sensor_data = {}
 
    ## Post Sample
    def post_sample(self):
        self.add_log_entry('NODE', 'Sending Sample to Server')
        try:
            sample = {
                'time' : datetime.strftime(datetime.now(), '%Y-%m-%d %H:%M:%S'),
                'node' : self.hostname,
                'data' : self.sensor_data
            }
            dump = json.dumps(sample)
            req = urllib2.Request(self.POST_URL)
            req.add_header('Content-Type','application/json')
            response = urllib2.urlopen(req, dump)
            self.add_log_entry('NODE', 'Response %s' % str(response.getcode()))
        except Exception as error:
            self.add_log_entry('ERROR', str(error))

    ## Ping
    def new_sample(self):
        self.read_arduino()
        self.post_sample()
    
    ## Shutdown
    def shutdown(self):
        self.add_log_entry('NODE', 'Shutting Down')
        if self.ARDUINO_ENABLED:
            try:
                self.arduino.close()
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