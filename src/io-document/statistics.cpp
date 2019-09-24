/**
 * by Mazoea s.r.o.
 */

#include "io-document/statistics.h"
#include "io-document/elements.h"
#include "unilib/text.h"
#include <limits>

namespace maz {
    namespace doc {

        namespace {

            enum HEIGHT_STATUS {
                STATUS_LOW = 1,
                STATUS_UPPER = 2,
                STATUS_NUM = 4,
                STATUS_SIMIL_LOW = 8,
                STATUS_SIMIL_UPPER = 16
            };

            enum LOW_UP { LOWER, UPPER, NONE };

            LOW_UP is_in_lower_upper_similar(char letter)
            {
                size_t pos = unilib::cfp::lower_upper_chars_similar.find(letter);
                if (std::string::npos == pos)
                {
                    return NONE;
                }
                return (1 == pos % 2) ? UPPER : LOWER;
            }

            /**
             * Get word statistic that are well defined, number characters.
             */
            void init_sure(
                word_statistics& stats,
                word_statistics::letter_iterator start,
                word_statistics::letter_iterator end)
            {
                word_statistics s;

                int ySureUpper_cnt = 0;

                // go through all letters
                for (word_statistics::letter_iterator it = start; it != end; ++it)
                {
                    ++s.counts.letters;

                    if (!unilib::is_ascii_char(it->text)) continue;

                    char letter = it->text[0];

                    bool char_AZ = ('A' <= letter && letter <= 'Z');
                    if (char_AZ) ++s.counts.uppers;

                    bool char_az = ('a' <= letter && letter <= 'z');
                    if (char_az) ++s.counts.lows;

                    if (!(char_az || char_AZ)) ++s.counts.ascii_but_not_azAZ;

                    // small like interpunction
                    if (unilib::is_in(letter, unilib::cfp::small_interpunction))
                    {
                        ++s.counts.small_interpunction;
                    }

                    // count the letters & sum the letters high
                    if (unilib::is_in(letter, unilib::cfp::sure_lower_chars))
                    {
                        // for sure lowercase - "aemnr:+"
                        s.means.h_low_sure += it->bbox.height();
                        ++s.counts.lows_sure;
                        s.means.ylt_low_sure += it->bbox.ylt();
                        ++s.counts.y_lows_sure;

                    } else if (unilib::is_in(letter, unilib::cfp::sure_upper_chars_yu))
                    {
                        // for sure uppercase - "ABDEFGHIJKLMNPRTbdfhijklt!%?"
                        s.means.h_upper_sure += it->bbox.height();
                        ++s.counts.uppers_sure;
                        s.means.ylt_upper_sure += it->bbox.ylt();
                        ++ySureUpper_cnt;

                    } else if (unilib::is_in(letter, unilib::cfp::sure_upper_chars_yl))
                    {
                        // for sure uppercase - "pyqg"
                        s.means.h_upper_sure += it->bbox.height();
                        // TODO is this specific lowercase really that tall as uppercase
                        ++s.counts.uppers_sure;
                        s.means.ylt_low_sure += it->bbox.ylt();
                        ++s.counts.y_lows_sure;

                    } else if (maz_is_digit(letter))
                    {
                        // for number - "0123456789"
                        s.means.h_digit += it->bbox.height();
                        ++s.counts.digits;
                        s.means.ylt_digit += it->bbox.ylt();
                    }

                    // TODO(jms) !! look at is_normal_width
                    if (unilib::is_in(letter, unilib::cfp::normal_width))
                    {
                        s.means.w_letter_normal += it->bbox.width();
                        ++s.counts.w_normal;
                    }
                }

                if (0 < s.counts.lows_sure)
                {
                    s.means.h_low_sure /= s.counts.lows_sure;
                }
                if (0 < s.counts.y_lows_sure)
                {
                    s.means.ylt_low_sure /= s.counts.y_lows_sure;
                }
                if (0 < s.counts.uppers_sure)
                {
                    s.means.h_upper_sure /= s.counts.uppers_sure;
                }
                if (0 < ySureUpper_cnt)
                {
                    s.means.ylt_upper_sure /= ySureUpper_cnt;
                }

                if (0 < s.counts.digits)
                {
                    if (1 == s.counts.digits && 0 < s.counts.uppers_sure &&
                        s.means.h_digit < s.means.h_upper_sure / 2)
                    {
                        // skip very small height of one digit
                        s.means.h_digit = 0.;
                        s.means.ylt_digit = 0.;
                    } else
                    {
                        s.means.h_digit /= s.counts.digits;
                        s.means.ylt_digit /= s.counts.digits;
                    }
                }

                // width
                if (s.counts.w_normal)
                {
                    s.means.w_letter_normal /= s.counts.w_normal;
                }

                stats.means = s.means;
                stats.counts = s.counts;
            }

            /**
             * Find min/max of all but first letters.
             */
            void init_min_max(
                word_statistics& stats,
                word_statistics::letter_iterator start,
                word_statistics::letter_iterator end)
            {
                if (start == end) return;
                ++start;

                word_statistics::min_max_type min_max;
                min_max.min_yt_without_first = std::numeric_limits<double>::max();
                min_max.min_w_without_first = std::numeric_limits<double>::max();
                // go through all letters
                for (auto it = start; it != end; ++it)
                {
                    min_max.min_yt_without_first =
                        maz_min(it->bbox.ylt(), min_max.min_yt_without_first);
                    min_max.max_yb_without_first =
                        maz_max(it->bbox.yrb(), min_max.max_yb_without_first);
                    min_max.max_h_without_first =
                        maz_max(it->bbox.height(), min_max.max_h_without_first);
                    if (!unilib::is_in(it->text[0], unilib::cfp::too_narrow_chars))
                    {
                        min_max.min_w_without_first =
                            maz_min(it->bbox.width(), min_max.min_w_without_first);
                    }
                }

                if (min_max.min_yt_without_first == std::numeric_limits<double>::max())
                {
                    min_max.min_yt_without_first = 0.;
                }
                if (min_max.min_w_without_first == std::numeric_limits<double>::max())
                {
                    min_max.min_w_without_first = 0.;
                }

                stats.min_max = min_max;
            }

            /** Get word statistic for similar characters. */
            void update_similar_letter(
                word_statistics::letter_iterator it, word_statistics& stats, int status)
            {
                // Coefficient for max tolerance of height
                static const double MAX_TOLERANCE_FOR_HEIGHT = 1.5;

                double uHeight = 0;
                int h_char = to_int(it->bbox.height());

                LOW_UP lu = is_in_lower_upper_similar(it->text[0]);

                switch (status)
                {
                case STATUS_LOW:
                    // count the letters & sum the letters height
                    if (LOWER == lu)
                    { // for lowercase similar - "cosuvxzw"
                        if (fabs(stats.means.h_low_sure - h_char) < MAX_TOLERANCE_FOR_HEIGHT)
                        {
                            stats.means.h_low_similar += h_char;
                            stats.means.ylt_low_similar += to_int(it->bbox.ylt());
                            ++stats.counts.lows_similar;
                        }
                    } else if (UPPER == lu)
                    {
                        // for uppercase similar - "COSUVXZW"
                        if (h_char >
                            (stats.means.h_low_sure * word_statistics::COEFFIC_FROM_SMALL_TO_UPPER +
                             stats.means.h_low_sure) /
                                2)
                        {
                            stats.means.h_upper_similar += h_char;
                            stats.means.ylt_upper_similar += to_int(it->bbox.ylt());
                            ++stats.counts.uppers_similar;
                        }
                    }
                    break;
                case STATUS_UPPER:
                    if (LOWER == lu)
                    { // for lowercase similar - "cosuvxzw"
                        if (h_char < (stats.means.h_upper_sure /
                                          word_statistics::COEFFIC_FROM_SMALL_TO_UPPER +
                                      stats.means.h_upper_sure) /
                                         2)
                        {
                            stats.means.h_low_similar += h_char;
                            stats.means.ylt_low_similar += to_int(it->bbox.ylt());
                            ++stats.counts.lows_similar;
                        }
                    } else if (UPPER == lu)
                    {
                        // for uppercase similar - "COSUVXZW"
                        if (fabs(stats.means.h_upper_sure - h_char) < MAX_TOLERANCE_FOR_HEIGHT)
                        {
                            stats.means.h_upper_similar += h_char;
                            stats.means.ylt_upper_similar += to_int(it->bbox.ylt());
                            ++stats.counts.uppers_similar;
                        }
                    }
                    break;
                case STATUS_NUM:
                    if (LOWER == lu)
                    { // for lowercase similar - "cosuvxzw"
                        if (h_char <
                            (stats.means.h_digit / word_statistics::COEFFIC_FROM_SMALL_TO_UPPER +
                             stats.means.h_upper_sure) /
                                2)
                        {
                            stats.means.h_low_similar += h_char;
                            stats.means.ylt_low_similar += to_int(it->bbox.ylt());
                            ++stats.counts.lows_similar;
                        }
                    } else if (UPPER == lu)
                    {
                        // for uppercase similar - "COSUVXZW"
                        if (fabs(stats.means.h_digit - h_char) < MAX_TOLERANCE_FOR_HEIGHT)
                        {
                            stats.means.h_upper_similar += h_char;
                            stats.means.ylt_upper_similar += to_int(it->bbox.ylt());
                            ++stats.counts.uppers_similar;
                        }
                    }
                    break;
                case STATUS_LOW + STATUS_UPPER:
                    if (LOWER == lu)
                    { // for lowercase similar - "cosuvxzw"
                        if (h_char < (stats.means.h_low_sure + stats.means.h_upper_sure) / 2)
                        {
                            stats.means.h_low_similar += h_char;
                            stats.means.ylt_low_similar += to_int(it->bbox.ylt());
                            ++stats.counts.lows_similar;
                        }
                    } else if (UPPER == lu)
                    {
                        // for uppercase similar - "COSUVXZW"
                        if (h_char > (stats.means.h_low_sure + stats.means.h_upper_sure) / 2)
                        {
                            stats.means.h_upper_similar += h_char;
                            stats.means.ylt_upper_similar += to_int(it->bbox.ylt());
                            ++stats.counts.uppers_similar;
                        }
                    }
                    break;
                case STATUS_LOW + STATUS_NUM:
                    if (LOWER == lu)
                    { // for lowercase similar - "cosuvxzw"
                        if (h_char < (stats.means.h_low_sure + stats.means.h_digit) / 2)
                        {
                            stats.means.h_low_similar += h_char;
                            stats.means.ylt_low_similar += to_int(it->bbox.ylt());
                            ++stats.counts.lows_similar;
                        }
                    } else if (UPPER == lu)
                    {
                        // for uppercase similar - "COSUVXZW"
                        if (h_char > (stats.means.h_low_sure + stats.means.h_digit) / 2)
                        {
                            stats.means.h_upper_similar += h_char;
                            stats.means.ylt_upper_similar += to_int(it->bbox.ylt());
                            ++stats.counts.uppers_similar;
                        }
                    }
                    break;
                case STATUS_UPPER + STATUS_NUM:
                    uHeight = (stats.means.h_upper_sure + stats.means.h_digit) / 2;
                    if (LOWER == lu)
                    { // for lowercase similar - "cosuvxzw"
                        if (h_char <
                            (uHeight + uHeight / word_statistics::COEFFIC_FROM_SMALL_TO_UPPER) / 2)
                        {
                            stats.means.h_low_similar += h_char;
                            stats.means.ylt_low_similar += to_int(it->bbox.ylt());
                            ++stats.counts.lows_similar;
                        }
                    } else if (UPPER == lu)
                    {
                        // for uppercase similar - "COSUVXZW"
                        if (fabs(uHeight - h_char) < MAX_TOLERANCE_FOR_HEIGHT)
                        {
                            stats.means.h_upper_similar += h_char;
                            stats.means.ylt_upper_similar += to_int(it->bbox.ylt());
                            ++stats.counts.uppers_similar;
                        }
                    }
                    break;
                case STATUS_LOW + STATUS_UPPER + STATUS_NUM:
                    uHeight = (stats.means.h_upper_sure + stats.means.h_digit) / 2;
                    if (LOWER == lu)
                    { // for lowercase similar - "cosuvxzw"
                        if (h_char < (stats.means.h_low_sure + uHeight) / 2)
                        {
                            stats.means.h_low_similar += h_char;
                            stats.means.ylt_low_similar += to_int(it->bbox.ylt());
                            ++stats.counts.lows_similar;
                        }
                    } else if (UPPER == lu)
                    {
                        // for uppercase similar - "COSUVXZW"
                        if (h_char > (stats.means.h_low_sure + uHeight) / 2)
                        {
                            stats.means.h_upper_similar += h_char;
                            stats.means.ylt_upper_similar += to_int(it->bbox.ylt());
                            ++stats.counts.uppers_similar;
                        }
                    }
                    break;
                default:
                    break;
                }
            }

            void init_similar(
                word_statistics& stats,
                word_statistics::letter_iterator start,
                word_statistics::letter_iterator end)
            {
                int status = 0;

                if (0. != stats.means.h_low_sure)
                {
                    status += STATUS_LOW;
                }
                if (0. != stats.means.h_upper_sure)
                {
                    status += STATUS_UPPER;
                }
                if (0. != stats.means.h_digit)
                {
                    status += STATUS_NUM;
                }

                // go through all letters
                for (auto it = start; it != end; ++it)
                {
                    std::string& letter_text = it->text;
                    // skip special characters
                    // TODO(jms)
                    if (unilib::is_ascii_char(letter_text) &&
                        !unilib::is_in(letter_text[0], unilib::cfp::ascii_special_chars))
                    { // pure ASCII without special chars
                        // check similar letters and sum height of all letters with valid height
                        update_similar_letter(it, stats, status);
                    }
                    if (!unilib::is_ascii_char(letter_text) && unilib::is_upper_first(letter_text))
                    {
                        // UTF8 encoding 2 bytes
                        stats.means.ylt_upper_accent += it->bbox.ylt();
                        ++stats.counts.upppers_diacritic;
                    }
                }

                if (stats.counts.lows_similar)
                {
                    stats.means.h_low_similar /= stats.counts.lows_similar;
                    stats.means.ylt_low_similar /= stats.counts.lows_similar;
                }
                if (stats.counts.uppers_similar)
                {
                    stats.means.h_upper_similar /= stats.counts.uppers_similar;
                    stats.means.ylt_upper_similar /= stats.counts.uppers_similar;
                }

                if (stats.counts.upppers_diacritic)
                {
                    stats.means.ylt_upper_accent /= stats.counts.upppers_diacritic;
                }

                if (0 < stats.counts.lows_similar + stats.counts.lows_sure)
                {
                    double sum_heights =
                        (stats.means.h_low_similar * stats.counts.lows_similar +
                         stats.means.h_low_sure * stats.counts.lows_sure);
                    stats.means.h_low =
                        sum_heights / (stats.counts.lows_similar + stats.counts.lows_sure);
                }

                // one or both means could be 0 (cannot divide with 0)
                int yLow_cnt = (0 < stats.means.ylt_low_similar ? 1 : 0) +
                               (0 < stats.means.ylt_low_sure ? 1 : 0);
                if (0 < yLow_cnt)
                {
                    stats.means.ylt_low =
                        (stats.means.ylt_low_similar + stats.means.ylt_low_sure) / yLow_cnt;
                }

                // do not use counts.* because means can be computed using other conditions
                // e.g., digits
                int hUpper_cnt = (0 < stats.means.h_upper_similar ? 1 : 0) +
                                 (0 < stats.means.h_upper_sure ? 1 : 0) +
                                 (0 < stats.means.h_digit ? 1 : 0);
                if (hUpper_cnt)
                {
                    stats.means.h_upper = (stats.means.h_upper_similar + stats.means.h_upper_sure +
                                           stats.means.h_digit) /
                                          hUpper_cnt;
                    stats.means.ylt_upper = (stats.means.ylt_upper_similar +
                                             stats.means.ylt_upper_sure + stats.means.ylt_digit) /
                                            hUpper_cnt;
                }
            }

            void init_distances(
                word_statistics& stats,
                word_statistics::letter_iterator start,
                word_statistics::letter_iterator end)
            {
                if (start == end)
                {
                    return;
                }

                int cnt = 0;
                auto prev = start;
                while (++start != end)
                {
                    ++cnt;
                    bbox_type& prevbbox = prev->bbox;
                    bbox_type& bbox = start->bbox;

                    // border distance
                    double tmp = bbox.xlt() - prevbbox.xrb();
                    stats.min_max.max_dist_between_letters =
                        maz_max(tmp, stats.min_max.max_dist_between_letters);
                    stats.means.inter_letter_space += tmp;

                    // center distance
                    tmp = bbox.x_mid() - prevbbox.x_mid();
                    stats.min_max.max_dist_between_letter_centers =
                        maz_max(tmp, stats.min_max.max_dist_between_letter_centers);

                    prev = start;
                }

                if (cnt)
                {
                    stats.means.inter_letter_space /= cnt;
                }
            }

        } // namespace

        //
        // word stats
        //

        const double word_statistics::COEFFIC_FROM_SMALL_TO_ACCENTED_UPPER = 1.5;
        const double word_statistics::COEFFIC_FROM_SMALL_TO_UPPER = 1.3;
        const double word_statistics::COEFFIC_FROM_UPPER_TO_ACCENTED_UPPER = 1.15;

        void word_statistics::init(
            word_statistics::letter_iterator start, word_statistics::letter_iterator end)
        {
            // gather statistic for sure upper/lower case letters and for digits
            init_sure(*this, start, end);
            init_min_max(*this, start, end);
            init_similar(*this, start, end);
            init_distances(*this, start, end);
        }

        // =================================================================
        // line stats
        // =================================================================

        void line_statistics::init() {}

        void line_statistics::init(
            line_statistics::word_iterator start, line_statistics::word_iterator end)
        {
            init();

            static const int min_valid_height = 6;

            int font_size = 0;
            int cnt_y_l = 0;
            int cnt_y_u = 0;

            means.y_baseline = 0;
            line_type::iterator it_prev_word = end;
            line_type::iterator it = start;
            size_t total_cnt = 0;

            for (int pos = 0; it != end; ++it, ++pos)
            {
                word_type& word = **it;
                ++total_cnt;

                // is it a new word group?
                // - we can start counting
                if (it_prev_word != end)
                {
                    double baseline_diff = fabs(
                        (*it_prev_word)->detail.baseline.y_mid() - word.detail.baseline.y_mid());
                    double height_diff = fabs((*it_prev_word)->height() - word.height());
                    double smaller_h = maz_min((*it_prev_word)->height(), word.height());

                    // if h diff is bigger than half the smaller one and
                    multi_visual |= (height_diff > smaller_h / 2 && baseline_diff > smaller_h / 2);
                }
                it_prev_word = it;

                //
                means.h_word += word.height();

                // ignore words with CR or LF inside
                static constexpr int min_confidence = 45;
                if (word.contains_newline() || word.confidence < min_confidence)
                {
                    continue;
                }

                // only for boxes higher than min_valid_height
                if (min_valid_height <= word.height())
                {
                    if (0 == counts.words)
                    { // set first font size
                        font_size = word.detail.font_size;
                        ++counts.fonts;
                    } else if (font_size != word.detail.font_size)
                    { // increase font count (count of changes - not count of different fonts)
                        font_size = word.detail.font_size;
                        ++counts.fonts;
                    }

                    word_statistics word_stats = word.statistics();
                    if (0 < word_stats.means.h_low)
                    {
                        means.h_low += word_stats.means.h_low;
                        ++counts.lows;
                        if (0. != word_stats.means.ylt_low)
                        {
                            means.y_low += word_stats.means.ylt_low;
                            ++cnt_y_l;
                        }
                    }
                    if (0 < word_stats.means.h_upper)
                    {
                        means.h_upper += word_stats.means.h_upper;
                        ++counts.uppers;
                        if (0. != word_stats.means.ylt_upper)
                        {
                            means.y_upper += word_stats.means.ylt_upper;
                            ++cnt_y_u;
                        }
                    }
                    if (0 < word_stats.means.ylt_upper_accent)
                    {
                        means.y_upper_accent += word_stats.means.ylt_upper_accent;
                        ++counts.accented_upper;
                    }
                    if (0 < word_stats.means.w_letter_normal)
                    {
                        means.w_letter_normal += word_stats.means.w_letter_normal;
                        ++counts.w_normal;
                    }
                    means.y_baseline += to_int(word.detail.baseline.y_mid());
                    ++counts.words;
                    counts.letters += word_stats.counts.letters;
                }
            }
            if (0 < counts.lows) means.h_low /= counts.lows;
            if (0 < cnt_y_l) means.y_low /= cnt_y_l;
            if (0 < counts.uppers) means.h_upper /= counts.uppers;
            if (0 < cnt_y_u) means.y_upper /= cnt_y_u;

            if (0 < counts.accented_upper) means.y_upper_accent /= counts.accented_upper;
            if (0 < counts.w_normal) means.w_letter_normal /= counts.w_normal;

            if (0 < counts.words)
            {
                means.y_baseline /= counts.words;
            }

            if (0 < total_cnt)
            {
                means.h_word /= total_cnt;
            }

            // sanity checks
            {
                // h_upper must be at least 1.3 * means.h_low
                if (1 < counts.lows && 1 < counts.uppers)
                {
                    if (means.h_low * 1.2 > (means.h_upper + 1.))
                    {
                        // either set to 0 or try to guess the values
                        means.h_low *= 0.9;
                        means.h_upper = maz_max(means.h_upper, 1.3 * means.h_low);
                    }
                }
            }

            // skip words without more than 1 letter
            {
                int cnt = 0;
                auto it = start;
                while (it != end)
                {
                    if (1 < (*it)->letter_size())
                    {
                        ++cnt;
                        means.inter_letter_space += (*it)->statistics().means.inter_letter_space;
                    }
                    ++it;
                }
                if (cnt)
                {
                    means.inter_letter_space /= cnt;
                }
            }
        }

        // =================================================================
        // page stats
        // =================================================================

        void page_statistics::init(line_iterator line_s, line_iterator line_e)
        {
            // words is initialised at the beginning but
            // it might have changed after corrector steps
            counts = {};
            means = {};
            std::list<double> heights;

            while (line_s != line_e)
            {
                line_type& line = **line_s;

                ++counts.lines;
                heights.push_back(line.bbox.height());

                for (ptr_word pw : line)
                {
                    ++counts.words;
                    means.h_word += pw->height();
                    means.w_letter += pw->width();
                    counts.chars += static_cast<int>(pw->letter_size());
                    if (pw->detail.from_dict) ++(counts.from_dict);
                }
                ++line_s;
            }

            if (0 < counts.lines) means.h_line = rank(heights, 0.15);
            if (0 < counts.words) means.h_word /= counts.words;
            if (0 < counts.chars) means.w_letter /= counts.chars;
        }

        word_statistics::counts_type::counts_type(int uppers_similar)
            : uppers_similar(uppers_similar)
        {
        }

    } // namespace doc
} // namespace maz
