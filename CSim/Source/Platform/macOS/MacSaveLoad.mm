#include "SaveLoad.h"
#import <Cocoa/Cocoa.h>

std::string SaveLoad::GetLoadLocation() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setAllowedFileTypes:@[@"csim", @"CSIM"]];
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            return std::string([[url path] UTF8String]);
        }
    }
    return "";
}

std::string SaveLoad::GetSaveLocation() {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setAllowedFileTypes:@[@"csim", @"CSIM"]];
        [panel setNameFieldStringValue:@"MyCanvas.csim"];
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            return std::string([[url path] UTF8String]);
        }
    }
    return "";
}
