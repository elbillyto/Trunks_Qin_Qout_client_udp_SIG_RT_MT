![logo](http://artificialreality.free.fr/images/POSIX-logo.jpg)
==========================

## Synopsis
A solution to the **Concurrent , Realtime** multi-TRUNK's ETHER problem.
This also addresses **bidirectional communication** with bounded buffers.
Another covered issue is the **interleaving of RT Event signaling with Multithread concurrency**.
Each ETHER maintains 2 bounded circular FIFOs for output /input
Each TRUNK consumes synchronously the ETHER output circular FIFO
and after processing data (sending messages to an echo server down the line)
every TRUNK posts back results to the ETHER's input FIFO
At this moment a RT signal is raised and the handling function traces the
RT-event and its associated value to a file for further exploitation.
**Standard POSIX pthread** libraries are also used regarding thread POCs.
The project could be a cross platform example provided we have the posix pthread porting.


## Code Example


	printf("--> runETHER %d\n", a->id);

	for (o = 0; o < a->lock->Capacity; o++) {

		//generates capacity

		writelock(a->lock, a->id);
		queueAdd(fifoOUT, o + 1);
		printf("ETHER %d: Wrote %d\n", a->id, o + 1);
		writeunlock(a->lock);

	}

	//ETHER has generated the entire output, Notify
	printf("ETHER %d: Finishing generation...\n", a->id);
	printf("ETHER %d: Finished generation.\n", a->id);

	//reads qIN until Capacity to process results from TRUNKS
	for (i = 0; i < a->lock->Capacity; i++) {

		//reads capacity

		readlock(a->lock, a->id);
		queueDel(fifoIN, &d);
		printf("ETHER %d: read %d\n", a->id, d);
		readunlock(a->lock);
		sig_send(SIGRTMIN_1, d);
	}

	//ETHER has consumed the entire input, Notify
	printf("ETHER %d: Finishing...\n", a->id);
	printf("ETHER %d: Finished Processing.\n", a->id);
  
  
## Motivation

**POC** to addess **interleaving of RT Event signaling with Multithread concurrency**
along with communication issues down the line.

## Installation


## API Reference


## Tests


## Contributors

elbillyto
## License

This document and the project files are not copyrighted and are released into the public domain.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
