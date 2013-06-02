![logo](maxcpp.png) C
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
The major part of the files belong to a codeblocks projec that serves the purposes of POCs. except for a couple of files, that are targeted for SO Windows, these are cross platform examples.
**Standard POSIX pthread** libraries are also used regarding thread POCs.


## Code Example


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
