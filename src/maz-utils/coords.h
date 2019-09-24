/*
 *  Mazoea s.r.o.
 *  @author jm
 */

#pragma once

#include "maz-utils/utils.h"
#include <algorithm>
#include <list>
#include <sstream>

namespace maz {
    //
    // coords
    //
    template <typename MapType, typename T> void bbox_x1y1x2y2(MapType& val, T x1, T y1, T x2, T y2)
    {
        val["x1"] = x1;
        val["y1"] = y1;
        val["x2"] = x2;
        val["y2"] = y2;
    }

    template <typename MapType, typename T> MapType bbox_x1y1x2y2(T x1, T y1, T x2, T y2)
    {
        MapType val;
        val["x1"] = x1;
        val["y1"] = y1;
        val["x2"] = x2;
        val["y2"] = y2;
        return val;
    }

    template <typename MapType, typename T> void bbox_xywh(MapType& val, T x, T y, T w, T h)
    {
        val["x"] = x;
        val["y"] = y;
        val["w"] = w;
        val["h"] = h;
    }

    template <typename MapType, typename T> MapType bbox_xywh(T x, T y, T w, T h)
    {
        MapType val;
        val["x"] = x;
        val["y"] = y;
        val["w"] = w;
        val["h"] = h;
        return val;
    }

    /** Returns persect [0., 100.] of intersection. */
    template <typename T> double intersects(T l1, T r1, T l2, T r2)
    {
        if (r1 < l2 || r2 < l1) return 0.;

        T l = maz_max(l1, l2);
        T r = maz_min(r1, r2);
        double ret = (100. * (r - l)) / static_cast<double>(maz_min(r2 - l2, r1 - l1));
        return ret;
    }

    //========================================
    // rect_type
    //========================================

    /**
     * 0,0 = left top => yrb_ > ylt_
     */
    template <typename T, typename ArbitratyType = void*> struct rect_type
    {
        typedef T value_type;

        T xlt_, ylt_, xrb_, yrb_, ymid_, h_, w_;
        ArbitratyType ptr_;

        rect_type() : xlt_(T()), ylt_(T()), xrb_(T()), yrb_(T()), ptr_(nullptr) { update(); }

        rect_type(T xlt, T ylt, T xrb, T yrb, ArbitratyType ptr = ArbitratyType())
            : xlt_(xlt), ylt_(ylt), xrb_(xrb), yrb_(yrb), ptr_(ptr)
        {
            update();
        }

        T width() const { return w_; }
        T height() const { return h_; }
        T xlt() const { return xlt_; }
        T ylt() const { return ylt_; }
        T xrb() const { return xrb_; }
        T yrb() const { return yrb_; }
        T y_mid() const { return ymid_; }
        T x_mid() const { return xlt_ + w_ / 2; }

        //
        //
        //

        bool normalise()
        {
            if (xrb_ < 0 || yrb_ < 0) return false;

            if (xlt_ < 0) xlt_ = 0;
            if (ylt_ < 0) ylt_ = 0;
            return true;
        }

        void merge(const rect_type& n)
        {
            xlt_ = maz_min(xlt_, n.xlt_);
            ylt_ = maz_min(ylt_, n.ylt_);
            xrb_ = maz_max(xrb_, n.xrb_);
            yrb_ = maz_max(yrb_, n.yrb_);
            update();
        }

        /**
         * Returns intersection in percents [0., 100.].
         */
        double intersects(const rect_type& n, double min_pix = 0) const
        {
            // Does it intersect at all?
            if (ylt_ + min_pix > n.yrb_ || yrb_ - min_pix < n.ylt_) return 0.0;
            if (xlt_ + min_pix > n.xrb_ || xrb_ - min_pix < n.xlt_) return 0.0;

            auto top = maz_max(ylt_, n.ylt_);
            auto bottom = maz_min(yrb_, n.yrb_);
            auto inter_h = bottom - top;

            auto left = maz_max(xlt_, n.xlt_);
            auto right = maz_min(xrb_, n.xrb_);
            auto inter_v = right - left;

            auto min_area = maz_min(width() * height(), n.width() * n.height());

            return (100. * inter_v * inter_h) / min_area;
        }

        /**
         * Returns vertical intersection in percents [0., 100.].
         */
        double intersects_y(const rect_type& n) const
        {
            return maz::intersects(ylt_, yrb_, n.ylt_, n.yrb_);
        }

        /**
         * Returns horizontal intersection in percents [0., 100.].
         */
        double intersects_x(const rect_type& n, int* pix_cnt = nullptr) const
        {
            auto ret = maz::intersects(xlt_, xrb_, n.xlt_, n.xrb_);
            if (pix_cnt) *pix_cnt = to_int((ret / 100.) * maz_min(w_, n.w_));
            return ret;
        }

        bool contains(const rect_type& n, double min_pix = 0)
        {
            return (
                ylt_ + min_pix < n.ylt_ && n.yrb_ < yrb_ - min_pix && xlt_ + min_pix < n.xlt_ &&
                n.xrb_ < xrb_ - min_pix);
        }

        bool contains(T x, T y) { return (ylt_ <= y && y <= yrb_ && xlt_ <= x && x <= xrb_); }

        void relative_to(const rect_type& base) { relative_to(base.xlt(), base.ylt()); }

        void relative_to(T x, T y)
        {
            xlt_ = x + xlt_;
            ylt_ = y + ylt_;
            xrb_ = x + xrb_;
            yrb_ = y + yrb_;
            update();
        }

        void relative_to(const rect_type& n, int orientation)
        {
            switch (orientation)
            {
            case 0:
                xlt_ = n.xlt() + xlt_;
                ylt_ = n.ylt() + ylt_;
                xrb_ = n.xlt() + xrb_;
                yrb_ = n.ylt() + yrb_;
                break;
            case 2:
                xlt_ = n.xlt() + n.width() - xrb_;
                ylt_ = n.ylt() + n.height() - yrb_;
                xrb_ = xlt_ + w_;
                yrb_ = ylt_ + h_;
                break;
            case 1:
                ylt_ = n.ylt() + xlt_;
                xlt_ = n.xlt() + n.width() - yrb_;
                xrb_ = xlt_ + h_;
                yrb_ = ylt_ + w_;
                break;
            case 3:
                xlt_ = n.xlt() + ylt_;
                ylt_ = n.ylt() + n.height() - xrb_;
                xrb_ = xlt_ + h_;
                yrb_ = ylt_ + w_;
                break;
            }
            update();
        }

        void scale(double ratio)
        {
            xlt_ *= ratio;
            xrb_ *= ratio;
            ylt_ *= ratio;
            yrb_ *= ratio;
            update();
        }

        void update()
        {
            ymid_ = (ylt_ + yrb_) / static_cast<T>(2);
            h_ = yrb_ - ylt_;
            w_ = xrb_ - xlt_;
        }

        std::string to_string() const
        {
            std::ostringstream oss;
            oss << "[" << xlt_ << ", " << ylt_ << ", " << w_ << ", " << h_ << "]";
            return oss.str();
        }

        /**
         * Consider using `clip_to_document`.
         * Note:
         *  - `diff < 0` shrinks the bbox;
         */
        void extend(int diff)
        {
            xlt_ -= diff;
            ylt_ -= diff;
            xrb_ += diff;
            yrb_ += diff;
            update();
        }

        bool is_left_of(const rect_type& r) const { return (xlt_ < r.xlt_); }

    }; // struct

    template <typename T> rect_type<T> relative_to(rect_type<T> r, int x, int y)
    {
        r.relative_to(static_cast<T>(x), static_cast<T>(y));
        return r;
    }

    template <typename T> rect_type<T> relative_to(rect_type<T> r, T x, T y)
    {
        r.relative_to(x, y);
        return r;
    }

    template <typename T> void transpose(T& x, T& y, T deskew_rad, T center_x, T center_y)
    {
        T x_rel = x - center_x;
        T y_rel = y - center_y;

        T t_x = x_rel * cos(deskew_rad) - y_rel * sin(deskew_rad);
        T t_y = x_rel * sin(deskew_rad) + y_rel * cos(deskew_rad);

        x = t_x + center_x;
        y = t_y + center_y;
    }

    /**
     * Transpose left/top coordinate around center_x, center_y and right bottom and return the
     * result.
     */
    template <typename T>
    rect_type<T> transpose(rect_type<T> r, T deskew_deg, T center_x, T center_y)
    {
        T deskew_rad = DEGREE_TO_RADIAN * deskew_deg;
        T xlt = r.xlt();
        T ylt = r.ylt();
        transpose(xlt, ylt, deskew_rad, center_x, center_y);

        T xrb = r.xrb();
        T yrb = r.yrb();
        transpose(xrb, yrb, deskew_rad, center_x, center_y);

        return {xlt, ylt, xrb, yrb};
    }

    template <typename T> bool operator==(const rect_type<T>& lhs, const rect_type<T>& rhs)
    {
        return lhs.xlt_ == rhs.xlt_ && lhs.ylt_ == rhs.ylt_ && lhs.xrb_ == rhs.xrb_ &&
               lhs.yrb_ == rhs.yrb_;
    }
    template <typename T> bool operator!=(const rect_type<T>& lhs, const rect_type<T>& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename MapType, typename T, typename ArbitratyType>
    MapType bbox_x1y1x2y2(const rect_type<T, ArbitratyType>& r)
    {
        MapType val;
        val["x1"] = r.xlt_;
        val["y1"] = r.ylt_;
        val["x2"] = r.xrb_;
        val["y2"] = r.yrb_;
        return val;
    }

    template <typename MapType, typename ArbitratyType>
    rect_type<double, ArbitratyType>
    rect_from_x1y1x2y2(const MapType& m, ArbitratyType* ptr = nullptr)
    {
        assert(m.isMember("x1"));
        double xlt = m["x1"].asDouble();
        double ylt = m["y1"].asDouble();
        double xrb = m["x2"].asDouble();
        double yrb = m["y2"].asDouble();
        return rect_type<double, ArbitratyType>(xlt, ylt, xrb, yrb, ptr);
    }

    // todo template type traits
    template <typename MapType>
    rect_type<double> rect_from_xywh(const MapType& m, rect_type<double>)
    {
        assert(m.count("x"));
        double x = m["x"];
        double y = m["y"];
        double w = m["w"];
        double h = m["h"];
        return rect_type<double>(x, y, x + w, y + h);
    }

    template <typename MapType> rect_type<int> rect_from_xywh(const MapType& m, rect_type<int>)
    {
        assert(m.isMember("x"));
        int x = m["x"].asInt();
        int y = m["y"].asInt();
        int w = m["w"].asInt();
        int h = m["h"].asInt();
        return rect_type<int>(x, y, x + w, y + h);
    }

    template <typename T, typename Y>
    std::list<rect_type<T>>
    intersects(const rect_type<T>& bbox, Y start, Y end, double intersects_frac = 0.1)
    {
        std::list<rect_type<T>> ret;
        while (start != end)
        {
            if (intersects_frac < bbox.intersects(*start))
            {
                ret.push_back(*start);
            }
            ++start;
        }
        return ret;
    }

    /**
     * Returns true if `lhs` and `rhs` are similar bboxes.
     * `min_frac` should be <= 1.0
     */
    template <typename T>
    bool similar(const rect_type<T>& lhs, const rect_type<T>& rhs, double min_frac, T max_pix)
    {
        if (100. * min_frac > lhs.intersects(rhs)) return false;

        auto w_frac = maz_min(lhs.width(), rhs.width()) / maz_max(lhs.width(), rhs.width());
        if (min_frac > w_frac) return false;
        if (0 <= max_pix && max_pix < std::abs(lhs.width() - rhs.width())) return false;

        auto h_frac = maz_min(lhs.height(), rhs.height()) / maz_max(lhs.height(), rhs.height());
        if (min_frac > h_frac) return false;
        if (0 <= max_pix && max_pix < std::abs(lhs.height() - rhs.height())) return false;

        return true;
    }

    /**
     * Possible bounding box alignments.
     *
     * Note: MIDDLE does not automatically mean LEFT + RIGHT.
     */
    enum alignment { LEFT, RIGHT, BOTH, MIDDLE, NONE };

    template <typename T>
    alignment aligned(const rect_type<T>& b1, const rect_type<T>& b2, int range)
    {
        bool left = is_in_range(b1.xlt(), b2.xlt(), range);
        bool right = is_in_range(b1.xrb(), b2.xrb(), range);

        if (left && right) return BOTH;
        if (left) return LEFT;
        if (right) return RIGHT;

        if (is_in_range(b1.x_mid(), b2.x_mid(), range)) return MIDDLE;

        return NONE;
    }

    template <typename T>
    rect_type<T> clip_to_document(const rect_type<T>& bbox, T diff, const rect_type<T>& doc_bbox)
    {
        auto xlt = maz_max(doc_bbox.xlt(), bbox.xlt() - diff);
        auto ylt = maz_max(doc_bbox.ylt(), bbox.ylt() - diff);
        auto xrb = maz_min(doc_bbox.xrb(), bbox.xrb() + diff);
        auto yrb = maz_min(doc_bbox.yrb(), bbox.yrb() + diff);
        return {xlt, ylt, xrb, yrb};
    }

    template <typename T>
    rect_type<T>
    clip_to_document(const rect_type<T>& bbox, T h_diff, T v_diff, const rect_type<T>& doc_bbox)
    {
        auto xlt = maz_max(doc_bbox.xlt(), bbox.xlt() - h_diff);
        auto ylt = maz_max(doc_bbox.ylt(), bbox.ylt() - v_diff);
        auto xrb = maz_min(doc_bbox.xrb(), bbox.xrb() + h_diff);
        auto yrb = maz_min(doc_bbox.yrb(), bbox.yrb() + v_diff);
        return {xlt, ylt, xrb, yrb};
    }

    namespace distance {

        template <typename T> double euclidean(T x1, T y1, T x2, T y2)
        {
            T x = x2 - x1;
            T y = y2 - y1;
            return sqrt(x * x + y * y);
        }

        template <typename T> double euclidean_all(const rect_type<T>& r1, const rect_type<T>& r2)
        {
            auto r2_is_left = r2.xrb() < r1.xlt();
            auto r2_is_right = r1.xrb() < r2.xlt();
            auto r2_is_top = r2.yrb() < r1.ylt();
            auto r2_is_bottom = r1.yrb() < r2.ylt();

            // r2 right/bottom vs r1 left/top
            if (r2_is_left && r2_is_top)
            {
                return euclidean(r2.xrb(), r2.yrb(), r1.xlt(), r1.ylt());

                // r2 right/top vs r1 left/bottom
            } else if (r2_is_left && r2_is_bottom)
            {
                return euclidean(r2.xrb(), r2.ylt(), r1.xlt(), r1.yrb());

                // r1 right/bottom vs r2 left/top
            } else if (r2_is_right && r2_is_bottom)
            {
                return euclidean(r1.xrb(), r1.yrb(), r2.xlt(), r2.ylt());

                // r1 right/bottom vs r2 left/top
            } else if (r2_is_right && r2_is_top)
            {
                return euclidean(r1.xrb(), r1.ylt(), r2.xlt(), r2.yrb());
            }

            if (r2_is_left) return r1.xlt() - r2.xrb();
            if (r2_is_right) return r2.xlt() - r1.xrb();
            if (r2_is_top) return r1.ylt() - r2.yrb();
            if (r2_is_bottom) return r2.ylt() - r1.yrb();

            // intersects
            auto ratio = r1.intersects(r2) / 100.;
            return -ratio;
        }

        /**
         * Returns vertical distance between `r1` and `r2`.
         *
         * Note: Does not handle special overlapping cases.
         */
        template <typename T> T vertical(const rect_type<T>& r1, const rect_type<T>& r2)
        {
            if (r1.y_mid() > r2.y_mid())
            {
                return r1.ylt() - r2.yrb();
            } else
            {
                return r2.ylt() - r1.yrb();
            }
        }

        /**
         * Returns horizontal distance between `r1` and `r2`.
         *
         * Note: Does not handle special overlapping cases.
         */
        template <typename T> T horizontal(const rect_type<T>& r1, const rect_type<T>& r2)
        {
            if (r1.is_left_of(r2))
            {
                return r2.xlt() - r1.xrb();
            } else
            {
                return r1.xlt() - r2.xrb();
            }
        }

    } // namespace distance

    // default bbox typedef
    typedef rect_type<double> bbox_type;

    /** Serialize `bbox_type` to string. */
    std::string serialize(const bbox_type& b);

    /** Serialize `bbox_type` to a file. */
    void serialize(const char* file, const bbox_type& b);

} // namespace maz
