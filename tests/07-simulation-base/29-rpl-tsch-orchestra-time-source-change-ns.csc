<?xml version="1.0" encoding="UTF-8"?>
<simconf version="2023090101">
  <simulation>
    <title>RPL non-storing Orchestra TSCH forwarding after time-source change</title>
    <randomseed>123456</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>
      org.contikios.cooja.radiomediums.UDGM
      <transmitting_range>50.0</transmitting_range>
      <interference_range>100.0</interference_range>
      <success_ratio_tx>1.0</success_ratio_tx>
      <success_ratio_rx>1.0</success_ratio_rx>
    </radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      org.contikios.cooja.contikimote.ContikiMoteType
      <description>RPL+TSCH+Orchestra node</description>
      <source>[CONFIG_DIR]/code-orchestra-txrx/node.c</source>
      <commands>$(MAKE) TARGET=cooja clean
    $(MAKE) -j$(CPUS) node.cooja TARGET=cooja DEFINES=TEST_CONF_TX_NODE_ID=1,TEST_CONF_RX_NODE_ID=3,TEST_CONF_TX_START_SECONDS=360,TEST_CONF_REQUIRED_RX_COUNT=10</commands>
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
          <pos x="50.000000" y="50.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>1</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="50.000000" y="140.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>2</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="50.000000" y="95.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>3</id>
        </interface_config>
      </mote>
    </motetype>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>/*
 * Test for Orchestra/RPL non-storing forwarding after a time-source change.
 *
 * Phase 1: the root (node 1) is out of range of node 2. Node 3 joins the
 * root directly and relays its EBs; node 2 associates through node 3 and
 * selects it as its TSCH time source and RPL parent. Orchestra allocates a
 * Tx cell at hash(node 3) on node 2. The topology is root -- node 3 -- node 2.
 *
 * Phase 2: the root moves into range of node 2 but out of range of node 3.
 * Node 2 switches its TSCH time source and RPL parent to the root.
 * Orchestra's new_time_source() removes the Tx cell at hash(node 3) and
 * installs one at hash(root). Node 3 simultaneously loses its path to the
 * root and reparents to node 2, becoming its RPL child.
 *
 * The test then verifies that the root can still reach node 3 by forwarding
 * ten application packets through node 2. This exercises whether Orchestra
 * correctly maintains or re-installs the Tx cell needed to reach node 3 after
 * the time-source change.
 *
 * This showcases issue 3158, where the cell removed by new_time_source()
 * is never reinstated, select_packet() pins frames to timeslot hash(node 3)
 * where no Tx link exists, and the queue fills up.
 */
TIMEOUT(1200000, log.testFailed());

/* Move the root into node 2's range once the network has settled. */
GENERATE_MSG(300000, "move-root");

while(true) {
  YIELD();
  if(msg.equals("move-root")) {
    log.log("Moving root (node 1) into range of node 2\n");
    sim.getMoteWithID(1).getInterfaces().getPosition().setCoordinates(95, 140, 0);
  } else if(id == 3 &amp;&amp; msg.contains("Received all test packets")) {
    log.log("Node 3 received ten test packets\n");
    log.testOK();
  }
}</script>
      <active>true</active>
    </plugin_config>
    <bounds x="400" y="0" height="400" width="600" />
  </plugin>
</simconf>
