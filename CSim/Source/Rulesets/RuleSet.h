#pragma once
#include <string>
#include "Canvas.h"

constexpr auto BLOCK_X = 2;
constexpr auto BLOCK_Y = 2;
constexpr auto MAX_RULETAG_SIZE = 128;

class RuleSet {
public:
	mutable std::vector<unsigned char> ne;
	Canvas* canvas;
	RuleSet(Canvas* targetCanvas) {
		canvas = targetCanvas;
	};
	virtual ~RuleSet() = default;
	void calcGeneration(const int &x_start, const int &y_start, const int &x_end, const int &y_end) const;
	virtual void evalCell(const unsigned char& target, unsigned char dest[3]) const {/*NOthing*/ };
	
	virtual std::string getRuleTag() {
		return "BASE_CLASS";
	}
protected:
	virtual int countNeighbors(const int &r, const int &c, const int &w, const int &h) const;
	inline virtual void evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const { cell = 0; };
	

};