KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) modules
	-rm $*.mod.[co]
%.ko:
	make -C $(KDIR) M=$(PWD) $@
	-rm $*.mod.[co]
clean:
	make -C $(KDIR) M=$(PWD) $@
