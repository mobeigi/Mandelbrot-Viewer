//External header includes
#include "pixelColor.h"

//The following functions determine the intensity of colours for each pixel
//based on the value of steps. RED, GREEN and BLUE (RGB) are used to determine
//the colour for any given pixel in the set.

unsigned char stepsToRed (int steps){
    unsigned char red = 0;
    //Make red a fixed constant non-zero value
    red = 32;
    
    return red;
}

unsigned char stepsToBlue (int steps){
    unsigned char blue = 0;
    
    //Make blue have a minimum value &gt; (red and green)
    //Add the number of steps to this minmium value and never exceed 255 (this avoids wrapping around to zero)
    blue = 64 + (steps % 192);
    
    return blue;
}

unsigned char stepsToGreen (int steps){
    unsigned char green;
    //Make green a fixed constant non-zero value
    green = 32;
    
    return green;
}