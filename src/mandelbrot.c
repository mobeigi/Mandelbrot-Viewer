/* Mandelbrot Set - Task 2
* Due Date: Tuesday, 30th April 2013
* Authors: Mohammad Ghasembeigi, Scott Carver
* This program is designed to plot the Mandelbrot set using 256 iterations of detail.
* It then acts as a webserver which offers a viewer to explore the set.
* Alternatively, a bitmap can be requestion for any x, y and zoom through the url.
* Also outputs a bitmap to a file in current directory. Off by default; see BITMAP_OUTPUT_FILE definition
*
* Licensed under Creative Commons SA-BY-NC 3.0.
*/

//Standard libary includes
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

//External header includes
#include "pixelColor.h"
#include "mandelbrot.h"


//Definitionss

//Calculation related definitions
#define SIDE_LENGTH 512         //the width and height of the produced bitmaps;
#define MAXIMUM_ITERATIONS 256  //the number of iterations used to extract detail
#define DISTANCE_AWAY 4
#define NEGATIVE_REPEAT_DIFFERENCE 64   //mandelbrot set repeats every 64 zoom levels, use different to deal with negatives
#define SPACE ' '

//ASCII Constants
#define CASE_DIFFERENCE 32
#define UPPERCASE_START 97
#define UPPERCASE_END 122
#define LOWERCASE_START 65
#define LOWERCASE_END 90
#define TERMINATING_CHARACTER '\0'

//Bitmap releated definitions
#define BITMAP_OUTPUT_FILE 0 //output a file to current directory; 0 for no, non-zero for yes
#define BITMAP_FILE_OUTPUT_NAME "output.bmp"        //name of output file
#define BITMAP_HEADER_SIZE 14
#define BITMAP_INFO_HEADER_SIZE 40
#define BITMAP_OFFSET 54
#define BITMAP_BITS_PER_PIXEL 24            //bits per pixel determine total available colours
#define BITMAP_BACKGROUND_RBG_COLOUR 96     //background colour on the mandelbrot set

//Server related definitionsss
#define SIMPLE_SERVER_VERSION 1.971
#define REQUEST_BUFFER_SIZE 1000
#define DEFAULT_PORT 7191
#define NUMBER_OF_INPUTS 4
#define FILE_EXTENSION_STRING_LENGTH 4
#define TRUE 1

//Function Declarations
static double POWER(double number, double exponent);
static void test(void);

static void createBitmap(unsigned char* BMP_HEADER, unsigned char* BMP_INFO_HEADER, unsigned char* BMP);
static void serveBitmap(unsigned char* BMP_HEADER, unsigned char* BMP_INFO_HEADER, unsigned char* BMP, int socket);

static int waitForConnection (int serverSocket);
static int makeServerSocket (int portno);
static void serveHTML (int socket);
static void serveErrorHTML (int socket);

int main(int argc, char *argv[]) {
    test();
    
    //Declarations
    short int zoom = 0; //declare zoom and assign it to zero
    short int inside = 0; //stores non-zero if point in inside set
    
    double pixelDistance = 0; //Stores the distance between each pixel
    double x, y; //Bariables used for positioning of picture
    
    //BITMAP image variables
    unsigned char BMP[SIDE_LENGTH*SIDE_LENGTH] = {0}; //create an array of type unsigned short int with an element for each pixel in the picture and set all elements to zero
    unsigned char BMP_HEADER[BITMAP_HEADER_SIZE] = {0}; //create an array of type unsigned short int that is to store the information for the header
    unsigned char BMP_INFO_HEADER[BITMAP_INFO_HEADER_SIZE] = {0}; //Info header array which will store height/width and colour information
    
    // Create bitmap header by assigning decimal values or character counterpart
    // to the BMP_HEADER array, same is done for BMP_INFO_HEADER
    // The size of each part of the headers has been put in comments in square brackets, eg: [2 bytes]
    
    /* #### Write HEADER #### */
    
    //Headerfield Identity, BM on Windows 3.1x, 95, NT [2 bytes]
    BMP_HEADER[0] = 'B';
    BMP_HEADER[1] = 'M';
    
    //Size of Bitmap in bytes [4 bytes]
    BMP_HEADER[2] = 36;
    BMP_HEADER[3] = 0;
    BMP_HEADER[4] = 12;
    BMP_HEADER[5] = 0;
    
    // Reserved Bytes [4 bytes]
    // We will leave elements 6 - 9 as zero
    
    //Offset to begining of bitmap data
    BMP_HEADER[10] = BITMAP_OFFSET;
    BMP_HEADER[11] = 0;
    BMP_HEADER[12] = 0;
    BMP_HEADER[13] = 0;
    
    /* #### Write INFO HEADER #### */
    
    //Info Header size [4 bytes]
    BMP_INFO_HEADER[0] = BITMAP_INFO_HEADER_SIZE;
    BMP_INFO_HEADER[1] = 0;
    BMP_INFO_HEADER[2] = 0;
    BMP_INFO_HEADER[3] = 0;
    
    //Width [4 bytes] and Height [4 bytes]
    BMP_INFO_HEADER[4] = 0;
    BMP_INFO_HEADER[5] = 2;
    BMP_INFO_HEADER[6] = 0;
    BMP_INFO_HEADER[7] = 0;
    
    BMP_INFO_HEADER[8] = 0;
    BMP_INFO_HEADER[9] = 2;
    BMP_INFO_HEADER[10] = 0;
    BMP_INFO_HEADER[11] = 0;
    
    //Number of planes [2 bytes] and colour profile [2 bytes]
    BMP_INFO_HEADER[12] = 1;
    BMP_INFO_HEADER[13] = 0;
    BMP_INFO_HEADER[14] = BITMAP_BITS_PER_PIXEL;
    BMP_INFO_HEADER[15] = 0;
    
    //We will leave BMP_INFO_HEADER[16] to BMP_INFO_HEADER[40] as zero
    
    //
    //Serve image to the server
    //
    printf ("************************************\n");
    printf ("Starting simple server %f\n", SIMPLE_SERVER_VERSION);
    printf ("Coded and made awesome by: Mohammad Ghasembeigi and Scott Carver\n");
    
    int serverSocket = makeServerSocket (DEFAULT_PORT);
    printf ("Access this server at http://localhost:%d/\n", DEFAULT_PORT);
    printf ("************************************\n");
    
    char request[REQUEST_BUFFER_SIZE];
    
    unsigned short int numberServed = 0;    //keep track of number of properly served images
    unsigned short int numberErrors = 0;    //keep track of errors also
    
    //Create variables xPoint and yPoint which will be changed and passed to escapeSteps after each iteration
    double xPoint = 0.00;
    double yPoint = 0.00;
    
    //Two counters used for iteration of every pixel
    int col = 0; // Column
    int row = 0; // Rows
    
    int counter = 0; //A counter to help store the BITMAP array values
    
    while (TRUE) {
        
        
        int connectionSocket = waitForConnection (serverSocket);
        // wait for a request to be sent from a web browser, open a new
        // connection for this conversation
        
        // read the first line of the request sent by the browser
        int bytesRead;
        bytesRead = read (connectionSocket, request, (sizeof request)-1);
        assert (bytesRead >= 0);
        // were we able to read any data from the connection?
        
        // print entire request to the console
        printf (" *** Received http request ***\n %s\n", request);
        
        ////////////////////
        
        // Calculate variables
        char testChar;
        
        testChar = SPACE;
        
        //testChar will be a space if root directory is requested
        sscanf(request, "GET /%c HTTP" , &testChar);
        
        if (testChar == SPACE) {
            //Send HTML
            serveHTML(connectionSocket);
        }
        else {  //send the bitmap data
            
            //Create variable to store number of read in variables
            unsigned short int valuesRead = 0;
            
            //Create variable to store requested filetype in url
            char filetype[FILE_EXTENSION_STRING_LENGTH];
            filetype[FILE_EXTENSION_STRING_LENGTH-1] = TERMINATING_CHARACTER; //as we only want .BMP, write the terminating character to the last element
            
            //Read in the x, y and zoom values from the server
            //Let error equal the number of items that are read in
            valuesRead = sscanf(request, "GET /X%lf_Y%lf_Z%hd.%s HTTP" , &x, &y, &zoom, filetype);      //long float format operator is used to read in doubles, %hd used for short int
            
            counter = 0; //reset counter
            
            while (counter < FILE_EXTENSION_STRING_LENGTH ) {   //while the counter is less than the FILE_EXTENSION_STRING_LENGTH
                if (filetype[counter] >= UPPERCASE_START && filetype[counter] <= UPPERCASE_END) {
                    filetype[counter] -= CASE_DIFFERENCE;   //turn uppercase letter to lowercase letter
                }
                counter++;
            }
            
            //If less than NUMBER_OF_INPUTS have been read in, there has been a format error, send errorHTML
            //If the filetype requested is not a BMP (uppercase), send errorHTML
            if (valuesRead < NUMBER_OF_INPUTS || filetype[0] != 'B' || filetype[1] != 'M' || filetype[2]!= 'P' || filetype[3] != TERMINATING_CHARACTER) {
                //Send error HTML
                serveErrorHTML(connectionSocket);
                numberErrors++; //increment error count
            }
            else {
                
                // The following code deals with negative zooms
                // If the zoom is negative, the image produced at -zoom is identical
                // to the one produced at the positive zoom of (-zoom % NEGATIVE_REPEAT_DIFFERENCE) + NEGATIVE_REPEAT_DIFFERENCE
                // where the NEGATIVE_REPEAT_DIFFERENCE is the number required such that adding this number to the zoom produces the same image
                
                if (zoom < 0) {
                    zoom = (zoom % NEGATIVE_REPEAT_DIFFERENCE);
                    zoom += NEGATIVE_REPEAT_DIFFERENCE;
                }
                
                
                //Find pixelDistance by solving 2 to the power of the zoom
                pixelDistance = POWER(2, -zoom);
                
                //Set initial values of xPoint and yPoint based on entered, x, y and zoom
                xPoint = x - ( (SIDE_LENGTH/2) - 1) * pixelDistance;
                yPoint = y + ( (SIDE_LENGTH/2) - 1) * pixelDistance;
                
                
                col = 0; // reset column counter
                row = 0; // reset row counter
                
                counter = 0; //reset allocation counter
                
                while (row < SIDE_LENGTH) {
                    col = 0;    //reset col counter
                    
                    while( col < SIDE_LENGTH) {
                        
                        //Call function that interates mandelbrot equation; assign value to variable called inside
                        inside = escapeSteps(xPoint, yPoint);
                        
                        //Change inside so that if it equals maximum iterations, it will equal zero
                        inside %= MAXIMUM_ITERATIONS;
                        
                        // Write data to the array
                        // If point is inside the set, data will be number of steps
                        // Otherwise, the value in the array is setto 0
                        BMP[counter] = inside;
                        
                        xPoint += pixelDistance;
                        counter++;    //increment coutner so that next pixel is assigned in next interation
                        col++;
                    }
                    
                    xPoint = x - ( (SIDE_LENGTH/2) - 1) * pixelDistance;
                    yPoint -= pixelDistance;
                    row++;
                }
                
                //send the browser a simple html page using http
                printf (" *** Sending http response ***\n");
                
                //Send bitmap
                serveBitmap(BMP_HEADER, BMP_INFO_HEADER, BMP, connectionSocket);
                
                
                //Print laste generated image
                printf ("Last generated image: http://localhost:%d/X%lf_Y%lf_Z%hd.bmp\n", DEFAULT_PORT, x, y, zoom);
                
                numberServed++;
            }
        }
        
        //Print number of served pages
        printf ("*** So far served %d images ***\n", numberServed);
        
        //Print number of served error pages
        printf ("*** A total of %d errors have occured ***\n", numberErrors);
        
        // close the connection after sending the page- keep aust beautiful
        close(connectionSocket);
    }
    
    // close the server connection after we are done- keep aust beautiful
    printf ("** shutting down the server **\n");
    close (serverSocket);
    
    
    return EXIT_SUCCESS;
}

//Power function
// This function takes in a number and an exponent (both of type double) and produces the power.
// Works with negatives as well as 0.
static double POWER(double number, double exponent) {
    double answer = 0;
    double exp = 0;
    
    answer = number;    //let answer equal the input number
    exp = exponent;        //let exp equal the input exponent
    
    if (exponent == 0) { //if the exponent is 0, the answer is 1; case of 0/0 ignored
        answer = 1;
    }
    else {                         //otherwise
        if (exponent < 0)        //if input exponent is negative
        exp *= -1;        //make exp positive
        
        int counter = 0;            //create a counter
        while (counter < exp - 1) {   //while the counter is less than the exponent minus 1
            answer *= number;      //multiply the answer by the original input number
            counter++;              //increment counter
        }
        if (exponent < 0)                //if original exponent was negative
        answer = 1 / answer;    //let final answer equal reciprocal of answer
    }
    
    return answer;
}

int escapeSteps(double x, double y) {
    double real, imag;    //the  real and immaginary part of complex number
    double z, zi;        //the new real and immaginary part of complex number
    double complex;        //the complex number; found by adding real + imag
    
    unsigned short int steps = 0; //stores value for number of iterations
    
    //Initilise all variables to zero
    z = zi = real = imag = complex = 0;
    
    while (steps < MAXIMUM_ITERATIONS && complex < DISTANCE_AWAY) {
        
        //Calculate the real and imag numbers
        //Real part will equal: z^2-zi^2 + x
        //Imaginary part will equal: 2*z*zi + y
        
        real = (z*z) - (zi*zi) + x;
        imag = 2 * z * zi + y;
        
        z = real;    //make z equal the current real number
        zi = imag;    //make zi equal the current imag number
        
        //Calculate the complex number so that the condition complex < DISTANCE_AWAY is tested in next loop
        complex = real*real + imag*imag;
        
        steps++;
    }
    
    // Return the number of iterations
    // If the pixel is in the set, the number of iterations will be a value between 1 and 255
    // Otherwise, it will be 256
    
    return steps;
}

//Server HTML tileviewer to web server
static void serveHTML (int socket) {
    char* message;
    
    // first send the http response header
    message =
    "HTTP/1.0 200 Found\n"
    "Content-Type: text/html\n"
    "\n";
    printf ("about to send=> %s\n", message);
    write (socket, message, strlen (message));
    
    message =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<script src=\"https://almondbread.openlearning.com/tileviewer.js\"></script>\n"
    "</html>\n";
    write (socket, message, strlen (message));
}

//Server the Error HTML page
static void serveErrorHTML (int socket) {
    char* message;
    
    // first send the http response header
    message =
    "HTTP/1.0 200 Found\n"
    "Content-Type: text/html\n"
    "\n";
    printf ("about to send=> %s\n", message);
    write (socket, message, strlen (message));
    
    message =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<h1>You had one job...</h1><br><br>\n"
    "<p><strong>Format error</strong><p>\n"
    "<p>Unknown format (use /X(real)_Y(real)_Z(integer).bmp, e.g. /X-0.5_Y0.0_Z8.bmp)</p>\n"
    "</html>\n";
    write (socket, message, strlen (message));
}

//Serves Bitmap to web server
static void serveBitmap(unsigned char* BMP_HEADER, unsigned char* BMP_INFO_HEADER, unsigned char* BMP, int socket) {
    
    char* message;
    
    // send the http response header
    // (if you write stings one after another like this on separate
    // lines the c compiler kindly joins them togther for you into
    // one long string)
    message = "HTTP/1.0 200 OK\r\n"
    "Content-Type: image/bmp\r\n"
    "\r\n";
    printf ("about to send=> %s\n", message);
    write (socket, message, strlen (message));
    
    //Send the bitmap file
    
    //Send Header
    write (socket, BMP_HEADER, BITMAP_HEADER_SIZE );
    
    //Print Info Header
    write (socket, BMP_INFO_HEADER, BITMAP_INFO_HEADER_SIZE );
    
    int counter = 0;    //Reset counter
    int counter2 = 0;    //Create second counter for following loops
    
    // currentRow will store the starting row going from the last row to the first row
    // This will effectively go through every row backwards.
    // This is done due to the way that the bitmap data was allocated and how the bitmap file reads data (from bottem left to top right)
    int currentRow = 0;
    
    int steps = 0;    //steps will be assigned the value of the iteration count for each pixel in the following loops
    
    //We use an unsigned char that has 3 elements to send the bitmap data during the loop
    //Each element represents 1 byte in the bitmap
    unsigned char RGB[3] = {0};
    
    while (counter < SIDE_LENGTH) {
        
        currentRow = SIDE_LENGTH * SIDE_LENGTH;    //Set current row to the value of the last element in the array
        currentRow -= ((1 + counter) * SIDE_LENGTH);    // Take away (counter + 1) * side length
        
        
        counter2 = 0; //Reset counter2
        
        while (counter2 < SIDE_LENGTH) {
            //Determine interation number for current pixel and assign to variable steps
            steps = BMP[currentRow + counter2];
            
            if (steps) { //if steps is positive and hence in the set
                
                //Calculate the red, green and blue intensity based on the value of steps
                RGB[0] = stepsToBlue(steps);;
                RGB[1] = stepsToGreen(steps);
                RGB[2] = stepsToRed(steps);
            }
            else {
                //If not in set, make all 3 bytes of RGP equal the BITMAP_BACKGROUND_RBG_COLOUR colour
                RGB[0] = BITMAP_BACKGROUND_RBG_COLOUR + BITMAP_BACKGROUND_RBG_COLOUR;
                RGB[1] = BITMAP_BACKGROUND_RBG_COLOUR;
                RGB[2] = BITMAP_BACKGROUND_RBG_COLOUR;
            }
            
            //Send data for current pixel
            write (socket, RGB, sizeof(RGB));
            
            counter2++;
        }
        
        counter++;
    }
    
}

//Creates Bitmap in directory as BITMAP_FILE_OUTPUT_NAME
static void createBitmap(unsigned char* BMP_HEADER, unsigned char* BMP_INFO_HEADER, unsigned char* BMP) {
    FILE *bitmap;
    bitmap = fopen(BITMAP_FILE_OUTPUT_NAME, "w");
    
    //Print Header to bitmap file
    int counter = 0;    //create counter
    
    while (counter < BITMAP_HEADER_SIZE) {
        fprintf(bitmap,"%c", BMP_HEADER[counter]); //print stored data in array as characters
        counter++;
    }
    
    //Print Info Header to bitmap file
    counter = 0; //reset counter
    
    while (counter < BITMAP_INFO_HEADER_SIZE) {
        fprintf(bitmap,"%c", BMP_INFO_HEADER[counter]);    //print stored data in array as characters
        counter++;
    }
    
    // Declare/Initilise variables to store the RGB values
    // for pixels in the mandelbrot set
    unsigned short int red = 0;
    unsigned short int green = 0;
    unsigned short int blue = 0;
    
    
    counter = 0;    //Reset counter
    int counter2 = 0;    //Create second counter for following loops
    
    // currentRow will store the starting row going from the last row to the first row
    // This will effectively go through every row backwards.
    // This is done due to the way that the bitmap data was allocated and how the bitmap file reads data (from bottem left to top right)
    int currentRow = 0;
    
    int steps = 0;    //steps will be assigned the value of the iteration count for each pixel in the following loops
    
    while (counter < SIDE_LENGTH) {
        
        currentRow = SIDE_LENGTH * SIDE_LENGTH;    //Set current row to the value of the last element in the array
        currentRow -= ((1 + counter) * SIDE_LENGTH);    // Take away (counter + 1) * side length
        
        
        counter2 = 0; //Reset counter2
        
        while (counter2 < SIDE_LENGTH) {
            //Determine interation number for current pixel and assign to variable steps
            steps = BMP[currentRow + counter2];
            
            if (steps) { //if steps is positive and hence in the set
                
                //Calculate the red, green and blue intensity based on the value of steps
                red =  stepsToRed(steps);
                green = stepsToGreen(steps);
                blue = stepsToBlue(steps);
                
                fprintf(bitmap,"%c%c%c", blue, green, red);  //Print RBG colours in order of GREEN, BLUE, RED (as required by bitmap files)
            }
            else {
                fprintf(bitmap,"%c%c%c", BITMAP_BACKGROUND_RBG_COLOUR+BITMAP_BACKGROUND_RBG_COLOUR, BITMAP_BACKGROUND_RBG_COLOUR, BITMAP_BACKGROUND_RBG_COLOUR); //Print background colour if pixel is not in the set
            }
            counter2++;
        }
        
        counter++;
    }
    
    // Add some padding to end of file
    fprintf(bitmap,"%c%c%c%c", 0,0,0,0);
    
    fclose(bitmap);    //close the bitmap file
}

//Test multiple program functionality
static void test(void) {
    //Power function
    assert(POWER(2, 2) == 4);
    assert(POWER(2, -1) == 0.5);
    assert(POWER(5, 2) == 25);
    assert(POWER(50, 0) == 1);
    
    //escapeSteps
    assert (escapeSteps (100.0, 100.0) == 1);
    assert (escapeSteps (0.0, 0.0)     == 256);
    
    assert (escapeSteps (-1.5000000000000, -1.5000000000000) == 1);
    assert (escapeSteps (-1.4250000000000, -1.4250000000000) == 1);
    assert (escapeSteps (-1.3500000000000, -1.3500000000000) == 2);
    assert (escapeSteps (-1.2750000000000, -1.2750000000000) == 2);
    assert (escapeSteps (-1.2000000000000, -1.2000000000000) == 2);
    assert (escapeSteps (-1.1250000000000, -1.1250000000000) == 3);
    assert (escapeSteps (-1.0500000000000, -1.0500000000000) == 3);
    assert (escapeSteps (-0.9750000000000, -0.9750000000000) == 3);
    assert (escapeSteps (-0.9000000000000, -0.9000000000000) == 3);
    assert (escapeSteps (-0.8250000000000, -0.8250000000000) == 4);
    assert (escapeSteps (-0.7500000000000, -0.7500000000000) == 4);
    assert (escapeSteps (-0.6750000000000, -0.6750000000000) == 6);
    assert (escapeSteps (-0.6000000000000, -0.6000000000000) == 12);
    assert (escapeSteps (-0.5250000000000, -0.5250000000000) == 157);
    assert (escapeSteps (-0.4500000000000, -0.4500000000000) == 256);
    assert (escapeSteps (-0.3750000000000, -0.3750000000000) == 256);
    assert (escapeSteps (-0.3000000000000, -0.3000000000000) == 256);
    assert (escapeSteps (-0.2250000000000, -0.2250000000000) == 256);
    assert (escapeSteps (-0.1500000000000, -0.1500000000000) == 256);
    assert (escapeSteps (-0.0750000000000, -0.0750000000000) == 256);
    assert (escapeSteps (-0.0000000000000, -0.0000000000000) == 256);
    
    assert (escapeSteps (-0.5400000000000, 0.5600000000000) == 256);
    assert (escapeSteps (-0.5475000000000, 0.5650000000000) == 58);
    assert (escapeSteps (-0.5550000000000, 0.5700000000000) == 28);
    assert (escapeSteps (-0.5625000000000, 0.5750000000000) == 22);
    assert (escapeSteps (-0.5700000000000, 0.5800000000000) == 20);
    assert (escapeSteps (-0.5775000000000, 0.5850000000000) == 15);
    assert (escapeSteps (-0.5850000000000, 0.5900000000000) == 13);
    assert (escapeSteps (-0.5925000000000, 0.5950000000000) == 12);
    assert (escapeSteps (-0.6000000000000, 0.6000000000000) == 12);
    
    assert (escapeSteps (0.2283000000000, -0.5566000000000) == 20);
    assert (escapeSteps (0.2272500000000, -0.5545000000000) == 19);
    assert (escapeSteps (0.2262000000000, -0.5524000000000) == 19);
    assert (escapeSteps (0.2251500000000, -0.5503000000000) == 20);
    assert (escapeSteps (0.2241000000000, -0.5482000000000) == 20);
    assert (escapeSteps (0.2230500000000, -0.5461000000000) == 21);
    assert (escapeSteps (0.2220000000000, -0.5440000000000) == 22);
    assert (escapeSteps (0.2209500000000, -0.5419000000000) == 23);
    assert (escapeSteps (0.2199000000000, -0.5398000000000) == 26);
    assert (escapeSteps (0.2188500000000, -0.5377000000000) == 256);
    assert (escapeSteps (0.2178000000000, -0.5356000000000) == 91);
    assert (escapeSteps (0.2167500000000, -0.5335000000000) == 256);
    
    assert (escapeSteps (-0.5441250000000, 0.5627500000000) == 119);
    assert (escapeSteps (-0.5445000000000, 0.5630000000000) == 88);
    assert (escapeSteps (-0.5448750000000, 0.5632500000000) == 83);
    assert (escapeSteps (-0.5452500000000, 0.5635000000000) == 86);
    assert (escapeSteps (-0.5456250000000, 0.5637500000000) == 74);
    assert (escapeSteps (-0.5460000000000, 0.5640000000000) == 73);
    assert (escapeSteps (-0.5463750000000, 0.5642500000000) == 125);
    assert (escapeSteps (-0.5467500000000, 0.5645000000000) == 75);
    assert (escapeSteps (-0.5471250000000, 0.5647500000000) == 60);
    assert (escapeSteps (-0.5475000000000, 0.5650000000000) == 58);
    
    assert (escapeSteps (0.2525812510000, 0.0000004051626) == 60);
    assert (escapeSteps (0.2524546884500, 0.0000004049095) == 61);
    assert (escapeSteps (0.2523281259000, 0.0000004046564) == 63);
    assert (escapeSteps (0.2522015633500, 0.0000004044033) == 65);
    assert (escapeSteps (0.2520750008000, 0.0000004041502) == 67);
    assert (escapeSteps (0.2519484382500, 0.0000004038971) == 69);
    assert (escapeSteps (0.2518218757000, 0.0000004036441) == 72);
    assert (escapeSteps (0.2516953131500, 0.0000004033910) == 74);
    assert (escapeSteps (0.2515687506000, 0.0000004031379) == 77);
    assert (escapeSteps (0.2514421880500, 0.0000004028848) == 81);
    assert (escapeSteps (0.2513156255000, 0.0000004026317) == 85);
    assert (escapeSteps (0.2511890629500, 0.0000004023786) == 89);
    assert (escapeSteps (0.2510625004000, 0.0000004021255) == 94);
    assert (escapeSteps (0.2509359378500, 0.0000004018724) == 101);
    assert (escapeSteps (0.2508093753000, 0.0000004016193) == 108);
    assert (escapeSteps (0.2506828127500, 0.0000004013662) == 118);
    assert (escapeSteps (0.2505562502000, 0.0000004011132) == 131);
    assert (escapeSteps (0.2504296876500, 0.0000004008601) == 150);
    assert (escapeSteps (0.2503031251000, 0.0000004006070) == 179);
    assert (escapeSteps (0.2501765625500, 0.0000004003539) == 235);
    assert (escapeSteps (0.2500500000000, 0.0000004001008) == 256);
    
    assert (escapeSteps (0.3565670191423, 0.1094322101123) == 254);
    assert (escapeSteps (0.3565670191416, 0.1094322101120) == 255);
    assert (escapeSteps (0.3565670191409, 0.1094322101118) == 256);
    assert (escapeSteps (0.3565670950000, 0.1094322330000) == 222);
    assert (escapeSteps (0.3565670912300, 0.1094322318625) == 222);
    assert (escapeSteps (0.3565670874600, 0.1094322307250) == 222);
    assert (escapeSteps (0.3565670836900, 0.1094322295875) == 222);
    assert (escapeSteps (0.3565670799200, 0.1094322284500) == 222);
    assert (escapeSteps (0.3565670761500, 0.1094322273125) == 222);
    assert (escapeSteps (0.3565670723800, 0.1094322261750) == 222);
    assert (escapeSteps (0.3565670686100, 0.1094322250375) == 223);
    assert (escapeSteps (0.3565670648400, 0.1094322239000) == 223);
    assert (escapeSteps (0.3565670610700, 0.1094322227625) == 224);
    assert (escapeSteps (0.3565670573000, 0.1094322216250) == 225);
    assert (escapeSteps (0.3565670535300, 0.1094322204875) == 256);
    assert (escapeSteps (0.3565670497600, 0.1094322193500) == 256);
    assert (escapeSteps (0.3565670459900, 0.1094322182125) == 237);
    assert (escapeSteps (0.3565670422200, 0.1094322170750) == 233);
    assert (escapeSteps (0.3565670384500, 0.1094322159375) == 232);
    assert (escapeSteps (0.3565670346800, 0.1094322148000) == 232);
    assert (escapeSteps (0.3565670309100, 0.1094322136625) == 232);
    assert (escapeSteps (0.3565670271400, 0.1094322125250) == 233);
    assert (escapeSteps (0.3565670233700, 0.1094322113875) == 234);
    assert (escapeSteps (0.3565670196000, 0.1094322102500) == 243);
    
    //Pixel colour functions
    assert(stepsToRed(50) == 32);
    assert(stepsToRed(0) == 32);
    assert(stepsToRed(255) == 32);
    
    assert(stepsToGreen(77) == 32);
    assert(stepsToGreen(128) == 32);
    assert(stepsToGreen(0) == 32);
    
    assert(stepsToBlue(0) == 64);
    assert(stepsToBlue(255) == 127);
    assert(stepsToBlue(64) == 128);
    assert(stepsToBlue(191) == 255);
    
}

// start the server listening on the specified port number
static int makeServerSocket (int portNumber) {
    
    // create socket
    int serverSocket = socket (AF_INET, SOCK_STREAM, 0);
    assert (serverSocket >= 0);
    // error opening socket
    
    // bind socket to listening port
    struct sockaddr_in serverAddress;
    bzero ((char *) &serverAddress, sizeof (serverAddress));
    
    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port        = htons (portNumber);
    
    // let the server start immediately after a previous shutdown
    int optionValue = 1;
    setsockopt (
    serverSocket,
    SOL_SOCKET,
    SO_REUSEADDR,
    &optionValue,
    sizeof(int)
    );
    
    int bindSuccess =
    bind (
    serverSocket,
    (struct sockaddr *) &serverAddress,
    sizeof (serverAddress)
    );
    
    assert (bindSuccess >= 0);
    // if this assert fails wait a short while to let the operating
    // system clear the port before trying again
    
    return serverSocket;
}

// wait for a browser to request a connection,
// returns the socket on which the conversation will take place
static int waitForConnection (int serverSocket) {
    // listen for a connection
    const int serverMaxBacklog = 10;
    listen (serverSocket, serverMaxBacklog);
    
    // accept the connection
    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof (clientAddress);
    int connectionSocket =
    accept (
    serverSocket,
    (struct sockaddr *) &clientAddress,
    &clientLen
    );
    
    assert (connectionSocket >= 0);
    // error on accept
    
    return (connectionSocket);
}