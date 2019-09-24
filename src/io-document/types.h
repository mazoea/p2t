//
// author: jm (Mazoea s.r.o.)
// date: 2015
//
#pragma once

#include "io-document/json/json.hpp"
#include "maz-utils/coords.h"
#include "maz-utils/utils.h"
#include <list>

namespace maz {

    // json
    using json = nlohmann::json;

    typedef json json_dict;
    typedef json json_arr;

    inline json_arr make_json_arr() { return json::array(); }

    inline json_arr make_json_dict() { return json::object(); }

    //====================================
    // document value holder
    //====================================

    namespace doc {
        // fwd declarations
        struct page_type;
        struct line_type;

        class word_type;

        class letter_type;

        // holds value information
        using maz::bbox_type;

        typedef std::string utf8_string;

        // basic types
        typedef double confidence_type;
        typedef double double_type;

        // complex
        typedef std::vector<std::string> strings_type;
        typedef std::list<bbox_type> bboxes_type;

        typedef shared_ptr<page_type> ptr_page;
        typedef std::vector<ptr_page> pages_type;

        typedef shared_ptr<line_type> ptr_line;
        typedef std::vector<ptr_line> lines_type;

        typedef shared_ptr<word_type> ptr_word;
        typedef std::list<ptr_word> words_type;

        // cannot be vector because we are changing it's in loop
        typedef std::list<letter_type> letters_type;

        template <typename BBOX_TYPE> maz::json_dict bbox2json(const BBOX_TYPE& bbox)
        {
            return bbox_xywh<maz::json_dict, int>(
                to_int(bbox.xlt_),
                to_int(bbox.ylt_),
                to_int(bbox.xrb_ - bbox.xlt_),
                to_int(bbox.yrb_ - bbox.ylt_));
        }

        template <typename BBOX_TYPE> BBOX_TYPE json2bbox(const maz::json_dict& item, BBOX_TYPE b)
        {
            return rect_from_xywh(item, b);
        }

        template <typename T> void json2strings(const maz::json_arr& item, T& arr)
        {
            for (auto e : item)
            {
                arr.push_back(e);
            }
        }

        template <typename T> void add_arr2json(maz::json_arr& item, const T& arr)
        {
            typename T::const_iterator it = arr.begin();
            for (; it != arr.end(); ++it)
            {
                item.push_back(*it);
            }
        }

        template <typename T> void add_bboxarr2json(maz::json_arr& item, const T& arr)
        {
            typename T::const_iterator it = arr.begin();
            for (; it != arr.end(); ++it)
            {
                item.emplace_back(bbox2json(*it));
            }
        }

        /** Return merge of all the boxes. */
        inline doc::bbox_type merge(const doc::bboxes_type& arr)
        {
            if (arr.empty()) return {};
            auto s = arr.begin();
            auto e = arr.end();
            doc::bbox_type b = *s;
            while (++s != e)
                b.merge(*s);
            return b;
        }

        inline doc::bbox_type uber_bbox(const doc::bboxes_type& arr) { return merge(arr); }

        template <typename T, typename Getter> inline doc::bbox_type uber_bbox(T s, T e, Getter get)
        {
            doc::bboxes_type arr;
            for (; s != e; ++s)
                arr.push_back(get(*s));
            return uber_bbox(arr);
        }

        inline void sort_x(doc::bboxes_type& arr)
        {
            arr.sort([](const doc::bbox_type& b1, const doc::bbox_type& b2) {
                return b1.xlt() < b2.xlt();
            });
        }

        template <typename T, typename U> inline void sort_x(T& arr, U ftor)
        {
            arr.sort(
                [ftor](const typename T::value_type& e1, const typename T::value_type& e2) -> bool {
                    return ftor(e1).xlt() < ftor(e2).xlt();
                });
        }

        /**
         * Representation of an element that we know its textual representation.
         */
        struct text_section
        {
            doc::bbox_type bbox;
            doc::utf8_string text;
        };

        /**
         * Basic performance indicators.
         */
        struct basic_perf_times
        {
            std::string date_start;
            std::string date_end;
            double cpu_time{0.};

            basic_perf_times() = default;
        };

    } // namespace doc
} // namespace maz
