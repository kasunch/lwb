CONTIKI_PROJECT = lwb-ip-test
all: $(CONTIKI_PROJECT)

CFLAGS += -DRF_CHANNEL=26 -DUART1_CONF_TX_WITH_INTERRUPT=0 -DCOOJA=1

PROJECTDIRS += ../lwb
include ../lwb/Makefile.lwb

PROJECTDIRS += uip6
include uip6/Makefile.uip6
CFLAGS += -fno-strict-aliasing

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

