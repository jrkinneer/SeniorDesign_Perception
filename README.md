# Perception Code

## main.cpp
This code contains functionality to start streaming from the camera, perform a sliding window over the main image, and attempt to locate a qr code, and consequently a cube in the window

## qrCode.h
contains all the helper functions used in main as well as the struct definition that holds all necessary data for qr code detection

## dependencies
make sure to have opencv installed and built
I used this tutorial to verify that I did it right: https://www.geeksforgeeks.org/how-to-install-opencv-in-c-on-linux/

when using the visual tools of opencv like imshow, you may get an error like: 
`undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE`
to fix this run:
`unset GTK_PATH`
in the same terminal you are trying to run the code

**DO THIS WITH CAUTION, I DON'T ACTUALLY KNOW WHAT THIS COMMAND DOES, IT JUST WORKS FOR ME**

make sure to build with the same flags that are mentioned in the tasks.json in the .vscode folder

**see each file for detailed documentation about function logic**
