// This program demonstrates how to initialize a frame buffer for a
// 1024 x 768 display, and how to draw on it using a simple checker board
// pattern.

// Included header files
#include "uart.h"
#include "framebuffer.h"
#include "gpio.h"
#include "systimer.h"

// Function prototypes
unsigned short get_SNES();
void init_GPIO9_to_output();
void set_GPIO9();
void clear_GPIO9();
void init_GPIO11_to_output();
void set_GPIO11();
void clear_GPIO11();
void init_GPIO10_to_input();
unsigned int get_GPIO10();

/*
maze legend:
    0 = path
    1 = wall
    2 = player
    3 = exit
*/
int maze[12][16] = 
    {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 
        {1, 0, 9, 0, 9, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 9, 0, 9, 0, 9, 0, 0, 0, 9, 9, 0, 1},
        {1, 0, 9, 9, 9, 0, 9, 0, 9, 9, 9, 9, 9, 9, 0, 1},
        {1, 0, 0, 9, 0, 0, 9, 0, 0, 0, 0, 0, 0, 9, 0, 1},
        {1, 9, 0, 0, 0, 9, 9, 9, 9, 9, 0, 9, 9, 9, 0, 1},
        {1, 0, 0, 9, 0, 9, 0, 0, 0, 9, 0, 9, 0, 0, 0, 1},
        {1, 0, 9, 9, 0, 9, 0, 9, 9, 9, 0, 9, 0, 9, 9, 1},
        {1, 0, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 3},
        {1, 9, 0, 9, 0, 0, 0, 9, 0, 9, 0, 9, 9, 9, 0, 1},
        {1, 0, 0, 9, 0, 9, 0, 0, 0, 9, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

int original[12][16] = 
    {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 
        {1, 0, 9, 0, 9, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 9, 0, 9, 0, 9, 0, 0, 0, 9, 9, 0, 1},
        {1, 0, 9, 9, 9, 0, 9, 0, 9, 9, 9, 9, 9, 9, 0, 1},
        {1, 0, 0, 9, 0, 0, 9, 0, 0, 0, 0, 0, 0, 9, 0, 1},
        {1, 9, 0, 0, 0, 9, 9, 9, 9, 9, 0, 9, 9, 9, 0, 1},
        {1, 0, 0, 9, 0, 9, 0, 0, 0, 9, 0, 9, 0, 0, 0, 1},
        {1, 0, 9, 9, 0, 9, 0, 9, 9, 9, 0, 9, 0, 9, 9, 1},
        {1, 0, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 3},
        {1, 9, 0, 9, 0, 0, 0, 9, 0, 9, 0, 9, 9, 9, 0, 1},
        {1, 0, 0, 9, 0, 9, 0, 0, 0, 9, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

//my maze functions
int getRow();
int getCol();
void move(int choice);
void move2(int choice);
int moveValid(int playerRow, int playerCol);
int hitWall(int currentHp);
int isDestructable(int wall);
void newGame();

//flags
int gameStarted = 0;
int gameWon = 0;

// starting point of program
void main()
{
    unsigned short data, currentState = 0xFFFF;

    // Set up the UART serial port
    uart_init();
    
    // Set up GPIO pin #9 for output (LATCH output)
    init_GPIO9_to_output();
    
    // Set up GPIO pin #11 for output (CLOCK output)
    init_GPIO11_to_output();
    
    // Set up GPIO pin #10 for input (DATA input)
    init_GPIO10_to_input();
    
    // Clear the LATCH line (GPIO 9) to low
    clear_GPIO9();
    
    // Set CLOCK line (GPIO 11) to high
    set_GPIO11();
    
    // Print out a message to the console
    uart_puts("SNES Controller Program starting.\n");

    //char c;

    // Initialize the UART terminal
    uart_init();

    // Initialize the frame buffer
    initFrameBuffer();

    // Loop forever, echoing characters received from the console
    // on a separate line with : : around the character
    while (1) 
    {
        // Read data from the SNES controller
        data = get_SNES();

        // Write out data if the state of the controller has changed
        if (data != currentState) 
        {
            // Write the data out to the console in hexadecimal
            uart_puts("0x");
            uart_puthex(data);
            uart_puts("\n");

            // Record the state of the controller
            currentState = data;
        }

        /*
            SNES controller input

            a      - 0x00000100
            b      - 0x00000001
            x      - 0x00000200
            y      - 0x00000002

            up     - 0x00000010
            down   - 0x00000020
            left   - 0x00000040
            right  - 0x00000080

            start  - 0x00000008
            select - 0x00000004

            bonus:

            x up   - 0x00000210
            x down - 0x00000220
            x left - 0x00000240
            x right- 0x00000280
        */

        //up
        if ((data == 0x00000010) && (gameStarted == 1) && (gameWon == 0))
        {
            if (maze[getRow()-1][getCol()] == 10)
            {
                move2(1);
            }
            else
            {
                move(1);
            }
        }
        //down
        else if ((data == 0x00000020) && (gameStarted == 1) && (gameWon == 0))
        {
            if (maze[getRow()+1][getCol()] == 10)
            {
                move2(2);
            }
            else
            {
                move(2);
            }
        }
        //left
        else if ((data == 0x00000040) && (gameStarted == 1) && (gameWon == 0))
        {
            if (maze[getRow()][getCol()-1] == 10)
            {
                move2(3);
            }
            else
            {
                move(3);
            }
        }
        //right
        else if ((data == 0x00000080) && (gameStarted == 1) && (gameWon == 0))
        {
            if (maze[getRow()][getCol()+1] == 10)
            {
                move2(4);
            }
            else
            {
                move(4);
            }
        }
        //destroy up
        else if ((data == 0x00000210) && (gameStarted == 1) && (gameWon == 0) && (isDestructable(maze[getRow()-1][getCol()]) == 1))
        {
            maze[getRow()-1][getCol()] = hitWall(maze[getRow()-1][getCol()]);
        }
        //destroy down
        else if ((data == 0x00000220) && (gameStarted == 1) && (gameWon == 0) && (isDestructable(maze[getRow()+1][getCol()]) == 1))
        {
            maze[getRow()+1][getCol()] = hitWall(maze[getRow()+1][getCol()]);
        }
        //destroy right
        else if ((data == 0x00000280) && (gameStarted == 1) && (gameWon == 0) && (isDestructable(maze[getRow()][getCol()+1]) == 1))
        {
            maze[getRow()][getCol()+1] = hitWall(maze[getRow()][getCol()+1]);
        }
        //destroy left
        else if ((data == 0x00000240) && (gameStarted == 1) && (gameWon == 0) && (isDestructable(maze[getRow()][getCol()-1]) == 1))
        {
            maze[getRow()][getCol()-1] = hitWall(maze[getRow()][getCol()-1]);
        }
        //start
        else if (data == 0x00000008)
        {
            //if game is not started yet, start the game
            if (gameStarted == 0)
            {
                //loads player to starting location
                maze[2][0] = 2;
                gameStarted = 1;
            }
            //if game is won, restart the game
            if (gameWon == 1)
            {
                //clears player off the exit
                maze[8][15] = 3;
                gameStarted = 0;
                gameWon = 0;
                newGame();
            }
        }

        // Draw on the frame buffer and display it
        displayFrameBuffer(maze);

        // Delay 
        microsecond_delay(1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       get_SNES
//
//  Arguments:      none
//
//  Returns:        A short integer with the button presses encoded with 16
//                  bits. 1 means pressed, and 0 means unpressed. Bit 0 is
//                  button B, Bit 1 is button Y, etc. up to Bit 11, which is
//                  button R. Bits 12-15 are always 0.
//
//  Description:    This function samples the button presses on the SNES
//                  controller, and returns an encoding of these in a 16-bit
//                  integer. We assume that the CLOCK output is already high,
//                  and set the LATCH output to high for 12 microseconds. This
//                  causes the controller to latch the values of the button
//                  presses into its internal register. We then clock this data
//                  to the CPU over the DATA line in a serial fashion, by
//                  pulsing the CLOCK line low 16 times. We read the data on
//                  the falling edge of the clock. The rising edge of the clock
//                  causes the controller to output the next bit of serial data
//                  to be place on the DATA line. The clock cycle is 12
//                  microseconds long, so the clock is low for 6 microseconds,
//                  and then high for 6 microseconds. 
//
////////////////////////////////////////////////////////////////////////////////

unsigned short get_SNES()
{
    int i;
    unsigned short data = 0;
    unsigned int value;
	
	
    // Set LATCH to high for 12 microseconds. This causes the controller to
    // latch the values of button presses into its internal register. The
    // first serial bit also becomes available on the DATA line.
    set_GPIO9();
    microsecond_delay(12);
    clear_GPIO9();
	
    // Output 16 clock pulses, and read 16 bits of serial data
    for (i = 0; i < 16; i++) {
	// Delay 6 microseconds (half a cycle)
	microsecond_delay(6);
		
	// Clear the CLOCK line (creates a falling edge)
	clear_GPIO11();
		
	// Read the value on the input DATA line
	value = get_GPIO10();
		
	// Store the bit read. Note we convert a 0 (which indicates a button
	// press) to a 1 in the returned 16-bit integer. Unpressed buttons
	// will be encoded as a 0.
	if (value == 0) {
	    data |= (0x1 << i);
	}
		
	// Delay 6 microseconds (half a cycle)
	microsecond_delay(6);
		
	// Set the CLOCK to 1 (creates a rising edge). This causes the
	// controller to output the next bit, which we read half a
	// cycle later.
	set_GPIO11();
    }
	
    // Return the encoded data
    return data;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       init_GPIO9_to_output
//
//  Arguments:      none
//
//  Returns:        void
//
//  Description:    This function sets GPIO pin 9 to an output pin without
//                  any pull-up or pull-down resistors.
//
////////////////////////////////////////////////////////////////////////////////

void init_GPIO9_to_output()
{
    register unsigned int r;
    
    // Get the current contents of the GPIO Function Select Register 0
    r = *GPFSEL0;

    // Clear bits 27 - 29. This is the field FSEL9, which maps to GPIO pin 9.
    // We clear the bits by ANDing with a 000 bit pattern in the field.
    r &= ~(0x7 << 27);

    // Set the field FSEL9 to 001, which sets pin 9 to an output pin.
    // We do so by ORing the bit pattern 001 into the field.
    r |= (0x1 << 27);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 0
    *GPFSEL0 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 9. We follow the
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

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 9 to
    // clock in the control signal for GPIO pin 9. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 9);

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

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       set_GPIO9
//
//  Arguments:      none
//
//  Returns:        void
//
//  Description:    This function sets the GPIO output pin 9
//                  to a 1 (high) level.
//
////////////////////////////////////////////////////////////////////////////////

void set_GPIO9()
{
    register unsigned int r;
	  
    // Put a 1 into the SET9 field of the GPIO Pin Output Set Register 0
    r = (0x1 << 9);
    *GPSET0 = r;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       clear_GPIO9
//
//  Arguments:      none
//
//  Returns:        void
//
//  Description:    This function clears the GPIO output pin 9
//                  to a 0 (low) level.
//
////////////////////////////////////////////////////////////////////////////////

void clear_GPIO9()
{
    register unsigned int r;
	  
    // Put a 1 into the CLR9 field of the GPIO Pin Output Clear Register 0
    r = (0x1 << 9);
    *GPCLR0 = r;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       init_GPIO11_to_output
//
//  Arguments:      none
//
//  Returns:        void
//
//  Description:    This function sets GPIO pin 11 to an output pin without
//                  any pull-up or pull-down resistors.
//
////////////////////////////////////////////////////////////////////////////////

void init_GPIO11_to_output()
{
    register unsigned int r;
    
    // Get the current contents of the GPIO Function Select Register 1
    r = *GPFSEL1;

    // Clear bits 3 - 5. This is the field FSEL11, which maps to GPIO pin 11.
    // We clear the bits by ANDing with a 000 bit pattern in the field.
    r &= ~(0x7 << 3);

    // Set the field FSEL11 to 001, which sets pin 9 to an output pin.
    // We do so by ORing the bit pattern 001 into the field.
    r |= (0x1 << 3);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 1
    *GPFSEL1 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 11. We follow the
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

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 11 to
    // clock in the control signal for GPIO pin 11. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 11);

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

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       set_GPIO11
//
//  Arguments:      none
//
//  Returns:        void
//
//  Description:    This function sets the GPIO output pin 11
//                  to a 1 (high) level.
//
////////////////////////////////////////////////////////////////////////////////

void set_GPIO11()
{
    register unsigned int r;
	  
    // Put a 1 into the SET11 field of the GPIO Pin Output Set Register 0
    r = (0x1 << 11);
    *GPSET0 = r;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       clear_GPIO11
//
//  Arguments:      none
//
//  Returns:        void
//
//  Description:    This function clears the GPIO output pin 11
//                  to a 0 (low) level.
//
////////////////////////////////////////////////////////////////////////////////

void clear_GPIO11()
{
    register unsigned int r;
	  
    // Put a 1 into the CLR11 field of the GPIO Pin Output Clear Register 0
    r = (0x1 << 11);
    *GPCLR0 = r;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       init_GPIO10_to_input
//
//  Arguments:      none
//
//  Returns:        void
//
//  Description:    This function sets GPIO pin 10 to an input pin without
//                  any internal pull-up or pull-down resistors. Note that
//                  a pull-down (or pull-up) resistor must be used externally
//                  on the bread board circuit connected to the pin. Be sure
//                  that the pin high level is 3.3V (definitely NOT 5V).
//
////////////////////////////////////////////////////////////////////////////////

void init_GPIO10_to_input()
{
    register unsigned int r;
    
    
    // Get the current contents of the GPIO Function Select Register 1
    r = *GPFSEL1;

    // Clear bits 0 - 2. This is the field FSEL10, which maps to GPIO pin 10.
    // We clear the bits by ANDing with a 000 bit pattern in the field. This
    // sets the pin to be an input pin.
    r &= ~(0x7 << 0);

    // Write the modified bit pattern back to the
    // GPIO Function Select Register 1
    *GPFSEL1 = r;

    // Disable the pull-up/pull-down control line for GPIO pin 10. We follow the
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

    // Write to the GPIO Pull-Up/Down Clock Register 0, using a 1 on bit 10 to
    // clock in the control signal for GPIO pin 10. Note that all other pins
    // will retain their previous state.
    *GPPUDCLK0 = (0x1 << 10);

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

////////////////////////////////////////////////////////////////////////////////
//
//  Function:       get_GPIO10
//
//  Arguments:      none
//
//  Returns:        1 if the pin level is high, and 0 if the pin level is low.
//
//  Description:    This function gets the current value of pin 10.
//
////////////////////////////////////////////////////////////////////////////////

unsigned int get_GPIO10()
{
    register unsigned int r;
	  
	  
    // Get the current contents of theminicom -b 115200 -D /dev/ttyUSB0GPIO Pin Level Register 0
    r = *GPLEV0;
	  
    // Isolate pin 10, and return its minicom -b 115200 -D /dev/ttyUSB0alue (a 0 if low, or a 1 if high)
    return ((r >> 10) & 0x1);
}

/**
 * This method returns the row index of the player
 */
int getRow()
{
    int playerRow = 0;
    for (int row = 0; row<12; row++)
    {
            for (int col = 0; col<16; col++)
            {
                if (maze[row][col] == 2)
                {
                    playerRow = row;
                }
            } 
    }
    return playerRow;
}
/**
 * This method returns the column index of the player
 */
int getCol()
{
    int playerCol = 0;
    for (int row = 0; row<12; row++)
    {
            for (int col = 0; col<16; col++)
            {
                if (maze[row][col] == 2)
                {
                    playerCol = col;
                }
            } 
    }
    return playerCol;
}
/**
 * This method moves the player on the grid
 */
void move(int choice)
{
    int playerRow = getRow();
    int playerCol = getCol();

    // remove player from current position
    maze[playerRow][playerCol] = 10;


    //up
    if (choice == 1)
    {
        playerRow -= 1;
    }
    //down
    else if (choice == 2)
    {
        playerRow += 1;
    }
    //left
    else if (choice == 3)
    {
        playerCol -= 1;
    }
    //right
    else if (choice == 4)
    {
        playerCol += 1;
    }

    //if this new maze[r][c] is pink, leave that old one as white
    //else

    // sets player to new position if move is valid
    if (moveValid(playerRow, playerCol) == 1)
    {
        if (maze[playerRow][playerCol] == 3)
        {
            maze[playerRow][playerCol] = 1337;
            gameWon = 1;
        }
        else
        {
            maze[playerRow][playerCol] = 2;
        }
    }
    else // player does not move, returns to origin
    {
        //up
        if (choice == 1)
        {
            playerRow += 1;
        }
        //down
        else if (choice == 2)
        {
            playerRow -= 1;
        }
        //left
        else if (choice == 3)
        {
            playerCol += 1;
        }
        //right
        else if (choice == 4)
        {
            playerCol -= 1;
        }
        maze[playerRow][playerCol] = 2;	
    }
}
/**
 * This method moves the player on the grid
 */
void move2(int choice)
{
    int playerRow = getRow();
    int playerCol = getCol();

    // remove player from current position
    maze[playerRow][playerCol] = 0;


    //up
    if (choice == 1)
    {
        playerRow -= 1;
    }
    //down
    else if (choice == 2)
    {
        playerRow += 1;
    }
    //left
    else if (choice == 3)
    {
        playerCol -= 1;
    }
    //right
    else if (choice == 4)
    {
        playerCol += 1;
    }

    //if this new maze[r][c] is pink, leave that old one as white
    //else

    // sets player to new position if move is valid
    if (moveValid(playerRow, playerCol) == 1)
    {
        if (maze[playerRow][playerCol] == 3)
        {
            maze[playerRow][playerCol] = 1337;
            gameWon = 1;
        }
        else
        {
            maze[playerRow][playerCol] = 2;
        }
    }
    else // player does not move, returns to origin
    {
        //up
        if (choice == 1)
        {
            playerRow += 1;
        }
        //down
        else if (choice == 2)
        {
            playerRow -= 1;
        }
        //left
        else if (choice == 3)
        {
            playerCol += 1;
        }
        //right
        else if (choice == 4)
        {
            playerCol -= 1;
        }
        maze[playerRow][playerCol] = 2;	
    }
}
/**
 * This method returns true if player is moving into an valid space
 * @param playerRow The row index of player
 * @param playerCol The column index of player
 */
int moveValid(int playerRow, int playerCol)
{
    //0 is false
    //1 is true
    int flag;

    //hits a wall
    if ((maze[playerRow][playerCol] == 1) || (maze[playerRow][playerCol] == 5) || (maze[playerRow][playerCol] == 6) || (maze[playerRow][playerCol] == 7) || (maze[playerRow][playerCol] == 8) || (maze[playerRow][playerCol] == 9))
    {
        flag = 0;
    }
    else
    {
        flag = 1;
    }
    return flag;
}
//reduces hp of wall
int hitWall(int currentHp)
{
    int hp;

    if(hp == 5)
    {
        hp = 0;
    }
    else
    {
        hp = currentHp - 1;
    }
    return hp;
}
//checks if wall is dissolvable
int isDestructable(int wall)
{
    int flag;
    if ((wall > 4) && (wall < 10))
    {
        flag = 1;
    }
    else
    {
        flag = 0;
    }
    return flag;
}
//resets the maze to original
void newGame()
{
    for (int i = 0; i < 12; i++)
    {
        for (int j= 0; j < 16; j++)
        {
            maze[i][j] = original[i][j];
        }
    }
}
