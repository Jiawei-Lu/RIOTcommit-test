# name of your application
APPLICATION = filesystem

# If no BOARD is found in the environment, use this default:
BOARD ?= native


# Blacklisting msp430-based boards, as file syscalls are not supported
FEATURES_BLACKLIST += arch_msp430

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1
USEMODULE += gnrc_netif
USEMODULE += netdev_eth
USEMODULE += xtimer
# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gcoap
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo
#USEMODULE +=pm_layered
# Required by gcoap example
USEMODULE += od
USEMODULE += fmt
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

#rpl
USEMODULE += gnrc_pktdump
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6
USEMODULE += netstats_rpl


# rtc
FEATURES_REQUIRED += periph_rtc
FEATURES_OPTIONAL += periph_rtc_ms
FEATURES_OPTIONAL += periph_rtc_mem

DISABLE_MODULE += periph_init_rtc


# Use MTD (flash layer)
USEMODULE += mtd
# USEMODULE += mtd_sdcard

# Use VFS
USEMODULE += vfs

# Use a file system
USEMODULE += littlefs
#USEMODULE += littlefs2
#USEMODULE += spiffs
#USEMODULE += fatfs_vfs
USEMODULE += constfs
#USEMODULE += devfs

include $(RIOTBASE)/Makefile.include
