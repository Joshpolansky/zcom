CC = gcc -Wall -Wextra -pedantic -O0 -g3
LM = -lm
# optimized compiling
CX = icc -Wall -Wremarks -Wcheck -O3 -vec_report0
LMX =
#CX = gcc -Wall -O3 -g -march=pentium4 -mfpmath=sse -funroll-all-loops
#LMX = -lm
deps = ising2.c ising2.h rng.h util.h Makefile
bin = $(patsubst %.c,%, $(wildcard test*.c))
bin_d = $(patsubst %.c,%_d, $(wildcard test*.c))

all: $(bin) $(bin_d)

$(bin_d) : %_d : %.c $(deps)
	$(CC) -o $@ $< $(LM)

$(bin) : % : %.c $(deps)
	$(CX) -o $@ $< $(LMX)

clean:
	rm -f *~ *.out MTSEED *.dat $(bin) $(bin_d)

.PHONY: clean all

