//
// author: jm (Mazoea s.r.o.)
// date: 2016
//
#pragma once

#include "io-document/types.h"

namespace maz {
    namespace doc {

        class word_type;

        /**
         * This include statistical information based on individual letters, baseline and other
         * information provided by the OCR.
         */
        struct word_statistics
        {
            typedef letters_type::iterator letter_iterator;

            static const double COEFFIC_FROM_SMALL_TO_ACCENTED_UPPER;
            static const double COEFFIC_FROM_SMALL_TO_UPPER;
            static const double COEFFIC_FROM_UPPER_TO_ACCENTED_UPPER;

            // =================================================================

            /**
             * Stores different means values.
             */
            struct means_type
            {
                double ylt_low{0.0};
                double ylt_upper{0.0};
                double ylt_upper_accent{0.0};
                double h_low{0.0};
                double h_upper{0.0};

                double ylt_low_sure{0.0};
                double ylt_low_similar{0.0};
                double ylt_upper_sure{0.0};
                double ylt_upper_similar{0.0};
                double ylt_digit{0.0};
                double h_low_sure{0.0};
                double h_low_similar{0.0};

                double h_upper_sure{0.0};
                double h_upper_similar{0.0};
                double h_digit{0.0};

                double w_letter_normal{0.0};

                // mean of spaces between letters in words
                double inter_letter_space{0.0};

                means_type() = default;
            };

            /**
             * Stores different counts values.
             */
            struct counts_type
            {
                // accidentally, "letter" can contain more chars
                int letters{0};
                // ascii but not alpha e.g., dot/dash
                int ascii_but_not_azAZ{0};
                int lows{0};
                int uppers{0};
                int w_normal{0};
                int digits{0};

                int lows_sure{0};
                int lows_similar{0};
                int y_lows_sure{0};
                int uppers_sure{0};
                int uppers_similar{0};
                int upppers_diacritic{0};
                int small_interpunction{0};

                counts_type() = default;
                explicit counts_type(int uppers_similar);
            };

            /**
             * Stores different min_max values.
             */
            struct min_max_type
            {
                // min_y, max_y and min_width for table leftover
                // maximum y top, minimum y bottom, minimum width but without the first character
                // (cap)
                double min_yt_without_first{0.0};
                double max_yb_without_first{0.0};
                double min_w_without_first{0.0};
                double max_h_without_first{0.0};
                double max_dist_between_letters{0.0};
                double max_dist_between_letter_centers{0.0};

                min_max_type() = default;
            };

            // =================================================================

            means_type means;
            counts_type counts;
            min_max_type min_max;

            // =================================================================

            word_statistics() = default;

            word_statistics(letter_iterator start, letter_iterator end) { init(start, end); }

          private:
            void init(letter_iterator start, letter_iterator end);
        };

        /**
         * This include statistical information based on individual letters, baseline and other
         * information provided by the OCR.
         */
        struct line_statistics
        {
            typedef words_type::iterator word_iterator;

            struct means_type
            {
                double y_low{0.0};
                double y_upper{0.0};
                double y_upper_accent{0.0};
                double h_low{0.0};
                double h_upper{0.0};
                int y_baseline{-1};
                double w_letter_normal{0.0};
                // mean of spaces between letters in words
                double inter_letter_space{0.0};
                double h_word{0.};

                means_type() = default;
            };

            struct counts_type
            {
                // accidentally, "letter" can contain more chars
                int letters{0};
                int words{0};
                int fonts{0};
                int lows{0};
                int uppers{0};
                int accented_upper{0};
                int w_normal{0};

                counts_type() = default;
            };

            means_type means;
            counts_type counts;
            // line can contain more visual lines
            bool multi_visual{false};

            line_statistics() { init(); }
            line_statistics(word_iterator start, word_iterator end) { init(start, end); }

          private:
            void init();
            void init(word_iterator start, word_iterator end);
        };

        /**
         * Page statistics are special, they contain information from OCR that cannot be
         * dynamically updated and then other variables.
         */
        struct page_statistics
        {
            typedef lines_type::iterator line_iterator;

            struct means_type
            {
                double h_line{0.0};
                double h_word{0.0};
                double w_letter{0.0};

                means_type() = default;
            };

            struct counts_type
            {
                // accidentally, "letter" can contain more chars
                int words{0};
                int from_dict{0};
                int lines{0};
                int chars{0};

                counts_type() = default;
            };

            struct confidences_type
            {
                // changes through the process, at the end that will be the number
                double current{0};
                // original ocr confidence with rotation boost
                confidence_type ocr{0.};
                // sum of all word confs
                confidence_type total_sum{0.};
                // boosted when there are words from dict
                confidence_type boost{0.};

                confidences_type() = default;
            };

            means_type means;
            counts_type counts;
            confidences_type confs;

            // confidence that can be set in e.g., segmenter to 100
            bool found_correct_rotation{false};

            page_statistics() = default;

            int mean_confidence() const
            {
                return static_cast<int>((0 < counts.words) ? (confs.total_sum / counts.words) : 0);
            }

            void init(line_iterator s, line_iterator e);
        };

    } // namespace doc
} // namespace maz
