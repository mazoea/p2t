#========================================================================
#
# xpdf/cmake-xpdf.txt
#
# CMake script for the Xpdf tools.
#
# Copyright 2015 Glyph & Cog, LLC
#
#========================================================================

include_directories("${PROJECT_SOURCE_DIR}/src/xpdf")
include_directories("${PROJECT_SOURCE_DIR}/src/xpdf/goo")
include_directories("${PROJECT_SOURCE_DIR}/src/xpdf/fofi")

add_library(xpdf OBJECT  
  AcroForm.cc
  Annot.cc
  Array.cc
  BuiltinFont.cc
  BuiltinFontTables.cc
  Catalog.cc
  CharCodeToUnicode.cc
  CMap.cc
  Decrypt.cc
  Dict.cc
  Error.cc
  FontEncodingTables.cc
  Form.cc
  Function.cc
  Gfx.cc
  GfxFont.cc
  GfxState.cc
  GlobalParams.cc
  ImageOutputDev.cc
  JArithmeticDecoder.cc
  JBIG2Stream.cc
  JPXStream.cc
  Lexer.cc
  Link.cc
  NameToCharCode.cc
  Object.cc
  OptionalContent.cc
  Outline.cc
  OutputDev.cc
  Page.cc
  Parser.cc
  PDFDoc.cc
  PDFDocEncoding.cc
  PSTokenizer.cc
  SecurityHandler.cc
  Stream.cc
  TextString.cc
  UnicodeMap.cc
  UnicodeTypeTable.cc
  UnicodeMapAccent.cc
  TextAccent.cc
  XFAForm.cc
  XRef.cc
  Zoox.cc
)

set_target_properties(xpdf PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        COMPILE_FLAGS "${WARNING_FLAGS}"
)
