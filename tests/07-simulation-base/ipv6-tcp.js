TIMEOUT(300000);

while (true) {
  log.log(time + ":" + id + ":" + msg + "\n");
  if (msg.indexOf('Test OK') != -1) {
    log.testOK();
  }

  YIELD();
}
