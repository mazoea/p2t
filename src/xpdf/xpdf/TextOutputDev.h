//========================================================================
//
// TextOutputDev.h
//
// Copyright 1997-2012 Glyph & Cog, LLC
//
//========================================================================

#ifndef TEXTOUTPUTDEV_H
#define TEXTOUTPUTDEV_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <string>
#include <list>
#include <stdio.h>
#include "goo/gtypes.h"
#include "xpdf/GfxFont.h"
#include "xpdf/OutputDev.h"
#include "xpdf/TextAccent.h"

class GString;
class GList;
class GfxFont;
class GfxState;
class UnicodeMap;
class Link;

class TextWord;
class TextPool;
class TextLine;
class TextLineFrag;
class TextBlock;
class TextFlow;
class TextWordList;
class TextPage;

//------------------------------------------------------------------------

typedef void (*TextOutputFunc)(void *stream, const char *text, int len);

//------------------------------------------------------------------------
// TextFontInfo
//------------------------------------------------------------------------

class TextFontInfo {
public:

  TextFontInfo(GfxState *state);
  ~TextFontInfo();

  GBool matches(GfxState *state);

  // Get the font name (which may be NULL).
  GString *getFontName() { return fontName; }

  // Get font descriptor flags.
  GBool isFixedWidth() { return flags & fontFixedWidth; }
  GBool isSerif() { return flags & fontSerif; }
  GBool isSymbolic() { return flags & fontSymbolic; }
  GBool isItalic() { return flags & fontItalic; }
  GBool isBold() { return flags & fontBold; }
  GfxFontType type() { return (gfxFont) ? gfxFont->getType() : GfxFontType::fontUnknownType; }
private:

  GfxFont *gfxFont;
  GString *fontName;
  int flags;

  friend class TextWord;
  friend class TextPage;
};

//------------------------------------------------------------------------
// TextWord
//------------------------------------------------------------------------

class TextWord {
public:

  // Constructor.
  TextWord(GfxState *state, int rotA, double x0, double y0,
	   TextFontInfo *fontA, double fontSize);

  // Destructor.
  ~TextWord();

  // Add a character to the word.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy, int charPosA, int charLen,
	       Unicode u);

  // Merge <word> onto the end of <this>.
  void merge(TextWord *word);

  // Compares <this> to <word>, returning -1 (<), 0 (=), or +1 (>),
  // based on a primary-axis comparison, e.g., x ordering if rot=0.
  int primaryCmp(TextWord *word);

  // Return the distance along the primary axis between <this> and
  // <word>.
  double primaryDelta(TextWord *word);

  static int cmpYX(const void *p1, const void *p2);

  // Get the TextFontInfo object associated with this word.
  TextFontInfo *getFontInfo() { return font; }

  // Get the next TextWord on the linked list.
  TextWord *getNext() { return next; }

  int getLength() { return len; }
  Unicode getChar(int idx) { return text[idx]; }
  GString *getText();
  GString *getFontName() { return font->fontName; }
  void getColor(double *r, double *g, double *b)
    { *r = colorR; *g = colorG; *b = colorB; }
  void getBBox(double *xMinA, double *yMinA, double *xMaxA, double *yMaxA)
    { *xMinA = xMin; *yMinA = yMin; *xMaxA = xMax; *yMaxA = yMax; }
  void getCharBBox(int charIdx, double *xMinA, double *yMinA,
		   double *xMaxA, double *yMaxA);
  double getFontSize() { return fontSize; }
  int getRotation() { return rot; }
  int getCharPos() { return charPos[0]; }
  int getCharLen() { return charPos[len] - charPos[0]; }
  double getBaseline() { return base; }
  GBool getSpaceAfter() { return spaceAfter; }

  GBool isUnderlined() { return underlined; }
  Link *getLink() { return link; }

private:

  int rot;			// rotation, multiple of 90 degrees
				//   (0, 1, 2, or 3)
  double xMin, xMax;		// bounding box x coordinates
  double yMin, yMax;		// bounding box y coordinates
  double base;			// baseline x or y coordinate
  Unicode *text;		// the text
  double *edge;			// "near" edge x or y coord of each char
				//   (plus one extra entry for the last char)
  int *charPos;			// character position (within content stream)
				//   of each char (plus one extra entry for
				//   the last char)
  int len;			// length of text/edge/charPos arrays
  int size;			// size of text/edge/charPos arrays
  TextFontInfo *font;		// font information
  double fontSize;		// font size
  GBool spaceAfter;		// set if there is a space between this
				//   word and the next word on the line
  TextWord *next;		// next word in line

  double colorR,		// word color
         colorG,
         colorB;

  GBool underlined;
  Link *link;

  friend class TextPool;
  friend class TextLine;
  friend class TextBlock;
  friend class TextFlow;
  friend class TextWordList;
  friend class TextPage;
  
  friend class accented::word;
};

//------------------------------------------------------------------------
// TextPool
//------------------------------------------------------------------------

class TextPool {
public:

  TextPool();
  ~TextPool();

  TextWord *getPool(int baseIdx) { return pool[baseIdx - minBaseIdx]; }
  void setPool(int baseIdx, TextWord *p) { pool[baseIdx - minBaseIdx] = p; }

  int getBaseIdx(double base);

  void addWord(TextWord *word);

private:

  int minBaseIdx;		// min baseline bucket index
  int maxBaseIdx;		// max baseline bucket index
  TextWord **pool;		// array of linked lists, one for each
				//   baseline value (multiple of 4 pts)
  TextWord *cursor;		// pointer to last-accessed word
  int cursorBaseIdx;		// baseline bucket index of last-accessed word

  friend class TextBlock;
  friend class TextPage;
};

//------------------------------------------------------------------------
// TextLine
//------------------------------------------------------------------------

class TextLine {
public:

  TextLine(TextBlock *blkA, int rotA, double baseA);
  ~TextLine();

  void addWord(TextWord *word);

  // Return the distance along the primary axis between <this> and
  // <line>.
  double primaryDelta(TextLine *line);

  // Compares <this> to <line>, returning -1 (<), 0 (=), or +1 (>),
  // based on a primary-axis comparison, e.g., x ordering if rot=0.
  int primaryCmp(TextLine *line);

  // Compares <this> to <line>, returning -1 (<), 0 (=), or +1 (>),
  // based on a secondary-axis comparison of the baselines, e.g., y
  // ordering if rot=0.
  int secondaryCmp(TextLine *line);

  int cmpYX(TextLine *line);

  static int cmpXY(const void *p1, const void *p2);

  void coalesce(UnicodeMap *uMap);

  // Get the head of the linked list of TextWords.
  TextWord *getWords() { return words; }

  // Get the head of the linked list of TextWords.
  TextWord *getLastWord() { return lastWord; }

  // Get the next TextLine on the linked list.
  TextLine *getNext() { return next; }

  // Returns true if the last char of the line is a hyphen.
  GBool isHyphenated() { return hyphenated; }

  // jm 2012
  void getBBox(double *xMinA, double *yMinA, double *xMaxA, double *yMaxA)
    { *xMinA = xMin; *yMinA = yMin; *xMaxA = xMax; *yMaxA = yMax; }


private:

  TextBlock *blk;		// parent block
  int rot;			// text rotation
  double xMin, xMax;		// bounding box x coordinates
  double yMin, yMax;		// bounding box y coordinates
  double base;			// baseline x or y coordinate
  TextWord *words;		// words in this line
  TextWord *lastWord;		// last word in this line
  Unicode *text;		// Unicode text of the line, including
				//   spaces between words
  double *edge;			// "near" edge x or y coord of each char
				//   (plus one extra entry for the last char)
  int *col;			// starting column number of each Unicode char
  int len;			// number of Unicode chars
  int convertedLen;		// total number of converted characters
  GBool hyphenated;		// set if last char is a hyphen
  TextLine *next;		// next line in block

  std::string dbg_;

  friend class TextLineFrag;
  friend class TextBlock;
  friend class TextFlow;
  friend class TextWordList;
  friend class TextPage;
};

//------------------------------------------------------------------------
// TextBlock
//------------------------------------------------------------------------

class TextBlock {
public:

  TextBlock(TextPage *pageA, int rotA);
  ~TextBlock();

  void addWord(TextWord *word);

  void coalesce(UnicodeMap *uMap, double fixedPitch);

  // Update this block's priMin and priMax values, looking at <blk>.
  void updatePriMinMax(TextBlock *blk);

  static int cmpXYPrimaryRot(const void *p1, const void *p2);

  static int cmpYXPrimaryRot(const void *p1, const void *p2);

  int primaryCmp(TextBlock *blk);

  double secondaryDelta(TextBlock *blk);

  // Returns true if <this> is below <blk>, relative to the page's
  // primary rotation.
  GBool isBelow(TextBlock *blk);

  // Get the head of the linked list of TextLines.
  TextLine *getLines() { return lines; }

  // Get the next TextBlock on the linked list.
  TextBlock *getNext() { return next; }

private:

  TextPage *page;		// the parent page
  int rot;			// text rotation
  double xMin, xMax;		// bounding box x coordinates
  double yMin, yMax;		// bounding box y coordinates
  double priMin, priMax;	// whitespace bounding box along primary axis

  TextPool *pool;		// pool of words (used only until lines
				//   are built)
  TextLine *lines;		// linked list of lines
  TextLine *curLine;		// most recently added line
  int nLines;			// number of lines
  int charCount;		// number of characters in the block
  int col;			// starting column
  int nColumns;			// number of columns in the block

  TextBlock *next;
  TextBlock *stackNext;

  friend class TextLine;
  friend class TextLineFrag;
  friend class TextFlow;
  friend class TextWordList;
  friend class TextPage;
};


//------------------------------------------------------------------------
// TextLineFrag
//------------------------------------------------------------------------

class TextLineFrag {
public:

  TextLine *line;		// the line object
  int start, len;		// offset and length of this fragment
				//   (in Unicode chars)
  double xMin, xMax;		// bounding box coordinates
  double yMin, yMax;
  double base;			// baseline virtual coordinate
  int col;			// first column

  void init(TextLine *lineA, int startA, int lenA);
  void computeCoords(GBool oneRot);

  static int cmpYXPrimaryRot(const void *p1, const void *p2);
  static int cmpYXLineRot(const void *p1, const void *p2);
  static int cmpXYLineRot(const void *p1, const void *p2);
  static int cmpXYColumnPrimaryRot(const void *p1, const void *p2);
  static int cmpXYColumnLineRot(const void *p1, const void *p2);
};

//------------------------------------------------------------------------
// TextFlow
//------------------------------------------------------------------------

class TextFlow {
public:

  TextFlow(TextPage *pageA, TextBlock *blk);
  ~TextFlow();

  // Add a block to the end of this flow.
  void addBlock(TextBlock *blk);

  // Returns true if <blk> fits below <prevBlk> in the flow, i.e., (1)
  // it uses a font no larger than the last block added to the flow,
  // and (2) it fits within the flow's [priMin, priMax] along the
  // primary axis.
  GBool blockFits(TextBlock *blk, TextBlock *prevBlk);

  // Get the head of the linked list of TextBlocks.
  TextBlock *getBlocks() { return blocks; }

  // Get the next TextFlow on the linked list.
  TextFlow *getNext() { return next; }

private:

  TextPage *page;		// the parent page
  double xMin, xMax;		// bounding box x coordinates
  double yMin, yMax;		// bounding box y coordinates
  double priMin, priMax;	// whitespace bounding box along primary axis
  TextBlock *blocks;		// blocks in flow
  TextBlock *lastBlk;		// last block in this flow
  TextFlow *next;

  friend class TextWordList;
  friend class TextPage;
};


//------------------------------------------------------------------------
// TextWordList
//------------------------------------------------------------------------

class TextWordList {
public:

  // Build a flat word list, in content stream order (if
  // text->rawOrder is true), physical layout order (if <physLayout>
  // is true and text->rawOrder is false), or reading order (if both
  // flags are false).
  TextWordList(TextPage *text, GBool physLayout);

  ~TextWordList();

  // Return the number of words on the list.
  int getLength();

  // Return the <idx>th word from the list.
  TextWord *get(int idx);

private:

  GList *words;			// [TextWord]
};


// jm
//
//

struct frag_listener {
  
  virtual void start_page( size_t, GfxState* ) {};
  virtual void end_page( size_t cnt ) {};
  
  virtual void end_line( size_t cnt ) {};
  
  virtual void space( size_t cnt ) {};

  virtual void line( TextWord*, const char *, size_t ) {};
  virtual void line( TextLine*, const char *, size_t ) {};
  virtual void line(TextLineFrag*, const char *, size_t) {};

  virtual void image(int x, int y, int w, int h, bool inlineImg) {};
};


//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------

class TextPage {
public:

  // Constructor.
  TextPage(GBool rawOrderA);

  // Destructor.
  ~TextPage();

  // Start a new page.
  void startPage(GfxState *state);

  // End the current page.
  void endPage();

  // Update the current font.
  void updateFont(GfxState *state);

  // Begin a new word.
  void beginWord(GfxState *state, double x0, double y0);

  // Add a character to the current word.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy,
	       CharCode c, int nBytes, Unicode *u, int uLen);

  // Add <nChars> invisible characters.
  void incCharCount(int nChars);

  // Begin/end an "ActualText" span, where the char indexes are
  // supplied by a marked content operator rather than the text
  // drawing operators.
  void beginActualText(GfxState *state, Unicode *u, int uLen);
  void endActualText(GfxState *state);

  // End the current word, sorting it into the list of words.
  void endWord();

  // Add a word, sorting it into the list of words.
  void addWord(TextWord *word);

  // Add a (potential) underline.
  void addUnderline(double x0, double y0, double x1, double y1);

  // Add a hyperlink.
  void addLink(int xMin, int yMin, int xMax, int yMax, Link *link);

  // Coalesce strings that look like parts of the same line.
  void coalesce(GBool physLayout, double fixedPitch, GBool doHTML);

  // Find a string.  If <startAtTop> is true, starts looking at the
  // top of the page; else if <startAtLast> is true, starts looking
  // immediately after the last find result; else starts looking at
  // <xMin>,<yMin>.  If <stopAtBottom> is true, stops looking at the
  // bottom of the page; else if <stopAtLast> is true, stops looking
  // just before the last find result; else stops looking at
  // <xMax>,<yMax>.
  GBool findText(Unicode *s, int len,
		 GBool startAtTop, GBool stopAtBottom,
		 GBool startAtLast, GBool stopAtLast,
		 GBool caseSensitive, GBool backward,
		 GBool wholeWord,
		 double *xMin, double *yMin,
		 double *xMax, double *yMax);

  // Get the text which is inside the specified rectangle.
  GString *getText(double xMin, double yMin,
		   double xMax, double yMax);

  // Find a string by character position and length.  If found, sets
  // the text bounding rectangle and returns true; otherwise returns
  // false.
  GBool findCharRange(int pos, int length,
		      double *xMin, double *yMin,
		      double *xMax, double *yMax);

  // Dump contents of page to a file.
  void dump(void *outputStream, TextOutputFunc outputFunc,
    GBool physLayout, frag_listener*);

  // Get the head of the linked list of TextFlows.
  TextFlow *getFlows() { return flows; }

  // Build a flat word list, in content stream order (if
  // this->rawOrder is true), physical layout order (if <physLayout>
  // is true and this->rawOrder is false), or reading order (if both
  // flags are false).
  TextWordList *makeWordList(GBool physLayout);

private:

  void clear();
  void assignColumns(TextLineFrag *frags, int nFrags, int rot);

  GBool rawOrder;		// keep text in content stream order

  double pageWidth, pageHeight;	// width and height of current page
  TextWord *curWord;		// currently active string
  int charPos;			// next character position (within content
				//   stream)
  TextFontInfo *curFont;	// current font
  double curFontSize;		// current font size
  int nest;			// current nesting level (for Type 3 fonts)
  int nTinyChars;		// number of "tiny" chars seen so far
  GBool lastCharOverlap;	// set if the last added char overlapped the
				//   previous char
  Unicode *actualText;		// current "ActualText" span
  int actualTextLen;
  double actualTextX0,
         actualTextY0,
         actualTextX1,
         actualTextY1;
  int actualTextNBytes;

  TextPool *pools[4];		// a "pool" of TextWords for each rotation
  TextFlow *flows;		// linked list of flows
  TextBlock **blocks;		// array of blocks, in yx order
  int nBlocks;			// number of blocks
  int primaryRot;		// primary rotation
  GBool primaryLR;		// primary direction (true means L-to-R,
				//   false means R-to-L)
  TextWord *rawWords;		// list of words, in raw order (only if
				//   rawOrder is set)
  TextWord *rawLastWord;	// last word on rawWords list

  GList *fonts;			// all font info objects used on this
				//   page [TextFontInfo]

  double lastFindXMin,		// coordinates of the last "find" result
         lastFindYMin;
  GBool haveLastFind;

  GList *underlines;		// [TextUnderline]
  GList *links;			// [TextLink]

  std::list<std::string> dbg_;

  friend class TextLine;
  friend class TextLineFrag;
  friend class TextBlock;
  friend class TextFlow;
  friend class TextWordList;
};

//------------------------------------------------------------------------
// TextOutputDev
//------------------------------------------------------------------------

class TextOutputDev: public OutputDev {
public:

  // jm
  // - owner is the caller
  //
  frag_listener* listener_;

  void set_listener( frag_listener* listener ) {
    listener_ = listener;
  }
  frag_listener* get_listener() const {
    return listener_;
  }

  void set_output_fnc( TextOutputFunc func ) {
    outputFunc = func;
  }



  // Open a text output file.  If <fileName> is NULL, no file is
  // written (this is useful, e.g., for searching text).  If
  // <physLayoutA> is true, the original physical layout of the text
  // is maintained.  If <rawOrder> is true, the text is kept in
  // content stream order.
  TextOutputDev(char *fileName, GBool physLayoutA,
		double fixedPitchA, GBool rawOrderA,
		GBool append);

  // Create a TextOutputDev which will write to a generic stream.  If
  // <physLayoutA> is true, the original physical layout of the text
  // is maintained.  If <rawOrder> is true, the text is kept in
  // content stream order.
  TextOutputDev(TextOutputFunc func, void *stream,
		GBool physLayoutA, double fixedPitchA,
		GBool rawOrderA);

  // Destructor.
  virtual ~TextOutputDev();

  // Check if file was successfully created.
  virtual GBool isOk() { return ok; }

  //---- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown() { return gTrue; }

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar() { return gTrue; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return gFalse; }

  // Does this device need non-text content?
  virtual GBool needNonText() { return gFalse; }
  // Mazoea/2018/08 - upgrade to 3.04
  virtual GBool needImagesEvenIfText() { return gTrue; }

  // Does this device require incCharCount to be called for text on
  // non-shown layers?
  virtual GBool needCharCount() { return gTrue; }

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

  //----- save/restore graphics state
  virtual void restoreState(GfxState *state);

  //----- update text state
  virtual void updateFont(GfxState *state);

  //----- text drawing
  virtual void beginString(GfxState *state, GString *s);
  virtual void endString(GfxState *state);
  virtual void drawChar(GfxState *state, double x, double y,
			double dx, double dy,
			double originX, double originY,
			CharCode c, int nBytes, Unicode *u, int uLen);
  virtual void incCharCount(int nChars);
  virtual void beginActualText(GfxState *state, Unicode *u, int uLen);
  virtual void endActualText(GfxState *state);

  //----- path painting
  virtual void stroke(GfxState *state);
  virtual void fill(GfxState *state);
  virtual void eoFill(GfxState *state);

  //----- link borders
  virtual void processLink(Link *link);

  //----- special access

  // Find a string.  If <startAtTop> is true, starts looking at the
  // top of the page; else if <startAtLast> is true, starts looking
  // immediately after the last find result; else starts looking at
  // <xMin>,<yMin>.  If <stopAtBottom> is true, stops looking at the
  // bottom of the page; else if <stopAtLast> is true, stops looking
  // just before the last find result; else stops looking at
  // <xMax>,<yMax>.
  GBool findText(Unicode *s, int len,
		 GBool startAtTop, GBool stopAtBottom,
		 GBool startAtLast, GBool stopAtLast,
		 GBool caseSensitive, GBool backward,
		 GBool wholeWord,
		 double *xMin, double *yMin,
		 double *xMax, double *yMax);

  // Get the text which is inside the specified rectangle.
  GString *getText(double xMin, double yMin,
		   double xMax, double yMax);

  // Find a string by character position and length.  If found, sets
  // the text bounding rectangle and returns true; otherwise returns
  // false.
  GBool findCharRange(int pos, int length,
		      double *xMin, double *yMin,
		      double *xMax, double *yMax);

  // Build a flat word list, in content stream order (if
  // this->rawOrder is true), physical layout order (if
  // this->physLayout is true and this->rawOrder is false), or reading
  // order (if both flags are false).
  TextWordList *makeWordList();

  // Returns the TextPage object for the last rasterized page,
  // transferring ownership to the caller.
  TextPage *takeText();

  // Turn extra processing for HTML conversion on or off.
  void enableHTMLExtras(GBool doHTMLA) { doHTML = doHTMLA; }

  virtual void drawImage(GfxState *state, Object *ref, Stream *str,
                         int width, int height, GfxImageColorMap *colorMap,
                         int *maskColors, GBool inlineImg, GBool interpolate) override;

private:

  TextOutputFunc outputFunc;	// output function
  void *outputStream;		// output stream
  GBool needClose;		// need to close the output file?
				//   (only if outputStream is a FILE*)
  TextPage *text;		// text for the current page
  GBool physLayout;		// maintain original physical layout when
				//   dumping text
  double fixedPitch;		// if physLayout is true and this is non-zero,
				//   assume fixed-pitch characters with this
				//   width
  GBool rawOrder;		// keep text in content stream order
  GBool doHTML;			// extra processing for HTML conversion
  GBool ok;			// set up ok?
};

#endif
