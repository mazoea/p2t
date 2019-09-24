#ifndef _UNICODEMAPACCENTED_H_
#define _UNICODEMAPACCENTED_H_

#include <stdio.h>
#include <ctype.h>
#include <set>
#include <map>
#include "CharTypes.h"

class UnicodeMapAccent {

	// typedefs
public:
	typedef std::map<Unicode, Unicode> unicode_map;
	typedef std::map<Unicode,unicode_map > unicode_tri_map;

	// constants
public:
	static const Unicode acute_repr; 
	static const Unicode caron_repr; 
	static const Unicode ring_repr; 
	static const Unicode diaeresis_repr; 
	static const Unicode circumflex_repr; 
	static const Unicode dotlesi_repr; 
	static const Unicode grave_repr; 
	static const Unicode cedilla_repr; 


  static const int FOUND = 0;
  static const int NOT_FOUND = 10;
  static const int DO_NOT_TRANSLATE = 11;

	// variables
private:
	static const Unicode empty_fill = (Unicode)0x20;
	// normalized set
	static unicode_map set;
	// first the accent, than the character
	static unicode_tri_map map;
  // accents which should not be translated if not fond
  // - use map we are lazy
	static unicode_map map_not_translated_accents;

public:
	template<typename T>
	static bool exists( const T& map, typename T::key_type key ) {
		return 1 == map.count( key );
	}

  static void initialise();

	static bool accent( Unicode u ) {
		return exists( set, u );
	}
	static bool accented_letter( const Unicode u ) {
		// too lazy to do all unicode 
		if (u > 255)
			return true;
		else
			return isalnum( (int)u ) > 0;
	}

	/** 
	 * One must be a normal letter, the other must be an accent. 
	 * If positive than u1 will be the accent, and u2 the letter;
	 */
	static bool accented_pair( Unicode& u1, Unicode& u2 ) {
		if (accent(u1) && accented_letter(u2)) {
			return true;
		}else if ( accent(u2) && accented_letter(u1) ) {
			std::swap( u1, u2 );
			return true;
		}
		return false;
	}

	/** Normalize. */
	static Unicode normalize( Unicode accent ) {
		if ( !exists( set, accent) )
			return empty_fill;
		return set.find(accent)->second;
	}


	/** Translate. */
	static Unicode translate( Unicode accent, Unicode letter, int& found ) {
		accent = normalize(accent);
		if ( !exists( map, accent) || !exists( map.find(accent)->second,letter ) )
        {
            if ( exists( map_not_translated_accents, accent) )
            {
                found = DO_NOT_TRANSLATE;
            }else 
            {
                found = NOT_FOUND;
            }
	    	return letter;
        }
        found = FOUND;
		return map.find(accent)->second.find(letter)->second;
	}

};


#endif
