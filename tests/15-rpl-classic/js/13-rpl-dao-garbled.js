TIMEOUT(300000);

forwarding_error_occurred = false;
no_path_dao_received = false;

while(true) {
  YIELD();

  log.log(time + " " + "node-" + id + " "+ msg + "\n");

  // Verify the error-prone situation has occurred 
  if(id == 2 && msg.contains("RPL forwarding error")) {
    forwarding_error_occurred = true;
  }

  else if(id == 1) {
    // Fail if garbled DAO is received
    if(msg.contains("icmpv6 bad checksum")) {
      // FIXME Logic inverted to showcase issue. Revert when implementing fix.
      log.testOK();
      log.testFailed();
    }

    // Verify a no-path DAO has been received and parsed
    if(msg.contains(
        "DAO lifetime: 0, prefix length: 128 prefix: fd00::203:3:3:3")) {
      no_path_dao_received = true;
    }

    // Stop when the route has been removed
    else if(msg.contains("No more routes to fd00::203:3:3:3")) {
      // Route has been removed, verify it was preceded as expected
      if(forwarding_error_occurred && no_path_dao_received) {
        // FIXME Logic inverted to showcase issue. Revert when implementing fix.
        log.testFailed();
      }

      log.log("Forwarding error occurred: " + forwarding_error_occurred + "\n")
      log.log("No-path DAO received: " + no_path_dao_received + "\n")
      // FIXME Logic inverted to showcase issue. Revert when implementing fix.
      log.testOK();
    }
  }
}
