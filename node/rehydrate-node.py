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
from datetime import datetime, timedelta
from serial import Serial, SerialException
from ctypes import *
from cherrypy.process.plugins import Monitor
from cherrypy import tools
import logging
import socket
import pymongo
import random

# Constants
try:
    CONFIG_FILE = sys.argv[1]
except Exception as err:
    CONFIG_FILE = 'settings.json'

# Node
class ReHydrate:

    ## Add Log Entry
    def add_log_entry(self, origin, msg):
        date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
        print('[%s] %s %s' % (date, origin, msg))
    
    ## Init
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
        
        ## Initializers
        self.init_arduino()
        self.init_log()
        self.init_monitors()
        self.init_db()
     
    ## Init Monitors   
    def init_monitors(self):
        try:
            self.add_log_entry('CHERRYPY', 'Initializing Monitors')
            Monitor(cherrypy.engine, self.new_sample, frequency=self.SAMPLE_INTERVAL).subscribe()
            Monitor(cherrypy.engine, self.update_graphs, frequency=self.UPDATE_INTERVAL).subscribe()
        except Exception as error:
            self.add_log_entry('CHERRYPY', str(error))
    
    ## Init Arduino
    def init_arduino(self):
        try:
            self.add_log_entry('ARDUINO', 'Initializing Arduino')
            self.sensor_data = {}
            for i in range(3):
                try:
                    self.arduino = Serial(self.ARDUINO_DEV + str(i), self.ARDUINO_BAUD, timeout=self.ARDUINO_TIMEOUT)
                    break
                except Exception as error:
                    self.add_log_entry('ARDUINO', str(error))
        except Exception as error:
            self.arduino = None
            self.add_log_entry('ARDUINO', str(error))
    
    ## Init Log
    def init_log(self):
        try:
            self.add_log_entry('LOG', 'Initializing Log')
            self.log_path = '%s/%s' % (self.LOG_DIR, datetime.strftime(datetime.now(), self.LOG_FILE))
            logging.basicConfig(filename=self.log_path, level=logging.DEBUG)
        except Exception as error:
            self.add_log_entry('LOG', str(error))
    
    ## Init DB
    def init_db(self):
        try:
            self.add_log_entry('DATABASE', 'Initializing Local Database')
            self.client = pymongo.MongoClient(self.MONGO_ADDR, self.MONGO_PORT)
        except Exception as error:
            self.add_log_entry('DATABASE', str(error))
    
    ## Read Arduino
    def read_arduino(self):
        self.add_log_entry('ARDUINO', 'Reading Arduino')
        try:
            string = self.arduino.readline()
        except Exception as error:
            self.add_log_entry('ARDUINO ERROR', str(error))
        try:
            result = ast.literal_eval(string)
            sensor_data = self.check_data(result)
            return sensor_data
        except Exception as error:
            self.add_log_entry('PARSE ERROR', str(error))
            return {} #! WARNING: set to generate random data if no arduino
    
    ## Generate Random Data
    def random_data(self):
        self.add_log_entry('WARNING', 'Generating Random Data')
        d = {}
        for p in self.SENSORS.keys():
            d[p] = random.randrange(1024)
        return d
    
    ## Check data integritys  
    def check_data(self, data):
        for p in self.SENSORS.keys():
            try:
                if data[p] < 0 or data[p] > 1024:
                    del(data[p])
                    self.add_log_entry('CHECK ERROR', 'invalid range')     
            except Exception as error:
                self.add_log_entry('CHECK ERROR', 'Key does not exist: %s' % p)                
        return data
    
    ## Calculate PPM from serial bits
    #! Change to any units
    def calculate_ppm(self, data):
        try:
            self.add_log_entry('PROCESSING', 'Calculate PPM')  
            for p in self.SENSORS.keys():
                x_in = data[p]
                units = self.SENSORS[p]['UNITS']
                x = np.array(self.SENSORS[p]['X'])
                y = np.array(self.SENSORS[p]['Y'])
                d = np.array(self.SENSORS[p]['D'])
                z = np.polyfit(x, y, d) # the polynomial fit
                y_out = np.polyval(z, x_in)
                data['%s_%s' % (p, units)] = round(y_out, self.PPM_PRECISION)
            return data
        except Exception as error:
            self.add_log_entry('CONVERT ERROR', str(error))

    ## Calculate mV from serial bits
    def calculate_millivolt(self, data):
        try:
            self.add_log_entry('PROCESSING', 'Calculate mV')  
            for p in self.SENSORS.keys():
                mv_per_bit = self.SENSORS[p]["MV_PER_BIT"]
                mv_offset = self.SENSORS[p]["MV_OFFSET"]
                bit_at_zero = self.SENSORS[p]["BIT_AT_ZERO"]
                x_in = data[p]
                y_out = mv_per_bit * (x_in - bit_at_zero) + mv_offset
                data['%s_mV' % p] = round(y_out, self.MV_PRECISION)
            return data
        except Exception as error:
            self.add_log_entry('CONVERT ERROR', str(error))

    
    ## Post sample to server
    def post_sample(self, sensor_data):
        self.add_log_entry('POST', 'Sending Sample to Server')
        try:
            sample = {
                'time' : datetime.strftime(datetime.now(), '%Y-%m-%d %H:%M:%S'),
                'node' : self.hostname,
                'data' : sensor_data
            }
            dump = json.dumps(sample)
            req = urllib2.Request(self.POST_URL)
            req.add_header('Content-Type','application/json')
            response = urllib2.urlopen(req, dump)
            self.add_log_entry('POST', 'Response %s' % str(response.getcode()))
        except Exception as error:
            self.add_log_entry('POST ERROR', str(error))
    
    ## Store sample to database   
    def store_sample(self, sensor_data):
        for k in sensor_data:
            self.add_log_entry(k, '= %s' % sensor_data[k])
        try:
            date = datetime.strftime(datetime.now(), "%Y%m")
            db = self.client[date]
            col = db['samples']
            sensor_data['time'] = datetime.now() # need to provide a time
            doc_id = col.insert(sensor_data)
            self.add_log_entry('STORE', 'Doc ID: %s' % str(doc_id))
        except Exception as error:
            self.add_log_entry('STORE ERROR', str(error))
        
    ## Update Graphs
    def update_graphs(self):
        try:
            hours = self.GRAPH_RANGE
            date = datetime.strftime(datetime.now(), "%Y%m")
            db = self.client[date]
            col = db['samples']
            dt = datetime.now() - timedelta(hours=hours) # get datetime
            matches = col.find({'time':{'$gt':dt, '$lt':datetime.now()}})
            self.add_log_entry('DATABASE', 'Queried documents in time frame')
        except Exception as error:
            self.add_log_entry('GRAPH ERROR 1', str(error))
        try:
            with open('data/samples.json', 'w') as jsonfile:
                self.add_log_entry('GRAPHS', 'Updating samples.json')
                results = []
                for sample in matches:
                    for p in self.SENSORS.keys():
                        try:
                            point = {
                                'time': datetime.strftime(sample['time'], "%Y-%m-%d %H:%M:%S"),
                                'sensor_id' : p,
                                'reading' : sample[p],
                                'mV' : sample['%s_mV' % p],
                            }
                            results.append(point)
                        except Exception as error:
                            pass
                jsonfile.write(json.dumps(results, indent=True))
        except Exception as error:
            self.add_log_entry('GRAPH ERROR 3', str(error))
            
    ## Generate CSV
    def generate_csv(self):
        self.add_log_entry('CSV', 'Generating CSV')
        hours = self.GRAPH_RANGE #TODO
        date = datetime.strftime(datetime.now(), "%Y%m")
        db = self.client[date]
        col = db['samples']
        dt = datetime.now() - timedelta(hours=hours) # get datetime
        matches = col.find({'time':{'$gt':dt, '$lt':datetime.now()}})
        try:
            with open('data/samples.csv', 'w') as csvfile:
                sample = matches.next()
                del sample['_id']
                headers = [str(i) for i in sample.keys()]
                headers.append('\n')
                out = ','.join(headers)
                csvfile.write(out)
                for sample in matches:
                    del sample['_id']
                    a = [str(i) for i in sample.values()]
                    a.append('\n')
                    out = ','.join(a)
                    csvfile.write(out)
        except Exception as error:
            self.add_log_entry('CSV ERROR', str(error))
               
    ## New Sample
    def new_sample(self):
        try:
            if self.arduino:
                sensor_data = self.read_arduino()
                if self.check_data(sensor_data):
                    millivolt_data = self.calculate_millivolt(sensor_data)
                    proc_data = dict(millivolt_data.items())
                    self.post_sample(proc_data)
                    self.store_sample(proc_data)
            else:
                self.init_arduino()
        except Exception as error:
            self.add_log_entry('SAMPLE ERROR', str(error))
    
    ## Calibrate
    def calibrate(self):
        try:
            self.add_log_entry('CALIBRATE', 'Running calibration routine')
            minutes = self.CALIBRATION_INTERVAL
            date = datetime.strftime(datetime.now(), "%Y%m")
            db = self.client[date]
            col = db['samples']
            dt = datetime.now() - timedelta(minutes=minutes) # get datetime
            matches = col.find({'time':{'$gt':dt, '$lt':datetime.now()}})
            calibration_params = [p for p in self.SENSORS.keys()]
            calibration_data = []
            for sample in matches:
                calibration_point = [sample[p] for p in calibration_params]
                calibration_data.append(calibration_point)
            calibration_means = np.mean(calibration_data, axis=0)
            data = {}
            for i in range(len(calibration_means)):
                p = calibration_params[i]
                data[p] = calibration_means[i]
                calibration_cmd = "%s%d" % (self.SENSORS[p]['SET_CMD'], calibration_means[i])
                self.add_log_entry("SET", "Setting %s with: %s" % (p, calibration_cmd))
                self.arduino.write(calibration_cmd)

            # Generate calibration JSON
            with open('data/calibration.json', 'w') as jsonfile:
                data = self.calculate_millivolt(data)
                jsonfile.write(json.dumps(data, indent=True))
        except Exception as error:
            print str(error)
            
    ## Shutdown
    def shutdown(self):
        self.add_log_entry('MISC', 'Shutting Down')
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
            
    ## Handle Posts
    @cherrypy.expose
    def default(self, *args, **kwargs):
        try:
            if kwargs['type'] == 'calibrate':
                self.calibrate()
            if kwargs['type'] == 'csv':
                self.generate_csv()
            if kwargs['type'] == 'reload':
                self.GRAPH_RANGE = int(kwargs['range'])
                self.update_graphs()
        except Exception as err:
            self.add_log_entry('POST_ERROR', str(err))
        return None
    
# Main
if __name__ == '__main__':
    node = ReHydrate(config=CONFIG_FILE)
    currdir = os.path.dirname(os.path.abspath(__file__))
    cherrypy.server.socket_host = node.CHERRYPY_ADDR
    cherrypy.server.socket_port = node.CHERRYPY_PORT
    conf = {
        '/': {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'static')},
        '/data' : {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'data')}
    }
    cherrypy.quickstart(node, '/', config=conf)
