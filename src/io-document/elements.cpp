#include "io-document/elements.h"

namespace maz {
    namespace doc {

        //
        namespace {

            // have sane values in json
            const char* stringify(word_type::expected_word ew)
            {
                switch (ew)
                {
                case word_type::NO:
                    return "NO";
                case word_type::ORDINARY:
                    return "ORDINARY";
                case word_type::LEFTOVER:
                    return "LEFTOVER";
                default:
                    return "INVALID";
                }
            }

            // have sane values in json
            const char* stringify(word_type::segmented_type st)
            {
                switch (st)
                {
                case word_type::NORMAL:
                    return "";
                default:
                    return "INVALID";
                }
            }

            template <typename T> void relative_to(T& arr, int x, int y)
            {
                for (auto& elem : arr)
                {
                    elem.relative_to(x, y);
                }
            }

        } // namespace

        //===============================
        // document elements
        //===============================

        letter_type::letter_type(const maz::json_dict& item)
        {
            // cppcheck-suppress useInitializationList
            text = item["text"];
            confidence = item["confidence"];
            super_script = item.value("sup", false);
            sub_script = item.value("sub", false);
            bbox = json2bbox(item["bbox"], bbox);
        }

        maz::json_dict letter_type::to_json(output_detail dt) const
        {
            maz::json_dict d = make_json_dict();

            d["text"] = text;
            d["confidence"] = round<2>(confidence);
            d["bbox"] = bbox2json(bbox);
            json_arr choices_arr = maz::make_json_arr();

            static const size_t max_choices = 3;
            for (auto& c : choices)
            {
                if (text != c.first)
                {
                    json_arr one_choice = maz::make_json_arr();
                    one_choice[0] = c.first;
                    one_choice[1] = round<2>(c.second);
                    choices_arr.push_back(one_choice);
                    if (max_choices <= choices_arr.size())
                    {
                        break;
                    }
                }
            }
            d["choices"] = choices_arr;

            if (dt == full)
            {
                d["sup"] = super_script;
                d["sub"] = sub_script;
            }
            return d;
        }

        //

        word_detail_type::word_detail_type(const maz::json_dict& item)
        {
            font_size = item.value("font_size", -1);
            bold = item.value("bold", false);
            italics = item.value("italics", false);
            monospace = item.value("monospace", false);
            serif = item.value("serif", false);
            numeric = item.value("numeric", false);
            from_dict = item["from_dict"];
            underline = item.value("underline", false);
            small_caps = item.value("small", false);
            font = item.value("font", "");
            baseline = json2bbox(item["baseline"], baseline);

            if (item.count("letters"))
            {
                for (auto letter : item["letters"])
                {
                    letters.push_back(letter_type(letter));
                }
            }

            if (item.count("updates"))
            {
                json2strings(item["updates"], updates);
            }
            if (item.count("warnings"))
            {
                json2strings(item["warnings"], warnings);
            }
            if (item.count("info"))
            {
                json2strings(item["info"], info);
            }
        }

        letters_type::iterator
        word_detail_type::add_letter(letters_type::iterator it_pos, letter_type& new_letter)
        {
            return letters.insert(it_pos, new_letter);
        }

        confidence_type word_detail_type::confidence() const
        {
            confidence_type conf = 0.;
            letters_type::const_iterator it;
            for (it = letters.begin(); it != letters.end(); ++it)
            {
                conf += it->confidence;
            }
            return letters.empty() ? 0 : conf / letters.size();
        }

        maz::json_dict word_detail_type::to_json(output_detail dt) const
        {
            maz::json_dict d = make_json_dict();

            if (dt == full)
            {
                d["font"] = font;
                d["font_size"] = font_size;
                d["bold"] = bold;
                d["italics"] = italics;
                d["monospace"] = monospace;
                d["serif"] = serif;
                d["numeric"] = numeric;
                d["underline"] = underline;
                d["small"] = small_caps;
            }

            d["from_dict"] = from_dict;
            d["baseline"] = bbox2json(baseline);
            for (auto letter : letters)
            {
                d["letters"].emplace_back(letter.to_json(dt));
            }

            if (!updates.empty())
            {
                add_arr2json(d["updates"], updates);
            }
            if (!warnings.empty())
            {
                add_arr2json(d["warnings"], warnings);
            }
            if (!info.empty())
            {
                add_arr2json(d["info"], info);
            }

            return d;
        }

        //
        // word type
        //

        word_type::word_type(const maz::json_dict& item)
        {
            id = item.value("id", -1);
            bbox = json2bbox(item["bbox"], bbox);
            if (item.find("detail") != item.end())
            {
                detail = word_detail_type(item["detail"]);
            }
            confidence = item.value("confidence", -1);
            text_ = item["text"];
            orientation = item.value("orientation", 0);

            // v1
            if (item.find("original") != item.end())
            {
                palts_.push_back(ptr_word(new word_type(item["original"])));

                // v2
            } else if (item.find("alts") != item.end())
            {
                auto& arr = item["alts"];
                for (auto it = arr.begin(); it != arr.end(); ++it)
                {
                    palts_.push_back(ptr_word(new word_type(*it)));
                }
            }

            if (item.find("arbitrary") != item.end())
            {
                auto& d = item["arbitrary"];
                for (auto it = d.begin(); it != d.end(); ++it)
                {
                    ainfo[it.key()] = it.value();
                }
            }

            if (item.find("flags") != item.end())
            {
                flags_ = item["flags"];
            }
        }

        word_type::word_type(utf8_string t, bbox_type b, confidence_type c)
            : id(-1), bbox(b), text_(std::move(t))
        {
            set_confidence(c);
        }

        void word_type::clear()
        {
            id = -1;
            bbox = {};
            text_.clear();
            clear_alts();
            detail = {};
            update_from_letters();
            ainfo.clear();
            flag(IGNORE_WORD, true);
        }

        void word_type::alt(ptr_word palt)
        {
            // do not add equal alternatives
            if (palt->utf8_text() == utf8_text()) return;

            palts_.push_front(palt);
            palt->clear_alts();
        }

        void word_type::clear_alt(doc::ptr_word palt) { palts_.remove(palt); }

        word_statistics& word_type::statistics(bool recreate)
        {
            if (!pstats_ || recreate)
            {
                pstats_ =
                    shared_ptr<word_statistics>(new word_statistics(letter_begin(), letter_end()));
            }
            return *pstats_;
        }

        const char* word_type::utf8_c_str() { return text_.c_str(); }

        utf8_string word_type::utf8_text() const { return text_; }

        utf8_string& word_type::utf8_text() { return text_; }

        bool word_type::one_character() const { return 1 == text_.size(); }

        bool word_type::contains_newline() const { return unilib::contains_ascii("\r\n", text_); }

        bool word_type::empty(bool strip) const
        {
            if (!strip) return text_.empty();

            auto s(text_);
            maz::remove_whitespace(s);
            return s.empty();
        }

        size_t word_type::text_size() const { return text_.size(); }

        size_t word_type::letter_size() const { return detail.letters.size(); }

        std::string::value_type word_type::operator[](std::size_t idx) { return text_[idx]; }

        std::string::value_type word_type::operator[](letter_iterator idx)
        {
            return text_[text2letter(idx)];
        }

        char32_t word_type::uni_char(letter_iterator idx) { return unilib::utf8::first(idx->text); }

        word_type::letter_iterator
        word_type::erase(letter_iterator it_letter, size_t len, const char* tag)
        {
            return erase(text2letter(it_letter), len, it_letter, len, tag);
        }

        word_type::letter_iterator word_type::erase(
            size_t text_pos,
            size_t text_len,
            letter_iterator it_letter,
            size_t letter_len,
            const char* tag)
        {
            mark_update(tag);
            text_.erase(text_pos, text_len);
            return erase_letters(it_letter, letter_len);
        }

        word_type::letter_iterator word_type::erase_letters(letter_iterator it, size_t len)
        {
            auto it_end = it;
            std::advance(it_end, len);
            auto ret = detail.letters.erase(it, it_end);
            changed();
            return ret;
        }

        void word_type::update_from_letters()
        {
            changed();
            text_ = "";
            if (0 == letter_size())
            {
                bbox = bbox_type();
                return;
            }
            auto it = letter_begin();
            bbox = it->bbox;
            text_.append(it->text);
            ++it;
            for (; it != letter_end(); ++it)
            {
                bbox.merge(it->bbox);
                text_.append(it->text);
            }
        }

        size_t word_type::text2letter(word_type::letter_iterator it_end)
        {
            size_t pos = 0;
            const_letter_iterator it;
            for (it = letter_begin(); it != it_end; ++it)
            {
                while (pos < text_.size() && _isspace(text_[pos]) && !_isspace(it->text[0]))
                {
                    ++pos;
                }
                pos += it->text.size();
            }
            // can be equal when adding at the end
            assert(pos <= text_.size());
            return static_cast<int>(maz::maz_min(text_.size(), pos));
        }

        size_t word_type::text2letter(size_t letters_position)
        {
            return text2letter(get_letter_iterator(letters_position));
        }

        word_type::letter_iterator word_type::get_letter_iterator(size_t letters_position)
        {
            auto it = letter_begin();
            std::advance(it, letters_position);
            return it;
        }

        word_type::letter_iterator word_type::replace_letter(
            const unilib::utf8_string& c,
            word_type::letter_iterator it,
            int remove_letters_len,
            const char* tag)
        {
            mark_update(tag);

            size_t text_pos = text2letter(it);

            // remove letters
            if (0 < remove_letters_len)
            {
                // number of characters to remove does not need to be
                // number of letters, so let us find out the correct number
                size_t remove_characters_len = 0;
                auto it_l1 = it;
                for (int i = 0; i < remove_letters_len; ++i)
                {
                    remove_characters_len += it_l1->text.size();
                    ++it_l1;
                }
                text_.erase(text_pos, remove_characters_len);
                it = erase_letters(it, remove_letters_len);
            }

            // set letter characters - no big deal
            size_t letter_text_size = it->text.size();
            it->text = c;

            // we have to remove additional characters if the letter text contained
            // accented
            if (1 < letter_text_size)
            {
                text_.erase(text_pos, letter_text_size - 1);
            }

            if (!c.empty())
            {
                // set letter characters - big deal
                unilib::utf8_string::const_iterator it_c = c.begin();
                text_[text_pos] = *it_c;
                ++it_c;
                for (; it_c != c.end(); ++it_c)
                {
                    text_.insert(++text_pos, 1, *it_c);
                }
            }
            changed();
            return it;
        }

        void word_type::replace(
            utf8_string text,
            size_t text_pos,
            size_t remove_text_len,
            word_type::letter_iterator it,
            size_t remove_letters_len,
            const char* tag)
        {
            mark_update(tag);
            // first remove so we can set text on iterators
            if (0 < remove_letters_len)
            {
                it = erase_letters(it, remove_letters_len);
            }
            // set text - this will not work as expected when the text is > 1
            if (letter_end() != it)
            {
                it->text = text;
            }
            //
            if (remove_text_len != std::string::npos)
            {
                ++remove_text_len;
            }
            text_.replace(text_pos, remove_text_len, text);
            changed();
        }

        // The new contents are inserted before the character at position pos.
        void word_type::insert(
            utf8_string text,
            size_t text_pos,
            word_type::letter_iterator it_pos,
            letter_type& new_letter,
            const char* tag)
        {
            mark_update(tag);
            text_.insert(text_pos, text);
            detail.add_letter(it_pos, new_letter);
            changed();
        }

        void word_type::append(word_type& next, const char* glue)
        {
            assert(glue);

            bbox.merge(next.bbox);
            std::string sglue(glue);
            text_ += sglue + next.text_;
            detail.baseline.merge(next.detail.baseline);

            // add artifical letter if glue is not empty
            if (!sglue.empty() && 0 < letter_size() && 0 < next.letter_size())
            {
                auto newl = detail.letters.back();
                newl.bbox = {maz_min(bbox.xrb(), next.bbox.xlt()),
                             maz_min(bbox.ylt(), next.bbox.ylt()),
                             maz_max(bbox.xrb(), next.bbox.xlt()),
                             maz_max(bbox.yrb(), next.bbox.yrb())};
                newl.text = sglue;
                detail.letters.push_back(newl);
            }

            std::copy(next.letter_begin(), next.letter_end(), std::back_inserter(detail.letters));
            // not math. correct
            confidence = (confidence + next.confidence) / 2;
            //
            auto& ndetail = next.detail;
            std::copy(
                ndetail.warnings.begin(),
                ndetail.warnings.end(),
                std::back_inserter(detail.warnings));
            std::copy(ndetail.info.begin(), ndetail.info.end(), std::back_inserter(detail.info));
            changed();
        }

        void word_type::split(std::size_t pos, doc::words_type& words)
        {
            doc::ptr_word pw1(new doc::word_type);
            doc::ptr_word pw2(new doc::word_type);
            for (std::size_t i = 0; i < letter_size(); ++i)
            {
                auto& w = (i < pos) ? *pw1 : *pw2;
                auto itl = get_letter_iterator(i);
                w.detail.letters.push_back(*itl);
            }

            // copy mandatory information
            pw1->id = this->id;
            pw2->id = this->id * 10;
            pw1->confidence = confidence;
            pw2->confidence = confidence;
            pw1->orientation = orientation;
            pw2->orientation = orientation;
            pw1->type_ = type_;
            pw2->type_ = type_;

            // update bboxes
            pw1->update_from_letters();
            pw2->update_from_letters();

            // store it
            words.emplace_back(pw1);
            words.emplace_back(pw2);
            changed();
        }

        maz::json_dict word_type::to_json(output_detail dt) const
        {
            maz::json_dict d = make_json_dict();

            d["id"] = id;
            d["bbox"] = bbox2json(bbox);
            d["text"] = text_;
            d["confidence"] = round<2>(confidence);
            d["orientation"] = orientation;

            if (basic != dt)
            {
                d["flags"] = flags_;
                d["detail"] = detail.to_json(dt);
                // store if this word was expected
                std::string val = stringify(expected_);
                if (!val.empty())
                {
                    d["detail"]["expected"] = val;
                }
                val = stringify(type_);
                if (!val.empty())
                {
                    d["detail"]["type"] = val;
                }

                if (!palts_.empty())
                {
                    d["alts"] = make_json_arr();
                    for (auto palt : palts_)
                    {
                        d["alts"].push_back(palt->to_json(dt));
                    }
                }

                if (!ainfo.empty())
                {
                    d["arbitrary"] = ainfo;
                }
            }

            return d;
        }

        void word_type::relative_to(const bbox_type& n, int orientation)
        {
            // we set the orientation only to the main bbox so correctores
            // etc. do not get confused
            bbox.relative_to(n, orientation);
            relative_to_rest(to_int(n.xlt()), to_int(n.ylt()));
            changed();
        }

        void word_type::relative_to(int x, int y)
        {
            bbox.relative_to(x, y);
            relative_to_rest(x, y);
            changed();
        }

        void word_type::transpose(double dedeskew, double c_x, double c_y)
        {
            static constexpr double extend = 1.;
            // use big number for extend when the deskew is really big
            doc::bbox_type bounds = {0, 0, bbox.xrb() + 100 * extend, bbox.yrb() + 100 * extend};

            // transpose + extend
            bbox = maz::transpose(bbox, dedeskew, c_x, c_y);
            bbox = clip_to_document(bbox, extend, bounds);

            // transpose
            detail.baseline = maz::transpose(detail.baseline, dedeskew, c_x, c_y);
            for (auto& l : detail.letters)
            {
                l.bbox = maz::transpose(l.bbox, dedeskew, c_x, c_y);
            }
            // do not do it for alts - they should be ok
            changed();
        }

        void word_type::rotate(int orientation)
        {
            if (0 == letter_size()) return;

            auto b = detail.baseline;

            // word bbox is oriented correctly, always
            // use x position of the word bbox
            auto x = bbox.xlt();

            // rotate baseline
            switch (orientation)
            {
            case 1:
            {
                {
                    auto y = bbox.ylt() + (bbox.xrb() - b.xlt());
                    detail.baseline = bbox_type(x, y, x + std::abs(b.height()), y + b.width());
                }
                {
                    for (auto& l : detail.letters)
                    {
                        auto itx = x + (l.bbox.ylt() - bbox.ylt());
                        auto ity = bbox.ylt() + (bbox.xrb() - l.bbox.xrb());
                        l.bbox = bbox_type(itx, ity, itx + l.bbox.height(), ity + l.bbox.width());
                    }
                }
                break;
            }
            // upside down
            case 2:
            {
                {
                    auto y = b.yrb();
                    detail.baseline = bbox_type(x, y, bbox.xrb(), y + b.height());
                }
                {
                    for (auto& l : detail.letters)
                    {
                        auto itx = x + (bbox.xrb() - l.bbox.xlt());
                        auto ity = bbox.ylt() + (bbox.yrb() - l.bbox.yrb());
                        l.bbox = bbox_type(itx, ity, itx + l.bbox.height(), ity + l.bbox.width());
                    }
                }
                break;
            }
            case 3:
            {
                {
                    auto y = bbox.ylt() + (b.xlt() - bbox.xlt());
                    detail.baseline = bbox_type(x, y, x + std::abs(b.height()), y + b.width());
                }
                {
                    for (auto& l : detail.letters)
                    {
                        auto itx = x + (bbox.yrb() - l.bbox.yrb());
                        auto ity = bbox.ylt() + (l.bbox.xlt() - bbox.xlt());
                        l.bbox = bbox_type(itx, ity, itx + l.bbox.height(), ity + l.bbox.width());
                    }
                }
                break;
            }
            }
        }

        void word_type::reset_orientation()
        {
            switch (orientation)
            {
            case 1:
                rotate(3);
                break;
            case 2:
                rotate(2);
                break;
            case 3:
                rotate(1);
                break;
            default:
                break;
            }
            orientation = 0;
        }

        bbox_type word_type::real_letter_bbox(const_letter_iterator it)
        {
            // for case 1,3 only inter change them because they should be
            // inverse

            auto x = bbox.xlt();
            auto& l = *it;
            switch (orientation)
            {
            case 3:
            {
                auto itx = x + (l.bbox.ylt() - bbox.ylt());
                auto ity = bbox.ylt() + (l.bbox.xlt() - bbox.xlt());
                return bbox_type(itx, ity, itx + l.bbox.height(), ity + l.bbox.width());
            }
            // upside down
            case 2:
                // not implemented for now
                return it->bbox;
            case 1:
            {
                auto itx = x + (bbox.yrb() - l.bbox.yrb());
                auto ity = bbox.yrb() - (l.bbox.xrb() - bbox.xlt());
                return bbox_type(itx, ity, itx + l.bbox.height(), ity + l.bbox.width());
            }
            }

            return it->bbox;
        }

        void word_type::overwrite(word_type& w)
        {
            bbox = w.bbox;
            text_ = w.text_;
            detail.letters = w.detail.letters;
            detail.from_dict = w.detail.from_dict;
            detail.baseline = w.detail.baseline;
            orientation = w.orientation;
            confidence = maz_max(confidence, w.confidence);
            changed();
        }

        void word_type::swap_alt(ptr_word pw, const char* tag)
        {
            for (auto palt : palts_)
            {
                if (palt != pw) continue;

                auto& w = *palt;
                std::swap(bbox, w.bbox);
                std::swap(text_, w.text_);
                std::swap(detail, w.detail);
                // we swapped it because of a reason - use max of confidences
                confidence = maz_max(confidence, w.confidence);
                changed();
                mark_update(tag);
                break;
            }
        }

        void word_type::relative_to_rest(int x, int y)
        {
            // word["detail"]["baseline"]
            detail.baseline.relative_to(x, y);
            for (auto it = letter_begin(); it != letter_end(); ++it)
            {
                it->bbox.relative_to(x, y);
            }
            for (auto p : palts_)
            {
                p->relative_to(x, y);
            }
        }

        void word_type::mark_update(const char* tag)
        {
            if (tag == nullptr) return;
            detail.updates.push_back(tag);
        }

        void word_type::mark_warning(const char* tag, int decrease_conf)
        {
            if (tag == nullptr) return;
            detail.warnings.push_back(tag);
            // do not change if too low
            static constexpr int min_valid_conf = 40;
            if (min_valid_conf < confidence)
            {
                confidence = maz_max(0., confidence - decrease_conf);
            }
        }

        bool word_type::has_warning(const char* tag) const
        {
            if (tag == nullptr)
            {
                return !detail.warnings.empty() || type() != NORMAL;
            }
            for (auto& warning : detail.warnings)
            {
                if (warning == tag) return true;
            }
            return false;
        }

        bool word_type::has_update(const char* tag)
        {
            if (tag == nullptr)
            {
                return !detail.updates.empty();
            }
            for (auto& update : detail.updates)
            {
                if (update == tag) return true;
            }
            return false;
        }

        void word_type::info(const char* tag) { detail.info.push_back(tag); }

        void word_type::changed() { pstats_.reset(); }

        //
        // line_type
        //

        line_type::line_type(const maz::json_dict& item)
        {
            bbox = json2bbox(item["bbox"], bbox);

            for (const auto& it : item["words"])
            {
                push_back(ptr_word(new word_type(it)));
            }
        }

        utf8_string line_type::utf8_text(int max_x) const
        {
            utf8_string s;
            for (auto it = words_.begin(); it != words_.end(); ++it)
            {
                const auto& w = **it;
                if (0 < max_x && max_x < w.bbox.xlt()) break;
                s += w.utf8_text();
                s += " ";
            }
            rstrip(s);
            return s;
        }

        utf8_string line_type::utf8_alt_text(int max_x, doc::words_type* pwords)
        {
            static constexpr double conf_ratio = 0.9;
            utf8_string s;
            for (auto pw : words_)
            {
                if (0 < max_x && max_x < pw->bbox.xlt()) break;
                auto alts = pw->alts();
                if (!alts.empty())
                {
                    auto bestpalt = *std::max_element(
                        alts.begin(), alts.end(), [](doc::ptr_word pw1, doc::ptr_word pw2) {
                            return pw1->confidence < pw2->confidence;
                        });
                    if (bestpalt->confidence > pw->confidence * conf_ratio)
                    {
                        s += bestpalt->utf8_text() + " ";
                        if (pwords) pwords->push_back(bestpalt);
                        continue;
                    }
                }

                // default case
                s += pw->utf8_text() + " ";
                if (pwords) pwords->push_back(pw);
            }
            rstrip(s);
            return s;
        }

        line_statistics& line_type::statistics()
        {
            if (!pstats_)
            {
                pstats_ = shared_ptr<line_statistics>(new line_statistics(begin(), end()));
            }
            return *pstats_;
        }

        void line_type::pop_back()
        {
            words_.pop_back();
            changed();
        }

        void line_type::pop_front()
        {
            words_.pop_front();
            changed();
        }

        void line_type::push_back(const ptr_word& pword, bool no_change)
        {
            assert(std::none_of(
                words_.begin(), words_.end(), [pword](auto pw) { return pw == pword; }));

            words_.push_back(pword);
            if (!no_change) changed();
        }

        void line_type::push_front(const ptr_word& pword, bool no_change)
        {
            words_.push_front(pword);
            if (!no_change) changed();
        }

        void line_type::add(const ptr_word& pword)
        {
            assert(std::none_of(
                words_.begin(), words_.end(), [pword](auto pw) { return pw == pword; }));

            words_.push_back(pword);
            doc::sort_x(words_, [](const ptr_word& pw) { return pw->bbox; });
            changed();
        }

        line_type::iterator line_type::insert(iterator it, const ptr_word& pword)
        {
            assert(std::none_of(
                words_.begin(), words_.end(), [pword](auto pw) { return pw == pword; }));

            auto ret = words_.insert(it, pword);
            changed();
            return ret;
        }

        void line_type::clear()
        {
            words_.clear();
            changed();
        }

        line_type::iterator line_type::erase(line_type::iterator it_word)
        {
            auto it = words_.erase(it_word);
            changed();
            return it;
        }

        bool line_type::erase(ptr_word pword)
        {
            for (iterator it_word = begin(); it_word != end(); ++it_word)
            {
                if (*it_word == pword)
                {
                    erase(it_word);
                    return true;
                }
            }
            return false;
        }

        void line_type::update_bbox()
        {
            bbox = uber_bbox(words_.begin(), words_.end(), [](ptr_word pw) { return pw->bbox; });
        }

        doc::ptr_line line_type::split(const_iterator it)
        {
            doc::ptr_line pl(new line_type);
            while (!words_.empty())
            {
                doc::ptr_word pw = words_.back();
                // cannot compare after `pop_back`
                bool finish = (pw == *it);
                words_.pop_back();
                pl->add(pw);
                if (finish) break;
            }
            changed();

            return pl;
        }

        void line_type::merge(line_type& l)
        {
            for (auto& w_to_insert : l)
            {
                bool found = false;
                for (auto it = begin(); it != end(); ++it)
                {
                    if ((*it)->bbox.xlt() > w_to_insert->bbox.xlt())
                    {
                        insert(it, w_to_insert);
                        found = true;
                        break;
                    }
                }
                if (!found) push_back(w_to_insert);
            }
            l.clear();
        }

        maz::json_dict line_type::to_json(output_detail dt) const
        {
            maz::json_dict d = make_json_dict();

            if (empty()) return d;

            d["bbox"] = bbox2json(bbox);
            for (ptr_word pword : words_)
            {
                d["words"].emplace_back(pword->to_json(dt));
            }

            return d;
        }

        void line_type::changed()
        {
            update_bbox();
            pstats_.reset();
        }

        words_type line_type::at(const bbox_type& bbox, double min_overlap) const
        {
            words_type ret;
            for (ptr_word pw : words_)
            {
                if (min_overlap > bbox.intersects(pw->bbox)) continue;
                ret.push_back(pw);
            }
            return ret;
        }

        // ==================================

        void visual_elements::add(ptr_value pb) { elements_.push_back(pb); }

        bool visual_elements::has(const std::string& key)
        {
            for (auto p : elements_)
            {
                if (p->key == key) return true;
            }
            return false;
        }

        visual_elements::ptr_value visual_elements::get(const std::string& key)
        {
            for (auto p : elements_)
            {
                if (p->key == key) return p;
            }
            throw std::runtime_error("no such element");
        }

        void visual_elements::relative_to(int x, int y)
        {
            for (auto p : elements_)
            {
                p->relative_to(x, y);
            }
        }

        maz::json_dict visual_elements::to_json() const
        {
            maz::json_dict d = make_json_dict();
            for (auto p : elements_)
                p->json(d);
            return d;
        }

        //
        //

        page_statistics& page_type::statistics(bool recreate)
        {
            if (!pstats_)
            {
                pstats_ = shared_ptr<page_statistics>(new page_statistics);
                recreate = true;
            }
            // some statistics are *not* retrievable from the current
            // data model - e.g., confidences. Therefore, once statistics are
            // set in `statistics(const page_statistics&)` method below, do not
            // overwrite information that cannot be parsed from data
            if (recreate)
            {
                pstats_->init(lines.begin(), lines.end());
            }
            return *pstats_;
        }

        void page_type::statistics(const page_statistics& ps)
        {
            pstats_ = shared_ptr<page_statistics>(new page_statistics(ps));
        }

        size_t page_type::size() const
        {
            size_t cnt = 0;
            for_each_word([&cnt](doc::ptr_word pw, doc::line_type&) { ++cnt; });
            return cnt;
        }

        void page_type::relative_to(int x, int y)
        {
            for (ptr_line pline : lines)
            {
                line_type& line = *pline;
                line.bbox.relative_to(x, y);
                for (ptr_word pword : line)
                {
                    pword->relative_to(x, y);
                }
            }
            ia.relative_to(x, y);
        }

        maz::json_dict page_type::to_json(output_detail dt) const
        {
            maz::json_dict d = make_json_dict();

            d["bbox"] = bbox2json(bbox);
            d["image_clip_bbox"] = bbox2json(image_clip_bbox);
            d["info"] = info;
            d["text"] = text;
            d["image"] = images;
            d["layout"] = layout;
            d["confidence"] = confidence;
            d["scale"] = scale;
            d["deskew"] = deskew;
            d["rotation"] = rotation;
            d["ia"] = ia.to_json();
            d["lines"] = maz::make_json_arr();

            for (auto line : lines)
            {
                d["lines"].emplace_back(line->to_json(dt));
            }

            return d;
        }

        utf8_string str(page_type& p)
        {
            utf8_string s;
            for (auto pl : p.lines)
            {
                std::ostringstream oss;
                oss << "[" << std::setfill('0') << std::setw(2) << pl->size()
                    << "]: " << pl->utf8_text() << std::endl;
                s += oss.str();
            }
            return s;
        }

        // ================================================

        words2d::words2d(const bbox_type& bbox, const doc::lines_type& lines, int step)
            : step_(step)
        {
            init(bbox);
            for (auto& l : lines)
            {
                for (auto pw : *l)
                {
                    if (0. < bbox.intersects(pw->bbox))
                    {
                        add(pw);
                    }
                }
            }
        }

        words2d::words2d(
            const bbox_type& bbox,
            doc::words_type::iterator s,
            doc::words_type::iterator e,
            int step)
            : step_(step)
        {
            init(bbox);
            while (s != e)
            {
                if (0. < bbox.intersects((*s)->bbox))
                {
                    add(*s);
                }
                ++s;
            }
        }

        void words2d::init(const bbox_type& b)
        {
            // initialise the matrix
            int max_h = static_cast<int>(b.yrb() / step_) + 1;
            int max_w = static_cast<int>(b.xrb() / step_) + 1;
            m_.resize(max_w);
            for (auto& r : m_)
            {
                r.resize(max_h);
            }
        }

        void words2d::add(doc::ptr_word pw)
        {
            auto& b = pw->bbox;
            for (auto x = b.xlt(); x < b.xrb(); x += step_)
            {
                size_t x_idx = static_cast<int>(x / step_);
                for (auto y = b.ylt(); y < b.yrb(); y += step_)
                {
                    size_t y_idx = static_cast<int>(y / step_);
                    if (x_idx >= m_.size() || y_idx >= m_[x_idx].size()) continue;

                    auto& p = m_[x_idx][y_idx];
                    if (p == nullptr)
                    {
                        p = pw;
                        break;
                    }
                }
            }
        }

        utf8_string words2d::str() const
        {
            utf8_string s;

            for (size_t i = 0; i < m_.size(); ++i)
            {
                auto& col = m_[i];
                // get count
                std::ostringstream oss;
                oss << "[" << col.size() << "]: ";
                s += oss.str();
                // get text
                for (auto pw : col)
                {
                    if (pw == nullptr) continue;
                    s += pw->utf8_text() + " ";
                }
                s += "\n";
            }

            return s;
        }

        void debug_words(doc::words2d& words)
        {
            words.columns([](size_t i, doc::words2d::column_words& col) {
                std::string s;
                for (auto pw : col)
                {
                    if (pw == nullptr) continue;
                    s += pw->utf8_text() + " ";
                }
                if (!s.empty()) slogger().warn(s);
            });
        }

        std::string word_wrap_glue(const doc::word_type& w1, const doc::word_type& w2)
        {
            if (w1.empty() || w2.empty()) return "";

            auto s1 = w1.utf8_text();
            auto s2 = w2.utf8_text();

            // empty letters can happen in unittests
            char last_ch = (0 < w1.letter_size()) ? w1.letters().back().text[0] : s1.back();
            char first_ch = (0 < w2.letter_size()) ? w2.letters().front().text[0] : s2.front();
            return word_wrap_glue(last_ch, first_ch);
        }

        std::string word_wrap_glue(char last_ch, char first_ch)
        {
            if (unilib::is_in(last_ch, "-/0123456789")) return "";
            if (unilib::is_in(first_ch, "-/")) return "";

            return " ";
        }

    } // namespace doc
} // namespace maz
