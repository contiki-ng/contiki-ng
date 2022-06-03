// Automatically fail after given time
TIMEOUT(10000, log.testFailed());

// When 'true' the whole output from motes is printed.
// Otherwise, only test summaries are logged.
var motes_output = true;

var failed = 0;
var succeeded = 0;

while(true) {
    YIELD();

    if (motes_output)
        log.log(time + " " + "node-" + id + " "+ msg + "\n");

    if(msg.contains("[=check-me=]") == false)
        continue;

    if (!motes_output)
        log.log(msg.substring(12) + "\n")

    if(msg.contains("FAILED"))
        failed += 1;

    if(msg.contains("SUCCEEDED"))
        succeeded += 1;

    if(msg.contains("DONE"))
        break;
}

log.log("\nFINISHED RUNNING " + (failed+succeeded) + " TEST CASES\n");
log.log("Fails: " + failed + ", Successes: " + succeeded + "\n");

if(failed != 1) // This is an example, one failure is expected.
    log.testFailed();

log.testOK();
