main_filename := fastcompare.c
header_filename := fastcompare.h
target_so := libfastcompare.so

CC = gcc
CFLAGS = -shared -o $(target_so) -fPIC


ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: clean libfastcompare

install: libfastcompare.so
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(target_so) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 $(header_filename) $(DESTDIR)$(PREFIX)/include/

libfastcompare:
	$(CC) $(CFLAGS) $(main_filename)

clean:
	rm -f $(target_so)