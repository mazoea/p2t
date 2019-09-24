/*
 *  Mazoea s.r.o.
 *  @author jm
 */

#define WIN32_LEAN_AND_MEAN

#ifdef WIN32
#ifdef DEBUG
//    #include <vld.h>
#endif
#endif

#include <stdlib.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <stdio.h>
// do not use abs (makes solaris gcc unhappy)
#include <math.h>

// xpdf
#include "goo/GString.h"
#include "goo/gmem.h"
#include "xpdf/Error.h"
#include "xpdf/GlobalParams.h"
#include "xpdf/Object.h"
#include "xpdf/PDFDoc.h"
#include "xpdf/TextOutputDev.h"
#include "xpdf/UnicodeMap.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "io-document/io-document.h"
#include "maz-utils/coords.h"
#include "maz-utils/fs.h"
#include "maz-utils/params.h"

#pragma message("Using freetype " _FREETYPELIB_VERSION_)

// include after xpdf to get also external lib version info
#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
#include <ft2build.h>
#include FT_FREETYPE_H
#endif
#include "os/version.h"

#ifndef _HAS_CPP0X
#pragma message("Using auto_ptr instead of unique_ptr!")
// not defined unique_ptr - ouch
#define unique_ptr auto_ptr
#endif

using namespace std;
using namespace maz;

//==============================
namespace {
    //==============================

    // not MT friendly
    static slogger logger_;

    //==============================
    // specifics
    //==============================

    string get_freetype_version()
    {
        int maj = 0, min = 0, pat = 0;
#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
        FT_Library lib;
        FT_Init_FreeType(&lib);
        FT_Library_Version(lib, &maj, &min, &pat);
#endif
        ostringstream oss;
        oss << "(" << maj << "." << min << "." << pat << ")";
        return oss.str();
    }

    //==============================
    // basic helpers
    //==============================

    template <typename T, typename U = double>
    doc::bbox_type get_bbox(T ftor, U offset_x = U(), U offset_y = U())
    {
        double x1, y1, x2, y2;
        ftor->getBBox(&x1, &y1, &x2, &y2);
        return doc::bbox_type(x1 - offset_x, y1 - offset_y, x2 - offset_x, y2 - offset_y);
    }

    doc::bbox_type get_bbox(PDFRectangle* rec, double ratio)
    {
        return doc::bbox_type(rec->x1 * ratio, rec->y1 * ratio, rec->x2 * ratio, rec->y2 * ratio);
    }

    doc::bbox_type rotate_bbox(const doc::bbox_type& bbox, int rotation)
    {
        // just one circle
        rotation = rotation % 360;
        if (rotation == 90 || rotation == 270)
        {
            return doc::bbox_type(bbox.ylt_, bbox.xlt_, bbox.yrb_, bbox.xrb_);
        }
        return bbox;
    }

    doc::bbox_type get_bbox(TextLineFrag* tlf)
    {
        return doc::bbox_type(tlf->xMin, tlf->yMin, tlf->xMax, tlf->yMax);
    }

    bool is_on_line(const doc::line_type& line, const doc::bbox_type& bbox)
    {
        static const int ACCEPTABLE_DIFF = 1;
        if (0 < line.size() && ACCEPTABLE_DIFF > fabs(line.bbox.ylt() - bbox.ylt()) &&
            ACCEPTABLE_DIFF > fabs(line.bbox.yrb() - bbox.yrb()))
        {
            return true;
        }
        return false;
    }

    doc::line_type& find_best_line(
        doc::document& doc,
        doc::lines_type& lines,
        const doc::bbox_type& line_bbox,
        const doc::bbox_type& words_bbox)
    {
        // empty lines
        if (0 == lines.back()->size())
        {
            lines.back()->bbox = line_bbox;
            return *(lines.back());

        } else
        {
            // if the frag is not like the last line, find the best one
            if (!is_on_line(*(lines.back()), words_bbox))
            {
                for (doc::lines_type::iterator it = lines.begin(); it != lines.end(); ++it)
                {
                    doc::line_type& line = **it;
                    if (is_on_line(line, words_bbox))
                    {
                        line.bbox.merge(line_bbox);
                        return line;
                    }
                }
                // we have not found the line (even if xpdf thinks the frag
                // is on the same line as another one) and so return a new
                // empty line
                doc.end_line();
                lines.back()->bbox = line_bbox;
                return *(lines.back());
            }

            // it is on the last line so merge the bboxes
            lines.back()->bbox.merge(line_bbox);
            return *(lines.back());
        }
    }

    //==============================
    // pdf library wrapper
    //==============================

    struct pdf_library_wrapper
    {

        static vector<string> errors_;
        bool ok_;

        pdf_library_wrapper(const string& encoding, const string& font_dir) : ok_(false)
        {
            //
            setErrorCallback(&pdf_library_wrapper::xpdf_err_clb, NULL);
            globalParams = new GlobalParams(NULL);
            globalParams->setTextEncoding(const_cast<char*>(encoding.c_str()));
            globalParams->setTextPageBreaks(gTrue);
            globalParams->setErrQuiet(gTrue);
            globalParams->setupBaseFonts(const_cast<char*>(font_dir.c_str()));
            globalParams->setMapUnknownCharNames(gTrue);
            globalParams->setMapNumericCharNames(gTrue);
            globalParams->setDrawAnnotations(gFalse);
            int white_chars[] = {0xa0};
            globalParams->setExtendedSpace(white_chars, sizeof(white_chars) / sizeof(int));

            ok_ = true;
        }
        ~pdf_library_wrapper()
        {
            delete globalParams;
            globalParams = NULL;
        }

        static void xpdf_err_clb(void* data, ErrorCategory ctg, int pos, char* msg)
        {
            if (errConfig == ctg)
            {
                // ignore
                return;
            }
            ostringstream oss;
            oss << "[" << pos << "] " << msg;
            errors_.push_back(oss.str());
        }

    }; // class pdf_lib

    vector<string> pdf_library_wrapper::errors_;

    class pager_type
    {
      public:
        static const std::string MAGIC_STRING;

      private:
        bool initialised_;
        std::string eop_;

      public:
        pager_type() : initialised_(false)
        {
            char eop_arr[8] = {};
            UnicodeMap* uMap = globalParams->getTextEncoding();
            if (uMap)
            {
                int len = uMap->mapUnicode(0x0c, eop_arr, sizeof(eop_arr));
                eop_ = std::string(eop_arr, len);
                initialised_ = true;
            }
        }

        bool end_of_page(const std::string& str) const { return str == eop_; }
    };
    const std::string
        pager_type::MAGIC_STRING("\n\npdf_to_text 8EA7C044DCEE61906958493ED7A42EE6\n\n");

    //==============================
    // xpdf specific
    //==============================

    void append_text_empty(void*, const char*, int) {}

    // MT unfriendly
    ofstream g_off;
    doc::document* g_doc = NULL;

    void append_text(void*, const char* ctext, int len)
    {
        assert(g_doc);
        static pager_type pager;
        string page_text = string(ctext, len);

        if (pager.end_of_page(page_text))
        {
            page_text = pager_type::MAGIC_STRING;
        }

        g_doc->append_text_utf8(page_text);
        if (g_off.is_open())
        {
            g_off << page_text;
        }
    }

    class images_element : public doc::visual_element
    {
      private:
        doc::bboxes_type bboxes_;

      public:
        images_element(const std::string& key, const doc::bboxes_type& bboxes)
            : doc::visual_element(key), bboxes_(bboxes)
        {
        }

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

    //==============================
    // output
    //==============================

    struct output_listener : frag_listener
    {

        // constants
        static constexpr int PDF_SPEC_DPI = 72;

      protected:
        //
        doc::document& doc_;

        // vars
        PDFDoc& pdf_raw;
        double dpi_ratio{0.};

        // trimbox
        bbox_type trim_bb_;

        doc::bboxes_type img_bboxes_;
        bool true_types_{true};

      public:
        //
        // ctor
        //

        output_listener(env_type& env, PDFDoc& pdfdoc, doc::document& doc)
            : doc_(doc), pdf_raw(pdfdoc)
        {
            // atoi - dirty but correct
            dpi_ratio = atof(env["dpi"].c_str()) / PDF_SPEC_DPI;

            doc_.expected_dpi(atoi(env["dpi"].c_str()), atoi(env["dpi"].c_str()));
            GString* metadata = pdf_raw.readMetadata();
            doc_.metadata(metadata ? metadata->getCString() : "");
            delete metadata;

            doc_.info("pdfversion", pdf_raw.getPDFVersion());
            doc_.info("encrypted", pdf_raw.isEncrypted());
            doc_.info("linearized", pdf_raw.isLinearized());
            doc_.info("embedded_files", pdf_raw.getNumEmbeddedFiles());
            doc_.info("page_count", pdf_raw.getNumPages());
        }

        //
        //
        //
        virtual void start_page(size_t spos, GfxState* state)
        {
            doc_.start_page();
            img_bboxes_.clear();
            int pos = static_cast<int>(spos);

            // rotation
            int pdf_rotation = pdf_raw.getPageRotate(pos);
            doc_.page_info("pdf_rotation", pdf_rotation);
            doc_.page_rotation(0);
            // use trimbox as bbox
            PDFRectangle* rec = pdf_raw.getCatalog()->getPage(pos)->getTrimBox();
            trim_bb_ = get_bbox(rec, dpi_ratio);
            // bbox must be rotated because pdf does store it without rotation
            doc_.page_bbox(rotate_bbox(trim_bb_, pdf_rotation));

            doc_.page_scale(1.0);
            doc_.page_skew(0.);

            // pdf specifics
            rec = pdf_raw.getCatalog()->getPage(pos)->getMediaBox();
            doc_.page_info("mediabox", get_bbox(rec, dpi_ratio));
            rec = pdf_raw.getCatalog()->getPage(pos)->getCropBox();
            doc_.page_info("cropbox", get_bbox(rec, dpi_ratio));
            rec = pdf_raw.getCatalog()->getPage(pos)->getTrimBox();
            doc_.page_info("trimbox", get_bbox(rec, dpi_ratio));
            rec = pdf_raw.getCatalog()->getPage(pos)->getBleedBox();
            doc_.page_info("bleedbox", get_bbox(rec, dpi_ratio));
        }

        virtual void end_page(size_t cnt)
        {
            typedef doc::visual_elements::ptr_value visual;
            doc_.page_ia().add(visual(new images_element("images", img_bboxes_)));
            img_bboxes_.clear();
            doc_.end_page(100);
            if (true_types_)
            {
                doc_.page_info("vectored", true);
            }
        }

        virtual void end_line(size_t cnt) { doc_.end_line(); }

        virtual void space(size_t cnt) { ; }

        //
        // which one is called is decided upon definition of rawOrder etc.
        //

        virtual void line(TextLineFrag* frag, const char* str, size_t str_len)
        {
            assert(frag);
            assert(str);
            if (1 > str_len) return;

            // if we just created new line, put the bboxes there
            doc::line_type& line = find_best_line(
                doc_, doc_.current_page().lines, get_bbox(frag->line), get_bbox(frag));

            typedef TextWord* buggered_iterator_type;
            buggered_iterator_type it = frag->line->getWords();
            buggered_iterator_type end = frag->line->getLastWord();

            while (it)
            {
                static int word_cnt = 0;

                GString* gtext = it->getText();
                const char* ctext = gtext->getCString();
                doc::ptr_word pword(new doc::word_type(
                    ctext,
                    // the coordinate system should be in target device
                    // meaning the `pdf_to_png` will match these coordinates
                    get_bbox(it),
                    100));
                delete gtext;

                bool is_vectored = false;
                // fill out details
                pword->id = word_cnt++;
                bbox_type baseline_bb = {pword->bbox.xlt(),
                                         it->getBaseline(),
                                         pword->bbox.xrb(),
                                         it->getBaseline() + 1.};
                pword->detail.baseline = baseline_bb;
                pword->detail.underline = (0 != it->isUnderlined());
                pword->detail.font_size = to_int(it->getFontSize());
                pword->detail.from_dict = false;
                pword->detail.numeric = false;
                if (it->getFontInfo())
                {
                    TextFontInfo* tfi = it->getFontInfo();
                    if (it->getFontName())
                    {
                        pword->detail.font = it->getFontName()->getCString();
                    }
                    pword->detail.bold = (0 != tfi->isBold());
                    pword->detail.italics = (0 != tfi->isItalic());
                    pword->detail.monospace = (0 != tfi->isFixedWidth());
                    pword->detail.serif = (0 != tfi->isSerif());
                    // todo letters
                    is_vectored =
                        (tfi->type() == GfxFontType::fontTrueType ||
                         tfi->type() == GfxFontType::fontTrueTypeOT);
                }
                if (!is_vectored) true_types_ = false;

                doc_.append_word(line, pword);
                if (it == end) break;

                it = it->getNext();
            }
        }

        virtual void line(TextWord*, const char*, size_t) { assert(!"not implemented"); }
        virtual void line(TextLine*, const char*, size_t) { assert(!"not implemented"); }

        virtual void image(int x, int y, int w, int h, bool inlineImg)
        {
            img_bboxes_.push_back(doc::bbox_type(x, y, x + w, y + h));
        };

    }; // struct outputter

    //==============================
    // what to do with a page
    //==============================

    struct pdf_extractor
    {
        // ouch, have to stick with prev to C++11x versions :(
        std::auto_ptr<output_listener> outputter_ptr;

        std::auto_ptr<TextOutputDev> textout_ptr_;
        env_type& env_;
        doc::document& document_;

        pdf_extractor(env_type& env, PDFDoc& pdfdoc, doc::document& document)
            : textout_ptr_(new TextOutputDev(&append_text_empty, &(std::cout), gTrue, 0, gFalse)),
              env_(env), document_(document)
        {
            if (!(textout_ptr_->isOk()))
            {
                logger_.error("PDF text is not valid, trying anyway.");
            }

            // type to use
            //
            if ("json" == env_["type"])
            {
                outputter_ptr.reset(new output_listener(env_, pdfdoc, document));
                // inform the output device we want more info than usual
                textout_ptr_->set_listener(outputter_ptr.get());

            } else if ("metadata" == env_["type"])
            {
                outputter_ptr.reset(new output_listener(env_, pdfdoc, document));
                // do not inform, collect only metadata in constructor
            }

            // output if we specify output-file param or type is text
            //
            if ("text" == env_["type"] || "" != env_["output-file"])
            {
                g_doc = &document;
                textout_ptr_->set_output_fnc(&append_text);
                // open global file
                if ("" != env_["output-file"])
                {
                    g_off.open(
                        env_["output-file"].c_str(),
                        fstream::out | fstream::binary | fstream::trunc);
                }
            }
        }

        ~pdf_extractor() { g_off.close(); }

        void operator()(PDFDoc& pdf, int first_page, int last_page = -1)
        {
            double dpi = 0.0;
            value(env_["dpi"], dpi);
            int rotate = 0;
            value(env_["rotate"], rotate);

            if (-1 == last_page)
            {
                pdf.displayPage(
                    textout_ptr_.get(),
                    first_page,
                    dpi,
                    dpi,
                    rotate,
                    gTrue,   // useMediaBox
                    gTrue,   // crop
                    gFalse); // printing
            } else
            {
                pdf.displayPages(
                    textout_ptr_.get(),
                    first_page,
                    last_page,
                    dpi,
                    dpi,
                    rotate,
                    gTrue,
                    gTrue,
                    gFalse);
            }
        }
    };

    //==============================
    // helpers
    //==============================

    string help()
    {
        return string("Pdf to text utility v" _PROG_VERSION_ PR_INFO
                      " utility extracts text/json info from a pdf file.\n"
                      "Arguments:\n"
                      "  --help        produces this help message\n"
                      "  --version     shows full version info\n"
                      "  --file        input file (*.pdf)\n"
                      "  --page        comma separated page(s) to extract e.g., =\"1,3\" (starting "
                      "from 1)\n"
                      "  --encoding    encoding to use (default is utf-8)\n"
                      "  --font-dir    directory used for specific fonts (e.g., *.pfb)\n"
                      "  --dpi         dpi used for output device (default is 300)\n"
                      "  --type        output type - json/text/metadata (default is json)\n"
                      "  --output-file path to output file\n"
                      "\n"
                      "Examples:\n"
                      "  pdf_to_text --help\n"
                      "  pdf_to_text --file=input.pdf --font=dir=\".\"\n"
                      "\n"
                      "Note: pages are separated by:"
                      "\n") +
               pager_type::MAGIC_STRING;
    }

    string version_str()
    {
        string _dynamic_version(get_freetype_version());
        return version() + string("[" _LIBS_VERSION_ " ") + _dynamic_version + string("]");
    }

    //==============================
    // helpers
    //==============================

    env_type get_options(char** argv, size_t argc)
    {
        // remove the first exe path
        ++argv;
        --argc;

        env_type args;

        typedef std::vector<std::string> args_type;
        args_type args_arr(argv, argv + argc);

        try
        {
            for (args_type::iterator it = args_arr.begin(); it != args_arr.end(); ++it)
            {
                if (*it == "--help")
                {
                    args["help"] = "";
                    break;
                } else if (*it == "--version")
                {
                    args["version"] = "";
                    break;
                } else if (parse_option(args, *it, "file"))
                    continue;
                else if (parse_option(args, *it, "page"))
                    continue;
                else if (parse_option(args, *it, "encoding"))
                    continue;
                else if (parse_option(args, *it, "font-dir"))
                    continue;
                else if (parse_option(args, *it, "dpi"))
                    continue;
                else if (parse_option(args, *it, "type"))
                    continue;
                else if (parse_option(args, *it, "output-file"))
                    continue;
                else if (parse_option(args, *it, "out"))
                    continue;
            }

        } catch (exception&)
        {
            // invalid param
        }

        return args;

    } // get_options

    //==============================
} // namespace
//==============================

//==============================
// constants
//==============================

namespace {

    // default values
    const string DEFAULT_ENCODING("UTF-8");
    const string DEFAULT_FONT_DIR(".");
    const string DEFAULT_DPI("300");
    const string NO_ROTATE("0");
    const string TYPE("json");

    bool valid_page_num(int page_num) { return page_num <= 0; }

    void output_json(env_type& env, doc::document& doc)
    {
        if ("cout" == env["out"])
        {
            std::cout << doc.to_json() << std::flush;
        } else if (!env["out"].empty())
        {
            maz::fs::file::write(env["out"], doc.to_json(), 2);
        }
    }

} // namespace

//==============================
// main
//==============================

int main(int argc, char** argv)
{
    int ret_code = maz::OK;

    // parameter parsing
    env_type env = get_options(argv, argc);

    if (env.end() == env.find("encoding")) env["encoding"] = DEFAULT_ENCODING;
    if (env.end() == env.find("font-dir")) env["font-dir"] = DEFAULT_FONT_DIR;
    if (env.end() == env.find("dpi")) env["dpi"] = DEFAULT_DPI;
    if (env.end() == env.find("rotate")) env["rotate"] = NO_ROTATE;
    if (env.end() == env.find("type")) env["type"] = TYPE;
    if (env.end() == env.find("output-file")) env["output-file"] = "";
    if (env.end() == env.find("out")) env["out"] = "cout";

    // help
    if (1 == argc || env.end() != env.find("help"))
    {
        std::cout << help();
        return maz::OK;
    }

    // version
    if (1 == argc || env.end() != env.find("version"))
    {
        std::cout << version_str() << std::endl;
        return maz::OK;
    }

    // check sanity
    if (env.end() == env.find("file"))
    {
        std::cout << help();
        logger_.error("Missing --file option.");
        return maz::INVALID_PARAM;
    }

    string file = env["file"];
    string encoding = env["encoding"];
    string font_dir = env["font-dir"];

    typedef vector<int> pages_type;
    pages_type pages;
    if (env.end() != env.find("page"))
    {
        value(env["page"], pages);
        size_t len_orig = pages.size();
        pages.erase(std::remove_if(pages.begin(), pages.end(), valid_page_num), pages.end());
        if (len_orig != pages.size())
        {
            ret_code = maz::INVALID_PARAM;
        }
    }

    //
    // init lib, textify
    //

    try
    {
        //
        doc::document doc(env, file);
        doc.info_command_line(argc, argv);
        doc.info_env();

        // pdf lib init & work
        pdf_library_wrapper pdf_lib(encoding, font_dir);
        if (!pdf_lib.ok_)
        {
            return maz::INVALID_PARAM;
        }

        // open pdf
        std::unique_ptr<PDFDoc> pdf(new PDFDoc(new GString(file.c_str())));
        if (!pdf->isOk())
        {
            throw std::runtime_error(std::string("PDF is not valid, stopping."));
        }

        bool should_output_json = "text" != env["type"];
        pdf_extractor extract(env, *pdf, doc);

        try
        {
            {
                TIMER_PROBE_THIS_FNC(doc);
                if (pages.empty())
                {
                    int first_page = 1;
                    int last_page = pdf->getNumPages();
                    extract(*pdf, first_page, last_page);

                } else
                {

                    // do it for selected pages
                    for (pages_type::const_iterator it = pages.begin(); it != pages.end(); ++it)
                    {
                        if (*it > pdf->getNumPages())
                        {
                            logger_.error("Invalid page number! ");
                            continue;
                        }

                        extract(*pdf, *it);
                    }
                }
            }

            // if only text should be written do not output json
            if (should_output_json)
            {
                doc.populate();
                output_json(env, doc);
            }

        } catch (std::exception& e)
        {
            if (should_output_json)
            {
                doc.exception(e.what());
                output_json(env, doc);
            } else
            {
                throw;
            }
            return maz::EXCEPTION;
        }

    } catch (std::exception& e)
    {
        logger_.error("exception - ", e.what());
        return maz::EXCEPTION;
    }

    return ret_code;
}
