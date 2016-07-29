'''
SWMM Output File Wrapper for the New OutputAPI.

Author: Bryant E. McDonnell
Date: 1/10/2016


'''
from ctypes import *
from _toolkitpyswmm import *

__author__ = 'Bryant E. McDonnell (bemcdonnell@gmail.com)'
__copyright__ = 'Copyright (c) 2016 Bryant E. McDonnell'
__license__ = ''
__vcs_id__ = ''
__version__ = '1.0'

class _Opaque(Structure):
    '''
    Used soley for passing the pointer to the smoapu struct to API
    '''    
    pass

class _ReturnIDS(Structure):
    pass

_ReturnIDS._fields_ = [
    ("ID", c_char_p),
    ("next", POINTER(_ReturnIDS))]

class SwmmOutputObjects:
    def __init__(self, dllLoc = './data/outputAPI_winx86.dll'):
        '''
        Instantiate python Wrapper Object and build Wrapper functions.
        '''
        self.smoapiBinName = {}
        self.smoapiBinInd = {}
        self.smoapiIndName = {}

        try:
            self.swmmdll = CDLL(dllLoc)    
        except:
            raise Exception('Failed to Open Linked Library')
     

        #### Initializing DLL Function List
        #Open File Function Handle
        self._openBinFile = self.swmmdll.SMR_open
        self._free = self.swmmdll.SMO_free
        self._close = self.swmmdll.SMO_close
        
        self._getProjectSize = self.swmmdll.SMO_getProjectSize
        self._getTimes = self.swmmdll.SMO_getTimes
        self._getStartTime = self.swmmdll.SMO_getStartTime
        #Object Series Function Handles
        self._getSubcatchSeries = self.swmmdll.SMO_getSubcatchSeries
        self._getNodeSeries = self.swmmdll.SMO_getNodeSeries
        self._getLinkSeries = self.swmmdll.SMO_getLinkSeries
        self._getSystemSeries = self.swmmdll.SMO_getSystemSeries

        #Object Attribure Function Handles
        self._getSubcatchAttribute = self.swmmdll.SMO_getSubcatchAttribute
        self._getNodeAttribute = self.swmmdll.SMO_getNodeAttribute
        self._getLinkAttribute = self.swmmdll.SMO_getLinkAttribute
        self._getSystemAttribute = self.swmmdll.SMO_getSystemAttribute

        #Object Result Function Handles
        self._getSubcatchResult = self.swmmdll.SMO_getSubcatchResult
        self._getNodeResult = self.swmmdll.SMO_getNodeResult
        self._getLinkResult = self.swmmdll.SMO_getLinkResult
        self._getSystemResult = self.swmmdll.SMO_getSystemResult

        #Object ID Return Functions
        self._getSubcatchIDs = self.swmmdll.SMO_getSubcatchIDs
        self._getSubcatchIDs.restype = POINTER(_ReturnIDS)
        self._getNodeIDs = self.swmmdll.SMO_getNodeIDs
        self._getNodeIDs.restype = POINTER(_ReturnIDS)
        self._getLinkIDs = self.swmmdll.SMO_getLinkIDs
        self._getLinkIDs.restype = POINTER(_ReturnIDS)

        #clear ID
        self._freeIDList = self.swmmdll.SMO_freeIDList
        
        #Array Builder
        self._newOutValueArray = self.swmmdll.SMO_newOutValueArray
        self._newOutValueArray.argtypes = [POINTER(_Opaque), c_int, c_int, POINTER(c_int), POINTER(c_int)]
        self._newOutValueArray.restype = POINTER(c_float)

        #Series Builder
        self._newOutValueSeries = self.swmmdll.SMO_newOutValueSeries
        self._newOutValueSeries.argtypes = [POINTER(_Opaque), c_int, c_int, POINTER(c_int), POINTER(c_int)]
        self._newOutValueSeries.restype = POINTER(c_float)        

        #SWMM Date num 2 String
        self.SWMMdateToStr = self.swmmdll.datetime_dateToStr
        
        #SWMM Time num 2 String
        self.SWMMtimeToStr = self.swmmdll.datetime_timeToStr
        
    def OpenBinFile(self, OutLoc):
        '''
        Opens New Bin file and indexes the smoapi pointer to outfile.
        If this function is called more than once with difference *.out
        files, the tool will keep track of each *.out file in dictionaries.

        Calling CloseBinFile will close the indexed outputfile of choice.

        Each retrieve data function, by default, assumes the output file indexed as 0.
        This is advantageous under the condition that the user opens 1 *.out file. If
        more than one *.out file is indexed, the final function argument takes is the
        *.out file the user would like data from. 
        '''
        nm = OutLoc.replace('\\','/')
        nm = nm.split('/')
        nm = nm[-1]
        nm = nm.replace('.out','')
        nm = nm.replace('.','')
        
        if nm not in self.smoapiBinName:
            smoapi = pointer(_Opaque())
            ret = self._openBinFile(OutLoc, byref(smoapi))
            print ret, smoapi
            if len(self.smoapiBinInd.keys()) > 0: ind = max(self.smoapiBinInd.keys())+1
            else: ind = 0
            self.smoapiBinInd[ind] = smoapi
            self.smoapiBinName[nm] = ind
            self.smoapiIndName[ind] = nm
        return 0
    
    def CloseBinFile(self, OutInd = 0, OutName = None):
        if OutName == None:
            if OutInd in self.smoapiBinInd.keys():
                ret = self._close(self.smoapiBinInd[OutInd])
                self.smoapiBinInd.pop(OutInd, None)
                self.smoapiBinName.pop(self.smoapiIndName[OutInd], None)
                self.smoapiIndName.pop(OutInd, None)
        else:
            if OutName in self.smoapiBinName.keys():
                ret = self._close(self.smoapiBinName[OutName])
                self.smoapiBinInd.pop(self.smoapiBinName[OutName], None)
                self.smoapiBinName.pop(OutName, None)
                self.smoapiIndName.pop(self.smoapiBinName[OutName], None)            
        return 0

    def CloseALL(self):
        '''
        Purpose: Closes all Output Files
        '''
        for OutInd in self.smoapiIndName.keys():
            self.CloseBinFile(OutInd = OutInd)
                 
    def get_SubcatchIDs(self, OutInd = 0):
        '''
        Purpose: Returns Element IDs dictionary for Subcatchments
        '''
        smoapi = self.smoapiBinInd[OutInd]
        ErrNo1 = c_int() 
        id_List = self._getSubcatchIDs(smoapi, byref(ErrNo1))
        
        IDList = []
        IDDict = {}

        i = 0
        while i < self.get_ProjectSize(SM_subcatch, OutInd = OutInd):
            IDName = id_List.contents.ID
            IDList.append(IDName)
            IDDict[IDName]=i
            id_List = id_List.contents.next
            i+=1

        self._freeIDList(id_List)
        return IDDict

    def get_NodeIDs(self, OutInd = 0):
        '''
        Purpose: Returns Element IDs dictionary for Nodes
        '''
        smoapi = self.smoapiBinInd[OutInd]
        ErrNo1 = c_int() 
        id_List = self._getNodeIDs(smoapi, byref(ErrNo1))
        
        IDList = []
        IDDict = {}

        i = 0
        while i < self.get_ProjectSize(SM_node, OutInd = OutInd):
            IDName = id_List.contents.ID
            IDList.append(IDName)
            IDDict[IDName]=i
            id_List = id_List.contents.next
            i+=1

        self._freeIDList(id_List)
        return IDDict

    def get_LinkIDs(self, OutInd = 0):
        '''
        Purpose: Returns Element IDs dictionary for Nodes
        '''
        smoapi = self.smoapiBinInd[OutInd]
        ErrNo1 = c_int() 
        id_List = self._getLinkIDs(smoapi, byref(ErrNo1))
        
        IDList = []
        IDDict = {}

        i = 0
        while i < self.get_ProjectSize(SM_link, OutInd = OutInd):
            IDName = id_List.contents.ID
            IDList.append(IDName)
            IDDict[IDName]=i
            id_List = id_List.contents.next
            i+=1

        self._freeIDList(id_List)
        return IDDict

    def get_Units(self, SMO_elementType, OutInd = 0):
        '''
        Purpose: Returns pressure and flow units
        '''
        smoapi = self.smoapiBinInd[OutInd]
        x = c_int()
        ret = self._getProjectSize(smoapi, SMO_elementType, byref(x))
        return x.value

    def get_Times(self, SMO_timeElementType, OutInd = 0):
        '''
        Purpose: Returns report and simulation time related parameters.
        '''
        smoapi = self.smoapiBinInd[OutInd]
        timeElement = c_int()
        ret = self._getTimes(smoapi, SMO_timeElementType, byref(timeElement))
        return timeElement.value
    
    def get_StartTime(self, OutInd = 0):
        '''
        Prupose: Returns the simulation start datetime as double.
        '''
        smoapi = self.smoapiBinInd[OutInd]
        StartTime = c_double()
        ErrNo1 = self._getStartTime(smoapi, byref(StartTime))
        return StartTime.value

    def get_StrStartTime(self, OutInd=0):
        '''
        Purpose: Uses SWMM5 Convestion Functions to Pull DateTime String.
                This can ideally be post processed by user.
        '''
        _StartTime = self.get_StartTime(OutInd = OutInd)
        _date = int(_StartTime)
        _time = _StartTime - _date

        #Pull Date String
        DateStr = create_string_buffer(50)
        self.SWMMdateToStr(c_double(_date), byref(DateStr))
        DATE = DateStr.value
        
        #Pull Time String
        TimeStr = create_string_buffer(50)
        self.SWMMtimeToStr(c_double(_time), byref(TimeStr))
        TIME = TimeStr.value
        return DATE+' '+TIME

    def get_ProjectSize(self, SMO_elementCount, OutInd = 0):
        '''
        Purpose: Returns number of elements of a specific element type
        '''
        smoapi = self.smoapiBinInd[OutInd]
        numel = c_int()
        ret = self._getProjectSize(smoapi, SMO_elementCount, byref(numel))        
        
        return numel.value

    def get_NodeSeries(self, NodeInd, NodeAttr, SeriesStartInd = 0, SeriesLen = -1, OutInd = 0):
        '''
        Purpose: Get time series results for particular attribute. Specify series
        start and length using seriesStart and seriesLength respectively.

        SeriesLen = -1 Default input: Gets data from Series Start Ind to end
        
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if SeriesLen > self.get_Times(numPeriods, OutInd) :
            raise Exception("Outside Number of TimeSteps")
        elif SeriesLen == -1:
            SeriesLen = self.get_Times(numPeriods, OutInd) + 1 - SeriesLen
        if NodeInd > self.get_ProjectSize(nodeCount, OutInd) -1:
            raise Exception("Outside Number of Nodes")            
            
        sLength = c_int()
        ErrNo1 = c_int()            
        SeriesPtr = self._newOutValueSeries(smoapi, SeriesStartInd,\
                                            SeriesLen, byref(sLength), byref(ErrNo1))
        #Check Error
        ErrNo2 = self._getNodeSeries(smoapi, NodeInd, NodeAttr, \
                                  SeriesStartInd, sLength.value, SeriesPtr)
        BldArray = [SeriesPtr[i] for i in range(sLength.value)]
        self._free(SeriesPtr)
        
        return BldArray

    def get_LinkSeries(self, LinkInd, LinkAttr, SeriesStartInd = 0, SeriesLen = -1, OutInd = 0):
        '''
        Purpose: Get time series results for particular attribute. Specify series
        start and length using seriesStart and seriesLength respectively.

        SeriesLen = -1 Default input: Gets data from Series Start Ind to end
        
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if SeriesLen > self.get_Times(numPeriods, OutInd) :
            raise Exception("Outside Number of TimeSteps")
        elif SeriesLen == -1:
            SeriesLen = self.get_Times(numPeriods, OutInd) + 1 - SeriesLen
        if LinkInd > self.get_ProjectSize(linkCount, OutInd) -1:
            raise Exception("Outside Number of Nodes")
        
        sLength = c_int()
        ErrNo1 = c_int()            
        SeriesPtr = self._newOutValueSeries(smoapi, SeriesStartInd,\
                                            SeriesLen, byref(sLength), byref(ErrNo1))
        #Check Error
        ErrNo2 = self._getLinkSeries(smoapi, LinkInd, LinkAttr, \
                                  SeriesStartInd, sLength.value, SeriesPtr)
        BldArray = [SeriesPtr[i] for i in range(sLength.value)]
        self._free(SeriesPtr)
        
        return BldArray

    def get_SubcatchSeries(self, SubcInd, SubcAttr, SeriesStartInd = 0, SeriesLen = -1, OutInd = 0):
        '''
        Purpose: Get time series results for particular attribute. Specify series
        start and length using seriesStart and seriesLength respectively.

        SeriesLen = -1 Default input: Gets data from Series Start Ind to end
        
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if SeriesLen > self.get_Times(numPeriods, OutInd) :
            raise Exception("Outside Number of TimeSteps")
        elif SeriesLen == -1:
            SeriesLen = self.get_Times(numPeriods, OutInd) + 1 - SeriesLen
        if SubcInd > self.get_ProjectSize(subcatchCount, OutInd) -1:
            raise Exception("Outside Number of Nodes")             
            
        sLength = c_int()
        ErrNo1 = c_int()            
        SeriesPtr = self._newOutValueSeries(smoapi, SeriesStartInd,\
                                            SeriesLen, byref(sLength), byref(ErrNo1))
        #Check Error
        ErrNo2 = self._getSubcatchSeries(smoapi, SubcInd, SubcAttr, \
                                  SeriesStartInd, sLength.value, SeriesPtr)
        BldArray = [SeriesPtr[i] for i in range(sLength.value)]
        self._free(SeriesPtr)
        
        return BldArray

    def get_SystemSeries(self, SysAttr, SeriesStartInd = 0, SeriesLen = -1, OutInd = 0):
        '''
        Purpose: Get time series results for particular attribute. Specify series
        start and length using seriesStart and seriesLength respectively.

        SeriesLen = -1 Default input: Gets data from Series Start Ind to end
        
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if SeriesLen > self.get_Times(numPeriods, OutInd) :
            raise Exception("Outside Number of TimeSteps")
        elif SeriesLen == -1:
            SeriesLen = self.get_Times(numPeriods, OutInd) + 1 - SeriesLen

        sLength = c_int()
        ErrNo1 = c_int()            
        SeriesPtr = self._newOutValueSeries(smoapi, SeriesStartInd,\
                                            SeriesLen, byref(sLength), byref(ErrNo1))
        #Check Error
        ErrNo2 = self._getSystemSeries(smoapi, SysAttr, \
                                  SeriesStartInd, sLength.value, SeriesPtr)
        BldArray = [SeriesPtr[i] for i in range(sLength.value)]
        self._free(SeriesPtr)
        
        return BldArray

    def get_NodeAttribute(self, NodeAttr, TimeInd, OutInd = 0):
        '''
        Purpose: For all nodes at given time, get a particular attribute
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getAttribute,\
                                             SM_node, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getNodeAttribute(smoapi, TimeInd, NodeAttr, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray        

    def get_LinkAttribute(self, LinkAttr, TimeInd, OutInd = 0):
        '''
        Purpose: For all links at given time, get a particular attribute
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getAttribute,\
                                             SM_link, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getLinkAttribute(smoapi, TimeInd, LinkAttr, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray 

    def get_SubcatchAttribute(self, SubcAttr, TimeInd, OutInd = 0):
        '''
        Purpose: For all subcatchments at given time, get a particular attribute
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getAttribute,\
                                             SM_subcatch, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getSubcatchAttribute(smoapi, TimeInd, SubcAttr, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray

    def get_SystemAttribute(self, SysAttr, TimeInd, OutInd = 0):
        '''
        Purpose: For all the system at given time, get a particular attribute
        '''
        smoapi = self.smoapiBinInd[OutInd]
        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getAttribute,\
                                             SM_sys, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getSystemAttribute(smoapi, TimeInd, SysAttr, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray

    def get_NodeResult(self, NodeInd, TimeInd, OutInd = 0):
        '''
        Purpose: For a node at given time, get all attributes
        '''
        smoapi = self.smoapiBinInd[OutInd]
        
        if NodeInd > self.get_ProjectSize(nodeCount, OutInd) -1:
            raise Exception("Outside Number of Nodes")
        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getResult,\
                                             SM_node, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getNodeResult(smoapi, TimeInd, NodeInd, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray

    def get_LinkResult(self, LinkInd, TimeInd, OutInd = 0):
        '''
        Purpose: For a link at given time, get all attributes
        '''
        smoapi = self.smoapiBinInd[OutInd]
        
        if LinkInd > self.get_ProjectSize(linkCount, OutInd) -1:
            raise Exception("Outside Number of Links")
        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getResult,\
                                             SM_link, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getLinkResult(smoapi, TimeInd, LinkInd, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray

    def get_SubcatchResult(self, SubcInd, TimeInd, OutInd = 0):
        '''
        Purpose: For a subcatchment at given time, get all attributes
        '''
        smoapi = self.smoapiBinInd[OutInd]
        
        if SubcInd > self.get_ProjectSize(subcatchCount, OutInd) -1:
            raise Exception("Outside Number of Subcatchments")
        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getResult,\
                                             SM_subcatch, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getSubcatchResult(smoapi, TimeInd, SubcInd, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray

    def get_SystemResult(self, TimeInd, OutInd = 0):
        '''
        Purpose: For the system at given time, get all attributes
        '''
        smoapi = self.smoapiBinInd[OutInd]

        if TimeInd > self.get_Times(numPeriods, OutInd)-1:
            raise Exception("Outside Number of TimeSteps")
        
        alength = c_int()
        ErrNo1 = c_int()
        ValArrayPtr = self._newOutValueArray(smoapi, getResult,\
                                             SM_sys, byref(alength), byref(ErrNo1))
        ErrNo2 = self._getSystemResult(smoapi, TimeInd, ValArrayPtr)
        BldArray = [ValArrayPtr[i] for i in range(alength.value)]
        self._free(ValArrayPtr)
        return BldArray
