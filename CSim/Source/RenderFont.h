#pragma once
#include <ft2build.h>
#include <map>
#include <string>
#include <freetype/freetype.h>

/*
	This class describes a font sheet to be used to render text to the screen

	Use stb_truetype for now???
*/

class RenderFontLibrary {

public:

private:

	/*
		A font face table

		std::string - name
		FT_Face - Font face data

		Search for the font face by name
	*/
	//static FT_Library library;
	//static std::map<std::string, FT_Face> fontLibrary;


	class RenderFont {
	private:
		RenderFont(std::string filename, std::string name);
	public:

		//FT_Face font;

	};
};
