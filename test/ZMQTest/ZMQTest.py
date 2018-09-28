#!/usr/bin/env python
#

"""
usage: ZMQTest.py [-h] [-n NPROCESSES] [-o OUTPUT] ip

test acquisition and zmq

positional arguments:
  ip                    det ip

optional arguments:
  -h, --help            show this help message and exit
  -n NPROCESSES, --nProcesses NPROCESSES
                        number of zmq poll processes
  -o OUTPUT, --output OUTPUT
                        log file name
"""


import argparse
import logging
import os
import time
import multiprocessing
import zmq
from tools import DEigerClient

__author__ = "SasG"
__date__ = "2018.07.26"


def startLogging(fname="log.log"):
    logging.basicConfig(filename=fname, level=logging.DEBUG,
                        format='%(asctime)s - %(levelname)s - %(message)s')
    logging.info('Started logging to %s' %fname)

def getArgs():
    parser = argparse.ArgumentParser(description='test acquisition and zmq')
    parser.add_argument('ip', type=str, default="192.168.30.34", help='det ip')
    parser.add_argument('-n', '--nProcesses', type=int, default=3, help="number of zmq poll processes")
    parser.add_argument('-o', '--output', type=str, default="receiver.log", help='log file name')

    return parser.parse_args()

def initDetector(ip):
    camera = DEigerClient.DEigerClient(args.ip)
    #logging.info("restarting DAQ %s" %ip)
    #camera.sendSystemCommand("restart")
    #time.sleep(1)
    logging.info("initialize detector %s" %ip)
    camera.sendDetectorCommand("initialize")
    configureDetector(ip)

def configureDetector(ip):
    camera = DEigerClient.DEigerClient(ip)
    configs = [(camera.setDetectorConfig, "frame_time", 0.1),
                (camera.setDetectorConfig, "count_time", 0.1),
                (camera.setDetectorConfig, "ntrigger", 10),
                (camera.setDetectorConfig, "nimages", 10),
                (camera.setDetectorConfig, "trigger_mode", "ints"),
                (camera.setDetectorConfig, "threshold_energy", 6000),
                (camera.setDetectorConfig, "compression", "bslz4"),

                (camera.setDetectorConfig, "test_mode", -1),

                (camera.setFileWriterConfig, "mode", "disabled"),

                (camera.setStreamConfig, "mode", "enabled"),
                (camera.setStreamConfig, "header_detail", "all"),
                ]

    for command, param, key in configs:
        logging.info("setting %s %s to %s" %(command.__name__, param, key))
        command(param, key)

def acquire(ip):
    camera = DEigerClient.DEigerClient(ip)
    try:
        frame_time = float(camera.detectorConfig("frame_time")["value"])
        nimages = int(camera.detectorConfig("nimages")["value"])
        ntrigger = int(camera.detectorConfig("ntrigger")["value"])
        exposureTime = frame_time * nimages * ntrigger
        triggerMode = camera.detectorConfig("trigger_mode")["value"]

        tstart = time.time()

        logging.info(camera.sendDetectorCommand("arm"))
        tDelta = time.time()-tstart
        logging.info("arm command took %.3f seconds" %tDelta)

        for trigger in range(ntrigger):
            logging.info("triggering %04d" %trigger)
            logging.info(camera.sendDetectorCommand("trigger"))

    except Exception as e:
        logging.exception(e)
        raise e

    finally:
        logging.info(camera.sendDetectorCommand("abort"))

class ZMQStream():
    def __init__(self, host, id, apiPort=80, streamPort=9999):
        """
        create stream listener object
        """
        self._host = host
        self._streamPort = streamPort
        self._apiPort = apiPort
        self._id = id

        self.connect()

    def connect(self):
        """
        open ZMQ pull socket
        return receiver object
        """

        context = zmq.Context()
        receiver = context.socket(zmq.PULL)
        receiver.connect("tcp://{0}:{1}".format(self._host,self._streamPort))

        self._receiver = receiver
        logging.info("initialized stream receiver %03d for host tcp://%s:%s" %(self._id, self._host,self._streamPort))
        return self._receiver

    def receive(self):
        """
        receive and return zmq frames if available
        """
        if self._receiver.poll(100): # check if message available
            frames = self._receiver.recv_multipart(copy = False)
            logging.info("%03d received zmq frames with length %d" %(self._id, len(frames)))
            return frames

    def close(self):
        """
        close and disable stream
        """
        logging.info("close ZMQ connection %03d" %self._id)
        return self._receiver.close()

def startStreamListener(ip, id):
    try:
        stream = ZMQStream(ip, id)
        while True:
            frames = stream.receive()

    except Exception as e:
        logging.exception(str(e))
        raise(e)
    finally:
        stream.close()

if __name__ == "__main__":
    args = getArgs()

    startLogging(args.output)
    initDetector(args.ip)

    processes = [multiprocessing.Process(target=startStreamListener, args=(args.ip, id))
                for id in range(args.nProcesses)]
    [process.start() for process in processes]

    acquire(args.ip)

    [process.terminate() for process in processes]
    [process.join() for process in processes]
