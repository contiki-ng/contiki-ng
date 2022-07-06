# benchmarks/result-visualization

Result visualization
--------------------

This example shows Contiki-NG performance visualization using both Cooja and testbed runs.

The following metrics are visualized:

* End-to-end Packet Delivery Ratio (PDR) — an application layer metric.
* The number of routing parent changes — a network layer metric.
* Packet Acknowledgement Rate from the routing parent — a link layer metric.
* Radio Duty Cycle (RDC) — a PHY layer metric.
* Charge consumption — a hardware-specific metric. In this example,
  it is calculated and visualized for the CC2650 System-on-Chip

Other metrics, for example, channel occupancy rate can be easily added by extracting
the additional information from the log file and plotting it.


The Cooja approach
------------------

The core idea of this approach is to run a simulation, get a log file,
extract metrics from that log file, and plot the metrics.

The steps are automated in the script `run-cooja.py`.
The script requires Python 3 and assumes that Cooja has lareayd been build.

When the `run-cooja.py` script is executed, it executes a Cooja simulation
using the simulation file `cooja.csc` and control file `coojalogger.js`.

The results are saved in the log file called `COOJA.testlog`.


The testbed approach
--------------------

We assume that the FIT IoT-Lab testbed is used.

The steps are not fully automated in this case, and these actions must be done manually:

1. The FIT IoT-Lab platform's support must be set up locally.
   Follow the tutorial https://www.iot-lab.info/legacy/tutorials/contiki-ng-compilation/index.html
   In short, run these command:
```
   git clone https://github.com/iot-lab/iot-lab.git
   cd iot-lab
   make setup-iot-lab-contiki-ng
```
2. Binary image of the test application must be built for the IoT-lab platform.
   Go to an example directory and run these commands, replacing PATH_TO_IOTLAB with the path in your system:
```
export ARCH_PATH=PATH_TO_IOTLAB/iot-lab/parts/iot-lab-contiki-ng/arch/
make TARGET=iotlab BOARD=m3 savetarget
make -j
```
3. Get FIT IoT-Lab ssh access and copy the binary image to a FIT IoT-Lab, for example, Grenoble.
   Follow the tutorial https://www.iot-lab.info/legacy/tutorials/ssh-access/index.html
   Then, run these commands
 ```
# copy to the FIT IoT lab server; change the login name from `elsts` to your own FIT IoT login
scp node.iotlab elsts@grenoble.iot-lab.info:
# login to FIT IoT lab server; change the login name from `elsts` to your own FIT IoT login
ssh elsts@grenoble.iot-lab.info
```
3. Nodes must be reserved in the FIT IoT-Lab testbed and an experiment started.
   For example, you can reserve these nodes at the Grenoble site:
```
119+123+127+131+135+139+143+147+151+156+159+163+167+177+179+181+183+185+188+190+193+195+197+199+200+203+205+207+211+215+219
```
4. The application image must be programmed on the nodes that are part of the experiment, and
   serial logger must be started on a FIT IoT-Lab testbed server.
   Run these commands on the server:
```
# set duration
export d=61
# set nodes to run on
export nodes='119+123+127+131+135+139+143+147+151+156+159+163+167+177+179+181+183+185+188+190+193+195+197+199+200+203+205+207+211+215+219'

export logfile=testbed.log

iotlab-experiment submit -n a2941 -d $d -l grenoble,m3,$nodes;
iotlab-experiment wait;
iotlab-node --update node.iotlab
serial_aggregator > $logfile
```
6. The collated log file must be downloaded from FIT IoT-Lab and saved as `testbed.log`.
```
scp elsts@grenoble.iot-lab.info:testbed.log .
```


Analyzing and plotting the results
----------------------------------

Once the log file has been ontained, the remaining steps are automated by the script `run-analysis.py`.

* For COOJA results, call `./run-analysis.py COOJA.testlog`
* For testbed results, call `./run-analysis.py testbed.log`

The script:

1. Extracts various metrics from the log file.
2. Plots the metrics using the matplotlib Python library and saves them to `.pdf` files.
