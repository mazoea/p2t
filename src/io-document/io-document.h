//
// author: jm (Mazoea s.r.o.)
// date: 2013
//
#pragma once

#include <fstream>
#include <functional>
#include <list>
#include <string>

// include the underlying implementation
#include "io-document/elements.h"
#include "io-document/statistics.h"
#include "io-document/types.h"
#include "maz-utils/params.h"
#include "maz-utils/utils.h"
#include "unilib/text.h"

namespace maz {

    // forward declaration
    namespace la {
        class element_layouter;
    }

    namespace doc {

        // doc
        struct document_value_holder
        {
            typedef pages_type::iterator iterator;
            typedef pages_type::const_iterator const_iterator;

            //
            basic_perf_times perf_times;
            strings_type exceptions;
            strings_type warnings;
            //
            maz::json_dict info;
            pages_type pages;

          public:
            //
            maz::json_dict to_json(output_detail dt = normal) const;
        };

        //====================================
        // line/word document structure
        //====================================

        /*
         * Interface to forms the words into line/word structure.
         */
        class iwords_structure
        {
          public:
            virtual void reformat(size_t pos, page_type& page) = 0;
            virtual bool add(ptr_word pword) = 0;
            virtual ~iwords_structure() = default;
        };

        //====================================
        // document type
        //====================================

        class document
        {
            //
            // typedefs
            //
          public:
            typedef shared_ptr<la::element_layouter> ptr_layouter;
            typedef simple_callback_at_delete1<std::function<void(void)>, perf_timer*> timer_type;

            //
            // variables
            //
          private:
            env_type& env_;
            document_value_holder doc_;
            // holds all performance info
            // it is a pointer so it shares the data between copies
            std::shared_ptr<perf_timer> ptimer_;

            // any special ordering of lines/words
            shared_ptr<iwords_structure> words_structure_;

            ptr_layouter layouter_;

            // ctor
          public:
            explicit document(env_type& env);
            document(env_type& env, const std::string& fname);

            //
            // page/doc info
            // - if no page num parameter is given, the last page is taken
            //
          public:
            // getters
            json_dict& info();
            const json_dict& info(const std::string& key) const;
            json_dict& info(const std::string& key);
            json_dict& page_info(const std::string& key);

            // doc info setter
            template <typename T> void info(const std::string& key, const T& val)
            {
                doc_.info[key] = val;
            }

            // page info setter
            template <typename T> void page_info(const std::string& key, const T& val)
            {
                current_page().info[key] = val;
            }

            // page info setter
            void page_info(const std::string& key, const doc::bbox_type& val);
            void page_info(const std::string& key, const doc::bboxes_type& val);
            void page_info(const std::string& key, const std::list<std::string>& val);

            // getter/setter for page features
            maz::json_arr& page_features_applied();

            template <typename T> void page_features_applied(const T& val)
            {
                current_page().info["applied"].push_back(val);
            }
            template <typename T> void page_features_applied(T first, T end)
            {
                while (first != end)
                {
                    page_features_applied(*first);
                    ++first;
                }
            }

            // getter/setter for layout
            json_dict& page_layout(const std::string& key = std::string());
            void page_layout(const std::string& key, const doc::bboxes_type& val);

            // specific
            std::string filename() const;
            void filename(const std::string& fname);

            // confidence, rotation, bbox, etc
            void page_confidence(confidence_type conf);
            confidence_type page_confidence() const;
            void page_rotation_confidence(int rotation, confidence_type conf);
            /** Returns `rotation_confidence` or creates empty and returns that. */
            maz::json_dict page_rotation_confidence();
            void page_bbox(const bbox_type& bbox);
            void page_image_clip_bbox(const bbox_type& bbox);
            void page_rotation(int rotation_degree);
            void page_skew(double_type skew);
            void page_scale(double_type scale);
            double_type page_scale() const;
            size_t page_count() const;

            // complex types
            void page_image(
                const std::string& key, const std::string& base64_s, const std::string& format);

            visual_elements& page_ia() { return current_page().ia; }
            void page_ia(const visual_elements& ia);

            //
            void expected_dpi(int hdpi, int vdpi);
            void metadata(const std::string& metadata);

            env_type& env();

            // generic info
            void info_command_line(int argc, char** argv);
            void info_env();

            // page
            void current_page(ptr_page ppage);
            page_type& current_page();
            ptr_page current_ppage()
            {
                return (doc_.pages.empty()) ? ptr_page() : doc_.pages[doc_.pages.size() - 1];
            }
            const page_type& current_page() const;
            page_type& page(size_t idx);
            const page_type& page(size_t idx) const;

            //
            // overall manipulation
            //
          public:
            void start_page();
            /** Useful for managing rotations runs. */
            void restart_page();
            void end_page(confidence_type confidence);

            void new_line();
            void end_line();

            //
            json_dict to_json();
            json_dict to_json(output_detail dt);
            bool from_json(const json_dict& json);

            // remove empty lines
            void populate();

            // if there was a clip, update coords
            // to match the bbox
            bbox_type from_clip_to_document(size_t idx);

            // e.g., y, x ordering
            void use_word_structure(iwords_structure* ws);

            // set & get layouter
            void layout(ptr_layouter l);

            ptr_layouter layout() const;

            /** Store start/end times that will be outputed to json. */
            void basic_perf_times(const basic_perf_times& pt) { doc_.perf_times = pt; }

            //
            // text manipulation
            //
          public:
            void append_text_utf8(const utf8_string& text);
            void text_utf8(const utf8_string& text);
            const char* page_text_utf8() const;

            //
            // word/line manipulation
            //
          public:
            // remove lines from first to last
            lines_type::iterator remove(const ptr_line& val);
            lines_type::iterator erase(lines_type::iterator first, lines_type::iterator last);
            bool erase(ptr_word pword, double min_overlap = 30.);
            bool add(ptr_word pword);
            void add(lines_type::iterator start, lines_type::iterator end);
            ptr_word append_word(line_type& line, ptr_word pword);

            /**
             * Remove words from the bounding box.
             */
            doc::words_type
            words_remove(int page_idx, const bbox_type& bbox, double min_overlap = 20.);

            words_type words_at(
                int page_idx,
                const bbox_type& bbox,
                double min_overlap = 10.,
                lines_type* plines = nullptr);

            void lines(size_t idx, const lines_type& lines);
            void clear_lines(size_t idx);

            /**
             * Removes empty lines and words that should be ignored.
             *
             * Returns `true` if at least one change was made.
             * See `word_type::clear()`.
             */
            void remove_empty(size_t idx);

            // loops over intermediate representation
            template <typename T> T for_each_word(T ftor, size_t idx = 0)
            {
                page_type& p = page(idx);
                return p.for_each_word(ftor);
            }

            template <typename T> T for_each_word_current_page(T ftor)
            {
                return for_each_word(ftor, doc_.pages.size() - 1);
            }

            template <typename T> T for_each_line(T ftor, size_t idx = 0)
            {
                page_type& p = page(idx);

                lines_type::iterator it_line = p.lines.begin();
                for (; it_line != p.lines.end(); ++it_line)
                {
                    ftor(**it_line);
                }
                return ftor;
            }

            //
            // debug
            //
          public:
            void exception(const char* exc_str);
            void warning(const char* warn_str);

            //
            // performance
            //
          public:
            perf_timer& performance();
            timer_type start_page_timer(const char* key, perf_timer* t = nullptr);

        }; // class document

// creates new variable that will live in the current block
// and will store performance info on dying
//
#define TIMER_PROBE_THIS_FNC(adoc) \
    doc::document::timer_type ____pt_fnc = (adoc).start_page_timer(CURRENT_FUNCTION);
#define TIMER_PROBE_THIS_FNC_NAME(adoc, name) \
    doc::document::timer_type ____pt_fnc = (adoc).start_page_timer(name);
// unique name for after variable
#define TIMER_PROBE_BLOCK(adoc, name) \
    doc::document::timer_type ____pt_##__LINE__ = (adoc).start_page_timer(name, ____pt_fnc.param);

#define TIMER_PROBE_THIS_FNC_OR_EMPTY(pdoc)                                                    \
    doc::document::timer_type ____pt_fnc = (pdoc) ? (pdoc)->start_page_timer(CURRENT_FUNCTION) \
                                                  : doc::document::timer_type([]() {}, nullptr);
#define TIMER_PROBE_BLOCK_OR_EMPTY(pdoc, name)                    \
    doc::document::timer_type ____pt_##__LINE__ =                 \
        (pdoc) ? (pdoc)->start_page_timer(name, ____pt_fnc.param) \
               : doc::document::timer_type([]() {}, nullptr);

        template <typename T>
        bool different_baselines(T start, T end, double diff_accepted, int min_count)
        {
            // different baselines
            int big_diff_cnt = 0;
            auto it = start;
            while (it != end)
            {
                auto it1 = it;
                while (++it1 != end)
                {
                    if (!is_in_range((*it)->bbox.y_mid(), (*it1)->bbox.y_mid(), diff_accepted))
                    {
                        ++big_diff_cnt;
                    }
                }
                ++it;
                if (big_diff_cnt >= min_count) return true;
            }
            return false;
        }

        /** Debug function that converts words to string. */
        std::string str(doc::words_type& ws);

        /**
         * Remove from base representation.
         */
        bool base_erase(doc::document& doc, doc::ptr_word pw);

    } // namespace doc

    void serialize(const char* file, const doc::bboxes_type& arr);

    /**
     * Returns words at `bbox` overlapping at least `min_overlap`.
     * Optionally returns pointer to lines where each word resides.
     *
     * NOTE: we cannot use binary search because of sorting based on `ylt`
     * but we require `yrb`.
     */
    doc::words_type words_at(
        const doc::lines_type& lines,
        const bbox_type& bbox,
        double min_overlap,
        doc::lines_type* plines = {});

} // namespace maz
