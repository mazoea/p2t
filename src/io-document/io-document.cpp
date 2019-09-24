/**
 * by Mazoea s.r.o.
 */

#include "io-document/io-document.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>

#include "maz-utils/coords.h"
#include "maz-utils/utils.h"

namespace maz {
    namespace doc {

        namespace {

            /**
             * Traverse blocks recursively and get the blocks in last running one.
             */
            std::list<perf_timer>* blocks_in_last_running(perf_timer& t)
            {
                std::list<perf_timer>* ret = nullptr;

                // the last one can be the only one active
                if (!t.blocks.empty())
                {
                    ret = blocks_in_last_running(t.blocks.back());
                    if (ret) return ret;
                }

                if (t.running()) return &t.blocks;

                return nullptr;
            }

            maz::json_arr to_json(const perf_timer& t)
            {
                maz::json_arr arr = make_json_arr();
                arr[0] = t.name;
                arr[1] = t.elapsed;
                arr[2] = maz::make_json_arr();
                for (auto& pt : t.blocks)
                {
                    arr[2].push_back(to_json(pt));
                }
                return arr;
            }

        } // namespace
        //

        maz::json_dict document_value_holder::to_json(output_detail dt) const
        {
            maz::json_dict d = make_json_dict();

            d["time_start"] = perf_times.date_start;
            d["time_end"] = perf_times.date_end;
            d["time_cpu"] = perf_times.cpu_time;
            d["exceptions"] = maz::make_json_arr();
            add_arr2json(d["exceptions"], exceptions);
            d["warnings"] = maz::make_json_arr();
            add_arr2json(d["warnings"], warnings);
            d["info"] = info;

            for (auto page : pages)
            {
                d["pages"].emplace_back(page->to_json(dt));
            }

            return d;
        }

        //===============================
        // document
        //===============================

        document::document(env_type& env) : env_(env){};

        document::document(env_type& env, const std::string& fname)
            : env_(env), ptimer_(new perf_timer("document"))
        {
            ptimer_->start();
            doc_.perf_times.date_start = now();
            filename(fname);
        }

        //
        //
        //

        void document::exception(const char* exc_str) { doc_.exceptions.push_back(exc_str); }

        void document::warning(const char* warn_str) { doc_.warnings.push_back(warn_str); }

        //
        //
        //
        json_dict& document::info() { return doc_.info; }

        json_dict& document::info(const std::string& key) { return doc_.info[key]; }

        const json_dict& document::info(const std::string& key) const { return doc_.info[key]; }

        json_dict& document::page_info(const std::string& key) { return current_page().info[key]; }

        void document::page_info(const std::string& key, const bbox_type& val)
        {
            current_page().info[key] = bbox2json(val);
        }
        void document::page_info(const std::string& key, const bboxes_type& val)
        {
            add_bboxarr2json(current_page().info[key], val);
        }

        void document::page_info(const std::string& key, const std::list<std::string>& val)
        {
            json_arr arr(val);
            current_page().info[key] = arr;
        }

        json_dict& document::page_layout(const std::string& key)
        {
            if (key.empty()) return current_page().layout;
            return current_page().layout[key];
        }

        void document::page_layout(const std::string& key, const bboxes_type& val)
        {
            add_bboxarr2json(current_page().layout[key], val);
        }

        json_arr& document::page_features_applied() { return current_page().info["applied"]; }

        //
        //
        //

        std::string document::filename() const { return info("filename"); }

        void document::filename(const std::string& fname) { info("filename", fname); }

        confidence_type document::page_confidence() const { return current_page().confidence; }

        void document::page_confidence(confidence_type conf) { current_page().confidence = conf; }

        void document::page_rotation_confidence(int rotation, confidence_type conf)
        {
            current_page().info["rotation_confidence"][to_string(rotation)] = conf;
        }

        maz::json_arr document::page_rotation_confidence()
        {
            // do not use `const` in the declaration of this function
            // because it would throw if the key is not found.
            // In case the key is not found, it will be created.
            return current_page().info["rotation_confidence"];
        }

        void document::page_bbox(const bbox_type& bbox) { current_page().bbox = bbox; }

        void document::page_image_clip_bbox(const bbox_type& bbox)
        {
            current_page().image_clip_bbox = bbox;
        }

        void document::page_rotation(int rotation_degree)
        {
            current_page().rotation = rotation_degree;
        }

        void document::page_skew(double_type skew) { current_page().deskew = skew; }

        void document::page_scale(double_type scale) { current_page().scale = scale; }

        double_type document::page_scale() const { return current_page().scale; }

        size_t document::page_count() const { return doc_.pages.size(); }

        void document::expected_dpi(int hdpi, int vdpi)
        {
            info("hdpi", hdpi);
            info("vdpi", vdpi);
        }

        void document::metadata(const std::string& metadata) { info("metadata", metadata); }

        env_type& document::env() { return env_; }

        void document::info_command_line(int argc, char** argv)
        {
            typedef std::vector<std::string> args_type;

            json_arr cmd_arr = maz::make_json_arr();
            args_type args(argv, argv + argc);
            for (auto arg : args)
            {
                cmd_arr.emplace_back(arg);
            }
            info("cmd", cmd_arr);
        }

        void document::info_env()
        {
            json_arr env_arr = maz::make_json_arr();
            for (auto& p : env_)
            {
                env_arr.emplace_back(std::string(p.first) + "=" + p.second);
            }
            info("env", env_arr);
        }

        //
        //
        //
        void document::page_image(
            const std::string& key, const std::string& base64_s, const std::string& format)
        {
            current_page().images[key]["data"] = base64_s;
            current_page().images[key]["format"] = format;
        }

        void document::page_ia(const visual_elements& ia)
        {
            current_page().ia = ia;
            for (auto& e : ia)
            {
                std::string val = "ia:" + e->key + ":" + to_string(e->bboxes().size());
                page_features_applied(val);
            }
        }

        //
        // text manipulation
        //

        void document::append_text_utf8(const utf8_string& t) { current_page().text.append(t); }

        void document::text_utf8(const utf8_string& text) { current_page().text = text; }

        const char* document::page_text_utf8() const { return current_page().text.c_str(); }

        ptr_word document::append_word(line_type& line, ptr_word pword)
        {
            if (pword->empty(true)) return ptr_word();

            line.push_back(pword);
            return pword;
        }

        doc::words_type
        document::words_remove(int page_idx, const bbox_type& bbox, double min_overlap)
        {
            doc::words_type removed;
            for_each_line(
                [bbox, &removed, min_overlap](doc::line_type& line) {
                    if (min_overlap > line.bbox.intersects_y(bbox)) return;
                    if (min_overlap > line.bbox.intersects_x(bbox)) return;

                    for (auto it = line.begin(); it != line.end();)
                    {
                        auto& w = **it;
                        if (2 * min_overlap < bbox.intersects(w.bbox))
                        {
                            removed.push_back(*it);
                            it = line.erase(it);
                        } else
                        {
                            ++it;
                        }
                    }
                },
                page_idx);
            return removed;
        }

        words_type document::words_at(
            int page_idx, const bbox_type& bbox, double min_overlap, lines_type* plines)
        {
            const page_type& p = page(page_idx);
            return maz::words_at(p.lines, bbox, min_overlap, plines);
        }

        bool document::erase(ptr_word pword, double min_overlap)
        {
            lines_type::iterator it_line = current_page().lines.begin();
            for (; it_line != current_page().lines.end(); ++it_line)
            {
                line_type& line = **it_line;
                if (min_overlap <= line.bbox.intersects(pword->bbox))
                {
                    if (line.erase(pword)) return true;
                }
            }
            return false;
        }

        lines_type::iterator document::remove(const ptr_line& val)
        {
            auto& ls = current_page().lines;
            for (auto it = ls.begin(); it != ls.end(); ++it)
                if (*it == val) return ls.erase(it);
            return ls.end();
        }

        lines_type::iterator document::erase(lines_type::iterator first, lines_type::iterator last)
        {
            return current_page().lines.erase(first, last);
        }

        bool document::add(ptr_word pword)
        {
            if (pword->empty(true)) return false;

            // more sophisticated
            if (words_structure_) return words_structure_->add(pword);

            // simple add
            if (current_page().lines.empty()) new_line();
            current_page().lines.back()->push_back(pword);
            return true;
        }

        void document::add(lines_type::iterator start, lines_type::iterator end)
        {
            auto start_y = (*start)->bbox.ylt();
            auto& ls = current_page().lines;
            for (auto it = ls.begin(); it != ls.end(); ++it)
            {
                // add it
                if ((*it)->bbox.ylt() > start_y)
                {
                    ls.insert(it, start, end);
                    return;
                }
            }
            ls.insert(ls.end(), start, end);
        }

        void document::end_line() { new_line(); }

        void document::new_line()
        {
            if (doc_.pages.empty()) return start_page();
            current_page().lines.push_back(ptr_line(new line_type));
        }

        void document::start_page()
        {
            doc_.pages.push_back(ptr_page(new page_type));
            current_page().text = "";
            new_line();
        }

        void document::restart_page()
        {
            doc_.pages.pop_back();
            start_page();
        }

        void document::end_page(confidence_type confidence) { page_confidence(confidence); }

        void document::clear_lines(size_t idx) { page(idx).lines.clear(); }

        void document::remove_empty(size_t idx)
        {
            // delete all empty lines
            lines_type& lines = page(idx).lines;

            // erase words to ignore
            for (auto& pl : lines)
            {
                pl->remove_if([](doc::ptr_word pw) {
                    bool remove = pw->has_flag(doc::word_type::IGNORE_WORD);
                    return remove;
                });
            }

            // erase empty lines
            erase(
                std::remove_if(
                    lines.begin(),
                    lines.end(),
                    [](const lines_type::value_type& line) { return line->empty(); }),
                lines.end());
        }

        void document::lines(size_t idx, const lines_type& lines) { page(idx).lines = lines; }

        //
        //
        //

        perf_timer& document::performance() { return *ptimer_; }

        // page timer [name, elapsed, blocks]
        document::timer_type document::start_page_timer(const char* key, perf_timer* t)
        {
            auto arr = (t) ? &(t->blocks) : blocks_in_last_running(*ptimer_);
            // insert a new probe at the end, start it and bind the
            // callback executed at ~ to `timer.end`
            arr->push_back(perf_timer(key));
            arr->back().start();
            return timer_type(std::bind(&perf_timer::end, &arr->back()), &arr->back());
        }

        //
        // conversion
        //

        maz::json_dict document::to_json(output_detail dt)
        {
            ptimer_->end();

            // if not stored explicitly - use now so we have at least some time
            if (doc_.perf_times.date_end.empty())
            {
                doc_.perf_times.date_end = now();
            }

            json_dict d = doc_.to_json(dt);
            d["performance"] = doc::to_json(*ptimer_);
            return d;
        }

        maz::json_dict document::to_json()
        {
            output_detail dt(normal);
            if (has_env(env_, "word-properties", "full")) dt = full;
            return to_json(dt);
        }

        bool document::from_json(const maz::json_dict& json)
        {
            // new page
            doc_.pages.push_back(ptr_page(new page_type));
            current_page().text = "";

            const maz::json_dict& page = json["pages"][0];

            if (page.count("info"))
            {
                auto& pi = page["info"];
                for (auto& k : {"from_dict", "words_count", "mean_confidence"})
                {
                    if (pi.find(k) != pi.end())
                    {
                        info(k, pi[k].get<int>());
                    }
                }
            }

            int rotation = page["rotation"];
            page_rotation(rotation);
            float scale = page["scale"];
            page_scale(scale);
            // old version support
            float skew = page.count("skew") ? page["skew"] : page["deskew"];
            page_skew(skew);
            page_bbox(json2bbox(page["bbox"], bbox_type()));
            // clip will be always the same as page bbox
            // what could not be the same is image clip but that
            // is not required to work for now
            // TODO(jm)
            if (page.count("image_clip_bbox"))
            {
                page_image_clip_bbox(json2bbox(page["image_clip_bbox"], bbox_type()));
            } else
            {
                page_image_clip_bbox(current_page().image_clip_bbox);
            }

            if (page.count("image"))
            {
                std::string key("binary");
                const json_dict& images = page["image"];
                if (images.count(key))
                {
                    page_image(key, images[key]["data"], images[key]["format"]);
                }
            }

            if (page.count("ia"))
            {
                typedef visual_elements::ptr_value visual;
                visual_elements ia_elements;

                const json_dict& ias = page["ia"];
                for (auto it = ias.begin(); it != ias.end(); ++it)
                {
                    if (!it.value().is_array()) continue;
                    if (it.value().empty()) continue;
                    if (0 == it.value().begin()->count("w")) continue;

                    bboxes_type bboxes;
                    for (const auto& v : it.value())
                        bboxes.push_back(json2bbox(v, bbox_type()));
                    ia_elements.add(visual(new bbox_element(it.key(), bboxes)));
                }
                page_ia(ia_elements);
            }

            // words
            const maz::json_arr& lines = json["pages"][0]["lines"];
            for (maz::json_arr::const_iterator it = lines.begin(); it != lines.end(); ++it)
            {
                current_page().lines.push_back(ptr_line(new line_type(*it)));
            }

            // confidence
            int confidence = page["confidence"];
            end_page(confidence);

            return true;
        }

        //
        //
        //
        page_type& document::page(size_t idx) { return *(doc_.pages[idx]); }

        const page_type& document::page(size_t idx) const { return *(doc_.pages[idx]); }

        void document::current_page(doc::ptr_page ppage)
        {
            doc_.pages.pop_back();
            doc_.pages.push_back(ppage);
        }

        page_type& document::current_page()
        {
            // create if empty
            if (doc_.pages.empty()) start_page();
            assert(0 < doc_.pages.size());
            // return last one
            return page(doc_.pages.size() - 1);
        }

        const page_type& document::current_page() const
        {
            assert(0 < doc_.pages.size());
            // return last one
            return page(doc_.pages.size() - 1);
        }

        void document::populate()
        {
            for (auto& page : doc_.pages)
            {
                // store only non null words/lines
                lines_type& lines = page->lines;
                auto line_it = lines.begin();
                while (line_it != lines.end())
                {
                    // delete in-place
                    if ((*line_it)->empty())
                    {
                        line_it = lines.erase(line_it);
                    } else
                    {
                        ++line_it;
                    }
                }
            }
        }

        /**
         * Be careful, call this function after all other doc member variables have been set,
         * so it can update the coordinates everywhere.
         */
        bbox_type document::from_clip_to_document(size_t idx)
        {
            page_type& p = page(idx);
            bbox_type clip = p.image_clip_bbox;
            bbox_type bbox = p.bbox;
            // if left top corner is the same then all coordinates are OK
            if (clip.xlt() == bbox.xlt() && clip.ylt() == bbox.ylt())
            {
                return clip;
            }
            // we have a clip that is different to page
            p.relative_to(to_int(clip.xlt()), to_int(clip.ylt()));
            return {};
        }

        void document::use_word_structure(iwords_structure* ws)
        {
            words_structure_.reset(ws);
            for (size_t i = 0; i < page_count(); ++i)
            {
                words_structure_->reformat(i, page(i));
            }
        }

        void document::layout(ptr_layouter l) { layouter_ = l; }

        document::ptr_layouter document::layout() const { return layouter_; }

        std::string str(doc::words_type& ws)
        {
            std::string s;
            for (auto& pw : ws)
            {
                std::ostringstream oss;
                oss << "id:" << pw->id << "[" << pw->utf8_text() << "] conf:" << pw->confidence
                    << " w: " << pw->bbox.width() << " h: " << pw->bbox.height()
                    << " x: " << pw->bbox.xrb() << " y: " << pw->bbox.ylt() << std::endl;
                s += oss.str();
            }
            return s;
        }

        bool base_erase(doc::document& doc, doc::ptr_word pw)
        {
            // cppcheck-suppress unreadVariable
            auto& w = *pw;
            return doc.erase(pw, 100.);
        }

    } // namespace doc

    // ================================

    void serialize(const char* file, const doc::bboxes_type& arr)
    {
        std::ofstream off(file);
        for (auto& b : arr)
            off << serialize(b) << "," << std::endl;
        off.close();
    }

    doc::words_type words_at(
        const doc::lines_type& lines,
        const bbox_type& bbox,
        double min_overlap,
        doc::lines_type* plines)
    {
        assert(1. < min_overlap);

        static const auto overlaps = [](const bbox_type& bb, const bbox_type& lb) {
            static constexpr double min_overlap_y = 10.;
            return min_overlap_y < bb.intersects_y(lb);
        };

        doc::words_type ret;

        // not smarties about start - see description of `words_at`
        doc::lines_type::const_iterator sel = lines.begin();

        for (auto it = sel; it != lines.end(); ++it)
        {
            doc::line_type& l = **it;
            if (overlaps(bbox, l.bbox))
            {
                auto words = l.at(bbox, min_overlap);
                if (plines) plines->insert(plines->end(), words.size(), *it);
                ret.splice(ret.end(), words);
            }

            // break if line too below but give it space if a word is big
            if (l.bbox.ylt() > (bbox.yrb() + 2 * bbox.height()))
            {
                break;
            }
        }
        return ret;
    }

} // namespace maz
