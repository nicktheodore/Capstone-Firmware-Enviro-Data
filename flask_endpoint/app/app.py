import os
import logging
import pygsheets
import pandas as pd

from app import SensorDataHandler
from flask import Flask, render_template, request

app = Flask(__name__)
# app.config.from_object('config')

datahandler = SensorDataHandler()

gc = pygsheets.authorize(service_file='/Users/nicolastheodore/FYDP/flask_endpoint/creds.json')

sheet = gc.open('Test Dunville')
worksheet = sheet[0]

@app.route('/')
def index():
    return 'Hello, Index!'

@app.route('/data', methods=['GET', 'POST'])
def sensor_data():
    if request.method == 'GET':
        columns = ['macAddr', 'temperature', 'humidity', 'pressure', 'readingID', 'timestamp', 'rssi']
        testdata= ['testmac', 'testtemp', 'testhum', 'testpress', 'testid', 'testts', 'testrssi']

        tmp_df = pd.DataFrame([dict(zip(columns, testdata)) for _ in range(100)])
        worksheet.append_table(tmp_df.values.tolist())

        return "Check the google sheet" #"GET methods are not supported"

    if request.headers['Content-Type'] == 'application/json':
        datahandler.update(request.json)
        tmp_df = datahandler.get_last_n()
        print(tmp_df)
        worksheet.append_table(tmp_df.values.tolist())
        print(request.json)
        return "Success <200>"

    return 'Error: No data'


# IMPORTANT: THE HOST MUST BE EQUAL TO '0.0.0.0' IN ORDER FOR THE ENDPOINTS TO BE PUBLICALLY VISIBLE
# PORT 5000 ALSO NECESSARY.
if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000, debug=True)