TIMEOUT(600000);

while(true) {
  YIELD();

  if(msg.contains("RPL Option Error: Dropping Packet")) {
    log.log(time + " " + "node-" + id + " "+ msg + "\n");
    log.testFailed();
  }

  if(msg.contains("Received all packets")) {
    log.log(time + " " + "node-" + id + " "+ msg + "\n");
    log.testOK();
  }
}
