'''

Model class for Cooja System Log Parser

'''
from datetime import datetime
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
from sqlalchemy.sql.expression import null
from sqlalchemy.orm import class_mapper

Base = declarative_base()
DBName = "Metrics.db"    
engine = create_engine('sqlite:///' + DBName, connect_args={'check_same_thread': False}, echo = False)
Session = sessionmaker(engine)
#Base = declarative_base()

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
    logfile = Column(String (200), nullable=False)
    experimentFile = Column(String (200), nullable=False)
    #dateRun = Column(DateTime, nullable=False)
    def run(self):
        runner = Runner(self.experimentFile,self.logfile)
        runner.run()
        

    
'''
Represents an experiment's record
    Parameters:
        simTime: Simulation time of record
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
    experiment_id = Column(Integer, ForeignKey('experiments.id')) # The ForeignKey must be the phyisical ID, not the Object.id
    experiment = relationship("Experiment", back_populates="records")


Experiment.records = relationship("Record", order_by = Record.id, back_populates="experiment")
MetaData().create_all(engine)