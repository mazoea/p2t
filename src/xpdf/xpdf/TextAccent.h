#ifndef TEXTACCENT_H
#define TEXTACCENT_H

#include "CharTypes.h"
#include <cstddef>

// forward declarationa
class TextWord;
class GfxState;


namespace accented {

//#define nullptr NULL

/*
 This class extends the handling of accentes characeters from 
 simple one to more complex one
*/
class word {

        //
        // types
        //
    private:

        struct measure_type {
           // ctor
           measure_type() {}
           measure_type( double x, double y, double w, double h, double font_size );
           // this char position
           double x, y, w, h;
           double space, baseline, delta;
           int rotation;
           double cur_font_size;

           // store info from previous char
           bool last_was_translated;
           bool last_invalid_unicode;
           double last_accent_width;
        
           // can be different when we translated a pair of characters
           // to an accented one
           int cur_word_len;

           // should we end the wor(l)d after this char
           bool space_indicating_end_word;
           // is on the same line
           bool on_the_same_line;
        };

    public:
        struct accented_pair_info {
            // is the pair a letter and an accent
            bool is_accented;
            // if previous is true than letter/accent is correct
            Unicode letter_u;
            Unicode accent_u;

            // x position of the char including redundant width
            // which is however used
            struct char_bbox {
                double x_start;
                double x_end;
                double w;
            };
            
            char_bbox letter;
            char_bbox accent;

            // do these two characters overlap
            bool overlap;
        };
	private:
		struct needs_changes {
			static const int NONE = 0x0;
			static const int NEEDS_BASELINE_UPDATE = 0x1;
			int changes;
			needs_changes() : changes(NONE) {}
		};


        //
        // constants
        //
    public:
        static const bool LOOSELY = true;
        static const bool NORMAL = false;
        static const Unicode UNICODE_UNKNOWN_PREFIX = (Unicode)-1;


        //
        // variables        
        //
    private:
        TextWord* xpdf_word_;
        measure_type new_char_measures_;
        accented_pair_info pair_info_;
		needs_changes changes_todo_;

        static measure_type last_char_measures_;

        static bool last_was_translated_;
        static bool last_invalid_unicode_;


        //
        // ctor        
        //
    public:
        word( TextWord* xpdf_word, const Unicode u[],
              double x, double y, double w, double h,
              double current_font_size );

        ~word();


        //
        // member functions
        //
    public:

        // fill information about the word and letter/accent
        void initialize( const Unicode u[] );

        // return a lot of information about the accented pair (if it really is)
        // - use accented_pair_info.is_accented to check whether it is really accented
        //
        accented_pair_info init_accented_with_char( Unicode new_char );

        // return true if the last character in the word is an accent
        bool last_is_accent();

        // translate accented pair into a character
        Unicode translate_to_one_unicode( int& found ) const;

        // special check whether to end a word
        bool last_translated_new_overlaps() const;
        // special check whether to end a word
        bool new_word_starts_with_accent( Unicode u );

        // can we do a translation of accent/letter pair to one accented letter
        bool can_translate_accent();

        // should we finish the current word and start a new one
        bool should_end_word() const;


        // translate the unicode according to font information to 
        // a unicode from specific range
        // - different fonts can have the same character-codes point to different
        //   letters so we must change the character code to font dependent number
        Unicode translate_invalid_unicode( const Unicode* u, GfxState* state );

        // default overlap of letters
        bool overlap() const;

	// strange attempt for a template method pattern with minimal changes
	//
	void 
	endingWord( TextWord* text_word ) {
	}

	void  
	beganWord( TextWord* text_word );


        //
        // static
        //
    public:

        // Returns true if the argument was set to invalid unicode
        // - could not be translated with pdf code char -> unicode mappings
        //   or similar
        // - new feature is to return the codes for translation in external program
        //
        static bool is_invalid_unicode( const Unicode* u ) {
            return NULL != u 
                    && u[0] == u[1] && u[0] == UNICODE_UNKNOWN_PREFIX;
        }   


        // static setter of indicator whether in the last step we
        // mixed the accent+letter into one accented char
        static void last_was_translated( bool value ) {
            last_was_translated_ = value;
        }

        // static setter of indicator whether in the last step there
        // was an untranslatable character
        static void last_was_invalid_unicode( bool value ) {
            last_invalid_unicode_ = value;
        }


        //
        // helpers functions
        //
    private:

        // reset static variables
        void reset_static();

        // set is overlapping (check only for accented)
        void accent_overlapping( accented_pair_info& info );

        // according to accented / not accented compute if space between chars is 
        // greater which means new word
        bool space_indicating_end_word( bool loosely=NORMAL) const;

        // is the new char and the wor don the same line
        bool on_the_same_line( bool loosely=LOOSELY );

        // get few basic metrics
        void get_basic_metrics( double& baseline, double& space, double& delta ) const;

        // convert metrics to x,y
        void convert_to_human_xy( accented_pair_info::char_bbox& first, 
                                  accented_pair_info::char_bbox& second ) const;


};


} // namespace

#endif
