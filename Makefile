source_files := src/cluster.c src/hashmap.c src/utils.c src/queue.c src/fastcompare.c
header_dir := include
target_so := libfastcompare.so
arch := -mavx2

CC = $(COMP)
CFLAGS = $(arch) $(DEBUG) -shared -o $(target_so) -fPIC


ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: clean libfastcompare setenv
init: libfastcompare setenv
install: install_op install_ldc

setenv:
	echo 'export LD_LIBRARY_DIR=$LD_LIBRARY_DIR:'$(DESTDIR)$(PREFIX)'/lib/' >> ~/.bashrc

setenvpwd:
	echo 'export LD_LIBRARY_DIR=$LD_LIBRARY_DIR:'$(pwd) >> ~/.bashrc

install_ldc:
	ldconfig -n $(DESTDIR)$(PREFIX)/lib/

install_op: 
	mv ./$(target_so) $(DESTDIR)$(PREFIX)/lib/
	mkdir -p $(DESTDIR)$(PREFIX)/include/fastcompare
	cp $(header_dir)/* $(DESTDIR)$(PREFIX)/include/fastcompare
	

libfastcompare:
	$(CC) $(CFLAGS) -I$(header_dir) $(source_files)

clean:
	rm -f $(target_so)

standalone:
	gcc -mavx2 -ggdb -I$(header_dir) $(source_files)