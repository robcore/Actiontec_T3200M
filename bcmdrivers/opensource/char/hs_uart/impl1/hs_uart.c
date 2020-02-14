/*
* <:copyright-BRCM:2013:GPL/GPL:standard
*
*    Copyright (c) 2013 Broadcom Corporation
*    All Rights Reserved
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*
* :>
*/
// BCMFORMAT: notabs reindent:uncrustify:bcm_minimal_i3.cfg

/* Description: Bluetooth Serial port driver for the BCM963XX. */

#include <linux/version.h>
#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/tty_flip.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <board.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <linux/bcm_colors.h>
#ifdef __arm__
#include <mach/hardware.h>
#endif

#include <linux/serial_core.h>
#include "hs_uart_dma.h"

#include <asm/uaccess.h> /*copy_from_user*/
#include <linux/proc_fs.h>

/* Note on locking policy: All uart ops take the
   port->lock, except startup shutdown and termios */

/******* Defines & Types *******/
#define HS_UART_PRINT_CHARS       0
#define HS_UART_LOOPBACK_ENABLE   0
#define HS_UART_DUMP_REGS         0
#define HS_UART_API_DEBUG         0
#define HS_UART_MIN_BAUD          9600     /* Arbitrary value, this can be as low as 9600 */
#define HS_UART_USE_HIGHRATE      0        /* Highrate allows for baudrates higher than 3.125Mbps */
#define HS_UART_CALCULATE_DLHBR   1        /* 1 - Calculate dl/hbr values for any baudrates, 0 - use predefined values */

#undef HS_UART_DEBUG             /* undef it, just in case */
#if HS_UART_API_DEBUG
#  define HS_UART_DEBUG(fmt, args...) printk( KERN_DEBUG "hs_uart: " fmt, ## args)
#else
#  define HS_UART_DEBUG(fmt, args...) /* not debugging: nothing */
#endif

#define UART_NR     1
#define HS_UART_PORT    66
#define BCM63XX_ISR_PASS_LIMIT  256
/* generate HS_UART_TXFIFOAEMPTYTHRESH interrupt if tx fifo level falls below this number.
 * Must define a constant becuase hs_uart_console_write needs this and
 * HS_UART_REG(port)->fifocfg is not set until later. */
#define HS_UART_TXFIFOAEMPTYTHRESH      100 /* Threhsold indicates when TX FIFO has more room */
#define HS_UART_RXFIFOAFULLTHRESH    1 /* Threshold indicates when RX FIFO is not empty */
#define HS_UART_RXFIFOAFULLTHRESH_DMA  4 /* Threshold indicates when RX FIFO has the 4 byte hci packet header in it */
#define HS_UART_RXFIFOAFULLTHRESH_DMA_SWSLIP  5 /* Threshold indicates when RX FIFO has the 5 byte SLIP+hci packet header in it*/
#define HS_UART_MAX_PKT_HDR HS_UART_RXFIFOAFULLTHRESH_DMA_SWSLIP

#define HS_UART_REG(p) ((volatile HsUartCtrlRegs *) (p)->membase)

#ifndef IO_ADDRESS
#define IO_ADDRESS(x) (x)
#endif

#define HS_UART_PROC_ENTRY_ROOT "driver/hs_uart"
#define HS_UART_PROC_ENTRY HS_UART_PROC_ENTRY_ROOT "/xfer_mode"

typedef enum
{
   HS_UART_XFER_MODE_PIO = 0,
   HS_UART_XFER_MODE_DMA,
   HS_UART_XFER_MODE_DMA_SWSLIP,
   HS_UART_XFER_MODE_DMA_HWSLIP,
   HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC,
   HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK,
   HS_UART_XFER_MODE_MAX,
} HS_UART_XFER_MODE;

typedef struct
{
   HS_UART_XFER_MODE mode;
   char mode_desc[40];
} HS_UART_XFER_MODE_MAP;

typedef struct  {
#ifdef BIG_ENDIAN
   unsigned int seq_num:3;
   unsigned int ack_num:3;
   unsigned int data_integrity_check:1;
   unsigned int reliable_pkt:1;
   unsigned int pkt_type:4;
   unsigned int pkt_len:12;
   unsigned int header_checksum:8;
#else
   unsigned int header_checksum:8;
   unsigned int pkt_len:12;
   unsigned int pkt_type:4;
   unsigned int reliable_pkt:1;
   unsigned int data_integrity_check:1;
   unsigned int ack_num:3;
   unsigned int seq_num:3;
#endif
} HS_UART_HCI_HDR;

/******* Private prototypes *******/
static void deinit_hs_uart_port(struct uart_port * port );
static int init_hs_uart_port( struct uart_port * port );
static unsigned int hs_uart_tx_empty( struct uart_port * port );
static void hs_uart_set_mctrl( struct uart_port * port, unsigned int mctrl );
static unsigned int hs_uart_get_mctrl( struct uart_port * port );
static void hs_uart_stop_tx( struct uart_port * port );
static void hs_uart_start_tx( struct uart_port * port );
static void hs_uart_stop_rx( struct uart_port * port );
static void hs_uart_enable_ms( struct uart_port * port );
static void hs_uart_break_ctl( struct uart_port * port, int break_state );
static int hs_uart_startup( struct uart_port * port );
static void hs_uart_shutdown( struct uart_port * port );
static int hs_uart_set_baud_rate( struct uart_port *port, int baud );
static void hs_uart_set_termios( struct uart_port * port, struct ktermios *termios, struct ktermios *old );
static const char * hs_uart_type( struct uart_port * port );
static void hs_uart_release_port( struct uart_port * port );
static int hs_uart_request_port( struct uart_port * port );
static void hs_uart_config_port( struct uart_port * port, int flags );
static int hs_uart_verify_port( struct uart_port * port, struct serial_struct *ser );
static void hs_uart_tx_chars_dma(struct uart_port *port);
static int rx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length );
static int tx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length );
static int get_payload_len( char * pkt_header );
static void hs_uart_set_xfer_mode( HS_UART_XFER_MODE mode );
static ssize_t proc_write_mode(struct file *file, const char __user *buffer, unsigned long count, void *data);
static int proc_read_mode(char *buf, char **start, off_t offset, int len, int *unused_i, void *unused_v);
static int hs_uart_create_proc_entries( void );
static int hs_uart_remove_proc_entries( void );
void hs_uart_dump_regs(struct uart_port * port);

/******* Local Variables *******/
static struct proc_dir_entry *xfer_mode_entry;

HS_UART_XFER_MODE_MAP xfer_mode_map[] =
{
   { HS_UART_XFER_MODE_PIO                   , "Programmed I/O"},
   { HS_UART_XFER_MODE_DMA                   , "DMA"},
   { HS_UART_XFER_MODE_DMA_SWSLIP            , "DMA & S/W SLIP"},
   { HS_UART_XFER_MODE_DMA_HWSLIP            , "DMA & H/W SLIP"},
   { HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC      , "DMA & H/W SLIP & H/W TXCRC"},
   { HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK , "DMA & H/W SLIP & H/W TXCRC & RXCRC chk"},
   { HS_UART_XFER_MODE_MAX                   , "null"},
};

static HS_UART_XFER_MODE xfer_mode = HS_UART_XFER_MODE_PIO;

static struct uart_ops hs_uart_pops =
{
   .tx_empty     = hs_uart_tx_empty,
   .set_mctrl    = hs_uart_set_mctrl,
   .get_mctrl    = hs_uart_get_mctrl,
   .stop_tx      = hs_uart_stop_tx,
   .start_tx     = hs_uart_start_tx,
   .stop_rx      = hs_uart_stop_rx,
   .enable_ms    = hs_uart_enable_ms,
   .break_ctl    = hs_uart_break_ctl,
   .startup      = hs_uart_startup,
   .shutdown     = hs_uart_shutdown,
   .set_termios  = hs_uart_set_termios,
   .type         = hs_uart_type,
   .release_port = hs_uart_release_port,
   .request_port = hs_uart_request_port,
   .config_port  = hs_uart_config_port,
   .verify_port  = hs_uart_verify_port,
};

static struct uart_port hs_uart_ports[] =
{
   {
      .membase    = (void *)IO_ADDRESS(HS_UART_PHYS_BASE),
      .mapbase    = HS_UART_PHYS_BASE,
      .iotype     = SERIAL_IO_MEM,
      .irq        = INTERRUPT_ID_UART2,
      .uartclk    = 50000000,
      .fifosize   = 1040,
      .ops        = &hs_uart_pops,
      .flags      = ASYNC_BOOT_AUTOCONF,
      .line       = 0,
   }
};

static struct uart_driver hs_uart_reg =
{
   .owner            = THIS_MODULE,
   .driver_name      = "hsuart",
   .dev_name         = "ttyH",
   .major            = TTY_MAJOR,
   .minor            = 66,
   .nr               = UART_NR,
};

/******* Functions ********/

static int get_payload_len( char * pkt_header )
{
   HS_UART_HCI_HDR * hci_hdr = (HS_UART_HCI_HDR *)pkt_header;
   int payload_len = hci_hdr->pkt_len;

   switch( xfer_mode )
   {
      case HS_UART_XFER_MODE_DMA_SWSLIP:
         payload_len += 3; /* Add 2 bytes for CRC, 1 Byte for SLIP delim */
      break;

      case HS_UART_XFER_MODE_DMA_HWSLIP:
      case HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC:
         payload_len += 2; /* Add 2 bytes for CRC */
      break;

      case HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK:
         payload_len += 3; /* Add 2 bytes for CRC, 1 Byte for CRC check */
      break;

      default:
         /* Debug mode, hardocde payload to 10 bytes */
         payload_len = 10;
      break;
   }

   return payload_len;
}

/*
 * Enable ms
 */
static void hs_uart_enable_ms(struct uart_port *port)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
}

/*
 * Get MCR register
 */
static unsigned int hs_uart_get_mctrl(struct uart_port *port)
{
   static unsigned int hs_uart_uart_mctrl = 0;

   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);

   if( !(HS_UART_REG(port)->MSR & HS_UART_MSR_CTS_STAT) )
   {
      hs_uart_uart_mctrl |= TIOCM_CTS;
   }

   if( !(HS_UART_REG(port)->MSR & HS_UART_MSR_RTS_STAT) )
   {
      hs_uart_uart_mctrl |= TIOCM_RTS;
   }

   return hs_uart_uart_mctrl;
}

/*
 * Set MCR register
 */
static void hs_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);

   if( mctrl & TIOCM_RTS )
   {
      HS_UART_REG(port)->MCR |= HS_UART_MCR_PROG_RTS;
   }
   else
   {
      HS_UART_REG(port)->MCR &= ~HS_UART_MCR_PROG_RTS;
   }
}

/*
 * Set break state
 */
static void hs_uart_break_ctl(struct uart_port *port, int break_state)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
}

/*
 * Initialize serial port
 */
static int hs_uart_startup(struct uart_port *port)
{
   unsigned long flags;

   spin_lock_irqsave(&port->lock, flags);

   init_hs_uart_port( port );

   spin_unlock_irqrestore(&port->lock, flags);

   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
   return 0;
}

/*
 * Shutdown serial port
 */
static void hs_uart_shutdown(struct uart_port *port)
{
   unsigned long flags;

   spin_lock_irqsave(&port->lock, flags);

   deinit_hs_uart_port(port);

   spin_unlock_irqrestore(&port->lock, flags);

   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void hs_uart_release_port(struct uart_port *port)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int hs_uart_request_port(struct uart_port *port)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
   return 0;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int hs_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
   return 0;
}

/*
 * Disable tx transmission
 */
static void hs_uart_stop_tx(struct uart_port *port)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
   HS_UART_REG(port)->uart_int_en &= ~HS_UART_TXFIFOAEMPTY;
}

/*
 * Disable rx reception
 */
static void hs_uart_stop_rx(struct uart_port *port)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
   HS_UART_REG(port)->uart_int_en &= ~HS_UART_RXFIFOAFULL;
}

/*
 * Receive rx chars - Called from ISR
 */
static void hs_uart_rx_chars(struct uart_port *port)
{
   struct tty_struct *tty = port->state->port.tty;
   unsigned int max_count = 256;
   int i=0;
   unsigned short status;
   unsigned char ch, flag = TTY_NORMAL;
   unsigned char pkt_header[HS_UART_MAX_PKT_HDR];
   unsigned int payload_bytes = 0;
   unsigned int header_bytes = 0;
   char * rx_buff = NULL;

   if( xfer_mode >= HS_UART_XFER_MODE_DMA )
   {
      /* In DMA mode restrict direct FIFO reads to just the packet header */
      max_count = HS_UART_REG(port)->RFL;

      /* Disable receive DMA, enable programmed FIFO reads */
      HS_UART_REG(port)->HO_DMA_CTL &= ~HS_UART_DMA_CTL_DMA_EN;
   }

   /* Interrupt due to content in RX FIFO -> clear HS_UART_RXFIFOEMPTY status bit */
   HS_UART_REG(port)->uart_int_stat |= HS_UART_RXFIFOEMPTY;

   /* Local copy */
   status = HS_UART_REG(port)->uart_int_stat;

   /* Keep reading from RX FIFO as long as it is not empty */
   while ( !(status & HS_UART_RXFIFOEMPTY) && i < max_count )
   {
      ch = HS_UART_REG(port)->uart_data;

      if( xfer_mode >= HS_UART_XFER_MODE_DMA )
      {
         pkt_header[i] = ch;
      }

      i++;
      port->icount.rx++;

      /*
      * Note that the error handling code is
      * out of the main execution path
      */
      if (status & (HS_UART_RXBRKDET | HS_UART_RXPARITYERR))
      {
         if (status & HS_UART_RXBRKDET)
         {
            status &= ~(HS_UART_RXPARITYERR);
            port->icount.brk++;
            if (uart_handle_break(port))
               goto ignore_char;
         }
         else if (status & HS_UART_RXPARITYERR)
            port->icount.parity++;

         if (status & HS_UART_RXBRKDET)
            flag = TTY_BREAK;
         else if (status & HS_UART_RXPARITYERR)
            flag = TTY_PARITY;
      }

      /* Check overflow conditions */
      if (HS_UART_REG(port)->LSR & HS_UART_LSR_RX_OVERFLOW)
         port->icount.overrun++;

      if( xfer_mode == HS_UART_XFER_MODE_PIO )
      {
         /* Add char to flip buffer */
         tty_insert_flip_char(tty, ch, flag);
      }

#if HS_UART_PRINT_CHARS
      printk(KERN_INFO "%s: Received char: %c\n", __FUNCTION__, ch);
#endif

   ignore_char:
      status = HS_UART_REG(port)->uart_int_stat;
   }

   if( xfer_mode >= HS_UART_XFER_MODE_DMA )
   {
      /* Get number of bytes in header */
      header_bytes = max_count;

      /* Get number of bytes in payload - Skip first byte (SLIP delim) in SWSLIP mode */
      payload_bytes = get_payload_len( ( (xfer_mode == HS_UART_XFER_MODE_DMA_SWSLIP ) ? &pkt_header[1] : pkt_header ) );

      /* Disable RFL interrupt - will be re-enabled at end of DMA*/
      /* FIXME: Do we need to do this? Setting of AFMODE_EN should block AFULL interrupt */
      HS_UART_REG(port)->uart_int_en &= ~HS_UART_RXFIFOAFULL;

      /* Enable receive DMA */
      HS_UART_REG(port)->HO_DMA_CTL = (HS_UART_DMA_CTL_BURSTMODE_EN
                                       | HS_UART_DMA_CTL_AFMODE_EN
                                       | HS_UART_DMA_CTL_FASTMODE_EN
                                       | HS_UART_DMA_CTL_DMA_EN);

      /* Allocate RX DMA buffer */
      rx_buff = kzalloc( (header_bytes+payload_bytes), GFP_NOWAIT);

      if( rx_buff )
      {
         /* Copy over pkt header */
         memcpy( rx_buff, pkt_header, max_count );

         /* Issue DMA */
         hs_uart_issue_rx_dma_request( port, rx_buff, header_bytes, payload_bytes );
      }
      else
      {
         printk(KERN_WARNING "%s: Cannot allocate buffer for DMA receive ", __FUNCTION__);
      }
   }
   else
   {
      /* push flip buffer */
      tty_flip_buffer_push(tty);
   }
}

/*
 * Transmit tx chars
 */
static void hs_uart_tx_chars(struct uart_port *port)
{
   struct circ_buf *xmit = &port->state->xmit;

   HS_UART_REG(port)->uart_int_en &= ~HS_UART_TXFIFOAEMPTY;

   if (port->x_char)
   {
      /* Spin while TX FIFO still has characters to send */
      while (   (HS_UART_REG(port)->LSR & HS_UART_LSR_TX_DATA_AVAIL)
            && !(HS_UART_REG(port)->LSR & HS_UART_LSR_TX_HALT)
            &&  (HS_UART_REG(port)->MCR & HS_UART_MCR_TX_ENABLE)   );

      /* If TX stalls then return */
      if(  (HS_UART_REG(port)->LSR & HS_UART_LSR_TX_HALT) ||
          !(HS_UART_REG(port)->MCR & HS_UART_MCR_TX_ENABLE) )
      {
         /* TX has stalled - Do nothing but drain buffer and return */
      }
      else
      {
         /* TX FIFO is now empty - send character and return*/
         HS_UART_REG(port)->uart_data = port->x_char;
      }
      port->icount.tx++;
      port->x_char = 0;

      return;
   }

   if (uart_circ_empty(xmit) || uart_tx_stopped(port))
   {
      hs_uart_stop_tx(port);
      return;
   }

   /* Write data until TX-FIFO is full OR circbuff is empty */
   while (!(HS_UART_REG(port)->uart_int_stat & HS_UART_TXFIFOFULL))
   {
#if HS_UART_PRINT_CHARS
      printk(KERN_INFO "%s: Sent char: %c\n", __FUNCTION__, xmit->buf[xmit->tail]);
#endif
      if( HS_UART_REG(port)->MCR & HS_UART_MCR_TX_ENABLE )
         HS_UART_REG(port)->uart_data = xmit->buf[xmit->tail];
      xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
      port->icount.tx++;
      if (uart_circ_empty(xmit))
         break;
   }

   if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
      uart_write_wakeup(port);

   if (uart_circ_empty(xmit))
      hs_uart_stop_tx(port);
   else
   {
      /* We were not able to write all tx data to TX FIFO   *
       * Therefore enable interrupt for TX FIFO falling     *
       * below a threshold so that we can continue writing  *
       * rest of the circBuf                                */
      HS_UART_REG(port)->uart_int_en |= HS_UART_TXFIFOAEMPTY ;
   }
}

/*
 * Issue TX DMA request
 */
static void hs_uart_tx_chars_dma(struct uart_port *port)
{
   struct circ_buf *xmit = &port->state->xmit;
   unsigned int num_bytes = uart_circ_chars_pending(xmit);
   char * tx_buff = kzalloc(num_bytes, GFP_NOWAIT);
   int i=0;

   if( tx_buff )
   {
      while( !uart_circ_empty(xmit) && i < num_bytes )
      {
#if HS_UART_PRINT_CHARS
         printk(KERN_INFO "%s: Sent char: %c\n", __FUNCTION__, xmit->buf[xmit->tail]);
#endif
         tx_buff[i] = xmit->buf[xmit->tail];
         xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
         port->icount.tx++;
         i++;
      }

      if( (HS_UART_REG(port)->MCR & HS_UART_MCR_TX_ENABLE) )
      {
         /* Only Issue DMA request if TX is enabled */
         hs_uart_issue_tx_dma_request( port, tx_buff, num_bytes);
      }
   }
   else
   {
      printk(KERN_WARNING "%s: Cannot allocate buffer for DMA transmit ", __FUNCTION__);
   }

   /* Consumed all tx chars, stop further tx */
   hs_uart_stop_tx(port);
}


/*
 * Enable TX transmission
 */
static void hs_uart_start_tx(struct uart_port *port)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);

   if( xfer_mode >= HS_UART_XFER_MODE_DMA )
   {
      hs_uart_tx_chars_dma(port);
   }
   else
   {
      /* If TX is full, enable the TX FIFO almost empty interrupt and return */
      if( HS_UART_REG(port)->uart_int_stat & HS_UART_TXFIFOFULL )
      {
         HS_UART_REG(port)->uart_int_en |= HS_UART_TXFIFOAEMPTY;
      }
      else
      {
         /* IF TX FIFO is not full, attempt to write to it right away */
         hs_uart_tx_chars(port);
      }
   }
}

/*
 * hs_uart ISR
 */
static irqreturn_t hs_uart_int(int irq, void *dev_id)
{
   struct uart_port *port = dev_id;
   unsigned int status, pass_counter = BCM63XX_ISR_PASS_LIMIT;

   spin_lock(&port->lock);

   while ((status = HS_UART_REG(port)->uart_int_stat & HS_UART_REG(port)->uart_int_en))
   {
      /* We have recieved a character in the RX FIFO */
      if (status & HS_UART_RXFIFOAFULL)
         hs_uart_rx_chars(port);

      if (xfer_mode == HS_UART_XFER_MODE_PIO)
      {
         /* TX FIFO now has room */
         if (status & HS_UART_TXFIFOAEMPTY)
            hs_uart_tx_chars(port);
      }

      if (pass_counter-- == 0)
         break;
   }

   /* Clear interrupt at peripheral level */
   HS_UART_REG(port)->uart_int_stat = HS_UART_REG(port)->uart_int_stat;

   spin_unlock(&port->lock);

#if !defined(CONFIG_ARM)
   // Clear the interrupt
   BcmHalInterruptEnable (irq);
#endif

   return IRQ_HANDLED;
}

/*
 * Check if TX FIFO is empty
 */
static unsigned int hs_uart_tx_empty(struct uart_port *port)
{
    HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
   //When no data is available -> TX FIFO is empty
    return HS_UART_REG(port)->LSR & HS_UART_LSR_TX_DATA_AVAIL ? 0 : TIOCSER_TEMT;
}

/*
 * Set hs uart baudrate registers
 */
static int hs_uart_set_baud_rate( struct uart_port *port, int baud )
{
#if HS_UART_CALCULATE_DLHBR
   unsigned int extraCyc, intDiv;

   HS_UART_DEBUG(KERN_INFO "%s: requested baud: %d\n", __FUNCTION__, baud);

#if HS_UART_USE_HIGHRATE
   /* Calculate the integer divider */
   intDiv = port->uartclk / baud ;
   HS_UART_REG(port)->dlbr = 256 - intDiv;
   HS_UART_REG(port)->MCR |= HS_UART_MCR_HIGH_RATE;
#else
   /* Calculate the integer divider */
   intDiv = port->uartclk / ( 16*baud );
   HS_UART_REG(port)->dlbr = 256 - intDiv;
   HS_UART_REG(port)->MCR &= ~HS_UART_MCR_HIGH_RATE;

   /* Calculate the extra cycles of uartclk required to full-  *
    * -fill the bit timing requirements for required baudrate  */
   extraCyc = ( port->uartclk / baud ) - intDiv * 16;
   if( extraCyc )
   {
      /* Equally distribute the extraCycles at the start and end of bit interval */
      HS_UART_REG(port)->dhbr = ( extraCyc/2 | (extraCyc/2) << 4 ) + extraCyc % 2;
   }

   if( extraCyc > 16 )
   {
      printk(KERN_WARNING "hs_uart_set_baud_rate: Cannot set required extra cycles %d ", extraCyc);
   }

   HS_UART_DEBUG(KERN_INFO "dlbr: 0x%08x, dhbr: 0x%08x\n", (unsigned int)HS_UART_REG(port)->dlbr,
                                                           (unsigned int)HS_UART_REG(port)->dhbr );
#endif /* HS_UART_USE_HIGHRATE */

#else

   /* Use predefined values */
   switch( baud )
   {
      case 3000000:
      {
         HS_UART_REG(port)->dhbr = HS_UART_DHBR_3000K;
         HS_UART_REG(port)->dlbr = HS_UART_DLBR_3000K;
      }

    case 115200:
    default:
      {
         HS_UART_REG(port)->dhbr = HS_UART_DHBR_115200;
         HS_UART_REG(port)->dlbr = HS_UART_DLBR_115200;
         baud = 115200;
      }
    break;
   }
#endif /* HS_UART_CALCULATE_DLHBR */

   HS_UART_DEBUG(KERN_INFO "%s: Setting baudrate to: %d\n", __FUNCTION__, baud);

   return 0;
}

/*
 * Set terminal options
 */
static void hs_uart_set_termios(struct uart_port *port,
    struct ktermios *termios, struct ktermios *old)
{
   unsigned long flags;
   unsigned int baud;

   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);

   spin_lock_irqsave(&port->lock, flags);

   /* Disable RX/TX */
   HS_UART_REG(port)->LCR &= ~(HS_UART_LCR_RXEN | HS_UART_LCR_TXOEN);

   /* Ask the core to return selected baudrate value ( bps ) */
#if HS_UART_USE_HIGHRATE
   baud = uart_get_baud_rate(port, termios, old, HS_UART_MIN_BAUD, port->uartclk/8);
#else
   baud = uart_get_baud_rate(port, termios, old, HS_UART_MIN_BAUD, port->uartclk/16);
#endif

   /* Set baud rate registers */
   hs_uart_set_baud_rate(port, baud);

   /* Set stop bits */
   if (termios->c_cflag & CSTOPB)
      HS_UART_REG(port)->LCR |= HS_UART_LCR_STB;
   else
      HS_UART_REG(port)->LCR &= ~(HS_UART_LCR_STB);

   /* Set Parity */
   if (termios->c_cflag & PARENB)
   {
      HS_UART_REG(port)->LCR |= HS_UART_LCR_PEN;
      if (!(termios->c_cflag & PARODD))
         HS_UART_REG(port)->LCR |= HS_UART_LCR_EPS;
   }

   if (termios->c_cflag & CRTSCTS)
   {
      HS_UART_REG(port)->LCR |= HS_UART_LCR_RTSOEN;
      HS_UART_REG(port)->MCR = (HS_UART_REG(port)->MCR | HS_UART_MCR_AUTO_RTS) &
         ~HS_UART_MCR_PROG_RTS;
   }

   /* Update the per-port timeout */
   uart_update_timeout(port, termios->c_cflag, baud);

   /* Unused in this driver */
   port->read_status_mask = 0;
   port->ignore_status_mask = 0;

   /* Finally, re-enable the transmitter and receiver */
   HS_UART_REG(port)->LCR |= (HS_UART_LCR_RXEN | HS_UART_LCR_TXOEN);

   spin_unlock_irqrestore(&port->lock, flags);
}

/*
 * Check serial type
 */
static const char *hs_uart_type(struct uart_port *port)
{
    HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
    return port->type == HS_UART_PORT ? "HS_UART" : NULL;
}

/*
 * Configure/autoconfigure the port.
 */
static void hs_uart_config_port(struct uart_port *port, int flags)
{
   HS_UART_DEBUG(KERN_INFO "%s\n", __FUNCTION__);
   if (flags & UART_CONFIG_TYPE)
   {
      port->type = HS_UART_PORT;
   }
}

/*
 * Initialize serial port registers.
 */
static int init_hs_uart_port( struct uart_port * port )
{
   unsigned int temp;

   /* Disable TX/RX */
   HS_UART_REG(port)->LCR = 0;
   HS_UART_REG(port)->MCR = 0;

   /* Assign HC data */
   HS_UART_REG(port)->ptu_hc = HS_UART_PTU_HC_DATA;

   /* Route hsuart signals out on uart2 pins */
   UART1->prog_out |= ARMUARTEN;

   /* Disable/Clear interrupts */
   HS_UART_REG(port)->uart_int_stat = HS_UART_INT_MASK;
   HS_UART_REG(port)->uart_int_en = HS_UART_INT_MASK_DISABLE;

   /* Set baudrate - 115200bps based on a 50Mhz clock */
   HS_UART_REG(port)->dhbr = HS_UART_DHBR_115200;
   HS_UART_REG(port)->dlbr = HS_UART_DLBR_115200;

   /* Config FCR */
   HS_UART_REG(port)->FCR = 0;

   /* Config LCR - 1 Stop bit */
   HS_UART_REG(port)->LCR = 0; /* HS_UART_LCR_STB is 2 stop-bits */

   /* Config MCR - Enable baud rate adjustment */
   HS_UART_REG(port)->MCR = HS_UART_MCR_BAUD_ADJ_EN;

#if HS_UART_LOOPBACK_ENABLE
   /* Config MCR - Set loopback */
   HS_UART_REG(port)->MCR |= HS_UART_MCR_LOOPBACK;
#endif

   /* Set TX-almost-empty and RX-almost-full thresholds*/
   HS_UART_REG(port)->TFL = HS_UART_TXFIFOAEMPTYTHRESH;

   /* Set Recieve FIFO flow control register */
   HS_UART_REG(port)->RFC = HS_UART_RFC_NO_FC_DATA;

   /* Set escape character register */
   HS_UART_REG(port)->ESC = HS_UART_ESC_NO_SLIP_DATA;

   /* Clear DMA packet lengths, Enable TX DMAs*/
   if( xfer_mode >= HS_UART_XFER_MODE_DMA )
   {
      HS_UART_REG(port)->HOPKT_LEN  = 0;
      HS_UART_REG(port)->HIPKT_LEN  = 0;

      HS_UART_REG(port)->HI_DMA_CTL =  HS_UART_DMA_CTL_BURSTMODE_EN
                                       | HS_UART_DMA_CTL_AFMODE_EN
                                       | HS_UART_DMA_CTL_FASTMODE_EN
                                       | HS_UART_DMA_CTL_DMA_EN;

      HS_UART_REG(port)->HO_BSIZE   = HS_UART_HO_BSIZE_DATA;
      HS_UART_REG(port)->HI_BSIZE   = HS_UART_HI_BSIZE_DATA;
   }

   /* Set thresholds for the RX Almost Full Interrupt + SLIP settings (if enabled) */
   switch( xfer_mode )
   {
      case HS_UART_XFER_MODE_PIO:
         HS_UART_REG(port)->RFL = HS_UART_RXFIFOAFULLTHRESH;
      break;

      case HS_UART_XFER_MODE_DMA_SWSLIP:
         HS_UART_REG(port)->RFL = HS_UART_RXFIFOAFULLTHRESH_DMA_SWSLIP;
      break;

      case HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK:
         HS_UART_REG(port)->LCR |= HS_UART_LCR_SLIP_RX_CRC;
         /*FALLTHRU*/
      case HS_UART_XFER_MODE_DMA_HWSLIP_HWCRC:
         HS_UART_REG(port)->LCR |= HS_UART_LCR_SLIP_TX_CRC;
         /*FALLTHRU*/
      case HS_UART_XFER_MODE_DMA_HWSLIP:
         HS_UART_REG(port)->LCR |= HS_UART_LCR_SLIP;
         HS_UART_REG(port)->FCR |= HS_UART_FCR_SLIP_RESYNC;
         HS_UART_REG(port)->ESC = HS_UART_ESC_SLIP_DATA;
         HS_UART_REG(port)->RFL = HS_UART_RXFIFOAFULLTHRESH_DMA;
      break;

      default:
         HS_UART_REG(port)->RFL = HS_UART_RXFIFOAFULLTHRESH_DMA;
      break;
   }

   /* Clear RX FIFO */
   while( !(HS_UART_REG(port)->uart_int_stat & HS_UART_RXFIFOEMPTY) )
   {
      temp = HS_UART_REG(port)->uart_data;
   }

   /* Config LCR - RX/TX enables */
   HS_UART_REG(port)->LCR |= HS_UART_LCR_RXEN | HS_UART_LCR_TXOEN;

   /* Config MCR - TX state machine enable */
   HS_UART_REG(port)->MCR |= HS_UART_MCR_TX_ENABLE;

   /* Flush TX FIFO */
   while(   (HS_UART_REG(port)->LSR & HS_UART_LSR_TX_DATA_AVAIL)
      && !(HS_UART_REG(port)->LSR & HS_UART_LSR_TX_HALT)  );

#if HS_UART_DUMP_REGS
   hs_uart_dump_regs(port);
#endif

   /* Finally, clear status and enable interrupts on RX FIFO over a threshold */
   HS_UART_REG(port)->uart_int_stat = HS_UART_INT_MASK;
   HS_UART_REG(port)->uart_int_en = HS_UART_RXFIFOAFULL;

   return 0;
}

#if HS_UART_DUMP_REGS
static void hs_uart_dump_regs(struct uart_port * port)
{
   printk(KERN_INFO "ptu_hc        : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->ptu_hc        , (unsigned int)&HS_UART_REG(port)->ptu_hc       );
   printk(KERN_INFO "uart_data     : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->uart_data     , (unsigned int)&HS_UART_REG(port)->uart_data    );
   printk(KERN_INFO "uart_int_stat : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->uart_int_stat , (unsigned int)&HS_UART_REG(port)->uart_int_stat);
   printk(KERN_INFO "uart_int_en   : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->uart_int_en   , (unsigned int)&HS_UART_REG(port)->uart_int_en  );
   printk(KERN_INFO "dhbr          : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->dhbr          , (unsigned int)&HS_UART_REG(port)->dhbr         );
   printk(KERN_INFO "dlbr          : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->dlbr          , (unsigned int)&HS_UART_REG(port)->dlbr         );
   printk(KERN_INFO "ab0           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->ab0           , (unsigned int)&HS_UART_REG(port)->ab0          );
   printk(KERN_INFO "FCR           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->FCR           , (unsigned int)&HS_UART_REG(port)->FCR          );
   printk(KERN_INFO "ab1           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->ab1           , (unsigned int)&HS_UART_REG(port)->ab1          );
   printk(KERN_INFO "LCR           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->LCR           , (unsigned int)&HS_UART_REG(port)->LCR          );
   printk(KERN_INFO "MCR           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->MCR           , (unsigned int)&HS_UART_REG(port)->MCR          );
   printk(KERN_INFO "LSR           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->LSR           , (unsigned int)&HS_UART_REG(port)->LSR          );
   printk(KERN_INFO "MSR           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->MSR           , (unsigned int)&HS_UART_REG(port)->MSR          );
   printk(KERN_INFO "RFL           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->RFL           , (unsigned int)&HS_UART_REG(port)->RFL          );
   printk(KERN_INFO "TFL           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->TFL           , (unsigned int)&HS_UART_REG(port)->TFL          );
   printk(KERN_INFO "RFC           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->RFC           , (unsigned int)&HS_UART_REG(port)->RFC          );
   printk(KERN_INFO "ESC           : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->ESC           , (unsigned int)&HS_UART_REG(port)->ESC          );
   printk(KERN_INFO "HOPKT_LEN     : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->HOPKT_LEN     , (unsigned int)&HS_UART_REG(port)->HOPKT_LEN    );
   printk(KERN_INFO "HIPKT_LEN     : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->HIPKT_LEN     , (unsigned int)&HS_UART_REG(port)->HIPKT_LEN    );
   printk(KERN_INFO "HO_DMA_CTL    : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->HO_DMA_CTL    , (unsigned int)&HS_UART_REG(port)->HO_DMA_CTL   );
   printk(KERN_INFO "HI_DMA_CTL    : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->HI_DMA_CTL    , (unsigned int)&HS_UART_REG(port)->HI_DMA_CTL   );
   printk(KERN_INFO "HO_BSIZE      : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->HO_BSIZE      , (unsigned int)&HS_UART_REG(port)->HO_BSIZE     );
   printk(KERN_INFO "HI_BSIZE      : 0x%08x @ 0x%08x\n", (unsigned int)HS_UART_REG(port)->HI_BSIZE      , (unsigned int)&HS_UART_REG(port)->HI_BSIZE     );
}
#endif

/*
 * RX DMA complete callback - Copy data to flip buffer
 */
static int rx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length )
{
   int i = 0;
   struct tty_struct *tty = port->state->port.tty;
   unsigned char flag = TTY_NORMAL;

   /* Copy data */
   for( i=0; i< length; i++ )
   {
      tty_insert_flip_char(tty, buff_address[i], flag);
   }

   /* Free DMA buffer */
   kfree(buff_address);

   /* Send buffer to tty */
   tty_flip_buffer_push(tty);

   /* Disable RX DMA */
   HS_UART_REG(port)->HO_DMA_CTL &=  ~(HS_UART_DMA_CTL_BURSTMODE_EN
                                       | HS_UART_DMA_CTL_AFMODE_EN
                                       | HS_UART_DMA_CTL_FASTMODE_EN
                                       | HS_UART_DMA_CTL_DMA_EN);

   /* Re-enable RFL interrupt */
   HS_UART_REG(port)->uart_int_en |= HS_UART_RXFIFOAFULL; /* FIXME: This might be too late if tasklet is delayed */

   return 0;
}

/*
 * TX DMA complete callback
 */
static int tx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length )
{
   /* Free DMA buffer */
   kfree(buff_address);

   return 0;
}

/*
 * Set data transfer mode.
 */

static void hs_uart_set_xfer_mode( HS_UART_XFER_MODE mode )
{
   unsigned long flags[UART_NR];
   int i;
   unsigned int uart_active = 0;

   /* Disable ports */
   for (i = 0; i < UART_NR; i++)
   {
      spin_lock_irqsave(&((&hs_uart_ports[i])->lock),flags[i]);
      uart_active |= HS_UART_REG(&hs_uart_ports[i])->LCR & (HS_UART_LCR_RXEN | HS_UART_LCR_TXOEN);
      uart_active |= HS_UART_REG(&hs_uart_ports[i])->MCR & HS_UART_MCR_TX_ENABLE;
   }

   /* Only change mode if uart is not active */
   if( uart_active )
   {
      printk(KERN_ERR "%s: xfer_mode change not allowed when UART is active\n", __FUNCTION__);
   }
   else
   {
      /* Set xfer_mode */
      xfer_mode = mode;
   }

   /* Enable ports */
   for (i = 0; i < UART_NR; i++)
   {
      spin_unlock_irqrestore(&((&hs_uart_ports[i])->lock),flags[i]);
   }
}

/*
 * proc entry write handler.
 */
static ssize_t proc_write_mode(struct file *file, const char __user *buffer,
                                  unsigned long count, void *data)
{
   char mode;

   printk(KERN_INFO "%s: count: %d\n", __FUNCTION__, (int)count);

   copy_from_user(&mode, buffer, 1);
   mode -= '0';

   if( mode < HS_UART_XFER_MODE_MAX && mode >= HS_UART_XFER_MODE_PIO )
   {
      hs_uart_set_xfer_mode( (HS_UART_XFER_MODE)mode);
   }

   return count;
}

/*
 * proc entry read handler.
 */
static int proc_read_mode(char *buf, char **start, off_t offset,
                          int len, int *unused_i, void *unused_v)
{
   int outlen = 0;
   int i;

   len = sprintf(buf,"Current Transfer Mode: %d - %s\n", xfer_mode, xfer_mode_map[xfer_mode].mode_desc);
   buf += len;
   outlen += len;

   len = sprintf(buf,"Available Modes:\n");
   buf += len;
   outlen += len;

   for( i=HS_UART_XFER_MODE_PIO; i< HS_UART_XFER_MODE_MAX; i++ )
   {
      len = sprintf(buf,"%d - %s\n", i, xfer_mode_map[i].mode_desc );
      buf += len;
      outlen += len;
   }

   return outlen;
}

/*
 * Create proc entry
 */
static int hs_uart_create_proc_entries( void )
{
   int ret;

   proc_mkdir(HS_UART_PROC_ENTRY_ROOT, NULL);

   xfer_mode_entry = create_proc_entry(HS_UART_PROC_ENTRY, 0666, NULL);
   if(xfer_mode_entry != NULL)
   {
      xfer_mode_entry->read_proc = proc_read_mode;
      xfer_mode_entry->write_proc = proc_write_mode;
   }
   else
   {
      ret = -ENOMEM;
   }

   return ret;
}

/*
 * Remove proc entry
 */
static int hs_uart_remove_proc_entries( void )
{
   remove_proc_entry(HS_UART_PROC_ENTRY, NULL);
   return 0;
}

/*
 * hs_uart module init
 */
static int __init hs_uart_init(void)
{
   int ret;
   int i;

   printk(KERN_INFO "Bluetooth Serial: Driver $Revision: 1.00 $\n");

   /* Register driver with serial core */
   ret = uart_register_driver(&hs_uart_reg);

   if( ret < 0 )
      goto out;

   /* Initialize PL081 based DMA */
   ret = hs_uart_dma_init( rx_dma_comp_cb, tx_dma_comp_cb);

   if( ret < 0 )
      goto out;

   for (i = 0; i < UART_NR; i++)
   {
      /* Register port with serial core */
      ret = uart_add_one_port(&hs_uart_reg, &hs_uart_ports[i]);

      if( ret < 0 )
         goto out;

#if defined(CONFIG_ARM)
      /* for ARM it will always rearm!! */
      ret = BcmHalMapInterruptEx((FN_HANDLER)hs_uart_int,
                             (unsigned int)&hs_uart_ports[i],
                             hs_uart_ports[i].irq,
                             "hs_uart", INTR_REARM_YES,
                             INTR_AFFINITY_TP1_IF_POSSIBLE);
#else
      ret = BcmHalMapInterruptEx((FN_HANDLER)hs_uart_int,
                             (unsigned int)&hs_uart_ports[i],
                             hs_uart_ports[i].irq,
                             "hs_uart", INTR_REARM_NO,
                             INTR_AFFINITY_TP1_IF_POSSIBLE);
#endif
      if (ret != 0)
      {
         printk(KERN_WARNING "init_hs_uart_port: failed to register "
                             "intr %d rv=%d\n", hs_uart_ports[i].irq, ret);
         goto out;
      }
      else
      {
         HS_UART_DEBUG(KERN_INFO "init_hs_uart_port: Successfully registered "
                             "intr %d\n", hs_uart_ports[i].irq);
      }
   }

   /* Create proc entries */
   hs_uart_create_proc_entries();

   goto init_ok;

out:
   uart_unregister_driver(&hs_uart_reg);
init_ok:
   return ret;
}

/*
 * De-initialize serial port registers
 */
static void deinit_hs_uart_port(struct uart_port * port )
{
   /* Disable and clear interrupts */
   HS_UART_REG(port)->uart_int_en = 0;
   HS_UART_REG(port)->uart_int_stat = HS_UART_INT_MASK;

   /* Stop all DMAs */
   HS_UART_REG(port)->HI_DMA_CTL = 0;
   HS_UART_REG(port)->HO_DMA_CTL = 0;

   /* Config LCR - RX/TX disables */
   HS_UART_REG(port)->LCR &= ~(HS_UART_LCR_RXEN | HS_UART_LCR_TXOEN);

   /* Config MCR - TX state machine disable */
   HS_UART_REG(port)->MCR &= ~(HS_UART_MCR_LOOPBACK | HS_UART_MCR_TX_ENABLE);

}

/*
 * hs_uart module de-init
 */
static void __exit hs_uart_exit(void)
{
   int i;

   for (i = 0; i < UART_NR; i++)
   {
#if !defined(CONFIG_ARM)
      BcmHalInterruptDisable(hs_uart_ports[i].irq);
#endif
      free_irq(hs_uart_ports[i].irq, &hs_uart_ports[i]);
   }
   /* Init PL081 DMA settings */
   hs_uart_dma_deinit();

   /* Unregister with serial core */
   uart_unregister_driver(&hs_uart_reg);

   /* Remove proc entries */
   hs_uart_remove_proc_entries();

}

module_init(hs_uart_init);
module_exit(hs_uart_exit);

MODULE_DESCRIPTION("BCM63XX serial port driver $Revision: 3.00 $");
MODULE_LICENSE("GPL");
