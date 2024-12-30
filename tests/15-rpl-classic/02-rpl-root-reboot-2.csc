<?xml version="1.0" encoding="UTF-8"?>
<simconf version="2022112801">
  <simulation>
    <title>My simulation</title>
    <speedlimit>10.0</speedlimit>
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
      <description>mote</description>
      <source>[CONTIKI_DIR]/examples/libs/shell/example.c</source>
      <commands>$(MAKE) TARGET=cooja clean
$(MAKE) example.cooja TARGET=cooja MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC -j</commands>
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
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiEEPROM</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="50.0" y="0.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>3</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="100.0" y="0.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>2</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="150.0" y="0.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>1</id>
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiRS232
          <history>ip-addr~;routes~;routing~;</history>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="0.0" y="0.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>4</id>
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
      <skin>org.contikios.cooja.plugins.skins.MoteTypeVisualizerSkin</skin>
      <viewport>0.9090909090909091 0.0 0.0 0.9090909090909091 125.81818181818181 173.0</viewport>
    </plugin_config>
    <bounds x="1" y="1" height="400" width="400" z="3" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.LogListener
    <plugin_config>
      <filter />
      <formatted_time />
      <coloring />
    </plugin_config>
    <bounds x="400" y="160" height="784" width="604" z="2" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Notes
    <plugin_config>
      <notes>Enter notes here</notes>
      <decorations>true</decorations>
    </plugin_config>
    <bounds x="680" y="0" height="160" width="944" z="1" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>var last_msg = 0;
var root_id = 4;
var ip_addresses = [];
var rpl_done = 60000; // 1 minute - expected time for all motes to be part of rpl instance

function MY_GENERATE_MSG(wait, string)
{
    last_msg += wait;
    GENERATE_MSG(last_msg, string);
}

/* All motes starts from scratch */
MY_GENERATE_MSG(1000, "set root");
MY_GENERATE_MSG(rpl_done, "get ip addresses");
MY_GENERATE_MSG(1000, "check ip addresses");

/* Root restarts and acts as root before rejoining existing rpl instance */
MY_GENERATE_MSG(1000, "remove root " + root_id);
MY_GENERATE_MSG(1000, "add root " + root_id);
MY_GENERATE_MSG(1000, "set root");
MY_GENERATE_MSG(rpl_done, "get ip addresses");
MY_GENERATE_MSG(1000, "check ip addresses");
/* Set root 2nd time should be ignored */
MY_GENERATE_MSG(100, "set root");
MY_GENERATE_MSG(1000, "get ip addresses");
MY_GENERATE_MSG(1000, "check ip addresses");

/* Root restarts and rejoins existing rpl instance */
MY_GENERATE_MSG(1000, "remove root " + root_id);
MY_GENERATE_MSG(1000, "add root " + root_id);
MY_GENERATE_MSG(rpl_done, "get ip addresses");
MY_GENERATE_MSG(1000, "check ip addresses");
/* Root starts acting as root again */
MY_GENERATE_MSG(1000, "set root");
MY_GENERATE_MSG(rpl_done, "get ip addresses");
MY_GENERATE_MSG(1000, "check ip addresses");

/* Sink test */

/* End test */
MY_GENERATE_MSG(1000, "end test");

while(true) {
    YIELD();
    if(msg.equals("set root")) {
        m = sim.getMoteWithID(root_id);
        write(m, "rpl-set-root 1");
        log.log("root set\n");
    } else if(msg.contains("get ip addresses")) {
        var motes = sim.getMotes();
        for(var i in motes) {
			write(motes[i], "ip-addr");
        }
    } else if(msg.contains("-- fd00::20")) {
		ip_addresses.push(msg);
    } else if(msg.startsWith("check ip addresses")) {
        var motes = sim.getMotes();
        var motes_found = [];
        for(var i in motes) {
			var ip_address = "fd00::" + (motes[i].getID() + 200);
			for(var j in ip_addresses) {
				if(ip_addresses[j].contains(ip_address)) {
					motes_found.push(motes[i].getID());
					break;
				}
			}
        }
		if(motes.length == motes_found.length) {
			log.log("all motes are in rpl\n");
		} else {
			log.log("some motes is missing, found:\n" + motes_found + "\n");
			if(!msg.contains("skip fail")) {
				log.testFailed(); /* Report test failure and quit */
			}
		}
        ip_addresses = [];
    } else if(msg.startsWith("remove root")) {
		//TODO: Use id from generated msg
        m = sim.getMoteWithID(root_id);
        sim.removeMote(m);
        log.log("root removed\n");
		//TODO: Save type and position, so it can be used in add
    } else if(msg.startsWith("add root")) {
		//TODO: Use id from generated msg
		//TODO: Use saved type and position
        m = sim.getMoteTypes()[0].generateMote(sim);
        m.getInterfaces().getMoteID().setMoteID(root_id);
        sim.addMote(m);
        log.log("root added\n");
    } else if(msg.equals("end test")) {
      break;
    }
}

log.testOK(); /* Report test success and quit */</script>
      <active>true</active>
    </plugin_config>
    <bounds x="1007" y="160" height="785" width="617" />
  </plugin>
</simconf>
