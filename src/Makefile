APPLICATION = cayenne

BOARD ?= lora-e5-dev

RIOTBASE ?= $(CURDIR)/../RIOT

# Default LoRa region is Europe and default band is 868MHz
LORA_REGION ?= EU868

USEMODULE += xtimer
USEMODULE += sx126x_stm32wl

USEPKG += semtech-loramac
USEMODULE += auto_init_loramac
USEPKG += cayenne-lpp

include $(RIOTBASE)/Makefile.include
