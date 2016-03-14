CC=gcc
GCCWARNINGS= -Wall
CFLAGS = -ggdb -W -Wall -pedantic  -std=gnu99  -Wno-unused-parameter -Wno-unused-variable -O2
LD=gcc
BIN = main
LIB = #-lpthread -lssl -lcrypto

SRCDIR=src
TMPDIR=obj
OBJDIR=$(TMPDIR)

SRCS=$(wildcard $(SRCDIR)/*.c)

OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o) 
DEPS = $(OBJS:%.o=%.d) 

all: $(BIN)

$(BIN): $(OBJS)
	$(LD) -o $@ $^ $(CFLAGS) $(LIB)


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(TMPDIR)/%.d: $(SRCDIR)/%.c
	$(CC) -MM -MD -MT $(OBJDIR)/$*.o -o $@ $<
	
-include $(DEPS)

	
.PHONY: info clean distclean veryclean

info:
	@echo "$(BIN) version: $(MAJOR).$(MINOR).$(BUILD)"
	@echo "$(BIN) commits: $(COMMIT)"

clean:
	rm -f $(OBJS) $(DEPS)

distclean: clean
	rm -rf $(BIN)

veryclean: distclean
	find . -type f -name "*~" -exec rm -f {} \;

re: 
	clear 
	make veryclean
	make
