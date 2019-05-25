TIMEOUT(25000, log.testFailed());

dst_lla = "fe80::202:2:2:2";
dst_ula = "fd00::202:2:2:2";
dst_mac = "0002.0002.0002.0002";
step = 0;
rpl_is_enabled = false;

while(1) {
  YIELD();
  log.log(time + " " + id + " "+ msg + "\n");

  if(msg.contains("Node ID: ")) {
    if(id == 1) {
      write(sim.getMoteWithID(1), "rpl-set-root 1");
    }
    step += 1;
  }

  if(msg.contains("Setting as DAG root")) {
    rpl_is_enabled = true;
  }

  if(step == 2 && time > 20000000) {
    write(sim.getMoteWithID(1), "ping " + dst_lla);
    step += 1;
  }

  if(step == 4 && time > 20000000) {
    write(sim.getMoteWithID(1), "ping " + dst_ula);
    step += 1;
  }

  if(msg.contains("Received ping reply")) {
    if(step == 3) {
      step += 1;
    } else {
      step += 1;
      write(sim.getMoteWithID(1), "ip-nbr");
    }
  }

  if(step == 6 && rpl_is_enabled) {
    /* when RPL is enabled, we skip examining ip-nbr results */
    log.testOK();
  }

  if(msg.contains("<->")) {
    re = /-- | <-> |, router|, state /;
    nc = msg.split(re);
    ip_addr = nc[1];
    ll_addr = nc[2];
    is_router = nc[3];
    state = nc[4].trim();
    if(ll_addr == dst_mac &&
       state == "Reachable") {
      if(step == 6 && ip_addr == dst_lla) {
        step += 1;
      } else if(step == 7 && ip_addr == dst_ula) {
        log.testOK();
      } else {
        /* unexpected case */
        log.testFailed();
      }
    } else {
      log.log(ip_addr + "\n");
      log.log(ll_addr + "\n");
      log.log(state + "\n");
      log.testFailed();
    }
  }
}
