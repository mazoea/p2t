//========================================================================
//
// Parser.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stddef.h>
#include "Object.h"
#include "Array.h"
#include "Dict.h"
#include "Decrypt.h"
#include "Parser.h"
#include "XRef.h"
#include "Error.h"

// Max number of nested objects.  This is used to catch infinite loops
// in the object structure.
#define recursionLimit 500

Parser::Parser(XRef *xrefA, Lexer *lexerA, GBool allowStreamsA) {
  xref = xrefA;
  lexer = lexerA;
  inlineImg = 0;
  allowStreams = allowStreamsA;
  lexer->getObj(&buf1);
  lexer->getObj(&buf2);
}

Parser::~Parser() {
  buf1.free();
  buf2.free();
  delete lexer;
}

Object *Parser::getObj(Object *obj, GBool simpleOnly,
		       Guchar *fileKey,
		       CryptAlgorithm encAlgorithm, int keyLength,
		       int objNum, int objGen, int recursion) {
  char *key;
  Stream *str;
  Object obj2;
  int num;
  DecryptStream *decrypt;
  GString *s, *s2;
  int c;

  // refill buffer after inline image data
  if (inlineImg == 2) {
    buf1.free();
    buf2.free();
    lexer->getObj(&buf1);
    lexer->getObj(&buf2);
    inlineImg = 0;
  }

  // array
  if (!simpleOnly && recursion < recursionLimit && buf1.isCmd("[")) {
    shift();
    obj->initArray(xref);
    while (!buf1.isCmd("]") && !buf1.isEOF())
      obj->arrayAdd(getObj(&obj2, gFalse, fileKey, encAlgorithm, keyLength,
			   objNum, objGen, recursion + 1));
    if (buf1.isEOF())
      error(errSyntaxError, getPos(), "End of file inside array");
    shift();

  // dictionary or stream
  } else if (!simpleOnly && recursion < recursionLimit && buf1.isCmd("<<")) {
    shift();
    obj->initDict(xref);
    while (!buf1.isCmd(">>") && !buf1.isEOF()) {
      if (!buf1.isName()) {
	error(errSyntaxError, getPos(),
	      "Dictionary key must be a name object");
	shift();
      } else {
      key = copyString(buf1.getName());
      shift();
      if (buf1.isEOF() || buf1.isError()) {
        gfree(key);
        break;
      }
	obj->dictAdd(key, getObj(&obj2, gFalse,
				 fileKey, encAlgorithm, keyLength,
				 objNum, objGen, recursion + 1));
      }
    }
    if (buf1.isEOF())
      error(errSyntaxError, getPos(), "End of file inside dictionary");
    // stream objects are not allowed inside content streams or
    // object streams
    if (allowStreams && buf2.isCmd("stream")) {
      if ((str = makeStream(obj, fileKey, encAlgorithm, keyLength,
			    objNum, objGen, recursion + 1))) {
	obj->initStream(str);
      } else {
	obj->free();
	obj->initError();
      }
    } else {
      shift();
    }

  // indirect reference or integer
  } else if (buf1.isInt()) {
    num = buf1.getInt();
    shift();
    if (buf1.isInt() && buf2.isCmd("R")) {
      obj->initRef(num, buf1.getInt());
      shift();
      shift();
    } else {
      obj->initInt(num);
    }

  // string
  } else if (buf1.isString() && fileKey) {
    s = buf1.getString();
    s2 = new GString();
    obj2.initNull();
    decrypt = new DecryptStream(new MemStream(s->getCString(), 0,
					      s->getLength(), &obj2),
				fileKey, encAlgorithm, keyLength,
				objNum, objGen);
    decrypt->reset();
    while ((c = decrypt->getChar()) != EOF) {
      s2->append((char)c);
    }
    delete decrypt;
    obj->initString(s2);
    shift();

  // simple object
  } else {
    buf1.copy(obj);
    shift();
  }

  return obj;
}

Stream *Parser::makeStream(Object *dict, Guchar *fileKey,
			   CryptAlgorithm encAlgorithm, int keyLength,
			   int objNum, int objGen, int recursion) {
  Object obj;
  BaseStream *baseStr;
  Stream *str;
  GFileOffset pos, endPos, length;

  // get stream start position
  lexer->skipToNextLine();
  if (!(str = lexer->getStream())) {
    return NULL;
  }
  pos = str->getPos();

  // check for length in damaged file
  if (xref && xref->getStreamEnd(pos, &endPos)) {
    length = endPos - pos;

  // get length from the stream object
  } else {
  dict->dictLookup("Length", &obj, recursion);
  if (obj.isInt()) {
      length = (GFileOffset)(Guint)obj.getInt();
    obj.free();
  } else {
    error(errSyntaxError, getPos(), "Bad 'Length' attribute in stream");
    obj.free();
    return NULL;
  }
  }

  // in badly damaged PDF files, we can run off the end of the input
  // stream immediately after the "stream" token
  if (!lexer->getStream()) {
    return NULL;
  }
  baseStr = lexer->getStream()->getBaseStream();

  // skip over stream data
  lexer->setPos(pos + length);

  // refill token buffers and check for 'endstream'
  shift();  // kill '>>'
  shift();  // kill 'stream'
  if (buf1.isCmd("endstream")) {
    shift();
  } else {
    error(errSyntaxError, getPos(), "Missing 'endstream'");
    // kludge for broken PDF files: just add 5k to the length, and
    // hope its enough
    length += 5000;
  }

  // make base stream
  str = baseStr->makeSubStream(pos, gTrue, length, dict);

  // handle decryption
  if (fileKey) {
    str = new DecryptStream(str, fileKey, encAlgorithm, keyLength,
			    objNum, objGen);
  }

  // get filters
  str = str->addFilters(dict, recursion);

  return str;
}

void Parser::shift() {
  if (inlineImg > 0) {
    if (inlineImg < 2) {
      ++inlineImg;
    } else {
      // in a damaged content stream, if 'ID' shows up in the middle
      // of a dictionary, we need to reset
      inlineImg = 0;
    }
  } else if (buf2.isCmd("ID")) {
    lexer->skipChar();		// skip char after 'ID' command
    inlineImg = 1;
  }
  buf1.free();
  buf1 = buf2;
  if (inlineImg > 0)		// don't buffer inline image data
    buf2.initNull();
  else
	lexer->getObj(&buf2);
}
