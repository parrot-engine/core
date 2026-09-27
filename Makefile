CC=gcc

CFLAGS=-std=c99 -Wall -Werror -pedantic-errors $(EXTRA_CFLAGS)

UNAME=$(shell uname -s)
ifeq ($(UNAME),Linux)
CFLAGS += -DPARROT_PLATFORM_UNIX -DPARROT_PLATFORM_LINUX
endif

OUTPUT=test

BEAR=bear

.PHONY: all clean test

all: $(OUTPUT)

$(OUTPUT):
	$(BEAR) -a -- $(CC) $(CFLAGS) -o $@ -lm test.c

clean:
	rm -r $(OUTPUT)
