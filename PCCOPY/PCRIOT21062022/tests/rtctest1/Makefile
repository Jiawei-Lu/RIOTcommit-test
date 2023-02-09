include ../Makefile.tests_common

USEMODULE += gnrc_netif
USEMODULE += netdev_eth
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif


FEATURES_REQUIRED = periph_rtc

DISABLE_MODULE += periph_init_rtc

USEMODULE += xtimer

include $(RIOTBASE)/Makefile.include
