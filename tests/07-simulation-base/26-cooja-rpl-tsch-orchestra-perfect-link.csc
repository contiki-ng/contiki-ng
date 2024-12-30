<?xml version="1.0" encoding="UTF-8"?>
<simconf version="2022112801">
  <simulation>
    <title>RPL+TSCH</title>
    <randomseed>1</randomseed>
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
      <description>RPL/TSCH Node</description>
      <source>[CONTIKI_DIR]/examples/6tisch/simple-node/node.c</source>
      <commands>$(MAKE) TARGET=cooja clean
$(MAKE) -j$(CPUS) node.cooja TARGET=cooja MAKE_WITH_ORCHESTRA=1 MAKE_WITH_STORING_ROUTING=1 MAKE_WITH_LINK_BASED_ORCHESTRA=1 DEFINES=LINK_STATS_CONF_PACKET_COUNTERS=1,TSCH_CONF_KEEPALIVE_TIMEOUT=1024,TSCH_CONF_MAX_KEEPALIVE_TIMEOUT=1024,ORCHESTRA_CONF_UNICAST_MIN_CHANNEL_OFFSET=2,ORCHESTRA_CONF_EB_MIN_CHANNEL_OFFSET=1,ORCHESTRA_CONF_EB_MAX_CHANNEL_OFFSET=1</commands>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Battery</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiVib</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRS232</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiBeeper</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRadio</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiButton</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiPIR</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiClock</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiLED</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiCFS</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="-1.285769821276336" y="38.58045647334346" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>1</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="-19.324109516886306" y="76.23135780254927" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>2</id>
        </interface_config>
      </mote>
    </motetype>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.Visualizer
    <plugin_config>
      <moterelations>true</moterelations>
      <skin>org.contikios.cooja.plugins.skins.IDVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.GridVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.TrafficVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.UDGMVisualizerSkin</skin>
      <viewport>1.7405603810040515 0.0 0.0 1.7405603810040515 47.95980153208088 -42.576134155447555</viewport>
    </plugin_config>
    <bounds x="1" y="1" height="230" width="236" z="3" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.LogListener
    <plugin_config>
      <filter />
      <formatted_time />
      <coloring />
    </plugin_config>
    <bounds x="273" y="6" height="394" width="1031" z="2" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.TimeLine
    <plugin_config>
      <mote>0</mote>
      <mote>1</mote>
      <showRadioRXTX />
      <showRadioChannels />
      <showRadioHW />
      <zoomfactor>16529.88882215865</zoomfactor>
    </plugin_config>
    <bounds x="0" y="412" height="311" width="1304" z="1" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>TIMEOUT(1860000); /* Time out after 31 minutes */&#xD;
while(true) {;&#xD;
  WAIT_UNTIL(id == 2 &amp;&amp; msg.contains("Link Stats"));&#xD;
  log.log(msg + "\n");&#xD;
  fields = msg.split(" ");&#xD;
  if(fields.length &gt;= 9) {&#xD;
    tx = parseInt(fields[5].split("=")[1]);&#xD;
    ack = parseInt(fields[6].split("=")[1]);&#xD;
    if(tx &gt; 0 &amp;&amp; tx == ack) {&#xD;
      log.testOK(); /* Report test success and quit */&#xD;
    }&#xD;
  }&#xD;
  YIELD();&#xD;
}</script>
      <active>true</active>
    </plugin_config>
    <bounds x="963" y="111" height="995" width="764" />
  </plugin>
</simconf>
