"""
EigerTest extends the functionality of DEigerClient for convenient
interactive work in a python shell. It should be used only for this
intended purpose or as an example how to use DEigerClient in your own
code. The ipython shell is highly recommended.

!Do NOT use this code in non-interactive scripts, python modules
or subclasses! Kittens will die and the detector may behave erratic!
For such purposes, please use DEigerClient or EIGER's REST API directly.

This code is
- NOT official DECTRIS code but written for personal use by the author
- NOT part of the software of EIGER detector systems.
- NOT well tested.
- only provided as is. Do not expect bug fixes, new features or any
  support or maintanence by DECTRIS.
- NOT to be distributed without consent of the author!
- not particularly well documented. Be prepared to read the code
  and simply try things out.

Modified: SteB
Version: 20150131

Author: Marcus Mueller
Contact: marcus.mueller@dectris.com
Version: 20140922
"""

import os
import re
import sys
import time, datetime

sys.path.insert(0,"/usr/local/dectris/python")
from eigerclient import DEigerClient
try:
    import dectris.albula
    from quickstart import Monitor
    MONITOR_AVAILABLE = True
except ImportError:
    MONITOR_AVAILABLE = False

#############################################################
### Configure IP and PORT of the EIGER system that you are
### accessing with this code below.
# DMZ system
#~ IP = "62.12.129.162"
#~ PORT = "4010"
#~ 
# fixed IP, direct connection
try:
  IP = sys.argv[1]
except:
  IP = "192.168.1.96"
print "IP", IP
PORT = "80"

# for printing detector config and values
LISTED_DETECTOR_PARAMS = [
    u'description',
    u'detector_number',
    u'data_collection_date',
    u'frame_time',
    u'nimages',
    u'ntrigger',
    u'trigger_mode',
    u'photon_energy',
    u'threshold_energy',
    u'element',
    u'count_time',
    u'detector_readout_time', 
    u'nframes_sum', 
    u'frame_count_time',
    u'frame_period',
    #u'sub_image_count_time',
    u'auto_summation',
    u'summation_nimages',
    u'bit_depth_readout',
    u'efficiency_correction_applied',
    u'flatfield_correction_applied',
    u'number_of_excluded_pixels',
    u'calibration_type',
    u'countrate_correction_applied',
    u'countrate_correction_bunch_mode',
    u'pixel_mask_applied',
    #u'pixel_mask',
    u'virtual_pixel_correction_applied',
    u'software_version',
    u'sensor_material',
    u'sensor_thickness',
    u'x_pixels_in_detector',
    u'y_pixels_in_detector',
    u'x_pixel_size',
    u'y_pixel_size',
    u'wavelength',
    u'detector_distance',
    u'beam_center_x',
    u'beam_center_y',
    u'detector_translation',
    u'detector_orientation']
CHECK_DOWNLOAD="/tmp/check_download"

# for filewriter config to have data bundles of ~ 1GB 
# 16M =  20 (45MB x  20)
#  4M =  80 (12MB x  80)
#  1M = 320 ( ?MB x 320)
NIMAGES_PER_FILE=10000


class EigerTest(DEigerClient):
    def setEnergy(self, photon_energy, threshold_ratio=0.5):
        self.setDetectorConfig("photon_energy", photon_energy)
        self.setDetectorConfig("threshold_energy", photon_energy * threshold_ratio)

    def setElement(self, element='Cu'):
        self.setDetectorConfig(u'element', element)

    def imageSeries(self, expp, nimages, auto_sum=False, ff_corr=True,
                    pixel_mask=True):
        self.setDetectorConfig("auto_summation", auto_sum)
        self.setDetectorConfig("frame_time", expp)
        # to avoid warm pixels in center of module
        if expp > 0.099:
            readout_time = 5.0e-04 # no simultaneous read/write
        else:
            # simultaneous read/write will give high duty cycle but also warm pixels
            readout_time = self.detectorConfig('detector_readout_time')['value']
        self.setDetectorConfig("count_time", expp - readout_time)
        self.setDetectorConfig("nimages", nimages)
        self.setDetectorConfig("flatfield_correction_applied", ff_corr)
        self.setDetectorConfig("pixel_mask_applied", pixel_mask)
        self.printConf('time|image')

    def printConf(self, regex='', full=0):
        for param in LISTED_DETECTOR_PARAMS:
            if full:
                if re.search(regex, param):
                    try:   
                        print str(param).ljust(35), ' = ', str(e.detectorConfig(param)['value']).ljust(35),'  ', str(e.detectorConfig(param)['min']).ljust(35),'  ', str(e.detectorConfig(param)['max']).ljust(35)
                    except:
                        print ""
                        pass
                        #print str(param).ljust(35), ' = ', str(e.detectorConfig(param)['value']).ljust(35)
            else:
                if re.search(regex, param):
                    print str(param).ljust(35), ' = ', e.detectorConfig(param)['value']



    def setFileNameBase(self, fnBase):
        self.setFileWriterConfig("name_pattern", fnBase)

    def setImagesPerFile(self, nimages=1000):
        self.setFileWriterConfig("nimages_per_file", nimages)

    def pHello(self, z="today"):
        print "Hello " + z

    def printFileWriterConfig(self):
        for param in self.fileWriterConfig():
            try:
                print param, ' = ', self.fileWriterConfig(param)['value']
            except RuntimeError as e:
                print "RuntimeError accessing %s: %s" % (param, e)

    def printFileWriterStatus(self):
        for param in self.fileWriterStatus():
            try:
                print param, ' = ', self.fileWriterStatus(param)['value']
            except RuntimeError as e:
                print "RuntimeError accessing %s: %s" % (param, e)

    def printTempHum(self, all_data=0):
        for param in self.detectorStatus():
            if all_data:      
                if re.search("temp|humidity", param):
                    print (param).ljust(35), ' = ', self.detectorStatus(param)['value']
            else:
                if re.search("th0", param):
                    print (param).ljust(35), ' = ', self.detectorStatus(param)['value']
             
    def printFW(self):
        for param in self.detectorStatus():
            if re.search("fw", param):
                print (param).ljust(35), ' = ', self.detectorStatus(param)['value']

    def printDetectorState(self):
        print "State: %s" % self.detectorStatus('state')['state']
        print "Error: %s" % self.detectorStatus('error')

    def setDetConMultiple(self, **params):
        """
        Convenience function to set a single or multiple detector
        configuration parameters in the form (parameter=value[, ...]).
        Multiple parameters are set in arbitrary order!
        You have to check the print output of this function whether
        you obtain the desired detector configuration
        """
        for p, data in params.items():
            changeList = self.setDetectorConfig(param=p, value=data, dataType=None)
            changes = ''
            for changed_param in changeList:
                changed_value = self.detectorConfig(changed_param)['value']
                changes += '%s = %s ; ' % (changed_param, str(changed_value))
            print "Setting:  %s = %s" % (p, str(data))
            print "Changing: " + changes[:-2]

    def purgeFiles(self, force=False):
        f_list = self.fileWriterFiles()
        if not force:
            print "Files on the detector control unit:"
            #~ [print(f) for f in f_list]
            for f in f_list:
                print f
        if force == True or raw_input('Do you really want to purge all '
                                      'these files? \n'
                                      'Then enter y. ') == 'y':
            [self.fileWriterFiles(f, 'DELETE') for f in f_list]
            #~ for f in f_list:
                #~ self.fileWriterFiles(f, 'DELETE')
        else:
            print "Aborting without deleting files."

    def startMonitor(self):
        m = None
        if not MONITOR_AVAILABLE:
            print "Monitor nor available. Check dependencies."
            print "Returning None."
        else:
            m = Monitor(self)
            m.start()
            print "Monitor started and monitor object returned."
            self._monitor = m
        return m

    def arm(self):
        self.sendDetectorCommand(u'arm')

    def trigger(self):
        self.sendDetectorCommand(u'trigger')

    def disarm(self):
        self.sendDetectorCommand(u'disarm')

    def initialize(self, element=None, energy=None):
        self.sendDetectorCommand(u'initialize')
        if element is not None and energy is not None:
            print "You cannot give element AND energy."
        elif element is not None:
            self.setElement(element)
        elif energy is not None:
            self.setEnergy(energy)

    def exposure(self, fnBase=None):
        """
        tvx style exposure command
        """
        if fnBase is not None:
            self.setFileNameBase(fnBase=fnBase)
        self.setDetectorConfig('data_collection_date',
                               self.fileWriterStatus('time')['value'])
        print "Arm ..."
        self.arm()
        print "Trigger ..."
        self.trigger()
        print "Disarm ..."
        time.sleep(1)
        self.disarm()
        print "DONE!"

    def download(self, downloadpath="/tmp"):
        try:
           matching = self.fileWriterFiles()
        except:
           print "could not get file list"
        if len(matching):  
            try:
                [self.fileWriterSave(i, downloadpath) for i in matching]
            except:
                print "error saveing - noting deleted"
            else:
                print "Downloaded ..." 
                for i in matching:
                    print i + " to " + str(downloadpath)
                [self.fileWriterFiles(i, method = 'DELETE') for i in matching]
                print "Deteted " + str(len(matching)) + " file(s)"

    def downloadD(self, downloadpath="/tmp"):
        open(CHECK_DOWNLOAD, 'a').close() 
        while os.path.exists(CHECK_DOWNLOAD): 
         try:
            matching = self.fileWriterFiles()
         except:
            print "could not get file list"
         time.sleep(1) 
         if len(matching)>0:  
            try:
                [self.fileWriterSave(i, downloadpath) for i in matching]
            except:
                print "error saveing - noting deleted"
            else:
                print "Downloaded ..." 
                for i in matching:
                    print i
                [self.fileWriterFiles(i, method = 'DELETE') for i in matching]
                print "Deteted " + str(len(matching)) + " file(s)"

    def stopDownloadD(self):
        os.remove(CHECK_DOWNLOAD) 


    def exp(self,frame_time=1,nimages=1,ntrigger=1000):
        """
        tvx style exposure command
        """
        self.setDetectorConfig("frame_time", frame_time)
        readout_time = self.detectorConfig('detector_readout_time')['value']
        self.setDetectorConfig("count_time", frame_time - readout_time)
        self.setDetectorConfig("nimages", nimages)
        self.setDetectorConfig("ntrigger",ntrigger)
        if nimages < NIMAGES_PER_FILE:
             nipf = nimages
        else:
             nipf = NIMAGES_PER_FILE
        self.setFileWriterConfig("nimages_per_file",nipf+1)
     
        print "Arm ..."
        self.arm()
        print "Trigger ..."
        print(datetime.datetime.now().strftime("%H:%M:%S.%f"))
        self.trigger()
        print(datetime.datetime.now().strftime("%H:%M:%S.%f"))
        print "Disarm ..."
        time.sleep(1)
        self.disarm()
        print "DONE!"
        self.printConf('time|image')

    def hdf_file_check(self):
        """
        tvx style exposure command
        """
        self.setDetectorConfig("omega_range_average",0.1)
        self.setDetectorConfig("frame_time", 1)
        readout_time = self.detectorConfig('detector_readout_time')['value']
        self.setDetectorConfig("count_time", 1 - readout_time)
        self.setDetectorConfig("nimages", 1)
        self.setDetectorConfig("ntrigger",1)
    
        self.setFileWriterConfig("nimages_per_file",1+1)
     
        print "Arm ..."
        self.arm()
        print "Trigger ..."
        self.trigger()
        print "Disarm ..."
        time.sleep(1)
        self.disarm()
        print "DONE!"
        self.printConf('time|image')

    def testLoop(self, nloops, purge=True, **kwargs):
        print "#" * 72 + "\nRunning test loop with %d series.\n" % nloops
        self.imageSeries(**kwargs)
        print "-" * 72
        for n in range(nloops):
            print "Running series %d" % (n + 1)
            self.exposure()
            if purge:
                print "Purging files."
                time.sleep(1)
                self.purgeFiles(force=True)
            print "-" * 72
        print "Finished %d series." % nloops
        print "#" * 72 + "\n"


if __name__ == '__main__':
    print "run init"
    e = EigerTest(host=IP, port=PORT, verbose=False)
    e.printDetectorState()

    #~ e.setDetectorConfig('auto_summation', False)
    #~ e.setDetectorConfig('countrate_correction_applied', True)
    #~ e.setDetectorConfig('element', 'Cu')

    #~ e.sendDetectorCommand(u'initialize')
    #~ e.setEnergy(12400)
    #~ e.imageSeries(expp=0.2, nimages=5)
    #~ e.printTempHum()
    # ~ e.setImagesPerFile(100)
    #~ e.printDetectorConfig()
    #~ e.printFileWriterConfig()
    #~ print "Exposure ..."
    #~ e.exposure(fnBase='test')
    #~ print "... done"
    #~ e.fileWriterSave(filename='test*', targetDir=os.path.curdir)


