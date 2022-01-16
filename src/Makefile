APPLICATION = cayenne

BOARD ?= lora-e5-dev

RIOTBASE ?= $(CURDIR)/../RIOT

# Default LoRa region is Europe and default band is 868MHz
LORA_REGION ?= EU868

USEMODULE += bmp280_i2c

#Flags pour le capteur I2C (chanel et adresse)
CFLAGS += -DBMX280_PARAM_I2C_DEV=I2C_DEV\(\0\)
CFLAGS += -DBMX280_PARAM_I2C_ADDR=0x77

USEMODULE += sx1272  # for a SX1272 radio device
USEMODULE += sx1276  # for a SX1276 radio device
USEMODULE += semtech_loramac_rx


USEMODULE += xtimer
USEMODULE += sx126x_stm32wl
USEMODULE += pir

FEATURES_REQUIRED = periph_adc
FEATURES_REQUIRED += periph_gpio_irq

USEPKG += semtech-loramac
USEMODULE += auto_init_loramac
USEPKG += cayenne-lpp

include $(RIOTBASE)/Makefile.include
