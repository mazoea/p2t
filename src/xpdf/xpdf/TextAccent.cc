#include "TextAccent.h"

#include <assert.h>
#include <map>
#include <sstream>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "TextOutputDev.h"
#include "GfxState.h"
#include "Error.h"

#include "UnicodeMapAccent.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace accented {

    namespace {
        
        // xpdf leftovers
        // Negative inter-character space width, i.e., overlap, which will
        // cause addChar to start a new word.
        const double MIN_DUP_BREAK_OVERLAP = 0.2;

        // xpdf leftovers
        // Inter-character space width which will cause addChar to start a new
        // word.
        const double MIN_WORD_BREAK_SPACE = 0.1;
        const double ACCENTED_CHAR_SURROUNDING_FACTOR = 1.5;

        // xpdf leftovers
        // Max difference in primary,secondary coordinates (as a fraction of
        // the font size) allowed for duplicated text (fake boldface, drop
        // shadows) which is to be discarded.
        const double DUP_MAX_PRI_DELTA = 0.1;
        const double DUP_MAX_SEC_DELTA = 0.2;

        // difference in baseline for on the same line consideration
        static const double BASELINE_DIFFERENCE = 0.3;


    } // namespace


    //
    // static
    //

    bool word::last_was_translated_ = false;
    bool word::last_invalid_unicode_ = false;
    word::measure_type word::last_char_measures_;


    //
    // public
    //

    word::word( TextWord* xpdf_word,
                const Unicode u[],
                double x,
                double y,
                double w,
                double h,
                double current_font_size
               ) : xpdf_word_( xpdf_word ), 
                   new_char_measures_( x,y,w,h,current_font_size )
    {
        // store several info from the last char
        new_char_measures_.last_invalid_unicode = word::last_invalid_unicode_;
        new_char_measures_.last_was_translated = word::last_was_translated_;
        reset_static();

        // if there is a word initialize it properly
        if ( nullptr != xpdf_word_ )
        {
            new_char_measures_.rotation = xpdf_word_->rot;
            initialize(u);
        }
    }

    word::~word() 
    {
        // store measures from last char
        word::last_char_measures_ = new_char_measures_;
    }

    void
    word::reset_static() 
    {
        word::last_invalid_unicode_ = false;
        word::last_was_translated_ = false;
    }


    void
    word::initialize( const Unicode u[] ) 
    {
        // be careful about the order of the initialization
        new_char_measures_.cur_word_len = xpdf_word_->len;
        get_basic_metrics( new_char_measures_.baseline, 
                           new_char_measures_.space, 
                           new_char_measures_.delta );
        
        // be careful when you initialize so all metrics are set before
        new_char_measures_.space_indicating_end_word = space_indicating_end_word();
        new_char_measures_.on_the_same_line = on_the_same_line(word::NORMAL);

        // is accented? => get the accent/letter correctly
        pair_info_ = init_accented_with_char( u[0] );
    }

    bool
    word::can_translate_accent()
    {
        bool can_translate = pair_info_.is_accented 
                && pair_info_.overlap 
                    // we can translate even if there is no space e.g., t'
                    //&& new_char_measures_.space_indicating_end_word
                        && on_the_same_line(word::LOOSELY);
        return can_translate;
    }

    bool 
    word::last_is_accent()
    {
        Unicode last_u = xpdf_word_->text[xpdf_word_->len-1];
        return UnicodeMapAccent::accent( last_u );
    }



    word::accented_pair_info
    word::init_accented_with_char( Unicode new_char ) 
    {
            assert( nullptr != xpdf_word_ );

        accented_pair_info pair = {};
        Unicode last_u = xpdf_word_->text[xpdf_word_->len-1];
        pair.accent_u = last_u;
        pair.letter_u = new_char;
        pair.is_accented = UnicodeMapAccent::accented_pair(pair.accent_u, pair.letter_u);

        // fill info properly if accented
        if ( pair.is_accented )
        {
            // convert to x/y
            // ´A
            if ( pair.accent_u == last_u ) 
            {
                convert_to_human_xy( pair.accent, pair.letter );
            
            // A´
            } else 
            {
                convert_to_human_xy( pair.letter, pair.accent );
                // it can happen that the ` can be too far away so use loose space_indicating_word_end
                new_char_measures_.space_indicating_end_word = space_indicating_end_word(LOOSELY);
            }
            
            // give it more space
            new_char_measures_.on_the_same_line = on_the_same_line(word::LOOSELY);
            // store whether it overlaps
            accent_overlapping( pair );
        }

        return pair;
    }

    void
    word::accent_overlapping(  accented_pair_info& pair_info  )
    {
            assert( nullptr != xpdf_word_ );
        const double acceptable_surrounding_accent = ACCENTED_CHAR_SURROUNDING_FACTOR * xpdf_word_->fontSize;
        const double will_based_on_font = DUP_MAX_PRI_DELTA * acceptable_surrounding_accent;

        // Note: the input to this function is a bit fictional
        //  we convert x/y/w/h to normal axis which do not need to represent
        //  actual pdf coordinates 100% - but it does not matter, we only use it for
        //  comparison between two characters
        // few rules:
        //  x_start <= x_end
        //  w >= 0

        // is inside
        // 1. criteria either left align or right align
        // 2. the middle of the character is +- the same
        //

        // 1.
        double space_between_letter_accent_x_start = 
            pair_info.accent.x_start - pair_info.letter.x_start;
        pair_info.overlap = fabs(space_between_letter_accent_x_start) < will_based_on_font;
        double space_between_letter_accent_x_end = 
            pair_info.accent.x_start + pair_info.accent.w - pair_info.letter.x_end;
        pair_info.overlap |= (fabs(space_between_letter_accent_x_end) < will_based_on_font);
        // okey, so one of the corners is very near - check both with very big space
        if ( pair_info.overlap )
        {
            bool both_corners_not_far = true;
            // take as the threshold the width of the letter
            const double diff_max = pair_info.letter.w;
            both_corners_not_far = fabs(space_between_letter_accent_x_start) < diff_max;
            both_corners_not_far &= fabs(space_between_letter_accent_x_end) < diff_max;
            // make extra condition for debugging
            if ( both_corners_not_far != pair_info.overlap )
            {
                pair_info.overlap = both_corners_not_far;
            }
        }

        // 2.
        double accent_center = pair_info.accent.x_start + pair_info.accent.w/2;
        double letter_center = (pair_info.letter.x_start+pair_info.letter.x_end)/2;
        pair_info.overlap |= (fabs(accent_center-letter_center) < will_based_on_font);


		// special handling for i 
		// http://www.fileformat.info/info/unicode/char/131/index.htm
		//
		if (pair_info.letter_u == UnicodeMapAccent::dotlesi_repr
				&& pair_info.accent_u== UnicodeMapAccent::acute_repr) 
		{
			static const double ADDITIONAL_SPACE = 0.5;
            // put there even more will
			pair_info.overlap = ( pair_info.letter.x_start - will_based_on_font - ADDITIONAL_SPACE < pair_info.accent.x_start 
									&& pair_info.accent.x_start < pair_info.letter.x_end + will_based_on_font + ADDITIONAL_SPACE );
		}
    }

    bool 
    word::new_word_starts_with_accent( Unicode u )
    {
        return on_the_same_line(word::LOOSELY) 
                && pair_info_.overlap 
                    && new_char_measures_.space_indicating_end_word
                        && overlap()
                            && UnicodeMapAccent::accent(u);
    }

    bool 
    word::last_translated_new_overlaps() const
    {
        return pair_info_.overlap && new_char_measures_.last_was_translated;
    }


    Unicode 
    word::translate_to_one_unicode( int& found ) const
    {
        return UnicodeMapAccent::translate( pair_info_.accent_u, 
                                            pair_info_.letter_u,
                                            found );
    }


    //
    // static
    //

    Unicode
    word::translate_invalid_unicode( const Unicode* u, GfxState* state ) 
    {
            assert( nullptr != u );

        // special private Unicode char so we do not end up with something valid
        static int _num = 0xE000;

        typedef std::map<Unicode, Unicode> charcode_to_num_type;
        typedef std::map<std::string, charcode_to_num_type> font_to_num_type;
        
        word::last_was_invalid_unicode( true );


        Ref ref = {0};
        if ( nullptr != state ) 
            state->getFont()->getEmbeddedFontID( &ref );

        std::ostringstream oss;
        oss << ref.num << " " << ref.gen;
        std::string font_name = oss.str();
        static font_to_num_type font_mapping;
        Unicode mapped_char = 0;
        charcode_to_num_type::iterator not_found = font_mapping[font_name].end();
        charcode_to_num_type::iterator found = font_mapping[font_name].find(u[2]);
        // we found mapping
        //
        if ( not_found != found ) {
            mapped_char = found->second;
        // insert mapping
        //
        }else {
            ++_num;
            charcode_to_num_type& char_map = font_mapping[font_name];
            char_map[u[2]] = _num;
            mapped_char = _num;

            // debug
            if (xpdf_word_) 
            {
                static const size_t word_text_len = 21;
                char word_text[word_text_len+1] = {0};
                for ( size_t i = 0; i < xpdf_word_->len && i < word_text_len; ++i )
                    word_text[i] = xpdf_word_->text[i];
                error(errSyntaxError, 0, "could not convert to unicode [%u] around (%s), using [%u]",
                                u[2], word_text, mapped_char );
            }else {
                error(errSyntaxError, 0, "could not convert to unicode [%u] starting new word, using [%u]",
                                u[2], mapped_char );
            }
        }

        return mapped_char;
    }



    //
    // private
    //

    void 
    word::get_basic_metrics( double& baseline, double& space, double& delta ) const
    {
        if ( nullptr == xpdf_word_ )
            return;

        switch (xpdf_word_->rot) {
            case 0:
              baseline = new_char_measures_.y;
              space = new_char_measures_.x - xpdf_word_->xMax;
              delta = new_char_measures_.x - xpdf_word_->edge[new_char_measures_.cur_word_len - 1];
              break;
            case 1:
              baseline = new_char_measures_.x;
              space = new_char_measures_.y - xpdf_word_->yMax;
              delta = new_char_measures_.y - xpdf_word_->edge[new_char_measures_.cur_word_len - 1];
              break;
            case 2:
              baseline = new_char_measures_.y;
              space = xpdf_word_->xMin - new_char_measures_.x;
              delta = xpdf_word_->edge[new_char_measures_.cur_word_len - 1] - new_char_measures_.x;
              break;
            case 3:
              baseline = new_char_measures_.x;
              space = xpdf_word_->yMin - new_char_measures_.y;
              delta = xpdf_word_->edge[new_char_measures_.cur_word_len - 1] - new_char_measures_.y;
              break;
        }

    }

    bool 
    word::should_end_word() const {
		bool simple_overlapping = overlap() && !new_char_measures_.last_invalid_unicode && !new_char_measures_.last_was_translated;
        
        // do not use as we still consider one line chars which have similar bboxes
        // see on_the_same_line()
        bool different_font_size = false;
        //bool different_font_size = new_char_measures_.cur_font_size != xpdf_word_->fontSize;

        bool too_big_space = new_char_measures_.space_indicating_end_word;
        bool not_on_the_same_line = !new_char_measures_.on_the_same_line;

        return simple_overlapping
                    || too_big_space
                        || not_on_the_same_line 
		                    || different_font_size;
    }


    bool 
    word::space_indicating_end_word(bool loosely) const 
    {
        double space = new_char_measures_.space;

        // accented chars - overlap and one of them is ',ˇ etc
	    // - this is (almost) like maaagic
	    //
        const double acceptable_surrounding_accent = ACCENTED_CHAR_SURROUNDING_FACTOR * xpdf_word_->fontSize;

        bool greater_space_than_expected = false;
	    if ( LOOSELY == loosely ||
             new_char_measures_.last_was_translated ||
                (overlap() && new_char_measures_.last_invalid_unicode) ) 
        { 
		    greater_space_than_expected = 
                space < -MIN_DUP_BREAK_OVERLAP * acceptable_surrounding_accent 
		            ||  space > MIN_WORD_BREAK_SPACE * acceptable_surrounding_accent;
	    }else {
		    greater_space_than_expected = 
                space < -MIN_DUP_BREAK_OVERLAP * xpdf_word_->fontSize 
                    || space > MIN_WORD_BREAK_SPACE * xpdf_word_->fontSize;
	    }

        return greater_space_than_expected;
    }

    bool 
    word::on_the_same_line( bool loosely ) 
    {
        const double min_font_size = std::min( xpdf_word_->fontSize, new_char_measures_.cur_font_size );
        const double acceptable_diff = BASELINE_DIFFERENCE * min_font_size;
//        const int acceptable_diff = BASELINE_DIFFERENCE * xpdf_word_->fontSize;

		//
		// must have the same rotation
		//
		if ( xpdf_word_->rot != new_char_measures_.rotation )
		{
			return false;
		}
        

        //
        // check the bottom baseline
        //
        bool on_same = false;
        if ( loosely != word::LOOSELY )
        {
            on_same = fabs(new_char_measures_.baseline - xpdf_word_->base)
                        < acceptable_diff;
        }else 
        {
            on_same = fabs(new_char_measures_.baseline - xpdf_word_->base)
                        < acceptable_diff*2;
        }

        //
        // check the top baseline
        //
        // consider as on the same line also when the font is different
        // - we must consider rotation here because of top border computation
        //  
        if ( !on_same )
        {
            double old_height = xpdf_word_->fontSize;
            double new_height = new_char_measures_.cur_font_size;

            // do not add/sub depends on rotation
			// we do not need it

			// check the upper baseline whether it is close enough
			double new_char_base = new_char_measures_.baseline;
				assert( new_char_base >= 0 );
            double new_char_base_top = new_char_base - new_height;
			
			double old_char_base = xpdf_word_->base;
				assert( old_char_base >= 0 );
            double old_char_base_top = old_char_base - old_height;

            double base_top_diff = fabs( new_char_base_top - old_char_base_top );
            // be more strict on the upper baseline
            if ( base_top_diff < acceptable_diff )
            {
                on_same = true;
            }

			//
			// check if we are on the ""left"" and +- not too far
			// e.g. regression #63 (" on the bottom is in fact " on the top but with lowered baseline )
			//
			if ( !on_same && loosely == word::NORMAL )
			{

				accented_pair_info::char_bbox last_letter = {};
				accented_pair_info::char_bbox new_letter = {};
				convert_to_human_xy( last_letter, new_letter );
				bool font_size_acceptable = fabs(xpdf_word_->fontSize - new_char_measures_.cur_font_size) < acceptable_diff;
				bool on_the_left_of_word = last_letter.x_end < new_letter.x_start;

				// if it is on the left and similar font, give it more will
				// todo: right-to-left reading
				if ( on_the_left_of_word && font_size_acceptable ) 
				{
					// base top is on the line with xpdf_word but base bottom must
					// be somewhere "near" then it is on the same line
					// char_bases are > 0
						assert( old_char_base >= 0 && new_char_base >= 0 );
						assert( old_char_base_top < old_char_base );
						assert( new_char_base_top < new_char_base );

					bool new_base_top_online = (old_char_base_top < new_char_base_top) && (new_char_base_top < old_char_base);
					bool new_base_online =  (old_char_base_top < new_char_base) && (new_char_base < old_char_base);
					
					double old_middle_base = (old_char_base + new_char_base) / 2;

					double new_bottom_old_middle_diff = fabs(old_middle_base - new_char_base);
					double new_top_old_middle_diff = fabs(old_middle_base - new_char_base_top);

					if ( new_base_top_online && new_bottom_old_middle_diff < acceptable_diff )
					{
						// hack a bit - set the baseline to the old word
						changes_todo_.changes |= needs_changes::NEEDS_BASELINE_UPDATE;
		                on_same = true;

					}else if (  new_base_online && new_top_old_middle_diff < acceptable_diff )
					{
						changes_todo_.changes |= needs_changes::NEEDS_BASELINE_UPDATE;
		                on_same = true;
					}

				}
			}

		}



        return on_same;
    }

    bool
    word::overlap() const 
    {
        bool overlap = fabs(new_char_measures_.delta) < DUP_MAX_PRI_DELTA * xpdf_word_->fontSize &&
            fabs(new_char_measures_.baseline - xpdf_word_->base) < DUP_MAX_SEC_DELTA * xpdf_word_->fontSize;
        return overlap;
    }

    void
    word::convert_to_human_xy( accented_pair_info::char_bbox& first, 
                               accented_pair_info::char_bbox& second  ) const
    {
            assert ( nullptr != xpdf_word_ );

        switch (xpdf_word_->rot) {
            case 0:
                   //assert ( new_char_measures_.x >= 0 ); //#130010360
                   assert ( word::last_char_measures_.x >= 0 );
                   //assert ( word::last_char_measures_.w >= 0 );
              // start from 0 - why not? ;)
              first.x_start = 0;
              first.x_end = first.x_start + word::last_char_measures_.w;
              first.w = first.x_end;
              // 
              second.w = new_char_measures_.w;
              second.x_start = new_char_measures_.x - word::last_char_measures_.x;
              second.x_end = second.x_start + second.w;
              break;
            case 1:
                   assert ( new_char_measures_.y >= 0 );
                   assert ( word::last_char_measures_.y >= 0 );
                   // can happen? 120064168.orig
                   //assert ( new_char_measures_.h >= 0 );
                   //assert ( word::last_char_measures_.h >= 0 );
              // start from 0 - why not? ;)
              first.x_start = 0;
              first.x_end = word::last_char_measures_.h;
              first.w = first.x_end;
              //
              second.w = new_char_measures_.h;
              second.x_start = new_char_measures_.y - word::last_char_measures_.y;
              second.x_end = second.x_start + second.w;
              break;
            case 2:
                    assert ( new_char_measures_.x >= 0 );
                    assert ( word::last_char_measures_.x >= 0 );
                    // can happen? 120047653.orig
                    //assert ( word::last_char_measures_.w <= 0 );
                first.x_start = 0;
                first.x_end = fabs(word::last_char_measures_.w);
                first.w = first.x_end;
                // 
                second.w = fabs(new_char_measures_.w);
                second.x_start = (word::last_char_measures_.x + first.x_end)
                                   - (new_char_measures_.x + second.w );
                second.x_end = second.w;
                break;
            case 3:
                   assert ( new_char_measures_.y >= 0 );
                   assert ( word::last_char_measures_.y >= 0 );
                   // can happen? 120064168.orig
                   //assert ( new_char_measures_.h <= 0 );
                   //assert ( word::last_char_measures_.h <= 0 );
              //
              first.x_start = 0;
              first.x_end = fabs(word::last_char_measures_.h);
              first.w = first.x_end;
              //
              second.w = fabs(new_char_measures_.h);
              second.x_start = word::last_char_measures_.y - new_char_measures_.y;
              second.x_end = second.x_start + second.w;
              break;
        }
        //assert( first.x_start >= 0 );
        //assert( first.x_end>= 0 );
        //assert( second.x_start >= 0 ); - can go behind the char
        //assert( second.x_end >= 0 );
    }

	void
	word::beganWord( TextWord* text_word ) {
		if ( changes_todo_.changes & word::needs_changes::NEEDS_BASELINE_UPDATE ) {
			if ( nullptr == text_word || nullptr == xpdf_word_ )
				return;
			text_word->base = xpdf_word_->base;
            // update max/min too and use the fact that "unitialised"
            // pair is set -1.0
            if ( text_word->xMax >= 0.0 ) {
                text_word->xMax = std::max( text_word->xMax, xpdf_word_->xMax );
                text_word->xMin = std::min( text_word->xMin, xpdf_word_->xMin );
            }else {
                text_word->yMax = std::max( text_word->yMax, xpdf_word_->yMax );
                text_word->yMin = std::min( text_word->yMin, xpdf_word_->yMin );
            }
		}
	}



    //
    // inner class
    //
    word::measure_type::measure_type( 
        double x_, double y_, double w_, double h_, double font_size_ 
    ) 
        : x(x_), y(y_), w(w_), h(h_), cur_font_size( font_size_)
    {
        rotation = 0;
        space = baseline = delta = 0.0;
        last_was_translated = last_invalid_unicode = false;
        last_accent_width = 0.0;
        cur_word_len = 0;
        space_indicating_end_word = false;
    }

   

} // namespace
