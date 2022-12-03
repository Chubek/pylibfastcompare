main_filename := fastcompare.c
header_filename := fastcompare.h
target_so := libfastcompare.so
arch := -march=native

CC = gcc
CFLAGS = $(arch) -shared -o $(target_so) -fPIC


ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: clean libfastcompare setenv
install: install_op install_ldc

setenv:
	echo 'export LD_LIBRARY_DIR=$LD_LIBRARY_DIR:'$(DESTDIR)$(PREFIX)'/lib/' >> ~/.bashrc

setenvpwd:
	echo 'export LD_LIBRARY_DIR=$LD_LIBRARY_DIR:'$(pwd) >> ~/.bashrc

install_ldc:
	ldconfig -n $(DESTDIR)$(PREFIX)/lib/

install_op: libfastcompare.so
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(target_so) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 $(header_filename) $(DESTDIR)$(PREFIX)/include/
	

libfastcompare:
	$(CC) $(CFLAGS) $(main_filename)

clean:
	rm -f $(target_so)

standalone:
	$(CC) $(arch) -ggdb $(main_filename)