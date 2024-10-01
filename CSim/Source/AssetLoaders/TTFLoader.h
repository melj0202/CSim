#include <ft2build.h>
#include <freetype/freetype.h>
#include "../CellLogger.h"


FT_Library library;
FT_Face face;


auto error = FT_Init_FreeType(&library);
if (error) {
	CellLogger::LogError("Failed to initialize freetype");
	std::exit(1);
}

//Hotdog... Thats a long path name. :)
error = FT_New_Face(library, 
	"../../Assets/Fonts/Handjet/Handjet-VariableFont_ELGR,ELSH,wght.ttf",
	0,
	&face);

if (error == FT_Err_Unknown_File_Format) {
	CellLogger::LogError("Failed to load font: Unknown file type");
	std::exit(1);
}
else if (error) {
	CellLogger::LogError("Failed to load font.");
	std::exit(1);
}