A solution to the Concurrent , Realtime multi-TRUNK's ETHER problem.
This also addresses bidirectional communication with bounded buffers.
Another covered issue is the interleaving of RT Event signaling with
Multithread concurrency.
Each ETHER maintains 2 bounded circular FIFOs for output /input
Each TRUNK consumes synchronously the ETHER output circular FIFO
and after processing data (sending messages to an echo server down the line)
every TRUNK posts back results to the ETHER's input FIFO
At this moment a RT signal is raised and the handling function traces the
RT-event and its associated value to a file for further exploitation.
