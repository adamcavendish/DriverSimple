obj-m += processdog.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	echo "make"
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	echo "rmmod"
	sudo rmmod processdog
	echo "insmod"
	sudo insmod ./processdog.ko is_test=1

