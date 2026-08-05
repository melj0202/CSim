#include "SaveLoad.h"
#import <Cocoa/Cocoa.h>

std::string SaveLoad::GetLoadLocation() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setAllowedFileTypes:@[@"illumo", @"ILLUMO"]];
        
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
        [panel setAllowedFileTypes:@[@"illumo", @"ILLUMO"]];
        [panel setNameFieldStringValue:@"MyCanvas.illumo"];
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            return std::string([[url path] UTF8String]);
        }
    }
    return "";
}
