/* A simple log file generator script */

TIMEOUT(300000); /* 300 seconds */

log.log("Starting COOJA logger\n");

timeout_function = function () {
    log.log("Script timed out.\n");
    log.testOK();
}

while (true) {
    if (msg) {
        log.log(time + " " + id + " " + msg + "\n");
    }

    YIELD();
}
