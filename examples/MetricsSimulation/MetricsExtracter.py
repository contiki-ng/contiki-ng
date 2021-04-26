
from Model import *
import re


def createNew():
    exper = Experiment()
    exper.name = "My Cooja sim"
    exper.parameters = "None"
    exper.experimentFile = "Metrics.csc"
    db.add(exper)
    db.commit()

def runExperimentByName(name):
    exper = db.query(Experiment).filter_by(name = name).first()
    print ("Name:" + exper.name + "\n\tFile: " + exper.experimentFile)
    exper.run()
    #session.save(exper)
    #session.commit()


def processRun():
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
    print(simTime)
