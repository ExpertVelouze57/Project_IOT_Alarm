APPLICATION = cayenne

BOARD ?= lora-e5-dev

RIOTBASE ?= $(CURDIR)/../RIOT

# Default LoRa region is Europe and default band is 868MHz
LORA_REGION ?= EU868

USEMODULE += bmp280_i2c

#Flags pour le capteur I2C (chanel et adresse)
CFLAGS += -DBMX280_PARAM_I2C_DEV=I2C_DEV\(\0\)
CFLAGS += -DBMX280_PARAM_I2C_ADDR=0x77

USEMODULE += xtimer
USEMODULE += sx126x_stm32wl

USEPKG += semtech-loramac
USEMODULE += auto_init_loramac
USEPKG += cayenne-lpp

include $(RIOTBASE)/Makefile.include
