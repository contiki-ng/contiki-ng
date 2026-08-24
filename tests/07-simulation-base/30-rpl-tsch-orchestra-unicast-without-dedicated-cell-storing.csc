<?xml version="1.0" encoding="UTF-8"?>
<simconf version="2023090101">
  <simulation>
    <title>RPL storing mode Orchestra TSCH unicast to neighbour without dedicated cell</title>
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
$(MAKE) -j$(CPUS) node.cooja TARGET=cooja MAKE_WITH_STORING_ROUTING=1 DEFINES=TEST_CONF_TX_NODE_ID=2,TEST_CONF_RX_NODE_ID=3,TEST_CONF_USE_LINKLOCAL=1,TEST_CONF_TX_START_SECONDS=180,TEST_CONF_TX_INTERVAL_SECONDS=10,TEST_CONF_REQUIRED_RX_COUNT=10</commands>
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
          <pos x="50.000000" y="80.000000" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>2</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="50.000000" y="20.000000" />
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
 * Simulation of a unicast transmission to a link-local-reachable neighbour
 * for which no dedicated TSCH unicast cell exists.
 *
 * Node 2 and node 3 both join the DODAG as direct children of the root while
 * out of range of each other, so neither can become the other's RPL parent.
 * Node 3 is then moved next to node 2. The two are now neighbours and
 * link-local reachable, but remain siblings: neither is the other's RPL
 * parent or child, and neither holds a unicast cell dedicated to the other.
 *
 * Node 2 then sends application packets directly to node 3's link-local
 * address. Such link-local unicasts bypass RPL routing and are sent as
 * layer-2 unicasts straight to the neighbour.
 * The same situation arises in normal RPL operation whenever a node sends
 * a unicast DIO probe to a link-local-reachable neighbour.
 *
 * This showcases issue 3158 where Orchestra selects a dedicated unicast slot
 * that is not present in the schedule. The frames pile up in the TSCH queue
 * and are never transmitted. Correct behaviour is to use the common slotframe.
 */
TIMEOUT(560000, log.testFailed());

GENERATE_MSG(120000, "move-node-3");

while(true) {
  YIELD();

  if(msg.equals("move-node-3")) {
    log.log("Moving node 3 into range of its sibling node 2\n");
    sim.getMoteWithID(3).getInterfaces().getPosition().setCoordinates(80, 50, 0);
  }

  if(id == 3 &amp;&amp; msg.contains("Received all test packets")) {
    log.log("Node 3 received all test packets from its sibling node 2\n");
    log.testOK();
  }
}</script>
      <active>true</active>
    </plugin_config>
    <bounds x="400" y="0" height="400" width="600" />
  </plugin>
</simconf>
