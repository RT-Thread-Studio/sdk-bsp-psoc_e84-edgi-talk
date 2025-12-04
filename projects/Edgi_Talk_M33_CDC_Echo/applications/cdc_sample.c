#include "USB.h"
#include "USB_CDC.h"
#include <stdio.h>
#include "rtthread.h"

/*******************************************************************************
* Macros
********************************************************************************/
#define USB_CONFIG_DELAY_MS       (50u)    /* In milliseconds */
#define VENDOR_ID                 (0x058B)
#define PRODUCT_ID                (0x0282)
#define RESET_VAL                 (0u)
#define USB_ENABLE_FLAG           (0u)
#define USB_BULK_IN_INTERVAL      (0u)
#define USB_BULK_OUT_INTERVAL     (0u)
#define USB_INT_INTERVAL          (64u)


/*********************************************************************
* Information that are used during enumeration
**********************************************************************/
static const USB_DEVICE_INFO usb_device_info =
{
    VENDOR_ID,                    /* VendorId    */
    PRODUCT_ID,                   /* ProductId    */
    "Infineon Technologies",      /* VendorName   */
    "CDC Code Example",           /* ProductName  */
    "12345678"                    /* SerialNumber */
};


/*******************************************************************************
* Global Variables
********************************************************************************/
static USB_CDC_HANDLE usb_cdc_handle;
static char           read_buffer[USB_HS_BULK_MAX_PACKET_SIZE];
static char           write_buffer[USB_HS_BULK_MAX_PACKET_SIZE];


/*********************************************************************
* Function Name: usb_add_cdc
**********************************************************************
* Summary:
*  Add communication device class to USB stack
*
* Parameters:
*  void
*
* Return:
*  void
**********************************************************************/
static void usb_add_cdc(void)
{
    static U8             out_buffer[USB_HS_BULK_MAX_PACKET_SIZE];
    USB_CDC_INIT_DATA     init_data;
    USB_ADD_EP_INFO       ep_bulk_in;
    USB_ADD_EP_INFO       ep_bulk_out;
    USB_ADD_EP_INFO       ep_int_in;

    memset(&init_data, 0, sizeof(init_data));
    ep_bulk_in.Flags          = USB_ENABLE_FLAG;               /* Flags not used */
    ep_bulk_in.InDir          = USB_DIR_IN;                    /* IN direction (Device to Host) */
    ep_bulk_in.Interval       = USB_BULK_IN_INTERVAL;          /* Interval not used for Bulk endpoints */
    ep_bulk_in.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (512B for Bulk in High-Speed) */
    ep_bulk_in.TransferType   = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    init_data.EPIn  = USBD_AddEPEx(&ep_bulk_in, NULL, 0);

    ep_bulk_out.Flags         = USB_ENABLE_FLAG;               /* Flags not used */
    ep_bulk_out.InDir         = USB_DIR_OUT;                   /* OUT direction (Host to Device) */
    ep_bulk_out.Interval      = USB_BULK_OUT_INTERVAL;         /* Interval not used for Bulk endpoints */
    ep_bulk_out.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (512B for Bulk in High-Speed) */
    ep_bulk_out.TransferType  = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    init_data.EPOut = USBD_AddEPEx(&ep_bulk_out, out_buffer, sizeof(out_buffer));

    ep_int_in.Flags           = USB_ENABLE_FLAG;               /* Flags not used */
    ep_int_in.InDir           = USB_DIR_IN;                    /* IN direction (Device to Host) */
    ep_int_in.Interval        = USB_INT_INTERVAL;              /* Interval of 8 ms (64 * 125us) */
    ep_int_in.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE ;   /* Maximum packet size (64 for Interrupt) */
    ep_int_in.TransferType    = USB_TRANSFER_TYPE_INT;         /* Endpoint type - Interrupt */
    init_data.EPInt = USBD_AddEPEx(&ep_int_in, NULL, 0);

    usb_cdc_handle = USBD_CDC_Add(&init_data);
}


int cdc_sample(void)
{
    int num_bytes_received = 0;
    int num_bytes_to_write = 0;

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    rt_kprintf("\x1b[2J\x1b[;H");

    rt_kprintf("****************** "
               "PSOC Edge MCU: CDC echo using emUSB-device"
               "****************** \r\n\n");

    /* Initializes the USB stack */
    USBD_Init();

    /* Endpoint Initialization for CDC class */
    usb_add_cdc();

    /* Set device info used in enumeration */
    USBD_SetDeviceInfo(&usb_device_info);

    /* Start the USB stack */
    USBD_Start();


    for (;;)
    {
        /* Wait for configuration */
        while (USB_STAT_CONFIGURED != (USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)))
        {
            rt_thread_mdelay(USB_CONFIG_DELAY_MS);
        }

        num_bytes_received = USBD_CDC_Receive(usb_cdc_handle, read_buffer, sizeof(read_buffer), 0);

        rt_memcpy(write_buffer + num_bytes_to_write, read_buffer, num_bytes_received);

        num_bytes_to_write +=  num_bytes_received;

        if ((num_bytes_to_write > RESET_VAL) && (read_buffer[num_bytes_received - 1] == '\n'))
        {

            /* Sending one packet to host */
            USBD_CDC_Write(usb_cdc_handle, write_buffer, num_bytes_to_write, 0);

            /* Waits for specified number of bytes to be written to host */
            USBD_CDC_WaitForTX(usb_cdc_handle, 0);

            /* If the last sent packet is exactly the maximum packet
            *  size, it is followed by a zero-length packet to assure
            *  that the end of the segment is properly identified by
            *  the terminal.
            */

            if (num_bytes_to_write == sizeof(write_buffer))
            {
                /* Sending zero-length packet to host */
                USBD_CDC_Write(usb_cdc_handle, NULL, 0, 0);

                /* Waits for specified number of bytes to be written to host */
                USBD_CDC_WaitForTX(usb_cdc_handle, 0);
            }

            num_bytes_to_write = RESET_VAL;
        }

    }
}
MSH_CMD_EXPORT(cdc_sample, cdc_sample);
