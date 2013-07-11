CONTIKI_PROJECT = lwb
all: $(CONTIKI_PROJECT)

ifdef LWB_CONF
CFLAGS += -D$(LWB_CONF)
endif

APPDIRS += ./configs

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

