.PHONY: all
all: shell.out

shell.out: shell.c parser.c parser.h
	gcc -o $@ $^

.PHONY: clean
clean:
	rm -f *.out