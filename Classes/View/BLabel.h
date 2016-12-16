#ifndef __B_LABEL_H__
#define __B_LABEL_H__

#include "cocos2d.h"
#include "CppUtils.h"

USING_NS_CC;
using namespace std;

class BLabel: public cocos2d::Label {
public:
	CREATE_FUNC(BLabel)
	;
	static BLabel* createWithTTF(TTFConfig config,
			TextHAlignment textAlignment);
	static BLabel* createWithTTF(TTFConfig config, const std::string& text,
			TextHAlignment textAlignment);
	void showText(string text, float durationPerSplitedString, char splitBy);

private:

};

#endif
