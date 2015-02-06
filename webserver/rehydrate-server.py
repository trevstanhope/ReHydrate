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
from datetime import datetime


# Constants
MONGO_ADDR = "127.0.0.1"
MONGO_PORT = 27017
MONGO_DBNAME = "rehydrate1"

# Objects
app = Flask(__name__)
client = pymongo.MongoClient(MONGO_ADDR, MONGO_PORT)
db = client[MONGO_DBNAME]

# 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), "%m/%b/%Y %H:%M:%S")
    addr = MONGO_ADDR
    print('%s - - [%s] "%s" %s -' % (addr, date, task, msg))

def check_key(sample):
    try:
        node_id = sample['node']
        
    except Exception as error:
        pretty_print('ERROR', str(error))
        return False

@app.route('/', methods=['GET', 'POST'])
def index():

    if request.method == 'GET':
        return render_template('index.html')
        
    elif request.method == 'POST':
        sample = json.loads(request.data)
        valid=check_key(sample)
        if valid:
            node_id = sample['node']
            collection = db[node_id]
            check_key=
            doc_id = collection.insert(sample)
            pretty_print('MONGODB', doc_id)
            return '200'
        else:
            return '666'
    else:
        print request.form['time']
        return '200'

## 
@app.route('/graph/<node_id>', methods=['GET'])
def graph(node_id):
    return render_template('graph.html', node_id=node_id)

if __name__ == '__main__':
    app.run( 
        host="127.0.0.1",
        debug=True,
        port=int("5000")
    )
