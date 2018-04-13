TIMEOUT(100000);

/* This script checks that the stack usage is dynamically changing */

var re = /stack usage: (\d+)/i;

var minusage = 10000;
var maxusage = 0;

while(true) {
  log.log("> " + msg + "\n");

  var found = msg.match(re);

  if(found) {
    var n = parseInt(found[1]);
    minusage = minusage < n ? minusage : n;
    maxusage = maxusage > n ? maxusage : n;

    if(minusage < 800 && maxusage >= 1000) {
      log.testOK();
    }
  }
  YIELD();
}
