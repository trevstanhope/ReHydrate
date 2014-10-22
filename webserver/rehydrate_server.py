from flask import *
app = Flask(__name__)

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'GET':
        return render_template('index.html')
    else:
        print request.form['time']
        return '200'

if __name__ == '__main__':
    app.run( 
        host="127.0.0.1",
        debug=True,
        port=int("5000")
    )
