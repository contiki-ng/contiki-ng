This is a border router that runs embedded in a node. The node runs a full
6LoWPAN stack, and acts as a DAG root. It interfaces to the outside world
via a serial line. On the host Operating System, `tunslip6` is used to create
a tun interface and bridge it to the RPL border router. This is achieved with
makefile targets `connect-router` and `connect-router-cooja`.
