# Instrumenting Contiki NG applications with energy usage estimation

> See also: the documentation of the Energest API at [doc:energest].

> See also: the performance benchmark and visualization example at [examples/benchmarks/result-visualization/run-analysis.py](https://github.com/contiki-ng/contiki-ng/blob/develop/examples/benchmarks/result-visualization/run-analysis.py)

Contiki-NG includes [Energest](https://dl.acm.org/citation.cfm?id=1278979) module which can be used to implement lightweight, software-based energy estimation approach for resource-constrained IoT devices. The Energest module accounts the time a system has spent in various states. Using this information together with a system's hardware power consumption model, the developer can estimate the energy consumption of the system.

## Using the Energest Module

Contiki-NG comes with a system service called `simple-energest`. Using it is the fastest way to get started.

To enable the `simple-energest` service, add this line to an application's `Makefile`:

    MODULES += os/services/simple-energest

## Energest Module capabilities

There are five predefined Energest states:

| Type                    | Purpose                            |
|-------------------------|------------------------------------|
|`ENERGEST_TYPE_CPU`      | The CPU is active.                 |
|`ENERGEST_TYPE_LPM`      | The CPU is in low power mode.      |
|`ENERGEST_TYPE_DEEP_LPM` | The CPU is in deep low power mode. |
|`ENERGEST_TYPE_TRANSMIT` | The radio is transmitting.         |
|`ENERGEST_TYPE_LISTEN`   | The radio is listening.            |

Most Contiki-NG platforms support tracking of all of these state. However, there are exceptions, for example, the Texas Instruments `simplelink` account all low-power modes under `ENERGEST_TYPE_LPM` because its not possible to distinguish between the low-power modes without modifying the Texas Instruments core SDK.


## Interpreting Simple Energest output

Once the Simple Energest service is enabled by setting the `MODULES` variable in the `Makefile`, it will print a summary message once per minute. An example message on an emulated Zolertia Z1 node:

    [INFO: Energest  ] --- Period summary #2 (60 seconds)
    [INFO: Energest  ] Total time  :    1966080
    [INFO: Energest  ] CPU         :      10374/   1966080 (5 permil)
    [INFO: Energest  ] LPM         :    1955706/   1966080 (994 permil)
    [INFO: Energest  ] Deep LPM    :          0/   1966080 (0 permil)
    [INFO: Energest  ] Radio Tx    :        106/   1966080 (0 permil)
    [INFO: Energest  ] Radio Rx    :     104802/   1966080 (53 permil)
    [INFO: Energest  ] Radio total :     104908/   1966080 (53 permil)

The fields are:

* `Period summary #2 (60 seconds)` - #2 is the sequence number of the accounting period i.e. since the last printout. 60 seconds is the duration of the period.
* `Total time  :    1966080` - the number here refers to the total number of ticks in the accounting period (60 seconds). As the value of the `RTIMER_ARCH_SECOND` constant on the Z1 platform is equal to 32768, we can expect 32768 multiplied by the number of seconds. 32768 * 60 = 1966080.
* `CPU         :      10374/   1966080 (5 permil)` - the first number `10374` here refers to the number of ticks spent in the CPU active mode. `5 permil` is the approximate proportion of time spent in this state. It is equal to 10374 divided by 1966080, subject to rounding errors. 1 permil is equal to 0.1 percent.

The rest of the fields follow the same template.

## Platform's current consumption model

In order to convert these ticks into useful values, a platform-specific current consumption model is necessary. In short, if you know how much the platform is consuming in each state, you can use the Energest output to convert the ticks to useful numbers.

Luckily, the Zolertia Z1 platform provides current consumption values of its main components in its [datasheet](http://zolertia.sourceforge.net/wiki/images/e/e8/Z1_RevC_Datasheet.pdf). (Caveat: datasheet values not always are trustworthy, it is better to do your own measurements!)

The low-power mode on this platform in Contiki-NG corresponds to CPU being in the standby mode (0.5 uA current consumption from the datasheet) and the various other components being in Power Down or standby modes (20 uA for radio, >2 uA for all other components together). Summing up, we can approximate that with 23 uA current consumption. The rest of modes can be taken directly from the datasheet.

| State              | Current consumption                |
|--------------------|------------------------------------|
|CPU active          | 10 mA                              |
|CPU low power mode  | 23 uA                              |
|Radio Rx            | 18.8 mA                            |
|Radio Tx            | 17.4 mA                            |


## Estimating charge consumption and energy consumption

Let's assume that these variable are already known:

* `ticks` - the number of ticks spent in a state
* `RTIMER_ARCH_SECOND` - the number of ticks per second
* `current_mA` - the current consumption in that state in mA
* `voltage` - the voltage provided by the system to the component (radio or CPU)
* `period_sec` - the duration of the accounting period in seconds
* `period_ticks` - the duration of the accounting period in period_ticks

The following metrics can now be computed for the state during the measurement period:

* **average current consumption** (in milliamperes, mA)

    `state_avg_current_mA = (ticks * current_mA) / (RTIMER_ARCH_SECOND * period_sec) = (ticks * current_mA) / period_ticks`

* **charge consumption** (in millicoulumbs, mC)

    `state_charge_mC = (ticks * current_mA) / RTIMER_ARCH_SECOND`

* **average power consumption** (in milliwats, mW)

    `state_power_mW = avg_current_mA * voltage`

* **energy consumption** (in millijoules, mJ)

     `state_energy_mJ = state_charge_mC * voltage`

or, alternatively:

     state_energy_mJ = state_power_mW * period_sec

Total values for the whole system can be obtained by simply summing the values of all tracked states.

*Tip*: that despite the name "Energest", the most useful metric is in fact the **charge consumption**.
The energy consumption depends on the voltage, which may change over time (for example, when the battery
discharges) and depends on platform-specific design decisions not under control of Contiki-NG application
developer (for example, the operating voltage, which may be different from the voltage provided by the battery).

*Tip*: it's often more useful to express charge consumption in milliampere hours (mAh), as batteries
typically report this metric. A millicoulumb (mC) is simply a milliampere second. To convert between the values:

    value_mAh = value_mC / 3600.0
    value_mC = value_mAh * 3600.0


## Code example

One code example that interprets the Simple Energest output can be found in https://github.com/contiki-ng/contiki-ng/blob/develop/examples/benchmarks/result-visualization/run-analysis.py Here we show how to construct another one.

Let's assume that you have a `COOJA.testlog` file formatted like this:

```
1107305 2 [INFO: Main      ] Starting Contiki-NG-release/v4.5-149-g1c0b472-dirty
1110081 2 [INFO: Main      ] - Routing: RPL Lite
1112715 2 [INFO: Main      ] - Net: sicslowpan
1114921 2 [INFO: Main      ] - MAC: TSCH
1118219 2 [INFO: Main      ] - 802.15.4 PANID: 0x81a5
1123248 2 [INFO: Main      ] - 802.15.4 TSCH default hopping sequence length: 4
1125408 2 [INFO: Main      ] Node ID: 2
```

Here is a Python code that parses the log output, computes the summary metrics for each node and prints them:
```python
INPUT_FILE = "COOJA.testlog"

# From Z1 node datasheet
CURRENT_MA = {
    "CPU" : 10,
    "LPM" : 0.023,
    "Deep LPM" : 0, # not used by Z1 nodes
    "Radio Rx" : 18.8,
    "Radio Tx" : 17.4,
}

STATES = list(CURRENT_MA.keys())

VOLTAGE = 3.0 # assume 3 volt batteries
RTIMER_ARCH_SECOND = 32768

def main():
    node_ticks = {}
    node_total_ticks = {}
    
    with open(INPUT_FILE, "r") as f:
        for line in f:
            if "INFO: Energest" not in line:
                continue
            fields = line.split()
            try:
                node = int(fields[1])
            except:
                continue

            if node not in node_ticks:
                # initialize to zero
                node_ticks[node] = { u : 0  for u in STATES }
                node_total_ticks[node] = 0

            try:
                state_index = 5
                state = fields[state_index]
                tick_index = state_index + 2
                if state not in STATES:
                    state = fields[state_index] + " " + fields[state_index+1]
                    tick_index += 1
                    if state not in STATES:
                        # add to the total time
                        if state == "Total time":
                            node_total_ticks[node] += int(fields[tick_index])
                        continue
                # add to the time spent in specific state
                ticks = int(fields[tick_index][:-1])
                node_ticks[node][state] += ticks
            except Exception as ex:
                print("Failed to process line '{}': {}".format(line, ex))

    nodes = sorted(node_ticks.keys())
    for node in nodes:
        total_avg_current_mA = 0
        period_ticks = node_total_ticks[node]
        period_seconds = period_ticks / RTIMER_ARCH_SECOND
        for state in STATES:
            ticks = node_ticks[node].get(state, 0)
            current_mA = CURRENT_MA[state]
            state_avg_current_mA = ticks * current_mA / period_ticks
            total_avg_current_mA += state_avg_current_mA
        total_charge_mC = period_ticks * total_avg_current_mA / RTIMER_ARCH_SECOND
        total_energy_mJ = total_charge_mC * VOLTAGE
        print("Node {}: {:.2f} mC ({:.3f} mAh) charge consumption, {:.2f} mJ energy consumption in {:.2f} seconds".format(
            node, total_charge_mC, total_charge_mC / 3600.0, total_energy_mJ, period_seconds))

if __name__ == "__main__":
    main()
```


Example output:
```
Node 1: 3917.93 mC (1.088 mAh) charge consumption, 11753.79 mJ energy consumption in 3599.99 seconds
Node 2: 5699.00 mC (1.583 mAh) charge consumption, 17097.01 mJ energy consumption in 3599.99 seconds
```


The code can be easily adapted to log files generated by real nodes in a testbed.


[doc:energest]: /doc/programming/Energest

