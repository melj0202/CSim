#include <Illumo/Platform/SaveLoad.h>

#import <Cocoa/Cocoa.h>

std::string
SaveLoad::GetLoadLocation(const SaveLoadDialogSpec& specification)
{
  (void)specification;
  @autoreleasepool {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    [panel setAllowedFileTypes:@[ @"illumo", @"ILLUMO" ]];

    if ([panel runModal] == NSModalResponseOK) {
      NSURL* url = [[panel URLs] firstObject];
      return std::string([[url path] UTF8String]);
    }
  }
  return "";
}

std::string
SaveLoad::GetSaveLocation(const SaveLoadDialogSpec& specification)
{
  @autoreleasepool {
    NSSavePanel* panel = [NSSavePanel savePanel];
    [panel setAllowedFileTypes:@[ @"illumo", @"ILLUMO" ]];
    NSString* defaultFilename =
      [NSString stringWithUTF8String:specification.defaultFilename.c_str()];
    [panel setNameFieldStringValue:defaultFilename];

    if ([panel runModal] == NSModalResponseOK) {
      NSURL* url = [panel URL];
      return std::string([[url path] UTF8String]);
    }
  }
  return "";
}
