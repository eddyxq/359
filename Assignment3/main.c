/* This program uses the Raspberry Pi GPIO pins to drive 3 LEDs and respond to 2 
 * push button switches.
 *
 * The Program will have 2 states:
 * 
 *		State 1:	LEDs are illuminated one after the other in the pattern:  
 *					1, 2, 3, 1, 2, 3, etc. Each will be on for about 0.5 second.
 *
 *		State 2:	LEDs will be turned on one after the other in the pattern:  
 *					3, 2, 1, 3, 2, 1, etc. Each will be lit for about 0.25 second. 
 */

// Include files
#include "uart.h"
#include "sysreg.h"
#include "gpio.h"
#include "irq.h"
#include "systimer.h"

// Function prototypes
void init_GPIO23_to_risingEdgeInterrupt();
void init_GPIO24_to_fallingEdgeInterrupt();

void init_GPIO17_to_output();
void set_GPIO17();
void clear_GPIO17();

void init_GPIO27_to_output();
void set_GPIO27();
void clear_GPIO27();

void init_GPIO22_to_output();
void set_GPIO22();
void clear_GPIO22();

// Declare a global shared variable
unsigned int sharedValue;

// Starting point of the program
void main()
{
    unsigned int r;
    unsigned int localValue;
    sharedValue = 0;

    // Set up the UART serial port
    uart_init();
    
    // Query the current exception level
    r = getCurrentEL();
    
    // Print out the exception level
    uart_puts("Current exception level is:  0x");
    uart_puthex(r);
    uart_puts("\n");
    
    // Get the SPSel value
    r = getSPSel();
    
    // Print out the SPSel value
    uart_puts("SPSel is:  0x");
    uart_puthex(r);
    uart_puts("\n");
        
    // Query the current DAIF flag values
    r = getDAIF();
    
    // Print out the DAIF flag values
    uart_puts("Initial DAIF flags are:  0x");
    uart_puthex(r);
    uart_puts("\n");
    
    // Print out initial values of the Interrupt Enable Register 2
    r = *IRQ_ENABLE_IRQS_2;
    uart_puts("Initial IRQ_ENABLE_IRQS_2 is:  0x");
    uart_puthex(r);
    uart_puts("\n");

    // Print out initial values the GPREN0 register (rising edge interrupt
    // enable register)
    r = *GPREN0;
    uart_puts("Initial GPREN0 is:  0x");
    uart_puthex(r);
    uart_puts("\n");
 
    // Initialize the sharedValue global variable and
    // and set the local variable to be same value
    localValue = sharedValue = 0;
    
    // Initialize all the required pins
    init_GPIO23_to_risingEdgeInterrupt();
    init_GPIO24_to_fallingEdgeInterrupt();
    init_GPIO17_to_output();
    init_GPIO27_to_output();
    init_GPIO22_to_output();

    // Enable IRQ Exceptions
    enableIRQ();
    
    // Query the DAIF flag values
    r = getDAIF();
    
    // Print out the new DAIF flag values
    uart_puts("\nNew DAIF flags are:  0x");
    uart_puthex(r);
    uart_puts("\n");
    
    // Print out new value of the Interrupt Enable Register 2
    r = *IRQ_ENABLE_IRQS_2;
    uart_puts("New IRQ_ENABLE_IRQS_2 is:  0x");
    uart_puthex(r);
    uart_puts("\n");
    
    // Print out new value of the GPREN0 register
    r = *GPREN0;
    uart_puts("New GPREN0 is:  0x");
    uart_puthex(r);
    uart_puts("\n");
    
    // Print out a message to the console
    uart_puts("\nRising Edge IRQ program starting.\n");
    
    // Loop forever, waiting for interrupts to change the shared value
    while (1) 
    {
        if (sharedValue == 0)
        {
            set_GPIO17();
            clear_GPIO27();
            clear_GPIO22();
            microsecond_delay(500000);

            clear_GPIO17();
            set_GPIO27();
            clear_GPIO22();
            microsecond_delay(500000);

            clear_GPIO17();
            clear_GPIO27();
            set_GPIO22();
            microsecond_delay(500000);
        }
        else
        {
            clear_GPIO17();
            clear_GPIO27();
            set_GPIO22();
            microsecond_delay(250000);

            clear_GPIO17();
            set_GPIO27();
            clear_GPIO22();
            microsecond_delay(250000);

            set_GPIO17();
            clear_GPIO27();
            clear_GPIO22();
            microsecond_delay(250000);
        }
    }
}

// This function sets GPIO pin 23 to an input pin without
// any internal pull-up or pull-down resistors.
void init_GPIO23_to_risingEdgeInterrupt()
{
    register unsigned int r;
    
    // Get the current contents of the GPIO Function Select Register 2
    r = *GPFSEL2;

    // We clear the bits by ANDing with a 000 bit pattern in the field. This
    // sets the pin to be an input pin
    r &= ~(0x7 << 9);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 2
    *GPFSEL2 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 17. We follow the
    // procedure outlined on page 101 of the BCM2837 ARM Peripherals manual. We
    // will pull down the pin using an external resistor connected to ground.

    // Disable internal pull-up/pull-down by setting bits 0:1
    // to 00 in the GPIO Pull-Up/Down Register 
    *GPPUD = 0x0;

    // Wait 150 cycles to provide the required set-up time 
    // for the control signal
    r = 150;
    while (r--) {
        asm volatile("nop");
    }

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 17 to
    // clock in the control signal for GPIO pin 17. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 23);

    // Wait 150 cycles to provide the required hold time
    // for the control signal
    r = 150;
    while (r--) {
        asm volatile("nop");
    }

    // Clear all bits in the GPIO Pull-Up/Down Clock Register 0
    // in order to remove the clock
    *GPPUDCLK0 = 0;
    
    // Set pin 23 to so that it generates an interrupt on a rising edge.
    // We do so by setting bit 23 in the GPIO Rising Edge Detect Enable
    // Register 0 to a 1 value (p. 97 in the Broadcom manual).
    *GPREN0 = (0x1 << 23);
    
    // Enable the GPIO IRQS for ALL the GPIO pins by setting IRQ 52
    // GPIO_int[3] in the Interrupt Enable Register 2 to a 1 value.
    // See p. 117 in the Broadcom Peripherals Manual.
    *IRQ_ENABLE_IRQS_2 = (0x1 << 20);
}

// This function sets GPIO pin 24 to an input pin without
// any internal pull-up or pull-down resistors.
void init_GPIO24_to_fallingEdgeInterrupt()
{
    register unsigned int r;
    
    // Get the current contents of the GPIO Function Select Register 2
    r = *GPFSEL2;

    // We clear the bits by ANDing with a 000 bit pattern in the field. This
    // sets the pin to be an input pin
    r &= ~(0x7 << 12);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 2
    *GPFSEL2 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 24. We follow the
    // procedure outlined on page 101 of the BCM2837 ARM Peripherals manual. We
    // will pull down the pin using an external resistor connected to ground.

    // Disable internal pull-up/pull-down by setting bits 0:1
    // to 00 in the GPIO Pull-Up/Down Register 
    *GPPUD = 0x0;

    // Wait 150 cycles to provide the required set-up time 
    // for the control signal
    r = 150;
    while (r--) {
        asm volatile("nop");
    }

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 24 to
    // clock in the control signal for GPIO pin 24. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 24);

    // Wait 150 cycles to provide the required hold time
    // for the control signal
    r = 150;
    while (r--) {
        asm volatile("nop");
    }

    // Clear all bits in the GPIO Pull-Up/Down Clock Register 0
    // in order to remove the clock
    *GPPUDCLK0 = 0;
    
    // Set pin 24 to so that it generates an interrupt on a rising edge.
    // We do so by setting bit 17 in the GPIO Rising Edge Detect Enable
    // Register 0 to a 1 value (p. 97 in the Broadcom manual).
    *GPFEN0 = (0x1 << 24);
    
    // Enable the GPIO IRQS for ALL the GPIO pins by setting IRQ 52
    // GPIO_int[3] in the Interrupt Enable Register 2 to a 1 value.
    // See p. 117 in the Broadcom Peripherals Manual.
    *IRQ_ENABLE_IRQS_2 = (0x1 << 20);
}

// This function sets GPIO pin 17 to an output pin without
// any pull-up or pull-down resistors.
void init_GPIO17_to_output()
{
    register unsigned int r;
    
    // Get the current contents of the GPIO Function Select Register 1
    r = *GPFSEL1;

    // We clear the bits by ANDing with a 000 bit pattern in the field.
    r &= ~(0x7 << 21);

    // Set the field FSEL23 to 001, which sets pin 17 to an output pin.
    // We do so by ORing the bit pattern 001 into the field.
    r |= (0x1 << 21);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 1
    *GPFSEL1 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 17. We follow the
    // procedure outlined on page 101 of the BCM2837 ARM Peripherals manual. The
    // internal pull-up and pull-down resistor isn't needed for an output pin.

    // Disable pull-up/pull-down by setting bits 0:1
    // to 00 in the GPIO Pull-Up/Down Register 
    *GPPUD = 0x0;

    // Wait 150 cycles to provide the required set-up time 
    // for the control signal
    r = 150;
    while (r--) {
      asm volatile("nop");
    }

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 17 to
    // clock in the control signal for GPIO pin 17. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 17);

    // Wait 150 cycles to provide the required hold time
    // for the control signal
    r = 150;
    while (r--) {
      asm volatile("nop");
    }

    // Clear all bits in the GPIO Pull-Up/Down Clock Register 0
    // in order to remove the clock
    *GPPUDCLK0 = 0;
}

//This function sets the GPIO output pin 23 to a 1 (high) level.
void set_GPIO17()
{
	  register unsigned int r;
	  
	  // Put a 1 into the SET23 field of the GPIO Pin Output Set Register 0
	  r = (0x1 << 17);
	  *GPSET0 = r;
}

// This function clears the GPIO output pin 23 to a 0 (low) level.
void clear_GPIO17()
{
	  register unsigned int r;
	  
	  // Put a 1 into the CLR23 field of the GPIO Pin Output Clear Register 0
	  r = (0x1 << 17);
	  *GPCLR0 = r;
}

// This function sets GPIO pin 27 to an output pin without
// any pull-up or pull-down resistors.

void init_GPIO27_to_output()
{
    register unsigned int r;
    
    // Get the current contents of the GPIO Function Select Register 2
    r = *GPFSEL2;

    // We clear the bits by ANDing with a 000 bit pattern in the field.
    r &= ~(0x7 << 21);

    // Set the field FSEL23 to 001, which sets pin 27 to an output pin.
    // We do so by ORing the bit pattern 001 into the field.
    r |= (0x1 << 21);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 2
    *GPFSEL2 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 27. We follow the
    // procedure outlined on page 101 of the BCM2837 ARM Peripherals manual. The
    // internal pull-up and pull-down resistor isn't needed for an output pin.

    // Disable pull-up/pull-down by setting bits 0:1
    // to 00 in the GPIO Pull-Up/Down Register 
    *GPPUD = 0x0;

    // Wait 150 cycles to provide the required set-up time 
    // for the control signal
    r = 150;
    while (r--) {
      asm volatile("nop");
    }

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 27 to
    // clock in the control signal for GPIO pin 27. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 27);

    // Wait 150 cycles to provide the required hold time
    // for the control signal
    r = 150;
    while (r--) {
      asm volatile("nop");
    }

    // Clear all bits in the GPIO Pull-Up/Down Clock Register 0
    // in order to remove the clock
    *GPPUDCLK0 = 0;
}

// This function sets the GPIO output pin 23 to a 1 (high) level.
void set_GPIO27()
{
	  register unsigned int r;
	  
	  // Put a 1 into the SET27 field of the GPIO Pin Output Set Register 0
	  r = (0x1 << 27);
	  *GPSET0 = r;
}

// This function clears the GPIO output pin 27 to a 0 (low) level.
void clear_GPIO27()
{
	  register unsigned int r;
	  
	  // Put a 1 into the CLR27 field of the GPIO Pin Output Clear Register 0
	  r = (0x1 << 27);
	  *GPCLR0 = r;
}

// This function sets GPIO pin 22 to an output pin without
// any pull-up or pull-down resistors.
void init_GPIO22_to_output()
{
    register unsigned int r;
    
    // Get the current contents of the GPIO Function Select Register 2
    r = *GPFSEL2;

    // We clear the bits by ANDing with a 000 bit pattern in the field.
    r &= ~(0x7 << 6);

    // Set the field FSEL22 to 001, which sets pin 22 to an output pin.
    // We do so by ORing the bit pattern 001 into the field.
    r |= (0x1 << 6);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 2
    *GPFSEL2 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 22. We follow the
    // procedure outlined on page 101 of the BCM2837 ARM Peripherals manual. The
    // internal pull-up and pull-down resistor isn't needed for an output pin.

    // Disable pull-up/pull-down by setting bits 0:1
    // to 00 in the GPIO Pull-Up/Down Register 
    *GPPUD = 0x0;

    // Wait 150 cycles to provide the required set-up time 
    // for the control signal
    r = 150;
    while (r--) {
      asm volatile("nop");
    }

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 22 to
    // clock in the control signal for GPIO pin 22. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 22);

    // Wait 150 cycles to provide the required hold time
    // for the control signal
    r = 150;
    while (r--) {
      asm volatile("nop");
    }

    // Clear all bits in the GPIO Pull-Up/Down Clock Register 0
    // in order to remove the clock
    *GPPUDCLK0 = 0;
}

// This function sets the GPIO output pin 22 to a 1 (high) level.
void set_GPIO22()
{
	  register unsigned int r;
	  
	  // Put a 1 into the SET22 field of the GPIO Pin Output Set Register 0
	  r = (0x1 << 22);
	  *GPSET0 = r;
}

// This function clears the GPIO output pin 22 to a 0 (low) level.

void clear_GPIO22()
{
	  register unsigned int r;
	  
	  // Put a 1 into the CLR22 field of the GPIO Pin Output Clear Register 0
	  r = (0x1 << 22);
	  *GPCLR0 = r;
}
