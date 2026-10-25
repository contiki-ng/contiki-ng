<?xml version="1.0" encoding="UTF-8"?>
<simconf version="2023090101">
  <simulation>
    <title>TSCH re-association with a full neighbor table</title>
    <randomseed>123456</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>
      org.contikios.cooja.radiomediums.UDGM
      <transmitting_range>50.0</transmitting_range>
      <interference_range>50.0</interference_range>
      <success_ratio_tx>1.0</success_ratio_tx>
      <success_ratio_rx>1.0</success_ratio_rx>
    </radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      org.contikios.cooja.contikimote.ContikiMoteType
      <description>RPL Classic + TSCH node</description>
      <source>[CONTIKI_DIR]/examples/6tisch/simple-node/node.c</source>
      <commands>$(MAKE) TARGET=cooja clean
    $(MAKE) -j$(CPUS) node.cooja TARGET=cooja MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC DEFINES=NBR_TABLE_CONF_MAX_NEIGHBORS=5</commands>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Battery</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiVib</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRS232</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiBeeper</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRadio</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiButton</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiPIR</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiClock</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiLED</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiCFS</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiEEPROM</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="0.000000" y="0.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>1</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="0.000000" y="40.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>2</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="5.000000" y="40.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>3</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="-5.000000" y="40.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>4</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="10.000000" y="40.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>5</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="0.000000" y="-40.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>6</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="0.000000" y="80.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>7</id>
        </interface_config>
      </mote>
    </motetype>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>/*
 * Regression test for contiki-ng issue 2943: a node whose neighbor table is
 * full can no longer associate to a TSCH network.
 *
 * Topology: the root (node 1) sits at the origin, a cluster of four nodes
 * (2 to 5) sits around, node 6 is in range of the root yet outside the
 * cluster. Node 7 is the node under test, it is in range of the cluster but not
 * the root and node 6.
 *
 * Phase 1: Node 7 fills its neighbor table (NBR_TABLE_CONF_MAX_NEIGHBORS=5).
 * After 180 seconds the table is assumed full and node 7 is moved away.
 *
 * Phase 2: node 7 is moved far away from everyone. It loses synchronization
 * and leaves the network.
 *
 * Phase 3: node 7 is moved next to node 6, its only reachable neighbor. Node
 * 6 is not in node 7's neighbor table, so associating to it requires evicting
 * one of the stale entries. The RPL Classic neighbor policy used to reject
 * every NBR_TABLE_REASON_MAC addition on a full table, which left node 7 cut
 * off from the network for good, repeatedly logging "did not associate".
 *
 * The test passes only if node 7 associates again in phase 3. */

TIMEOUT(900000, log.testFailed());

var phase = 0;
var association_failures = 0;

while(true) {
  YIELD();

  if(phase == 0) {
    // Phase 1: wait until enough simulation time has passed for node 7's
    // neighbor table to fill.
    if(time &gt;= 180000000) {
      log.log("Moving node 7 out of range (table should be full by now)\n");
      sim.getMoteWithID(7).getInterfaces().getPosition().setCoordinates(0, 1000, 0);
      phase = 1;
    }
  } else if(id == 7) {
    if(phase == 1) {
      // Phase 2: wait until node 7 has given up on its old network.
      if(msg.contains("leaving the network")) {
        log.log("Node 7 left the network, moving it into range of node 6\n");
        sim.getMoteWithID(7).getInterfaces().getPosition().setCoordinates(0, -80, 0);
        phase = 2;
      }
    } else {
      // Phase 3: node 7 has to evict a stale entry to associate to node 6.
      if(msg.contains("association done")) {
        log.log("Node 7 associated to node 6 with a full neighbor table\n");
        log.testOK();
      } else if(msg.contains("did not associate")) {
        association_failures += 1;
        if(association_failures &gt;= 10) {
          log.log("Node 7 failed to associate " + association_failures
                  + " times: the neighbor policy rejects the new time source\n");
          log.testFailed();
        }
      }
    }
  }
}</script>
      <active>true</active>
    </plugin_config>
    <bounds x="400" y="0" height="400" width="600" />
  </plugin>
</simconf>
