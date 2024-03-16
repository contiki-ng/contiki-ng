TIMEOUT(50000);

var done = 0;

while(done < sim.getMotes().length) {
    YIELD();

    log.log(time + " " + "node-" + id + " "+ msg + "\n");

    if(msg.contains("=check-me=") == false) {
        continue;
    }

    if(msg.contains("FAILED")) {
        log.testFailed();
    }

    if(msg.contains("DONE")) {
        done++;
    }
}
log.testOK();
