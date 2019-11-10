import json
import requests
import serial 

port = serial.Serial('/dev/cu.SLAB_USBtoUART', 115200)
ids = []

host = 'http://localhost:5000'


b = '4df43c69-2028-48ef-8dd5-bb8fe8f5b73c'
s = '8a8a4a44-21b7-4992-aae7-c1f56e97261e'

def start():
    r = requests.post(f'{host}/start', json={"ids": ids})
    print(r)


while True:
    line = None
    try:
       line = port.readline().decode().strip()
    except UnicodeDecodeError as e:
        print('error', e)
    if line is None:
        continue

    if line == 'ready':
        print('/ready')
        ids = []
    elif line == 'start':
        print('/start {{"ids": {}}}'.format(ids))
        start()
        ids.clear()
    elif line.startswith('uuid'):
        uuid = line.split(' ')[1]
        if uuid not in ids:
            ids.append(uuid)
            print('/add ' + uuid)
        else:
            print('/add ignoring duplicate uuid')
    else:
        print('unrecognized command:', line)