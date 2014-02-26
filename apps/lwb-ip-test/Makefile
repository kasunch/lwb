CONTIKI_PROJECT = lwb-ip-test
all: $(CONTIKI_PROJECT)

CFLAGS += -DRF_CHANNEL=26 -DUART1_CONF_TX_WITH_INTERRUPT=0

WITH_UIP=1

CFLAGS += -fno-strict-aliasing

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

