;TIMEOUT(25000, log.testFailed());

dst_lla = "fe80::202:2:2:2";
dst_ula = "fd00::202:2:2:2";
dst_mac = "0002.0002.0002.0002";
step = 0;

while(1) {
  YIELD();
  log.log(time + " " + id + " "+ msg + "\n");

  if(msg.contains("Node ID: ")) {
    if(id == 1) {
      write(sim.getMoteWithID(1), "rpl-set-root 1");
    }
    step += 1;
  }

  if(step == 2 && time > 20000000) {
    write(sim.getMoteWithID(1), "ping " + dst_ula);
    step += 1;
  }

  if(msg.contains("Received ping reply")) {
    write(sim.getMoteWithID(1), "ip-nbr");
  }

  if(msg.contains("<->")) {
    re = /-- | <-> |, router|, state /;
    nc = msg.split(re);
    ip_addr = nc[1];
    ll_addr = nc[2];
    is_router = nc[3];
    state = nc[4].trim();
    /* in RPL case, nbr doesn't have ula. */
    if((ip_addr == dst_lla || ip_addr == dst_ula) &&
       ll_addr == dst_mac &&
       state == "Reachable") {
      log.testOK();
    } else {
      log.log(ip_addr + "\n");
      log.log(ll_addr + "\n");
      log.log(state + "\n");
      log.testFailed();
    }
  }
}
