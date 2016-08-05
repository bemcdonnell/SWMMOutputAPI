import sys
sys.path.append('..')

from datetime import datetime, timedelta
from SWMMOutputReader import *
import matplotlib.pyplot as plt

OutputCollections = SwmmOutputObjects('../data/outputAPI_winx86.dll')
OutputCollections.OpenBinFile('C:/PROJECTCODE/SWMMOutputAPI/testing/OutputTestModel522_SHORT.out')

Periods = OutputCollections.get_Times(numPeriods)
StartTime = OutputCollections.get_StartTime()
DeltaT = OutputCollections.get_Times(reportStep)#seconds
OutputCollections.get_ProjectSize(SM_subcatch)
##SUBC = OutputCollections.get_SubcatchIDs()
##NODE = OutputCollections.get_NodeIDs()
##LINK = OutputCollections.get_LinkIDs()
##
##TM =OutputCollections.get_StrStartTime() 
##DTime = datetime.strptime(TM,'%Y-%b-%d %H:%M:%S')
##
##DateSeries = [DTime +timedelta(seconds = ind*DeltaT) for ind in range(Periods)]
##DataSeries1 = OutputCollections.get_LinkSeries(LINK['C1:C2'],flow_rate_link)
##DataSeries2= OutputCollections.get_LinkSeries(LINK['C2'],flow_rate_link)
##DataSeries3 = OutputCollections.get_LinkSeries(LINK['C3'],flow_rate_link)
##
##fig = plt.figure()
##ax1 = fig.add_subplot(111)
##ax1.plot(DateSeries, DataSeries1, label = 'C1:C2')
##ax1.plot(DateSeries, DataSeries2, label = 'C2')
##ax1.plot(DateSeries, DataSeries3, label = 'C3')
##ax1.legend()
##plt.show()


OutputCollections.CloseBinFile()
