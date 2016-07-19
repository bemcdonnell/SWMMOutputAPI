import sys
sys.path.append('..')

from datetime import datetime, timedelta
from SWMMOutputReader import *
import matplotlib.pyplot as plt

OutputCollections = SwmmOutputObjects('../data/outputAPI_win.dll')
OUTFILES = ['C:\\PROJECTCODE\\SWMMOutputAPI\\testing\\OutputTestModel_LargeOutput.out',\
            'C:\\PROJECTCODE\\SWMMOutputAPI\\testing\\OutputTestModel522_SHORT.out',\
            'C:\\PROJECTCODE\\SWMMOutputAPI\\testing\\SSCM12_RPM_BAS_wRT-DSS_6Storm_NoCEPT330_01-03.out']

for indMAIN, val in enumerate(OUTFILES):
    OutputCollections.OpenBinFile(val)
    
    Periods = OutputCollections.get_Times(numPeriods, OutInd = indMAIN)
    print Periods
    StartTime = OutputCollections.get_StartTime(OutInd = indMAIN)
    DeltaT = OutputCollections.get_Times(reportStep,OutInd = indMAIN)#seconds
    ProjectSize = OutputCollections.get_ProjectSize(SM_node,OutInd = indMAIN)
    SUBC = OutputCollections.get_SubcatchIDs(OutInd = indMAIN)
    NODE = OutputCollections.get_NodeIDs(OutInd = indMAIN)
    LINK = OutputCollections.get_LinkIDs(OutInd = indMAIN)
    print len(NODE)
    TM =OutputCollections.get_StrStartTime(OutInd = indMAIN) 
    DTime = datetime.strptime(TM,'%Y-%b-%d %H:%M:%S')

#DateSeries = [DTime +timedelta(seconds = ind*DeltaT) for ind in range(Periods)]

#   OutputCollections.OpenBinFile('C:\\PROJECTCODE\\SWMMOutputAPI\\testing\\SSCM12_RPM_BAS_wRT-DSS_6Storm_NoCEPT330_01-03.out')

##SUBC1 = OutputCollections.get_SubcatchIDs(1)
##NODE1 = OutputCollections.get_NodeIDs(1)
##LINK1 = OutputCollections.get_LinkIDs(1)


##print "in"
##DataSeries1 = OutputCollections.get_NodeSeries(NODE['0034S0264'],invert_depth)
##DataSeries2= OutputCollections.get_LinkSeries(LINK['C2'],flow_rate_link)
##DataSeries3 = OutputCollections.get_LinkSeries(LINK['C3'],flow_rate_link)
##print "out"
##fig = plt.figure()
##ax1 = fig.add_subplot(111)
##ax1.plot(DateSeries, DataSeries1, label = 'C1:C2')
##ax1.plot(DateSeries, DataSeries2, label = 'C2')
##ax1.plot(DateSeries, DataSeries3, label = 'C3')
##ax1.legend()
##plt.show()






##OutputCollections.CloseBinFile()
