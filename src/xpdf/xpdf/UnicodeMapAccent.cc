#include "UnicodeMapAccent.h"

const Unicode UnicodeMapAccent::acute_repr = (Unicode)180; 
const Unicode UnicodeMapAccent::caron_repr = (Unicode)711; 
const Unicode UnicodeMapAccent::ring_repr  = (Unicode)730; 
const Unicode UnicodeMapAccent::diaeresis_repr = (Unicode)168; 
const Unicode UnicodeMapAccent::circumflex_repr = (Unicode)94; 
const Unicode UnicodeMapAccent::dotlesi_repr = (Unicode)0x131; 
const Unicode UnicodeMapAccent::grave_repr = (Unicode)0x300; 
const Unicode UnicodeMapAccent::cedilla_repr = (Unicode)0x327; 

namespace {

	UnicodeMapAccent::unicode_map init_set() {
		UnicodeMapAccent::unicode_map map;
	// ´
		map[(Unicode)180] = UnicodeMapAccent::acute_repr;
		map[(Unicode)714] = UnicodeMapAccent::acute_repr;
		map[(Unicode)769] = UnicodeMapAccent::acute_repr;
		map[(Unicode)0x2019] = UnicodeMapAccent::acute_repr;
	// `
		map[(Unicode)0x300] = UnicodeMapAccent::grave_repr;
		map[(Unicode)0x60] = UnicodeMapAccent::grave_repr;
	// ˇ
		map[(Unicode)711] = UnicodeMapAccent::caron_repr;
		map[(Unicode)780] = UnicodeMapAccent::caron_repr;
	// ^
		map[(Unicode)136] = UnicodeMapAccent::circumflex_repr;
		map[(Unicode)94] = UnicodeMapAccent::circumflex_repr;
		map[(Unicode)708] = UnicodeMapAccent::circumflex_repr;
		map[(Unicode)710] = UnicodeMapAccent::circumflex_repr;
		map[(Unicode)770] = UnicodeMapAccent::circumflex_repr;
	// ˚
		map[(Unicode)730] = UnicodeMapAccent::ring_repr;
		map[(Unicode)778] = UnicodeMapAccent::ring_repr;
	// ä
		map[(Unicode)168] = UnicodeMapAccent::diaeresis_repr;
		map[(Unicode)776] = UnicodeMapAccent::diaeresis_repr;
    // cedilla (french c)
		map[(Unicode)0x327] = UnicodeMapAccent::cedilla_repr;
		map[(Unicode)0xB8] = UnicodeMapAccent::cedilla_repr;

		return map;
   }

  	UnicodeMapAccent::unicode_map init_map_not_translated_accents() 
    {
		UnicodeMapAccent::unicode_map arr;
	    // ´
		arr[ UnicodeMapAccent::acute_repr ];
        return arr;
    }


	  template<typename T, typename K>
	  inline
	  void _insert_u( T& map, K letter, K mapping ) {
		  map.insert( std::make_pair( (Unicode)letter, (Unicode)mapping ) );
	  }

	  UnicodeMapAccent::unicode_tri_map init_map() 
    {
		  UnicodeMapAccent::unicode_tri_map map;
	  // ´ (a,e,i,o,u,y,l, dotless i, n, t )
	  {
		  Unicode accutes []		= 
		  { 0x41, 0x61, 0x45, 0x65, 0x49, 0x69, 0x4F, 0x6F, 0x55, 0x75, 0x59, 0x79, 0x4C, 0x6C, 0x131,  0x4E,0x6E,   
              0x74,0x54, };
		  Unicode accutes_map []	= 
		  { 0xC1, 0xE1, 0xC9, 0xE9, 0xCD, 0xED, 0xD3, 0xF3, 0xDA, 0xFA, 0xDD, 0xFD, 0x139, 0x13A, 0xED, 0x143,0x144, 
              0x165,0x164,  };
		  for ( size_t i = 0; i < sizeof(accutes)/sizeof(Unicode); ++i )
			  _insert_u( map[UnicodeMapAccent::acute_repr], accutes[i], accutes_map[i] );
	  }
	  // ` (a,e,i,o,u,n)
	  {
		  Unicode accutes []		= 
		  { 0x41, 0x61, 0x45, 0x65, 0x49, 0x69, 0x4F, 0x6F, 0x55, 0x75, };
		  Unicode accutes_map []	= 
		  { 0xC0, 0xE0, 0xC8, 0xE8, 0xCC, 0xEC, 0xD2, 0xF2, 0xD9, 0xF9, };
		  for ( size_t i = 0; i < sizeof(accutes)/sizeof(Unicode); ++i )
			  _insert_u( map[UnicodeMapAccent::grave_repr], accutes[i], accutes_map[i] );
	  }
	  // ˇ (c,d,n,r,s,t,z,e)
	  {
		  Unicode accutes []		= 
		  { 0x43,  0x63,  0x44,  0x64,  0x4E,  0x6E,  0x52,  0x72,  0x53,  0x73,  0x54,  0x74,  0x5A,  0x7A,  0x45,  0x65,  };
		  Unicode accutes_map []	= 
		  { 0x10C, 0x10D, 0x10E, 0x10F, 0x147, 0x148, 0x158, 0x159, 0x160, 0x161, 0x164, 0x165, 0x17D, 0x17E, 0x11A, 0x11B, };
		  for ( size_t i = 0; i < sizeof(accutes)/sizeof(Unicode); ++i )
			  _insert_u( map[UnicodeMapAccent::caron_repr], accutes[i], accutes_map[i] );
	  }
	  // ^ (all ascii - but take only few - others will be left like H^)
      // e,E, a,A, u,U,  i,I, dotless i,dotless I
	  {
		  Unicode accutes []		= 
		  { 0x65,0x45, 0x61,0x41, 0x75,0x55, 0x6F,0x4F, 0x69,0x49, 0x131,0x49 };
		  Unicode accutes_map []	= 
		  { 0xEA,0xCA, 0xE2,0xC2, 0xFB,0xDB, 0xF4,0xD4, 0xEE,0xCE, 0xEE,0xCE };
		  for ( size_t i = 0; i < sizeof(accutes)/sizeof(Unicode); ++i )
			  _insert_u( map[UnicodeMapAccent::circumflex_repr], accutes[i], accutes_map[i] );
	  }
	  // ˚ (u)
	  {
		  Unicode accutes []		= { 0x55,  0x75, };
		  Unicode accutes_map []	= { 0x16E, 0x16F, };
		  for ( size_t i = 0; i < sizeof(accutes)/sizeof(Unicode); ++i )
			  _insert_u( map[UnicodeMapAccent::ring_repr], accutes[i], accutes_map[i] );
	  }
	  // ä, u, o
	  {
		  Unicode accutes []		= { 0x41, 0x61, 0x55, 0x75, 0x4f, 0x6F, };
		  Unicode accutes_map []	= { 0xC4, 0xE5, 0xDC, 0xFC, 0xD6, 0xF6, };
		  for ( size_t i = 0; i < sizeof(accutes)/sizeof(Unicode); ++i )
			  _insert_u( map[UnicodeMapAccent::diaeresis_repr], accutes[i], accutes_map[i] );
	  }
	  // french c
	  {
		  Unicode accutes []		= { 0x43, 0x63, };
		  Unicode accutes_map []	= { 0xC7, 0xE7, };
		  for ( size_t i = 0; i < sizeof(accutes)/sizeof(Unicode); ++i )
			  _insert_u( map[UnicodeMapAccent::cedilla_repr], accutes[i], accutes_map[i] );
	  }

		  return map;
	  }

  }

void 
UnicodeMapAccent::initialise()
{
  static bool initialised_ = false;
  if ( initialised_ )
    return;
  
  initialised_ = true;
  set = init_set();
  map_not_translated_accents = init_map_not_translated_accents();
  map = init_map();
}


UnicodeMapAccent::unicode_map UnicodeMapAccent::set;
UnicodeMapAccent::unicode_map UnicodeMapAccent::map_not_translated_accents;
UnicodeMapAccent::unicode_tri_map UnicodeMapAccent::map;
