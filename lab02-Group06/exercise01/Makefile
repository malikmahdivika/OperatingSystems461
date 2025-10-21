cmp = gcc
ifeq ($(shell uname), Linux)
	
else
	$(error Sorry, I prefer Linux)
endif

.PHONY: all
all: gen_numbers.out select_arg.out gen_in_range.out

.PHONY: test
test: gen_numbers.out select_arg.out gen_in_range.out
	$(cmp)
	ifeq ($(?), 0)
	else
		$(error an outfile did not return 0 after executing.)
	endif


gen_numbers.out: gen_numbers.c utils.c utils.h
	$(cmp) -o $@ $^

select_arg.out: select_arg.c utils.c utils.h
	$(cmp) -o $@ $^

gen_in_range.out: gen_in_range.c
	$(cmp) -o $@ $<

.PHONY: clean
clean:
	rm -f *.out