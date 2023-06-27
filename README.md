# Character Device Driver Example
The driver adds a character device file `/dev/chfifo` which can be used as a FIFO queue.

## Details
The maximum size of the queue is 1000 bytes.
When reading, the read data is removed from the queue.
When writing, the data is added to the end of the queue.

Is is possible to open the device in three modes:

* **Shared** - (default) no extra flags.
	In this mode the file can be simultaneously opened multiple times.
* **Exclusive** - when opened with O_EXCL flag. ---> (TBD)
	In this mode the file can be opened only once at a time.
* **Multi** - when opened with O_CREAT flag. ---> (TBD)
	In this mode the file can be opened multiple times, but each time a new FIFO queue will be creted.

## Make options

* `make` - build the driver
* `make clean` - clean build files of the driver
* `make test-make` - build test executable

## Build prerequisites
The driver is targeted for Linux kernel v6.1.6. ---> (TBD)
