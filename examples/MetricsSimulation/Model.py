'''

Model class for Cooja System Log Parser

'''
from datetime import datetime
from re import S
import re
from types import new_class
from xml.dom import minidom
from numpy import apply_along_axis, median
import matplotlib
from sqlalchemy.sql.sqltypes import PickleType
matplotlib.use('Agg')
import threading

from sqlalchemy.sql.elements import TextClause
from Runner import Runner
from sqlalchemy import create_engine, MetaData, ForeignKey, Column, Integer, String, Float, DateTime, Boolean, engine
from sqlalchemy.orm import relationship
#Para realizar as alterações/consultas
from sqlalchemy.orm import sessionmaker
from sqlalchemy import func, inspect
from sqlalchemy import create_engine, MetaData
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
import json
from sqlalchemy.sql.expression import label, null
from sqlalchemy.orm import class_mapper
from sqlalchemy import orm



DBName = "Metrics.db"    
engine = create_engine('sqlite:///' + DBName, connect_args={'check_same_thread': False}, echo = False)
meta = MetaData()
meta.bind = engine
Base = declarative_base(metadata=meta)
Session = sessionmaker(bind=engine)
db = Session()


'''
Base model generic class
'''
class MyModel():
    def as_dict(self):
        return { c.name: getattr(self, c.name) for c in self.__table__.columns }
    def toJson(self):
        return json.dumps(self, default=lambda o: o._asdict(), sort_keys=True, indent=4)
    def _asdict(self):
        return {c.key: getattr(self, c.key)
            for c in inspect(self).mapper.column_attrs}
    def object_to_dict(obj, found=None):
        if found is None:
            found = set()
        mapper = class_mapper(obj.__class__)
        columns = [column.key for column in mapper.columns]
        get_key_value = lambda c: (c, getattr(obj, c).isoformat()) if isinstance(getattr(obj, c), datetime) else (c, getattr(obj, c))
        out = dict(map(get_key_value, columns))
        for name, relation in mapper.relationships.items():
            if relation not in found:
                found.add(relation)
                related_obj = getattr(obj, name)
                if related_obj is not None:
                    if relation.uselist:
                        out[name] = [object_to_dict(child, found) for child in related_obj]
                    else:
                        out[name] = object_to_dict(related_obj, found)
        return out


'''
Represents an experiment
    Parameters:
        name: Experiment Name
        parameters: Experiment parameters (Could be refactored)
        logFile: Physical log file (Could be excluded after a Live parsing)
        experimentFile: Cooja .csc simulation
        dateRun: Timestamp of running time (Null never run)
        records: Generated records after run
'''
class Experiment(Base, MyModel):
    __tablename__ = 'experiments'
    id = Column(Integer, primary_key=True)
    name = Column(String(50), nullable=False)
    parameters = Column(String (200), nullable=False)
    experimentFile = Column(String (200), nullable=False)
    #dateRun = Column(DateTime, nullable=False)
    def run(self):
        runner = Runner(str(self.experimentFile))
        newRun = Run()
        newRun.maxNodes = len(minidom.parse(self.experimentFile).getElementsByTagName('id'))+1 #To use the node.id directly untedns
        newRun.experiment = self
        newRun.start = datetime.now()
        runner.run()
        newRun.end = datetime.now()
        newRun.processRun()
        newRun.parameters = newRun.getParameters()
        db.add(newRun)
        self.runs.append(newRun)
        db.commit()

class Run(Base, MyModel):
    __tablename__ = "runs"
    id = Column(Integer,primary_key=True)
    start = Column(DateTime)
    end = Column(DateTime)
    maxNodes = Column(Integer)
    parameters = Column(PickleType)
    experiment_id = Column(Integer, ForeignKey('experiments.id')) # The ForeignKey must be the physical ID, not the Object.id
    experiment = relationship("Experiment", back_populates="runs")
    metric = relationship("Metrics", uselist=False, back_populates="run")

    def __str__(self) -> str:
        layer = {}
        try:
            layer['mac'] = self.parameters['MAKE_MAC'].split('_')[-1]
        except:
            layer['mac'] = "TSCH"
        try:
            layer['rpl'] = self.parameters['MAKE_ROUTING'].split('_')[-1]
        except:
            layer['rpl'] = "LITE"
        try:
            layer['net'] = self.parameters['MAKE_NET'].split('_')[-1]
        except:
            layer['net'] = "IPV6"
            
        return "ID: {} Exp: {} MAC: {mac} ROUTING: {rpl} NET: {net}".format(self.id ,self.experiment.id, **layer)


    def printNodesPosition(self):
        from mpl_toolkits.mplot3d import Axes3D
        import matplotlib.pyplot as plt
        import io
        import base64
        tempBuffer = io.BytesIO()
        plt.clf()
        nodes = []
        x = []
        y = []
        z = []
        for node, position in self.getNodesPosition().items():
            nodes.append(node)
            x.append(position['x'])
            y.append(position['y'])
            z.append(0)
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        ax.scatter(x, y, z, c='b', marker='o')
        i = 0
        for label in nodes:
            ax.text(x[i],y[i],z[i], label)
            i += 1

        ax.set_xlabel('X Position')
        ax.set_ylabel('Y Position')
        #ax.set_zlabel('Z Label')
        ax.set_title("Nodes position")
        ax.set_zlim3d(0,100)

        plt.savefig(tempBuffer, format = 'png')
        return base64.b64encode(tempBuffer.getvalue()).decode()

    def getNodesPosition(self):
        myData = {}
        for i in range(2,(self.maxNodes)):
            myData['n' + str(i)] = {}
        doc = minidom.parse(self.experiment.experimentFile)
        for i in doc.getElementsByTagName('mote'):
            try:
                myId = str(i.getElementsByTagName('id')[0].firstChild.data)
                x = float(i.getElementsByTagName('x')[0].firstChild.data)
                y = float(i.getElementsByTagName('y')[0].firstChild.data)
                myData['n' + myId] = {'x' : x, 'y' : y}
            except:
                None
        return myData


    def processRun(self):
        with open("COOJA.testlog", "r") as f:
            for line in f.readlines():
                if (line.startswith("Random") or line.startswith("Starting") or line.startswith("Script timed out") or line.startswith("TEST OK")):
                    continue
                if (line.startswith("Test ended at simulation time:")):
                    simTime = line.split(":")[1].strip()
                    continue
                fields = line.split()
                logTime = fields[0]
                logNode = fields[1]
                logDesc = re.findall("\[(.*?)\]", line)[0].split(":")
                logLevel = logDesc[0].strip()
                logType = logDesc[1].strip()
                data = line.split("]")[1].strip()
                newRecord = Record(logTime,logNode,logLevel,logType,data,self)
                db.add(newRecord)
                self.records.append(newRecord)
        #print(simTime)
    
    '''
    Adapted from: https://stackoverflow.com/questions/2804543/read-subprocess-stdout-line-by-line
    '''
    def getParameters(self):
        import subprocess
        proc = subprocess.Popen(['make','viewconf'],bufsize=1, universal_newlines=True, stdout=subprocess.PIPE)
        myDict = {}
        for param in ['radiomedium','transmitting_range','interference_range','success_ratio_tx','success_ratio_rx']:
            myDict[param] = str(minidom.parse(self.experiment.experimentFile).getElementsByTagName(param)[0].firstChild.data).strip()
        for line in iter(proc.stdout.readline,''):
            if not line:
                break
            if line.startswith("####"):
                line = line.split()
                try:
                    #print (line[1].split("\"")[1] , "value", line [4])
                    myDict[line[1].split("\"")[1]] = line [4]
                except IndexError:
                    if (line[1].split("\"")[1].startswith("MAKE")):
                        #print (line[1].split("\"")[1] , "value", line [3])
                        myDict[line[1].split("\"")[1]] = line [3]
                        continue
                    continue
        return myDict

'''
Represents an experiment's record
    Parameters:
        simTime: Simulation time of record (Microseconds 10^ -6)
        node: Number of the node that generates the record
        recordLevel: Level of record (Info, Warn, Debug)
        recordType: Type (App, Protocol, Layer, etc)
        rawData: Record string

'''
class Record(Base, MyModel):
    __tablename__ = 'records'
    id = Column(Integer, primary_key=True)
    simTime = Column(Integer, nullable=False)
    node = Column(Integer, nullable=False)
    recordLevel = Column(String(50), nullable=False)
    recordType = Column(String(50), nullable=False)
    rawData = Column(String(200), nullable=False)
    run_id = Column(Integer, ForeignKey('runs.id')) # The ForeignKey must be the physical ID, not the Object.id
    run = relationship("Run", back_populates="records")
    def __init__(self,simtime, node,level,type,data,run):
        self.simTime = simtime
        self.node = node
        self.recordLevel = level
        self.recordType = type
        self.rawData = data
        self.run = run
      
class Node(Base, MyModel):
    __tablename__ = 'nodes'
    id = Column(Integer, primary_key=True)
    simId = Column(Integer, primary_key=True)
    posX = Column(Integer, nullable=False)
    posY = Column(Integer, nullable=False)
    posZ = Column(Integer, nullable=False)

class Metrics(Base, MyModel):
    __tablename__ = 'metrics'
    id = Column(Integer, primary_key=True)
    run_id = Column(Integer, ForeignKey('runs.id')) # The ForeignKey must be the physical ID, not the Object.id
    run = relationship("Run", back_populates="metric")
    #application_id = Column(Integer, ForeignKey('application.id')) # The ForeignKey must be the physical ID, not the Object.id
    application = relationship("Application", uselist=False, back_populates="metric")
    tsch_id = Column(Integer, ForeignKey('tsch.id')) # The ForeignKey must be the physical ID, not the Object.id
    tsch = relationship("TSCH", back_populates="metric")

    def __init__(self, run):
        self.run = run
        #print("Self lenght:" , len(self.run.records) )
        self.application = Application(self)
        self.application.process()
        if run.parameters['MAKE_MAC'] ==  "MAKE_MAC_TSCH":
            self.tsch = TSCH(self)
        db.add(self)
        db.commit()

class Application(Base, MyModel):
    __tablename__ = 'application'
    id = Column(Integer, primary_key=True)
    metric_id = Column(Integer, ForeignKey('metrics.id')) # The ForeignKey must be the physical ID, not the Object.id
    metric = relationship("Metrics", back_populates="application")
    latency_id = Column(Integer, ForeignKey('latencies.id')) # The ForeignKey must be the physical ID, not the Object.id
    latency = relationship("Latency", back_populates="application")
    pdr_id = Column(Integer, ForeignKey('pdrs.id')) # The ForeignKey must be the physical ID, not the Object.id
    pdr = relationship("PDR", back_populates="application")


    def __init__(self,metric):
        self.metric = metric
        self.latency = Latency(self)
        self.pdr = PDR(self)

    def process(self):
        #data = db.query(Record).filter_by(run = run).filter_by(recordType = "App").all()
        data = self.metric.run.records
        for rec in data:
            if rec.rawData.startswith("app generate"):
                sequence = rec.rawData.split()[3].split("=")[1]
                node = int(rec.rawData.split()[4].split("=")[1])
                genTime = rec.simTime
                dstNode = 1 #That simulation doesn't define a customized sink
                #print("Node: " ,  node , "Seq: " , sequence , "Generation Time: ", genTime ,"Destination" , dstNode)
                newLatRec = AppRecord(genTime,node,dstNode,sequence)
                newLatRec.rcv = False
                self.records.append(newLatRec)
                continue
            
            if rec.rawData.startswith("app receive"):
                sequence = rec.rawData.split()[3].split("=")[1]
                recTime = rec.simTime
                srcNode = int (rec.rawData.split()[4].split("=")[1].split(":")[5], 16) # Converts Hex to Dec
                for record in self.records:
                    if (record.srcNode == srcNode and record.sqnNumb == sequence):
                        record.rcvPkg(recTime)
                        record.rcv = True
                #print("Node: " ,  srcNode  , "Seq: " , sequence , "Receive Time: ", recTime)
                        break

class RPL(Base, MyModel):
    __tablename__ = 'rpl'
    id = Column(Integer, primary_key=True)

'''
Represents a regular MAC message
'''
class MACMessage(MyModel):
    _lock = threading.Lock()
    origin = 0
    dest = 0
    enQueued = 0
    seqno = 0
    queueNBROccupied = 0
    queueNBRSize = 0
    queueGlobaOccupied = 0
    queueGlobaSize = 0
    headerLen = 0
    dataLen = 0
    sentTime = 0
    status = 0
    tries = 0
    rcvTime = 0
    isReceived = False
    isSent = False
    def __init__(self,origin,dest,enQue,seqno,queueNBROccupied,queueNBRSize,queueGlobaOccupied,queueGlobaSize,headerLen,dataLen):
        self.origin = origin
        self.dest = dest
        self.enQueued = enQue
        self.seqno = seqno
        self.queueNBROccupied = queueNBROccupied
        self.queueNBRSize = queueNBRSize
        self.queueGlobaOccupied = queueGlobaOccupied
        self.queueGlobaSize = queueGlobaSize
        self.headerLen = headerLen
        self.dataLen = dataLen
    def sent(self, time, status, tx):
        self.sentTime = time
        self.status = status
        self.tries = tx
        self.isSent = True
        #print("Queue->Sent: ", self.sentTime-self.enQueued)
    def receive(self, time):
        self.rcvTime = time
        self.isReceived = True
    def latency(self):
        if not self.isSent:
            raise Exception("Message didn't send")
        if not self.isReceived:
            raise Exception("Message not received")
        return self.rcvTime - self.enQueued
    def retransmissions(self):
        if not self.isSent:
            raise Exception("Message didn't send")
        if not self.isReceived:
            raise Exception("Message not received")
        return self.tries - 1
    def __str__(self) -> str:
        return "{self.origin}<->{self.dest} Q:{self.enQueued} S({self.isSent}):{self.sentTime} S({self.isReceived}):{self.rcvTime} Sq:{self.seqno}".format(self=self)


class TSCH(Base, MyModel):
    __tablename__ = 'tsch'
    id = Column(Integer, primary_key=True)
    metric = relationship("Metrics", uselist=False, back_populates="tsch")
    results = None

    def __init__(self,metric):
        self.metric = metric

    @orm.reconstructor
    def processFrames(self):
        results = {}
        for i in range(0,(self.metric.run.maxNodes)):
            results[str(i)] = []
        results['65535'] = []

        if self.metric.run.parameters['MAKE_MAC'].split('_')[-1] == "TSCH":
            data = db.query(Record).filter_by(run = self.metric.run).filter_by(recordType = "TSCH").all()
            for rec in data:
                if rec.rawData.startswith("send packet to"):
                    origin = int(rec.node)
                    dest = int(rec.rawData.split()[3].split('.')[0], 16)
                    enQueued = float(rec.simTime)
                    seqnum = int(rec.rawData.split()[6].replace(',',''))
                    queueNBROccupied = int(rec.rawData.split()[8].split('/')[0])
                    queueNBRSize = int(rec.rawData.split()[8].split('/')[1])
                    queueGlobaOccupied = int(rec.rawData.split()[9].replace(',','').split('/')[0])
                    queueGlobaSize = int(rec.rawData.split()[9].replace(',','').split('/')[1])
                    headerLen = int(rec.rawData.split()[11])
                    dataLen = int(rec.rawData.split()[12])
                    tschMsg = MACMessage(origin, dest, enQueued, seqnum, queueNBROccupied, queueNBRSize, queueGlobaOccupied, queueGlobaSize, headerLen, dataLen)
                    results[str(origin)].append(tschMsg)
                    continue
                if rec.rawData.startswith("packet sent to"):
                    dest = int(rec.rawData.split()[3].split('.')[0], 16)
                    seqnum = int(rec.rawData.split()[5].replace(',',''))
                    sentTime = float(rec.simTime)
                    status = int(rec.rawData.split()[7].replace(',',''))
                    tx = int(rec.rawData.split()[9].replace(',',''))
                    for msg in reversed(results[str(rec.node)]):
                        if msg.isSent:
                            continue
                        if ( msg.seqno == seqnum and msg.dest == dest):
                            time = sentTime - msg.enQueued 
                            if (time > 10000000):
                                continue
                                #print("Achei uma de seqN " , seqnum, " de ", str(rec.node)," para ",dest, " mas o tempo esta maior q 10s:", time)
                            else:
                                msg.sent(sentTime, status, tx)
                                #print("Depois: ",msg)
                                continue
            for rec in data:
                if rec.rawData.startswith("received from"): 
                    dest = int(rec.node)
                    origin = int(rec.rawData.split()[2].split('.')[0], 16)
                    #print (rec.rawData.split()[2].split('.')[0])
                    seqnum = int(rec.rawData.split()[5])
                    rcvTime = float(rec.simTime)
                    #print ("Iniciando a busca em ", str(origin), "por", seqnum, "dst: ", dest )
                    for msg in results[str(origin)]:
                        #print(msg)
                        if msg.isReceived:
                            #print("Recebida, passando...")
                            continue
                        if msg.seqno == seqnum and msg.dest == dest and msg.isSent and not msg.isReceived:
                            #print(rcvTime, ": Procurando a msg enviada por(",msg.origin,")", origin, " para (",msg.dest,")",dest, " com o seque ", seqnum, " as ", rcvTime)
                            time = rcvTime - msg.sentTime
                            #print(rcvTime, "Achei uma que foi enfileirada as, ", msg.sentTime, "Time: ", time)
                            #print(msg, "-> ", rec.rawData)
                            if time < 10000000:
                                #print("Recebendo a msg enviada por ", origin, " para ",dest, " com o seque ", seqnum)
                                msg.receive(rcvTime)
                                break
                            else:
                                None
        else:
            data = db.query(Record).filter_by(run = self.metric.run).filter_by(recordType = "CSMA").all()
        self.results =  results
        
    def processIngress(self):
        data = db.query(Record).filter_by(run = self.metric.run).filter_by(recordType = "TSCH").all()
        results = [[] for x in range(self.metric.run.maxNodes)]
        for rec in data:
            if rec.rawData.startswith("leaving the network"):
                results[rec.node].append(tuple((rec.simTime//1000,False)))
                continue
            if rec.rawData.startswith("association done"):
                results[rec.node].append(tuple((rec.simTime//1000, True)))
                continue
        return results
    
    def printIngress(self):
        import matplotlib.pyplot as plt
        import io
        import base64
        tempBuffer = io.BytesIO()
        plt.clf()
        index = 2
        results = self.processIngress()
        for i in results[2:]:
            started = 0
            x = [0,0]
            for j in i:
                if j[1]:
                    time = j[0]/1000
                    plt.plot(time, index, marker="^", color="green")
                    x[0] = time
                    x[1] = (3600) #sim end without node's disconnection
                else:
                    time = j[0]/1000
                    plt.plot(time, index, marker="v", color="red")
                    x[1] = time
                    #print("X: ", x," index: ",index)                    
                    plt.plot(x,[index,index])
                    x = [0,0]
            #print("X: ", x," index: ",index)
            plt.plot(x,[index,index])
            index +=1
        plt.xlabel("Simulation Time (S)")
        plt.ylabel("Nodes")
        plt.yticks(range(2,self.metric.run.maxNodes))
        plt.savefig(tempBuffer, format = 'png')
        return base64.b64encode(tempBuffer.getvalue()).decode()         
        #plt.show()

    def getNodesPDR(self) -> dict:
        nodesStats = {}
        for n in range(self.metric.run.maxNodes):
            nodesStats[n] = {"tx": 0, "ack":0}
        data = db.query(Record).filter_by(run = self.metric.run).filter_by(recordType = "Link Stats").all()
        for rec in data:
            tx = int(rec.rawData.split()[2].split("=")[1])
            ack = int(rec.rawData.split()[3].split("=")[1])
            nodesStats[rec.node]['tx'] = nodesStats[rec.node]['tx'] + tx
            nodesStats[rec.node]['ack'] = nodesStats[rec.node]['ack'] + ack
        return nodesStats

    def getPDR(self):
        nodesStats = self.getNodesPDR()
        total = 0
        totalAck = 0
        for n in range(self.metric.run.maxNodes):
            total = total + nodesStats[n]['tx']
            totalAck = totalAck + nodesStats[n]['ack']
        return {'PDR': round((totalAck/total)*100,2), 'tx' :total, 'ack': totalAck}

class PDR(Base, MyModel):
    __tablename__ = 'pdrs'
    id = Column(Integer, primary_key=True)
    application = relationship("Application", uselist=False, back_populates="pdr")

    def __init__(self, application) -> None:
        self.application = application


    def processResults(self):
        results = [[] for x in range(self.application.metric.run.maxNodes)]
        for rec in self.application.records:
            if rec.rcv:
                results[rec.srcNode].append(True)
            else:
                results[rec.srcNode].append(False)
        return results
    
    def getGlobalPDR(self):
        totalTrue = 0
        totalFalse = 0
        from collections import Counter
        for i in self.processResults():
            node = Counter(i)
            totalTrue += node[True]
            totalFalse += node[False]
        return (round(totalTrue/(totalFalse+totalTrue)*100,2))

    def printPDR(self):
        data = {}
        results = self.processResults()
        from collections import Counter
        index = 2
        totalGlobal = 0
        trueGlobal = 0
        import io
        import base64
        import matplotlib.pyplot as plt
        plt.clf()
        for i in results[2:]: # The first is the sink node
            node = Counter(i)
            total = node[True] + node[False]
            totalGlobal += total
            trueGlobal += node[True]
            pdr = round((node[True]/total)*100,2)
            #print ("Node" , index ,"Total: " , total, " False: ", node[False])
            #print ("PDR: " , pdr,"%")
            data[index] = pdr
            index += 1
            width = 0.8
            plt.text(((index-1) - (width/3)), pdr-2, str(pdr), color="black", fontsize=8)
        #print ("PDR Global: ",round((node[True]/total)*100,2))
        tempBuffer = io.BytesIO()
        plt.bar(data.keys(),data.values(), width=width, label="PDR")
        #plt.bar_label(data.values(), padding=2)
        plt.xticks(list(data.keys()))
        plt.ylim([0, 100])
        plt.xlabel("Nodes")
        plt.ylabel("PDR (%)")
        plt.legend()
        plt.gcf().set_size_inches(8,6)
        plt.savefig(tempBuffer, format = 'png')
        return base64.b64encode(tempBuffer.getvalue()).decode() 

class Latency(Base, MyModel):
    __tablename__ = 'latencies'
    id = Column(Integer, primary_key=True)
    #application_id = Column(Integer, ForeignKey('application.id')) # The ForeignKey must be the physical ID, not the Object.id
    application = relationship("Application", uselist=False, back_populates="latency") 
    def __init__(self,application) -> None:
        self.application = application

    def getNodes(self):
        nodes = []
        for i in range(self.application.metric.run.maxNodes):
            nodes.append(list())
            #print(len(self.nodes))
        for i in self.application.records:
            if (i.rcv):
                nodes[i.srcNode].append(tuple((i.genTime//1000, i.getLatency()//1000)))
        return nodes

    def latencyMean(self):
        records = self.application.records
        from numpy import mean
        start = 2
        nodes = self.getNodes()
        for i in nodes[start:]:
            valuesNodes = []
            values = []
            for j in i:
                values.append(j[1]/1000) # Miliseconds 10 ^ -3
                valuesNodes.append(j[1]/1000) # Miliseconds 10 ^ -3
            #print("Node:" + str(start) + " Size: " + str(len(valuesNodes)) + " Mean:" + str(round(mean(valuesNodes),2)))
            start += 1
        globalMean = round(mean(values),3)
        #print("Size: " + str(len(values)) + " Mean: " + str(globalMean))
        return globalMean

    def latencyMedian(self):
        records = self.application.records
        from numpy import mean
        start = 2
        nodes = self.getNodes()
        for i in nodes[start:]:
            valuesNodes = []
            values = []
            for j in i:
                values.append(j[1]/1000) # Miliseconds 10 ^ -3
                valuesNodes.append(j[1]/1000) # Miliseconds 10 ^ -3
            #print("Node:" + str(start) + " Size: " + str(len(valuesNodes)) + " Mean:" + str(round(mean(valuesNodes),2)))
            start += 1
        globalMedian = round(median(values),3)
        #print("Size: " + str(len(values)) + " Mean: " + str(globalMean))
        return globalMedian

    def getLatencyDataByNode(self):
        myData = {}
        for i in range(2,(self.application.metric.run.maxNodes)):
            myData['n' + str(i)] = []
        for rec in self.application.records:
            if rec.rcv:
                myData['n' + str(rec.srcNode)].append(rec.getLatency()/float(1000))
        return myData

    def printLatencyByNode(self):
        import io
        import base64
        import matplotlib.pyplot as plt
        plt.clf()
        tempBuffer = io.BytesIO()
        myData = self.getLatencyDataByNode()
        labels, data = myData.keys(), myData.values()
        plt.boxplot(data)
        plt.xticks(range(1, len(labels) + 1), labels)
        plt.title("Latency (ms)")
        plt.gcf().set_size_inches(8,6)
        plt.savefig(tempBuffer, format = 'png')
        return base64.b64encode(tempBuffer.getvalue()).decode()
        #plt.show()
    
    def printLatencyByNodesPosition(self):
        from mpl_toolkits.mplot3d import Axes3D
        import matplotlib.pyplot as plt
        import io
        import base64
        from numpy import mean
        import matplotlib.cm as cm
        import numpy as np
        tempBuffer = io.BytesIO()
        plt.clf()
        nodes = []
        x = []
        y = []
        z = []
        latData = self.getLatencyDataByNode()
        for node, position in self.application.metric.run.getNodesPosition().items():
            nodes.append(node)
            x.append(position['x'])
            y.append(position['y'])
            try:
                z.append(mean(latData[node]))
            except:
                # Node 1
                z.append(0)
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        #ax.scatter(x, y, z, c='b', marker='o')
        cmap = cm.get_cmap('rainbow')
        max_height = np.max(z)   # get range of colorbars
        min_height = np.min(z)
        # scale each z to [0,1], and get their rgb values
        rgba = [cmap((k-min_height)/max_height) for k in z]
        ax.bar3d(x, y, 0, 2, 2, z, color=rgba)
        i = 0
        for label in nodes:
            ax.text(x[i],y[i],z[i], label)
            i += 1
        colourMap = plt.cm.ScalarMappable(cmap=plt.cm.rainbow)
        colourMap.set_array(z)
        colBar = plt.colorbar(colourMap).set_label('Latency')
        ax.set_xlabel('X Position')
        ax.set_ylabel('Y Position')
        ax.set_zlabel('Latency (ms)')
        ax.set_title("Nodes latency (Mean)")
        #ax.set_zlim3d(0,100)
        plt.gcf().set_size_inches(8,6)
        plt.savefig(tempBuffer, format = 'png')
        return base64.b64encode(tempBuffer.getvalue()).decode()


    def printLatency(self):
        import io
        import base64
        import matplotlib.pyplot as plt
        plt.clf()
        tempBuffer = io.BytesIO()
        nodes = self.getNodes()
        for i in range(2,self.application.metric.run.maxNodes):
            x = [a[0]//1000 for a in nodes[i]] # Seconds
            y = [round(a[1]/1000,3) for a in nodes[i]] # Seconds
            plt.plot(x, y,linestyle="",marker=".", label = "Node "+str(i))
        myMean = self.latencyMean()
        myMedian = self.latencyMedian()
        plt.axhline(y = myMean, color = 'r', linestyle = '--',label="Mean: " + str(myMean))
        plt.axhline(y = myMedian, color = 'g', linestyle = '--',label="Median: " + str(myMedian))
        plt.xlabel("Simulation Time (s)")
        plt.ylabel("Delay (s)")
        plt.legend()
        #plt.show()
        plt.gcf().set_size_inches(8,6)
        plt.savefig(tempBuffer, format = 'png')
        return base64.b64encode(tempBuffer.getvalue()).decode()

class AppRecord(Base, MyModel):
    __tablename__ = 'apprecords'
    id = Column(Integer, primary_key=True)
    genTime = Column(Integer, nullable=False)
    rcvTime = Column(Integer)
    rcv = Column(Boolean)
    srcNode = Column(Integer, nullable=False)
    dstNode = Column(Integer, nullable=False)
    sqnNumb = Column(Integer, nullable=False)
    application_id = Column(Integer, ForeignKey('application.id')) # The ForeignKey must be the physical ID, not the Object.id
    application = relationship("Application", back_populates="records")

    def __init__(self, genTime, srcNode, dstNode, sqnNumb):
        self.genTime = genTime
        self.srcNode = srcNode
        self.dstNode = dstNode
        self.sqnNumb = sqnNumb
        self.rcv = False

    def rcvPkg(self, rcvTime):
        self.rcvTime = rcvTime
        self.rcv = True

    def getLatency(self):
        return self.rcvTime - self.genTime
    

Experiment.runs = relationship("Run", order_by = Run.id, back_populates="experiment")
Run.records = relationship("Record", order_by = Record.id, back_populates="run")
Application.records = relationship("AppRecord", order_by = AppRecord.id, back_populates="application")
meta.create_all()
