CONTIKI_PROJECT = lwb-test
all: $(CONTIKI_PROJECT)

CFLAGS += -DRF_CHANNEL=26 -DUART1_CONF_TX_WITH_INTERRUPT=0 #-DCOOJA=1

PROJECTDIRS += .
include Makefile.lwb

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

