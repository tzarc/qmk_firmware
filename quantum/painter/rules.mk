QUANTUM_PAINTER_ENABLE ?= no
VALID_QUANTUM_PAINTER_DRIVERS := qmk_oled_wrapper ili9341_spi st7789_spi
QUANTUM_PAINTER_DRIVERS ?=

QUANTUM_PAINTER_NEEDS_COMMS_SPI ?= no
QUANTUM_PAINTER_NEEDS_COMMS_I2C ?= no
QUANTUM_PAINTER_NEEDS_COMMS_PARALLEL ?= no

OPT_DEFS += -DQUANTUM_PAINTER_ENABLE
COMMON_VPATH += $(QUANTUM_DIR)/painter
SRC += \
    $(QUANTUM_DIR)/utf8.c \
    $(QUANTUM_DIR)/painter/qp.c \
    $(QUANTUM_DIR)/painter/qp_draw_core.c \
    $(QUANTUM_DIR)/painter/qp_draw_codec.c \
    $(QUANTUM_DIR)/painter/qp_draw_circle.c \
    $(QUANTUM_DIR)/painter/qp_draw_ellipse.c \
    $(QUANTUM_DIR)/painter/qp_draw_image.c \
    $(QUANTUM_DIR)/painter/qp_draw_text.c

# Handler for each driver
define handle_quantum_painter_driver
    CURRENT_PAINTER_DRIVER := $1

    ifeq ($$(filter $$(strip $$(CURRENT_PAINTER_DRIVER)),$$(VALID_QUANTUM_PAINTER_DRIVERS)),)
        $$(error "$$(CURRENT_PAINTER_DRIVER)" is not a valid Quantum Painter driver)

    else ifeq ($$(strip $$(CURRENT_PAINTER_DRIVER)),qmk_oled_wrapper)
        OLED_ENABLE := yes
        OLED_DRIVER := SSD1306
        OPT_DEFS += -DQUANTUM_PAINTER_QMK_OLED_WRAPPER_ENABLE
        COMMON_VPATH += $(DRIVER_PATH)/painter/qmk_oled_wrapper
        SRC += $(DRIVER_PATH)/painter/qmk_oled_wrapper/qp_qmk_oled_wrapper.c

    else ifeq ($$(strip $$(CURRENT_PAINTER_DRIVER)),ili9341_spi)
        QUANTUM_PAINTER_NEEDS_COMMS_SPI = yes
        OPT_DEFS += -DQUANTUM_PAINTER_ILI9341_ENABLE -DQUANTUM_PAINTER_ILI9XXX_SPI_ENABLE -DQUANTUM_PAINTER_ILI9341_SPI_ENABLE
        COMMON_VPATH += \
            $(DRIVER_PATH)/painter/ili9xxx \
            $(DRIVER_PATH)/painter/ili9xxx/ili9341
        SRC += \
            $(DRIVER_PATH)/painter/ili9xxx/qp_ili9xxx.c \
            $(DRIVER_PATH)/painter/ili9xxx/ili9341/qp_ili9341.c \
            $(DRIVER_PATH)/painter/ili9xxx/ili9341/qp_ili9341_spi.c

    else ifeq ($$(strip $$(CURRENT_PAINTER_DRIVER)),st7789_spi)
        QUANTUM_PAINTER_NEEDS_COMMS_SPI = yes
        OPT_DEFS += -DQUANTUM_PAINTER_ST7789_ENABLE -DQUANTUM_PAINTER_ST7XXX_SPI_ENABLE -DQUANTUM_PAINTER_ST7789_SPI_ENABLE
        COMMON_VPATH += \
            $(DRIVER_PATH)/painter/st77xx \
            $(DRIVER_PATH)/painter/st77xx/st7789
        SRC += \
            $(DRIVER_PATH)/painter/st77xx/qp_st77xx.c \
            $(DRIVER_PATH)/painter/st77xx/st7789/qp_st7789.c
            $(DRIVER_PATH)/painter/st77xx/st7789/qp_st7789_spi.c

    endif
endef

# Iterate through the listed drivers for the build, including what's necessary
$(foreach qp_driver,$(QUANTUM_PAINTER_DRIVERS),$(eval $(call handle_quantum_painter_driver,$(qp_driver))))

# If SPI comms is needed, set up the required files
ifeq ($(strip $(QUANTUM_PAINTER_NEEDS_COMMS_SPI)), yes)
    OPT_DEFS += -DQUANTUM_PAINTER_SPI_ENABLE
    QUANTUM_LIB_SRC += spi_master.c
    VPATH += $(DRIVER_PATH)/painter/comms
    SRC += \
        $(QUANTUM_DIR)/painter/qp_comms.c \
        $(DRIVER_PATH)/painter/comms/qp_comms_spi.c
endif

# If I2C comms is needed, set up the required files
ifeq ($(strip $(QUANTUM_PAINTER_NEEDS_COMMS_I2C)), yes)
    OPT_DEFS += -DQUANTUM_PAINTER_I2C_ENABLE
    QUANTUM_LIB_SRC += i2c_master.c
    VPATH += $(DRIVER_PATH)/painter/comms
    SRC += \
        $(QUANTUM_DIR)/painter/qp_comms.c \
        $(DRIVER_PATH)/painter/comms/qp_comms_i2c.c
endif

# If parallel comms is needed, set up the required files
ifeq ($(strip $(QUANTUM_PAINTER_NEEDS_COMMS_PARALLEL)), yes)
    OPT_DEFS += -DQUANTUM_PAINTER_PARALLEL_ENABLE
    VPATH += $(DRIVER_PATH)/painter/comms
    SRC += \
        $(QUANTUM_DIR)/painter/qp_comms.c \
        $(DRIVER_PATH)/painter/comms/qp_comms_parallel.c
endif
