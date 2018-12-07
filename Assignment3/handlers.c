// This file contains C functions to handle particular kinds of exceptions.
// Only a function to handle IRQ exceptions is currently implemented.

// Header files
#include "uart.h"
#include "gpio.h"
#include "irq.h"
#include "sysreg.h"

// Reference to the global shared value
extern unsigned int sharedValue;

// This function detects and handles the interrupts
void IRQ_handler()
{
    unsigned int r;

    // Print out exception type
    uart_puts("\nInside IRQ exception handler:\n");

    // Print out further information about the exception    
    r = getCurrentEL();
    uart_puts("    CurrentEL is:  0x");
    uart_puthex(r);
    uart_puts("\n");

    r = getDAIF();
    uart_puts("    DAIF is:  0x");
    uart_puthex(r);
    uart_puts("\n");

    r = *IRQ_PENDING_2;
    uart_puts("    IRQ_PENDING_2 is:  0x");
    uart_puthex(r);
    uart_puts("\n");
    
    r = *GPEDS0;
    uart_puts("    GPEDS0 is:  0x");
    uart_puthex(r);
    uart_puts("\n");
    	
    // Handle GPIO interrupts in general
    if (*IRQ_PENDING_2 == 0x00100000) 
	{
		// Handle the interrupt associated with GPIO pin 23
		if (*GPEDS0 == (0x1 << 23)) 
		{
			// Clear the interrupt by writing a 1 to the GPIO Event Detect
			// Status Register at bit 23
			*GPEDS0 = (0x1 << 23);
        
			// Handle the interrupt:
			// We do this by setting the sharedValue to 0
			sharedValue = 0;
		}
		// Handle the interrupt associated with GPIO pin 24
		if (*GPEDS0 == (0x1 << 24)) 
		{
			// Clear the interrupt by writing a 1 to the GPIO Event Detect
			// Status Register at bit 24
			*GPEDS0 = (0x1 << 24);
        
			// Handle the interrupt:
			// We do this by setting the sharedValue to 1
			sharedValue = 1;
		}
    }
    // Return to the IRQ exception handler stub
    return;
}
