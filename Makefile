obj-m := elevator.o
obj-y := start_elevator.oobj-y := stop_elevator.o obj-y := issue_request.o

PWD := $(shell pwd)
KDIR := /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f  *.ko *.mod.* Module.* modules.*
