// we want to see ten DIOs by the root at least, tweleve at most,
// during the test of 60 seconds when RPL_DIO_INTERVAL_MIN is set to
// 12 (4 seconds)
var node_id_of_root = 1
var expected_num_of_dios = 10;
var num_of_dios = 0;

TIMEOUT(60000, log.testFailed()); // ms

while(true) {
  YIELD();

  log.log(time + " node-" + id + " "+ msg + "\n");

  if(id == node_id_of_root) {
    if(msg.contains('Sending a multicast-DIO')) {
      num_of_dios++;
    }

    if(num_of_dios >= expected_num_of_dios) {
      log.testOK();
      break;
    }
  }
}
