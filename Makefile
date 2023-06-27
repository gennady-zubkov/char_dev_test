CONFIG_MODULE_SIG=n

MODULE_NAME = chfifo
TEST_NAME = test_app

obj-m += $(MODULE_NAME).o

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

test-make: $(TEST_NAME).c
	cc $(TEST_NAME).c -g -Wall -o $(TEST_NAME)

