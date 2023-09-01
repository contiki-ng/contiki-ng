<?xml version="1.0" encoding="UTF-8"?>
<simconf version="2022112801">
  <simulation>
    <title>Clock drift test</title>
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
      org.contikios.cooja.mspmote.Z1MoteType
      <description>Z1 Mote Type #z11</description>
      <source>[CONTIKI_DIR]/examples/6tisch/simple-node/node.c</source>
      <commands>$(MAKE) TARGET=z1 clean
      $(MAKE) -j$(CPUS) node.z1 TARGET=z1</commands>
      <firmware>[CONTIKI_DIR]/examples/6tisch/simple-node/node.z1</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.Msp802154Radio</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDefaultSerial</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspLED</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="-1.285769821276336" y="38.58045647334346" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspClock
          <deviation>0.9999975</deviation>
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>1</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="10.0" y="50.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
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
      <filter>ID:2</filter>
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
      <zoomfactor>1000.0</zoomfactor>
    </plugin_config>
    <bounds x="0" y="412" height="311" width="1304" z="1" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>TIMEOUT(600000); /* Time out after 10 minutes */&#xD;
/* Wait until TSCH drift is printed */&#xD;
log.log("Waiting for TSCH drift to be detected\n");&#xD;
counter = 0;&#xD;
while(true) {;&#xD;
  WAIT_UNTIL(msg.contains("drift"));&#xD;
  log.log(msg + "\n");&#xD;
  if(msg.contains("drift 2 ppm")) {&#xD;
    if(++counter &gt;= 5) {;&#xD;
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
