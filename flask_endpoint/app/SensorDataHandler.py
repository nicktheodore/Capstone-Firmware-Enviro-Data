import pandas as pd

class SensorDataHandler(object):
    """
    SensorDataHandler

    Stores, handles, and displays data received from Flask endpoint
    """
    columns = ['macAddr', 'temperature', 'humidity', 'pressure', 'readingID', 'timestamp', 'rssi']
    
    def __init__(self, upload_every=10):
        self._dataframe = pd.DataFrame(columns = self.columns)
        self._last_n = 0

    def update(self, data):
        """
        Updates the SensorDataHandler to store data in the form of {"macAddr": ..., "temperature": }

        :param data: json data received from the endpoint
        :return: length of the dataframe received
        """
        df_tmp = pd.DataFrame(data)
        self._dataframe = pd.concat([self._dataframe, df_tmp])
        self._last_n = len(df_tmp)
        return self._last_n

    def get_last_n(self):
        return self._dataframe.tail(self._last_n)
    