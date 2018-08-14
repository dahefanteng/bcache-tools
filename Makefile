
PREFIX=/usr
UDEVLIBDIR=/lib/udev
DRACUTLIBDIR=/lib/dracut
INSTALL=install
#CFLAGS+=-O2 -Wall -g

all: make-bcache probe-bcache bcache-super-show bcache-register depend bcache

install: make-bcache probe-bcache bcache-super-show
	$(INSTALL) -m0755 make-bcache bcache-super-show	bcache $(DESTDIR)${PREFIX}/sbin/
	$(INSTALL) -m0755 probe-bcache bcache-register		$(DESTDIR)$(UDEVLIBDIR)/
	$(INSTALL) -m0644 69-bcache.rules	$(DESTDIR)$(UDEVLIBDIR)/rules.d/
	$(INSTALL) -m0644 -- *.8 $(DESTDIR)${PREFIX}/share/man/man8/
	$(INSTALL) -D -m0755 initramfs/hook	$(DESTDIR)/usr/share/initramfs-tools/hooks/bcache
	$(INSTALL) -D -m0755 initcpio/install	$(DESTDIR)/usr/lib/initcpio/install/bcache
	$(INSTALL) -D -m0755 dracut/module-setup.sh $(DESTDIR)$(DRACUTLIBDIR)/modules.d/90bcache/module-setup.sh
#	$(INSTALL) -m0755 bcache-test $(DESTDIR)${PREFIX}/sbin/

clean:
	$(RM) -f bcache make-bcache probe-bcache bcache-super-show bcache-register bcache-test -- *.o

bcache-test: LDLIBS += `pkg-config --libs openssl` -lm
make-bcache: LDLIBS += `pkg-config --libs uuid blkid`
make-bcache: CFLAGS += `pkg-config --cflags uuid blkid`
make-bcache: bcache.o
probe-bcache: LDLIBS += `pkg-config --libs uuid blkid`
probe-bcache: CFLAGS += `pkg-config --cflags uuid blkid`
bcache-super-show: LDLIBS += `pkg-config --libs uuid`
bcache-super-show: CFLAGS += -std=gnu99
bcache-super-show: bcache.o
bcache-register: bcache-register.o

CFLAGS+=`pkg-config --cflags blkid uuid smartcols`
LDFLAGS+=`pkg-config --libs blkid uuid smartcols`

SRCS=bcache-main.c bcache.c lib.c make.c
depend: .depend
.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ > ./.depend;

include .depend

bcache: bcache-main.o bcache.o lib.o make.o
