CONTIKI_PROJECT = lwb
all: $(CONTIKI_PROJECT)

ifdef LWB_CONF
CFLAGS += -D$(LWB_CONF)
endif

APPDIRS += ./configs

CFLAGS += -DUART1_CONF_TX_WITH_INTERRUPT=1

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

