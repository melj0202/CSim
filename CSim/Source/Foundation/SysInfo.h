#pragma once

/*
    This defines the cluster of data that describes what is in the user's computer.
*/

namespace SysInfo {
    //Data about the user's GPU
    namespace GPUInfo {

    }

    //Data about the user's CPU
    namespace CPUInfo {
        
    }

    //Execute a bunch of functions to get information about the users computer
    extern void getSystemInfo();
}