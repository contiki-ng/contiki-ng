< ? xml version = "1.0" encoding = "UTF-8" ? >
  < simconf >
  < project EXPORT = "discard" >[APPS_DIR] / mrm < / project >
  < project EXPORT = "discard" >[APPS_DIR] / mspsim < / project >
  < project EXPORT = "discard" >[APPS_DIR] / avrora < / project >
  < project EXPORT = "discard" >[APPS_DIR] / serial_socket < / project >
  < project EXPORT = "discard" >[APPS_DIR] / powertracker < / project >
  < simulation >
  < title > edhoc - tests < / title >
  < speedlimit > 1.0 < / speedlimit >
  < randomseed > 123456 < / randomseed >
  < motedelay_us > 1000000 < / motedelay_us >
  < radiomedium >
  org.contikios.cooja.radiomediums.UDGM
  < transmitting_range > 50.0 < / transmitting_range >
  < interference_range > 100.0 < / interference_range >
  < success_ratio_tx > 1.0 < / success_ratio_tx >
  < success_ratio_rx > 1.0 < / success_ratio_rx >
  < / radiomedium >
  < events >
  < logoutput > 40000 < / logoutput >
  < / events >
  < motetype >
  org.contikios.cooja.contikimote.ContikiMoteType
  < identifier > mtype897 < / identifier >
  < description > EDHOC Server < / description >
  < source >[CONTIKI_DIR] / examples / edhoc - tests / edhoc - server / edhoc - test - server.c < / source >
  < commands > make edhoc - test - server.cooja TARGET = cooja < / commands >
  < moteinterface > org.contikios.cooja.interfaces.Position < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.Battery < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiVib < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiMoteID < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiRS232 < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiBeeper < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.RimeAddress < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiIPAddress < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiRadio < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiButton < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiPIR < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiClock < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiLED < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiCFS < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiEEPROM < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.Mote2MoteRelations < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.MoteAttributes < / moteinterface >
  < symbols > false < / symbols >
  < / motetype >
  < motetype >
  org.contikios.cooja.contikimote.ContikiMoteType
  < identifier > mtype229 < / identifier >
  < description > EDHOC Client < / description >
  < source >[CONTIKI_DIR] / examples / edhoc - tests / edhoc - client / edhoc - test - client.c < / source >
  < commands > make edhoc - test - client.cooja TARGET = cooja < / commands >
  < moteinterface > org.contikios.cooja.interfaces.Position < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.Battery < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiVib < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiMoteID < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiRS232 < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiBeeper < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.RimeAddress < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiIPAddress < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiRadio < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiButton < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiPIR < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiClock < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiLED < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiCFS < / moteinterface >
  < moteinterface > org.contikios.cooja.contikimote.interfaces.ContikiEEPROM < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.Mote2MoteRelations < / moteinterface >
  < moteinterface > org.contikios.cooja.interfaces.MoteAttributes < / moteinterface >
  < symbols > false < / symbols >
  < / motetype >
  < mote >
  < interface_config >
  org.contikios.cooja.interfaces.Position
  < x > 25.4877963009625 < / x >
  < y > 55.70291070606443 < / y >
  < z > 0.0 < / z >
  < / interface_config >
  < interface_config >
  org.contikios.cooja.contikimote.interfaces.ContikiMoteID
  < id > 1 < / id >
  < / interface_config >
  < interface_config >
  org.contikios.cooja.contikimote.interfaces.ContikiRadio
  < bitrate > 250.0 < / bitrate >
  < / interface_config >
  < interface_config >
  org.contikios.cooja.contikimote.interfaces.ContikiEEPROM
  < eeprom > AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA == < / eeprom >
  < / interface_config >
  < motetype_identifier > mtype897 < / motetype_identifier >
  < / mote >
  < mote >
  < interface_config >
  org.contikios.cooja.interfaces.Position
  < x > 65.11725744948401 < / x >
  < y > 55.614872629811714 < / y >
  < z > 0.0 < / z >
  < / interface_config >
  < interface_config >
  org.contikios.cooja.contikimote.interfaces.ContikiMoteID
  < id > 2 < / id >
  < / interface_config >
  < interface_config >
  org.contikios.cooja.contikimote.interfaces.ContikiRadio
  < bitrate > 250.0 < / bitrate >
  < / interface_config >
  < interface_config >
  org.contikios.cooja.contikimote.interfaces.ContikiEEPROM
  < eeprom > AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA == < / eeprom >
  < / interface_config >
  < motetype_identifier > mtype229 < / motetype_identifier >
  < / mote >
  < / simulation >
  < plugin >
  org.contikios.cooja.plugins.SimControl
  < width > 280 < / width >
  < z > 0 < / z >
  < height > 160 < / height >
  < location_x > 400 < / location_x >
  < location_y > 0 < / location_y >
  < / plugin >
  < plugin >
  org.contikios.cooja.plugins.Visualizer
  < plugin_config >
  < moterelations > true < / moterelations >
  < skin > org.contikios.cooja.plugins.skins.IDVisualizerSkin < / skin >
  < skin > org.contikios.cooja.plugins.skins.GridVisualizerSkin < / skin >
  < skin > org.contikios.cooja.plugins.skins.TrafficVisualizerSkin < / skin >
  < skin > org.contikios.cooja.plugins.skins.UDGMVisualizerSkin < / skin >
  < viewport > 4.675126078004704 0.0 0.0 4.675126078004704 - 39.79502479386521 - 80.21233590961599 < / viewport >
  < / plugin_config >
  < width > 400 < / width >
  < z > 2 < / z >
  < height > 400 < / height >
  < location_x > 1 < / location_x >
  < location_y > 1 < / location_y >
  < / plugin >
  < plugin >
  org.contikios.cooja.plugins.LogListener
  < plugin_config >
  < filter / >
  < formatted_time / >
  < coloring / >
  < / plugin_config >
  < width > 681 < / width >
  < z > 5 < / z >
  < height > 240 < / height >
  < location_x > 400 < / location_x >
  < location_y > 160 < / location_y >
  < / plugin >
  < plugin >
  org.contikios.cooja.plugins.TimeLine
  < plugin_config >
  < mote > 0 < / mote >
  < mote > 1 < / mote >
  < showRadioRXTX / >
  < showRadioHW / >
  < showLEDs / >
  < zoomfactor > 500.0 < / zoomfactor >
  < / plugin_config >
  < width > 1081 < / width >
  < z > 4 < / z >
  < height > 166 < / height >
  < location_x > 0 < / location_x >
  < location_y > 829 < / location_y >
  < / plugin >
  < plugin >
  org.contikios.cooja.plugins.Notes
  < plugin_config >
  < notes > Enter notes here < / notes >
  < decorations > true < / decorations >
  < / plugin_config >
  < width > 401 < / width >
  < z > 3 < / z >
  < height > 160 < / height >
  < location_x > 680 < / location_x >
  < location_y > 0 < / location_y >
  < / plugin >
  < plugin >
  org.contikios.cooja.serialsocket.SerialSocketServer
  < mote_arg > 0 < / mote_arg >
  < plugin_config >
  < port > 60001 < / port >
  < bound > true < / bound >
  < / plugin_config >
  < width > 362 < / width >
  < z > 1 < / z >
  < height > 116 < / height >
  < location_x > 8 < / location_x >
  < location_y > 404 < / location_y >
  < / plugin >
  < / simconf>
