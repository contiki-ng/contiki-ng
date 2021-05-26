TIMEOUT(3665000);

while(true) {
  log.log(time + ":" + id + ":" + msg + "\n");
  if (msg.equals('Data 60 received length 100')) {
    log.testOK();
  }
  if (msg.indexOf('parent switch:') != -1 && msg.indexOf('-> (NULL IP addr)') != -1) {
    /* root instability: should not happen in the simple test */
    log.testFailed();
  }

  YIELD();
}
