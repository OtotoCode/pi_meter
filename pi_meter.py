import smbus
import time
import os
import json
import math

bus = smbus.SMBus(1)

#arduino address
SLAVE_ADDRESS = 0x04

#normal state
CONFIG_END=0x00

#GET CONFIG MODE 0x1X
REQUEST_CONFIG=0x10
GET_INO_SLEEP=0x11    #ino_sleep_time (minutes)
GET_PI_SHUT=0x12      #pi shutdown wait time
GET_PI_ISW_TIME=0x13  #pi in sleep wakeup time

#SET CONFIG MODE 0x2X
CONFIG_SET=0x20
SET_INO_SLEEP=0x21    #ino_sleep_time (minutes)
SET_PI_SHUT=0x22      #pi shutdown wait time
SET_PI_ISW_SLEEP=0x23 #pi in sleep wakeup time

#i2c connect try counter
I2C_TRY=5

#thermistor config
THERM_B=4181
THERM_R1=3.00
THERM_R0=2.3
THERM_T0=298.15

#config read from config file
def read_conf():
    with open('conf.json','r') as f:
	file=f.read()
    return file

#get config from ino
def get_config(addr,name):
    TRY = I2C_TRY
    while TRY:
        try:
            tmp_conf = bus.read_byte_data(addr,name)
            return tmp_conf
        except IOError as e:
            print "get config IO error"
            TRY-=1
        except :
            print "get config Unexcepted error"
	    raise
        else:
            break

    if not TRY:
        raise

#check acc state
def check_state(addr):
    TRY = I2C_TRY
    while TRY:
        try:
            reading = int(bus.read_byte_data(addr,0x50))
            print(reading)
            if reading == 5:
                os.system("sudo /sbin/shutdown -h now")
        except IOError as e:
            print "check data IO error"
            TRY-=1
        except :
            print "check data Unexcepted error"
            raise
        else:
            break

    if not TRY:
        raise

#get car data
def get_data(addr,data):
    TRY = I2C_TRY
    while TRY:
        try:
            reading = int(bus.read_byte_data(addr,data))
            return reading
        except IOError as e:
            print "get car data IO error"
            TRY-=1
        except :
            print "get car date Unexcepted error"
            raise
        else:
            break

    if not TRY:
        raise

#set config to ino
def set_config(addr,cmd1,cmd2,cmd3,cmd4):
    TRY = I2C_TRY
    while TRY:
        try:
            bus.write_i2c_block_data(addr,cmd1,[cmd2,cmd3,cmd4])
            time.sleep(4.0)
        except IOError as e:
            print "set config IO error"
            TRY-=1
        except :
            print "set conf Unexcepted error"
            raise
        else:
            break

    if not TRY:
        raise

#get thermistor
def get_therm(tmp):
    temp = 0
    rr1 = THERM_R1 * tmp / (1024.0 - tmp)
    t = 1 / ( math.log( rr1/THERM_R0 ) / THERM_B  +  1/THERM_T0 )
    temp = (t - 273.5)
    return int(temp)

#get oil_press
def get_oil_press(tmp):
    vol = (5000/1023)*tmp
    press = (vol-600)/40
    return press

#write data
def write_data():
    m_data = {"wtmp":"" , "otmp":"" ,"opls": ""}
    m_data["wtmp"]=get_therm(w_temp)
    m_data["otmp"]=get_therm(o_temp)
    m_data["opls"]=get_oil_press(o_press)
    with open('meter_data.json','w') as f:
        json.dump(m_data, f, sort_keys=True, indent=4)

#main
if __name__ == '__main__':

    #config load
    config= json.loads(read_conf())
    set_ino_sleep= int(config["config"]["arduino"]["sleep"])
    set_pi_shut = int(config["config"]["pi"]["shut_wait"])
    set_pi_sleep = int(config["config"]["pi"]["on_sleep_wakeup_time"])

    #get current config

    config_ino_sleep = get_config(SLAVE_ADDRESS,GET_INO_SLEEP)
    config_pi_shut = get_config(SLAVE_ADDRESS,GET_PI_SHUT)
    config_pi_sleep = get_config(SLAVE_ADDRESS,GET_PI_ISW_TIME)

    #set config
    if(config_ino_sleep != set_ino_sleep):
        set_config(SLAVE_ADDRESS,SET_INO_SLEEP,set_ino_sleep,0x00,0x00)
        print "set conf set_ino_sleep"

    if(config_pi_sleep != set_pi_sleep ):
        set_config(SLAVE_ADDRESS,SET_PI_ISW_SLEEP,set_pi_sleep,0x00,0x00)
        print "set conf set_pi_sleep"

    if(config_pi_shut != set_pi_shut):
        set_config(SLAVE_ADDRESS,SET_PI_SHUT,set_pi_shut,0x00,0x00)
        print "set conf set_pi_shut"

    #main loop
    while True:
        check_state(SLAVE_ADDRESS)
        w_temp = get_data(SLAVE_ADDRESS,0x31)
        o_temp = get_data(SLAVE_ADDRESS,0x32)
        o_press = get_data(SLAVE_ADDRESS,0x33)
        b_level = get_data(SLAVE_ADDRESS,0x34)
	write_data()
        time.sleep(1.0)
