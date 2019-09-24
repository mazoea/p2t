//
// author: jm (Mazoea s.r.o.)
// date: 2014
//
#pragma once

#include <list>
#include <string>
#include <utility>

// include the underlying implementation
#include "io-document/statistics.h"
#include "io-document/types.h"
#include "unilib/text.h"

namespace maz {
    namespace doc {

        /**
         * Level of detail to include in the output.
         */
        enum output_detail { basic, normal, full };

        // =============================================================

        /**
         * This class represents one letter.
         */
        class letter_type
        {
          public:
            typedef std::pair<utf8_string, double> choice_type;
            typedef std::list<choice_type> choices_type;

          public:
            utf8_string text;
            confidence_type confidence{};
            bbox_type bbox;
            bool super_script{};
            bool sub_script{};
            // not working in this version of tess
            choices_type choices;

          public:
            //
            letter_type() = default;

            explicit letter_type(const json_dict& item);

            //
            template <typename T> void set_confidence(T conf)
            {
                confidence = round<2, confidence_type>(conf);
            }

            // compare text
            bool operator==(const letter_type& rhs) const { return text == rhs.text; }

            // compare text
            bool operator==(char c) const { return unilib::is_ascii_char(text) && text[0] == c; }

            bool operator!=(char c) const { return !(*this == c); }

            //
            json_dict to_json(output_detail dt = normal) const;
        };

        inline bool operator==(char c, const letter_type& l) { return l == c; }

        // =============================================================

        /*
         * Characteristics of a word.
         */
        struct word_detail_type
        {
            //
            int font_size{-1};
            bool bold{false};
            bool italics{false};
            bool monospace{false};
            bool serif{false};
            bool underline{false};
            bool numeric{false};
            bool from_dict{false};
            bool small_caps{false};
            std::string font;
            letters_type letters;
            bbox_type baseline;

            // string tags that are informative
            strings_type info;
            // string tags that track update changes
            strings_type updates;
            // string tags that contain warnings
            strings_type warnings;

            /** If present, this are the bounding boxes of `ccs` relative to the letter. */
            bboxes_type ccs;

            //
            word_detail_type() = default;

            explicit word_detail_type(const maz::json_dict& item);

            confidence_type confidence() const;

            letters_type::iterator
            add_letter(letters_type::iterator it_pos, letter_type& new_letter);

            //
            maz::json_dict to_json(output_detail dt = normal) const;
        };

        // =============================================================

        /*
         * One word
         * basic doc["pages"]["lines"]["words"]
         *
         * If orientation is not 0, the bbox is as you see it but
         * the bboxex of letters and baseline depend on the orientation.
         */
        class word_type
        {
          public:
            typedef utf8_string::iterator iterator;
            typedef utf8_string::const_iterator const_iterator;
            typedef letters_type::iterator letter_iterator;
            typedef letters_type::const_iterator const_letter_iterator;
            typedef std::map<std::string, std::string> arbitrary_info;

            //
            enum expected_word { NO, ORDINARY, LEFTOVER };
            enum segmented_type { NORMAL };

            // do not use enums because that is not portabable and extensible
            // enough
            static constexpr int INACCURATE_BBOXES = 1 << 0;
            static constexpr int IGNORE_WORD = 1 << 2;
            static constexpr int WRAPPED_WORD = 1 << 3;

            // leave the ordering because of debugger window in VS 2015
          public:
            int id{-1};
            bbox_type bbox;
            confidence_type confidence{-1.};
            word_detail_type detail;
            int orientation{0};
            // arbitrary info e.g., for training purposes
            arbitrary_info ainfo;

          private:
            utf8_string text_;
            expected_word expected_{NO};
            segmented_type type_{NORMAL};
            shared_ptr<word_statistics> pstats_;
            // allow applications to use this to hold special information
            unsigned int flags_{0};

            // other alternatives
            words_type palts_;

            // ctors
            //
          public:
            word_type() = default;

            explicit word_type(const char* utf8_text) : text_(utf8_text) {}

            explicit word_type(const maz::json_dict& item);

            word_type(utf8_string t, bbox_type b, confidence_type c);

          public:
            word_statistics& statistics(bool recreate = false);

            /**
             * Call this when the word should not be used anymore
             * but we have difficulties in directly removing it from the document.
             */
            void clear();

            words_type alts() const { return palts_; }
            void alt(ptr_word palt);
            void clear_alts() { palts_.clear(); }
            void clear_alt(doc::ptr_word palt);

            // height (depends on orientation)
            int height() const
            {
                return to_int(
                    (0 == orientation || 2 == orientation) ? bbox.height() : bbox.width());
            }

            int width() const
            {
                return to_int(
                    (0 == orientation || 2 == orientation) ? bbox.width() : bbox.height());
            }

            const letters_type& letters() const { return detail.letters; }

            // iterator like
            //
            const_letter_iterator letter_begin() const { return detail.letters.begin(); }
            letter_iterator letter_begin() { return detail.letters.begin(); }
            const_letter_iterator letter_end() const { return detail.letters.end(); }
            letter_iterator letter_end() { return detail.letters.end(); }
            // no checks!
            letter_type& letter_back() { return detail.letters.back(); }
            // no checks!
            letter_type& letter_front() { return detail.letters.front(); }

            // indicate we have changed the original word
            void mark_update(const char* tag);
            // indicate that the word is suspicious e.g.,
            // bboxes are overlapping etc.
            void mark_warning(const char* tag, int decrease_conf = 0);
            // return if the word is suspected
            // if tag is not nullptr return if the suspected tag contains it
            bool has_warning(const char* tag = nullptr) const;
            bool has_update(const char* tag = nullptr);

            // mark additional important information about
            // the state of the word
            void info(const char* tag);

            /**
             * Set whether the word was expected (e.g., it was
             * found during segmentation).
             */
            void expected(expected_word ew) { expected_ = ew; }
            expected_word expected() const { return expected_; }

            /**
             * Set whether the word is somehow strange (e.g., a letover)
             */
            void type(const segmented_type& tp) { type_ = tp; }
            segmented_type type() const { return type_; }

            // text getters
            const char* utf8_c_str();
            utf8_string utf8_text() const;
            utf8_string& utf8_text();
            bool one_character() const;
            bool contains_newline() const;
            bool empty(bool strip = false) const;
            // be careful, this will count > 1 for accented chars
            size_t text_size() const;
            size_t letter_size() const;
            utf8_string::value_type operator[](std::size_t idx);
            utf8_string::value_type operator[](letter_iterator idx);

            char32_t uni_char(letter_iterator idx);

            // a letter can have
            // * 1,2,3 characters (utf-8)
            // * spaces
            // therefore, mapping between position in letters array and text
            // is not straight forward
            size_t text2letter(size_t letters_position);
            size_t text2letter(letter_iterator it_end);
            // bounds are not checked
            letter_iterator get_letter_iterator(size_t letters_position);

            // erase char/letter consistently
            letter_iterator erase(letter_iterator it_letter, size_t len, const char* tag);
            // erase char/letter consistently
            letter_iterator erase(
                size_t text_pos,
                size_t text_len,
                letter_iterator it_letter,
                size_t letter_len,
                const char* tag);
            // iterator to the next item after the erased one
            letter_iterator erase_letters(letter_iterator it, size_t len);

            // update bbox and text from letters
            void update_from_letters();

            // remove_len < 0 means we do not want to add space for > 1B chars
            void replace_letter(
                const unilib::utf8_string& c, word_type::letter_iterator it, const char* tag)
            {
                replace_letter(c, it, 0, tag);
            }

            void replace_letter(char c, word_type::letter_iterator it, const char* tag)
            {
                assert(0 < c);
                replace_letter(unilib::utf8_string(1, c), it, 0, tag);
            }

            // NOT IMPLEMENTED for UNICODE/utf8_char
            // void replace_letter(char32_t c, word_type::letter_iterator it, const char* tag);

            letter_iterator replace_letter(
                const unilib::utf8_string& c,
                word_type::letter_iterator it,
                int remove_letters_len,
                const char* tag);

            // it can be letters.end() and no letter text will be changed
            void replace(
                utf8_string text,
                size_t text_pos,
                size_t remove_text_len,
                word_type::letter_iterator it,
                size_t remove_letters_len,
                const char* tag);

            void insert(
                utf8_string text,
                size_t text_pos,
                word_type::letter_iterator it_pos,
                letter_type& new_letter,
                const char* tag);

            //
            void append(word_type& next, const char* glue = " ");

            /** Split word based on letter position. */
            void split(std::size_t pos, doc::words_type& words);

            void set_confidence(confidence_type conf)
            {
                confidence = round<2, confidence_type>(conf);
            }

            //
            void relative_to(const bbox_type& n, int orientation);
            void relative_to(int x, int y);

            /** Transpose all bboxes in this word. */
            void transpose(double dedeskew, double center_x, double center_y);

            /**
             * Rotate bounding box of baseline and letters based on
             * top left most corner.
             *
             * This should be called when we get visually correct rotated
             * bboxes but we want to have bboxes always horizontally aligned.
             *
             * Accepted parameters (clockwise orientation):
             *     1 - turned right,;
             *     2 - upside down;
             *     3 - turned left.
             *
             * TODO(jm) this does not correctly handle the width and skew of the baseline.
             */
            void rotate(int orientation);
            void reset_orientation();

            /**
             * Return real letter bounding box in document coordinates (not the rotated stored in
             * the word itself).
             */
            bbox_type real_letter_bbox(const_letter_iterator it);

            /**
             * Copy contents from another word.
             */
            void overwrite(word_type& w);

            /**
             * Swap `this` and `pw_` from `palts_`.
             */
            void swap_alt(ptr_word pw, const char* tag);

            /**
             * Can be used for processors relying on that information.
             */
            void flag(unsigned f, bool set = false) { (set) ? flags_ = f : flags_ |= f; }
            bool has_flag(unsigned f) const { return 0 != (flags_ & f); }

            /**
             * Remove letters if `pred` returns true.
             */
            template <typename Pred> void remove_if(Pred pred, const char* tag)
            {
                size_t cnt = letter_size();
                detail.letters.remove_if(pred);
                if (cnt != letter_size())
                {
                    mark_update(tag);
                    update_from_letters();
                }
            }

            //
            json_dict to_json(output_detail dt = normal) const;

          private:
            // call when a change occurred
            void changed();
            void relative_to_rest(int x, int y);
        };

        // =============================================================

        // doc["pages"]["lines"]
        struct line_type
        {
            typedef words_type::iterator iterator;
            typedef words_type::const_iterator const_iterator;
            typedef words_type::reverse_iterator reverse_iterator;
            typedef words_type::const_reverse_iterator const_reverse_iterator;

            bbox_type bbox;

          private:
            words_type words_;
            shared_ptr<line_statistics> pstats_;

          public:
            line_type() = default;

            explicit line_type(const maz::json_dict& item);

            utf8_string utf8_text(int max_x = -1) const;
            utf8_string utf8_alt_text(int max_x = -1, doc::words_type* pwords = {});

            line_statistics& statistics();
            void update_bbox();
            words_type words() const { return words_; }

            // container like
            iterator find(const ptr_word& pword) { return std::find(begin(), end(), pword); }
            iterator erase(iterator it_word);
            bool erase(ptr_word pword);
            void pop_front();
            void pop_back();
            void push_back(const ptr_word& pword, bool no_change = false);
            void push_front(const ptr_word& pword, bool no_change = false);
            void add(const ptr_word& pword);
            iterator insert(iterator it, const ptr_word& pword);
            bool empty() const { return words_.empty(); }
            size_t size() const { return words_.size(); }
            void clear();

            /** Returns new line starting at `it_first_new`. */
            doc::ptr_line split(const_iterator it_first_new);

            // iterator like
            iterator begin() { return words_.begin(); }
            const_iterator begin() const { return words_.begin(); }
            iterator end() { return words_.end(); }
            const_iterator end() const { return words_.end(); }

            reverse_iterator rbegin() { return words_.rbegin(); }
            const_reverse_iterator rbegin() const { return words_.rbegin(); }
            reverse_iterator rend() { return words_.rend(); }
            const_reverse_iterator rend() const { return words_.rend(); }

            ptr_word& front() { return words_.front(); }
            ptr_word& back() { return words_.back(); }

            // insert based on x-axis
            void merge(line_type& l);
            maz::json_dict to_json(output_detail dt = normal) const;

            doc::confidence_type confidence() const
            {
                if (empty()) return 0.;
                return std::accumulate(
                           words_.begin(),
                           words_.end(),
                           0.,
                           [](double c, const doc::ptr_word& pw) { return c + pw->confidence; }) /
                       size();
            }

            /** Returns cumulative lenght of words' letter_size. */
            std::size_t letter_count() const
            {
                if (empty()) return 0;

                return std::accumulate(
                    words_.begin(),
                    words_.end(),
                    static_cast<size_t>(0),
                    [](size_t c, doc::ptr_word pw) { return c + pw->letter_size(); });
            }

            /** Default remove if. */
            template <typename T> std::size_t remove_if(T ftor)
            {
                std::size_t cnt = 0;
                for (auto it = begin(); it != end();)
                {
                    if (ftor(*it))
                    {
                        ++cnt;
                        it = erase(it);
                        continue;
                    }
                    ++it;
                }
                return cnt;
            }

            /** Find all words intersecting at least `min_overlap` with bbox. */
            words_type at(const bbox_type& bbox, double min_overlap) const;

          private:
            // call when a change occurred
            void changed();
        };

        template <typename T> int remove_if(line_type& line, T ftor)
        {
            int removed = 0;
            auto it = line.begin();
            while (it != line.end())
            {
                if (ftor(*it))
                {
                    it = line.erase(it);
                    ++removed;
                    continue;
                }
                ++it;
            }
            return removed;
        }

        // =============================================================

        struct visual_element
        {
            const std::string key;

            explicit visual_element(const std::string& k) : key(k) {}
            virtual void relative_to(int x, int y) = 0;
            virtual void json(maz::json_dict& js) = 0;
            virtual bboxes_type bboxes() { return bboxes_type(); }
            virtual ~visual_element() = default;
        };

        /**
         * Boxa information holder for document.
         */
        class bbox_element : public visual_element
        {
          protected:
            bboxes_type bboxes_;

          public:
            bbox_element(const std::string& key, const doc::bboxes_type& bboxes)
                : visual_element(key), bboxes_(bboxes)
            {
            }

            void add(bbox_type bbox) { bboxes_.push_back(bbox); }
            virtual void relative_to(int x, int y) override
            {
                for (auto& b : bboxes_)
                    b.relative_to(x, y);
            }

            virtual void json(maz::json_dict& d) override
            {
                d[key] = maz::make_json_arr();
                doc::add_bboxarr2json(d[key], bboxes_);
            }

            virtual doc::bboxes_type bboxes() override { return bboxes_; }

        }; // class

        // =============================================================

        /*
         * Represents different elements on page like
         * lines, images, barcodes.
         */
        struct visual_elements
        {
            typedef std::shared_ptr<visual_element> ptr_value;
            typedef std::list<ptr_value> elements_type;
            typedef elements_type::const_iterator const_iterator;

          private:
            elements_type elements_;

          public:
            void add(ptr_value b);
            ptr_value get(const std::string& key);
            bool has(const std::string& key);

            const_iterator begin() const { return elements_.begin(); }
            const_iterator end() const { return elements_.end(); }

            template <typename T> std::shared_ptr<T> get(const std::string& key)
            {
                auto pt = get(key);
                return std::dynamic_pointer_cast<T>(pt);
            }

            void relative_to(int x, int y);
            maz::json_dict to_json() const;
        };

        // =============================================================

        /*
         * One page of a document.
         */
        struct page_type
        {
            typedef lines_type::const_iterator const_iterator;

            bbox_type bbox;
            bbox_type image_clip_bbox;
            lines_type lines;
            visual_elements ia;

            utf8_string text;
            // various information
            maz::json_dict info;
            maz::json_dict images;
            // to not break compatibility, this should hold
            // other information regarding ia
            maz::json_dict layout;

            confidence_type confidence{-1.};
            double_type scale{-1.};
            double_type deskew{-1.};
            int rotation{-1};

          private:
            shared_ptr<page_statistics> pstats_;

          public:
            // cppcheck-suppress uninitMemberVar
            page_type() : info(make_json_dict()), images(make_json_dict()), layout(make_json_dict())
            {
            }

            /**
             * Return statistics but because we do not know when the underlying
             * page has changed, re-create them by default.
             */
            page_statistics& statistics(bool recreate = false);
            void statistics(const page_statistics& ps);

            // update all bboxes relative to x, y
            void relative_to(int x, int y);

            size_t size() const;

            template <typename T> T for_each_word(T ftor)
            {
                for (ptr_line pline : lines)
                    for (ptr_word pword : *pline)
                        ftor(pword, *pline);
                return ftor;
            }

            template <typename T> T for_each_word(T ftor) const
            {
                for (ptr_line pline : lines)
                    for (ptr_word pword : *pline)
                        ftor(pword, *pline);
                return ftor;
            }

            //
            maz::json_dict to_json(output_detail dt = normal) const;
        };

        /** Return string representation of lines - use for debugging only. */
        utf8_string str(page_type& p);

        /**
         * Simple dividing words into a grid with predefined step.
         */
        class words2d
        {
          public:
            typedef std::vector<doc::ptr_word> column_words;

          private:
            // first go COLUMNS then ROWS
            std::vector<column_words> m_;
            int step_;

          public:
            words2d(
                const bbox_type& bbox,
                doc::words_type::iterator s,
                doc::words_type::iterator e,
                int step = 30);
            words2d(const bbox_type& bbox, const doc::lines_type& lines, int step = 30);

            template <typename T> void columns(T ftor)
            {
                for (size_t i = 0; i < m_.size(); ++i)
                {
                    ftor(i, m_[i]);
                }
            }

            const column_words& operator[](size_t i) const { return m_[i]; }

            size_t size() const { return m_.size(); }

            utf8_string str() const;

            int x_start(size_t i) const { return static_cast<int>(i * step_); }

          private:
            void init(const bbox_type& b);
            void add(doc::ptr_word pw);
        };

        /**
         * Output words to std::cout;
         */
        void debug_words(doc::words2d& words);

        /**
         * Decide how to append the two words, e.g., with ` ` or without.
         */
        std::string word_wrap_glue(const doc::word_type& w1, const doc::word_type& w2);
        std::string word_wrap_glue(char last_ch, char first_ch);

    } // namespace doc
} // namespace maz
