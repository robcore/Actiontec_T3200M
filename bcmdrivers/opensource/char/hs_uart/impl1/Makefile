#
# Makefile for the hs_uart driver.
#

obj-$(CONFIG_BCM_HS_UART) += hs_uart_drv.o
hs_uart_drv-y := hs_uart.o hs_uart_dma.o

EXTRA_CFLAGS += -Werror -Wall
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/pmc
EXTRA_CFLAGS += -DEXPORT_SYMTAB
EXTRA_CFLAGS += -g

-include $(TOPDIR)/Rules.make

clean:
	rm -r core *.o *.a *.s .*.cmd *.ko
