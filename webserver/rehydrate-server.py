"""
Webserver for ReHydrate

MUST BE RUN ON 64bit MongoDB

"""
__author__ = 'Trevor Stanhope'
__version__ = 0.2 

# Modules
from flask import *
import os
import json
import pymongo

# Constants
MONGO_ADDR = "127.0.0.1"
MONGO_PORT = 27017
MONGO_DBNAME = "rehydrate1"

# Objects
app = Flask(__name__)
client = pymongo.MongoClient(MONGO_ADDR, MONGO_PORT)
db = client[MONGO_DBNAME]

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'GET':
        return render_template('index.html')
    elif request.method == 'POST':
        sample = json.loads(request.data)
        collection = db[sample['node']]
        collection.insert(sample)
        return '200'
    else:
        print request.form['time']
        return '200'

if __name__ == '__main__':
    app.run( 
        host="127.0.0.1",
        debug=True,
        port=int("5000")
    )
