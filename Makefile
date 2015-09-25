# First target is default target, if you just type:  make

FILE=shell.c

default: run

run: myshell
	./myshell

gdb: myshell
	gdb myshell

myshell: ${FILE}
	gcc -g -O0 -o myshell ${FILE}

emacs: ${FILE}
	emacs ${FILE}
vi: ${FILE}
	vi ${FILE}

clean:
	rm -f myshell a.out *~

# 'make' will view $v as variable and try to expand it.
# By typing $$, make will reduce it to a single '$' and pass it to the shell.
# The shell will view $dir as a variable and expand it.
dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
	dir=`basename $$PWD`; ls -l ../$$dir.tar.gz
