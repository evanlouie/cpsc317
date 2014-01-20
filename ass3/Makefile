# Compiler to be used
CC = gcc

# These three variables are used as flags when compiling the
# files. The current value includes all necessary flags for the OpenCV
# library, and to use pthreads and timers. If you use additional
# libraries you may need to add additional flags.
#
# The -Wall and -Werror flags mean that common warnings are shown and
# treated as errors. Although you may remove these flags so that your
# code compiles, you are strongly encouraged to fix the warnings
# instead, since they often lead to unexpected runtime errors.
CFLAGS = -w -Wall -Werror -g $(shell pkg-config --cflags opencv)
LDFLAGS = -w -Wall -Werror -g -pthread
LDLIBS = -lrt -lm $(shell pkg-config --libs opencv)

# This is the first target, corresponding to what should be built if
# no target is provided (i.e., when make runs without arguments).
all: rtspd
# cloudrtspd

# This rule indicates that rtspd and cloudrtspd is built from rtspd.o
# and main.o. If you use a different set of files to build it, you
# should change the list. Note that since no command is given to
# create the file, a default rule is used, which calls the compiler
# (CC variable above) with LDFLAGS and LDLIBS.
rtspd: rtspd.o main.o
# cloudrtspd: cloudrtspd.o main.o

# This rule indicates that rtspd.o depends on rtspd.c and rtspd.h,
# and so on. Although the gcc command will not include rtspd.h, you
# should make references to all user includes in this list, so that if
# you change the include but not the C file, the file is compiled as
# well. Note that the C file has to be the first file on the list,
# since the object is based on that file. Since no command is given to
# create the file, a default rule is used, which calls the compiler
# (CC variable above) with CFLAGS.
rtspd.o: rtspd.c rtspd.h
main.o: main.c rtspd.h
# cloudrtspd.o: cloudrtspd.c rtspd.h

# A clean rule is a good practice to simplify the removal of all
# generated files. If you run 'make clean' in a terminal, all these
# files are removed. This is useful both in case you think any of the
# object files is corrupted/has a different version than what you
# expect, or when you need to submit (handin) the files without the
# object files.
clean:
	-rm -rf main.o rtspd.o rtspd cloudrtspd.o cloudrtspd
