This is a demonstration of application-level time synchronization using TSCH.

In a TSCH network, all nodes are synchronized to a global time counter maintained
by the coordinator. This fact can be exploited to measure properties such as latency.

The modes periodically send their detected network uptime to the coordinator.
The coordinator receives these packets, prints its local network uptime
and the time difference. This time difference is equal to the end-to-end latency,
which includes both the time to prepare the packet, the time until an appropriate
slot in the TSCH schedule, the over-the-air time (negligible). If the packet
does not arrive with the first attempt, it also includes the retransmission time.

The nodes in this example do not have any notion of the wall-clock time.
That would need additional synchronization between the TSCH network and an external clock source.
For example, one can periodically distribute UNIX timestamps over the UART interface
on the border router to implement this feature. Alternatively, the data collected from
the TSCH network can be timestamped with just TSCH timestamps, and them on the external gateway
these timestamps could be converted to wall-clock time.