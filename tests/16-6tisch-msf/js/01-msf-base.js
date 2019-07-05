/*
 * Four nodes build a linear topology. The leaf node and its parent
 * move directly under the root at some point, when the hop-1 node
 * loses its two descendants and incoming traffic.
 *
 * We should see ADD transactions, DELETE transactions and CLEAR
 * transactions in this test.
 */

var node_1 = sim.getMoteWithID(1)
var node_2 = sim.getMoteWithID(2)
var node_3 = sim.getMoteWithID(3)
var node_4 = sim.getMoteWithID(4)

var node_3_4_is_moved = false;

var add_trans_count = 0;
var delete_trans_count = 0;
var clear_trans_count = 0;

function change_position(node, x, y, z) {
  var position = node.getInterfaces().getPosition();
  position.setCoordinates(x, y, z);
}

function print_position(node) {
  var position = node.getInterfaces().getPosition();
  log.log("node_" + node.getID() + " is at (" +
          position.getXCoordinate() + ", " +
          position.getYCoordinate() + ", " +
          position.getZCoordinate() + ")\n");
}

/* this test lasts 10min (600s); GENERATE_MSG() takes a timeout value in ms */
GENERATE_MSG(600000, "test is done");

/* initialize their positions */
change_position(node_1, 0, 0, 0);
change_position(node_2, 0, 30, 0);
change_position(node_3, 0, 60, 0);
change_position(node_4, 0, 90, 0);

/* print their positions */
print_position(node_1);
print_position(node_2);
print_position(node_3);
print_position(node_4);

while(true) {
  YIELD();

  if(msg.equals("test is done")) {
    log.log("ADD: " + add_trans_count +
            ", DELETE: " + delete_trans_count +
            ", CLEAR: " + clear_trans_count +
            "\n");
    /* we should see all of ADD, DELETE, and CLEAR in this test */
    if(add_trans_count > 0 &&
       delete_trans_count > 0 &&
       clear_trans_count > 0) {
      log.testOK();
    } else {
      log.testFailed();
    }
    log.log("done\n");
  }

  log.log(time + " node-" + id + " "+ msg + "\n");

  /* we don't expect any ERR from 6top or MSF in this scenario */
  if((id === 1 || id === 2) &&
     (msg.indexOf("[ERR : 6top       ]") !== -1 ||
      msg.indexOf("[ERR : MSF       ]") !== -1)) {
    log.testFailed();
  }

  if(msg.indexOf("sent an ADD request") !== -1) {
    add_trans_count += 1;
  } else if(msg.indexOf("sent a DELETE request") !== -1) {
    delete_trans_count += 1;
  } else if(msg.indexOf("sent a CLEAR request") !== -1) {
    clear_trans_count += 1;
  }

  /* at time of 7min (420s), move node-3 and node-4 */
  if((sim.getSimulationTimeMillis() >= 420000) &&
     (node_3_4_is_moved === false)) {
    change_position(node_3, 30, -30, 0);
    change_position(node_4, -30, -30, 0);
    log.log("change positions of node_3 and node_4\n");
    print_position(node_3);
    print_position(node_4);
    node_3_4_is_moved = true;
  }
}
