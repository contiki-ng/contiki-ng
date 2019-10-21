/*
 * Communication between node-1 and node-2 is bothered by the
 * jammer. Then, node-2 shuold perform to RELOCATE its TX cells which
 * are under being jammed.
 */

var jammer = sim.getMoteWithID(4);
var num_tx_cell = 0;

var relocate_trans_count = 0;

/* this test lasts 10min (600s); GENERATE_MSG() takes a timeout value in ms */
GENERATE_MSG(600000, "test is done");

while(true) {
  YIELD();

  if(msg.equals("test is done")) {
    log.log("RELOCATE count: " + relocate_trans_count + "\n");
    if(relocate_trans_count > 0) {
      log.testOK();
    } else {
      log.testFailed();
    }
  }

  log.log(time + " node-" + id + " "+ msg + "\n");

  if(msg.indexOf("sent a RELOCATE request") !== -1) {
    relocate_trans_count += 1;
  }

  if(id === 2){
    if(msg.indexOf("sent a RELOCATE request") !== -1) {
      write(jammer, "q");
    } else if(msg.indexOf("add_link sf=2 opt=Tx type=NORMAL") !== -1) {
      num_tx_cell += 1;
      if(num_tx_cell > 1) {
        /*
         * make jammer bother communication on the second TX cell
         * scheduled by node-2
         */
        ts_ch_str = msg.replace(/^.*ts=(\d+) ch=(\d+).*$/, "$1,$2");
        write(jammer, "s" + ts_ch_str);
      }
    }
  }
}
