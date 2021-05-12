'''

Model class for Cooja System Log Parser

'''
from datetime import datetime
from re import S
import re
from types import new_class
from xml.dom import minidom
from numpy import apply_along_axis

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
        db.add(newRun)
        self.runs.append(newRun)
        db.commit()
        #db.commit()
        
class Run(Base, MyModel):
    __tablename__ = "runs"
    id = Column(Integer,primary_key=True)
    start = Column(DateTime)
    end = Column(DateTime)
    maxNodes = Column(Integer)
    experiment_id = Column(Integer, ForeignKey('experiments.id')) # The ForeignKey must be the physical ID, not the Object.id
    experiment = relationship("Experiment", back_populates="runs")
    metric = relationship("Metrics", uselist=False, back_populates="run")
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

    def __init__(self, run):
        self.run = run
        print("Self lenght:" , len(self.run.records) )
        self.application = Application(self)
        self.application.process()
        db.add(self)
        db.commit()

class Application(Base, MyModel):
    __tablename__ = 'application'
    nodes = []
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
                self.records.append(newLatRec)
                continue
            
            if rec.rawData.startswith("app receive"):
                sequence = rec.rawData.split()[3].split("=")[1]
                recTime = rec.simTime
                srcNode = int (rec.rawData.split()[4].split("=")[1].split(":")[5], 16) # Converts Hex to Dec
                for record in self.records:
                    if (record.srcNode == srcNode and record.sqnNumb == sequence):
                        record.rcvPkg(recTime)
                #print("Node: " ,  srcNode  , "Seq: " , sequence , "Receive Time: ", recTime)
                        break
        
        



class RPL(Base, MyModel):
    __tablename__ = 'rpl'
    id = Column(Integer, primary_key=True)

class TSCH(Base, MyModel):
    __tablename__ = 'tsch'
    id = Column(Integer, primary_key=True)
    def processIngress(self, run):
        data = db.query(Record).filter_by(run = run).filter_by(recordType = "TSCH").all()
        results = [[] for x in run.maxNodes]
        for rec in data:
            if rec.rawData.startswith("leaving the network"):
                results[rec.node].append(tuple((rec.simTime//1000000,False)))
                continue
            if rec.rawData.startswith("association done"):
                results[rec.node].append(tuple((rec.simTime//1000000, True)))
                continue
        import matplotlib.pyplot as plt
        index = 2
        for i in results[2:]:
            started = 0
            x = []
            for j in i:
                if j[1]:
                    time = j[0]
                    plt.plot(time, index, marker="^", color="green")
                    x.append(time)
                    x.append(3600) #sim end without disconnection
                else:
                    time = j[0]
                    plt.plot(time, index, marker="v", color="red")
                    x[1] = time
                    plt.plot(x,[index,index])
                    x = []
            plt.plot(x,[index,index])
            index +=1
        #plt.axhline(y = self.latency(), color = 'r', linestyle = '--',label="Mean")
        plt.xlabel("Simulation Time (s)")
        plt.ylabel("Nodes")
        #plt.legend()
        plt.yticks(range(2,21))
        plt.show()

        return results



class PDR(Base, MyModel):
    __tablename__ = 'pdrs'
    id = Column(Integer, primary_key=True)
    application = relationship("Application", uselist=False, back_populates="pdr")

    def __init__(self, application) -> None:
        self.application = application

    def printPDR(self, latRecords):
        data = {}
        results = [[] for x in range(21)]
        for rec in latRecords:
            if rec.rcv:
                results[rec.srcNode].append(True)
            else:
                results[rec.srcNode].append(False)
        from collections import Counter
        index = 2
        for i in results[2:]: # The first is the sink node
            node = Counter(i)
            total = node[True] + node[False]
            pdr = round((node[True]/total)*100,2)
            #print ("Node" , index ,"Total: " , total)
            #print ("PDR: " , pdr,"%")
            data[index] = pdr
            index += 1
        #print (data)
        import matplotlib.pyplot as plt
        plt.bar(data.keys(),data.values(),label="PDR")
        plt.xticks(list(data.keys()))
        plt.ylim([0, 100])
        plt.xlabel("Nodes")
        plt.ylabel("PDR (%)")
        plt.legend()
        plt.show()    
        



class Latency(Base, MyModel):
    __tablename__ = 'latencies'
    id = Column(Integer, primary_key=True)
    #application_id = Column(Integer, ForeignKey('application.id')) # The ForeignKey must be the physical ID, not the Object.id
    application = relationship("Application", uselist=False, back_populates="latency") 
    def __init__(self,application) -> None:
        self.application = application

    def latency(self):
        nodes = []
        for i in range(self.application.metric.run.maxNodes):
            nodes.append(list())
            #print(len(self.nodes))
        for i in self.application.records:
            if (i.rcv):
                nodes[i.srcNode].append(tuple((i.genTime//1000, i.getLatency()//1000)))
        records = self.application.records
        from numpy import mean
        start = 2
        for i in nodes[start:]:
            valuesNodes = []
            values = []
            for j in i:
                values.append(j[1]/1000) # Miliseconds 10 ^ -3
                valuesNodes.append(j[1]/1000) # Miliseconds 10 ^ -3
            #print("Node:" + str(start) + " Size: " + str(len(valuesNodes)) + " Mean:" + str(round(mean(valuesNodes),2)))
            start += 1
        globalMean = round(mean(values),2)
        #print("Size: " + str(len(values)) + " Mean: " + str(globalMean))
        return globalMean


    def printLatency(self):
        import matplotlib.pyplot as plt
        for i in range(2,self.application.metric.run.maxNodes):
            x = [a[0]//1000 for a in self.application.nodes[i]] # Seconds
            y = [round(a[1]/1000,3) for a in self.application.nodes[i]] # Seconds
            plt.plot(x, y,linestyle="",marker=".", label = "Node "+str(i))
        plt.axhline(y = self.latency(), color = 'r', linestyle = '--',label="Mean")
        plt.xlabel("Simulation Time (s)")
        plt.ylabel("Delay (s)")
        plt.legend()
        plt.show()


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
