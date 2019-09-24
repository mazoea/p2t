//
// author: jm (Mazoea s.r.o.)
// date: 2012
//
#ifdef _WINDOWS
#ifndef NDEBUG
//#define VLDDEBUG
#ifdef VLDDEBUG
#pragma message("Compiling with VLDDEBUG")
#include <vld.h>
#endif
#endif
#endif

#pragma once

#ifndef TOSTRING
#define DO_EXPAND(VAL) VAL##1
#define EXPAND(VAL) DO_EXPAND(VAL)
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

#include "unilib/unicode.h"
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <time.h>
#include <utility>
#include <vector>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#define PTR_DEBUG(a, b)
//#define PTR_DEBUG(a, b) {std::cout << b << " " << std::hex << "0x" << int(a) << std::endl;}

namespace std {
    inline string to_string(const std::string& s) { return s; }
    inline string to_string(const char* s) { return s; }
} // namespace std

namespace maz {

    //===============================
    // typedefs
    //===============================

    typedef float l_float32;
    typedef int l_int32;
    typedef unsigned int l_uint32;

    template <typename T> T maz_max(T l, T r) { return (l < r) ? r : l; }
    template <typename T> T maz_min(T l, T r) { return (l < r) ? l : r; }

    template <typename T> int to_int(T v)
    {
        // be careful, negative rounding to int is not as expected
        // static_cast<int>(-116 + 0.5) = -115
        return (T() <= v) ? static_cast<int>(0.5 + v) : -static_cast<int>(0.5 + -v);
    }

    inline int to_int(unsigned int v) { return static_cast<int>(v); }
    inline int to_int(std::size_t v) { return static_cast<int>(v); }

    template <typename T> double rank(T& arr, double perc_to_remove)
    {
        if (arr.empty()) return 0.;

        arr.sort();
        int n = static_cast<int>(perc_to_remove * arr.size());
        int half_n = n / 2;
        // (optional) indicate we take the whole array
        if (1 > half_n) half_n = 0;

        auto s = std::next(arr.begin(), half_n);
        auto e = std::next(arr.end(), -half_n);

        double val = std::accumulate(s, e, 0.) / (arr.size() - (2 * half_n));
        return val;
    }

    template <typename T> double rank(T s, T e, double perc_to_remove)
    {
        std::list<typename T::value_type> arr;
        std::copy(s, e, std::back_inserter(arr));
        return rank(arr, perc_to_remove);
    }

    template <typename T, typename Getter> double rank(T s, T e, double perc_to_remove, Getter get)
    {
        std::list<decltype(get(*s))> arr;
        for (; s != e; ++s)
            arr.push_back(get(*s));
        return rank(arr, perc_to_remove);
    }

    template <typename T> double mean(T& arr) { return rank(arr, 0.); }

    /**
     * Returns `true` if the array consists of more than `accepted_ratio`
     * values reasonably near a specified pivot
     * with accepted min `acc_min` and `acc_max` ratio based on pivot selected
     * using rank of `pivot_perc_rank` value
     */
    template <typename T>
    double reasonable_mean(
        T s, T e, double pivot_perc_rank, double acc_min, double acc_max, double accepted_ratio)
    {
        double pivot = rank(s, e, pivot_perc_rank);
        auto mmin = static_cast<typename T::value_type>(acc_min * pivot);
        auto mmax = static_cast<typename T::value_type>(acc_max * pivot);
        auto cnt_ok =
            std::count_if(s, e, [=](typename T::value_type v) { return mmin <= v && v <= mmax; });
        auto min_ok = to_int(accepted_ratio * std::distance(s, e));
        return cnt_ok >= min_ok;
    }

    template <typename T>
    double reasonable_mean(
        T s, T e, typename T::value_type mmin, typename T::value_type mmax, double accepted_ratio)
    {
        auto cnt_ok =
            std::count_if(s, e, [=](typename T::value_type v) { return mmin <= v && v <= mmax; });
        auto min_ok = to_int(accepted_ratio * std::distance(s, e));
        return cnt_ok >= min_ok;
    }

    //===============================
    // constants
    //===============================

    // ret values
    extern const int OK;
    extern const int CONTINUE;
    extern const int VERSION;
    extern const int INVALID_PARAM;
    extern const int EXCEPTION;
    extern const int DONT_KNOW;

    extern const l_float32 DEGREE_TO_RADIAN;
    extern const l_float32 RADIAN_TO_DEGREE;

    //===============================
    // classes
    //===============================

    /** Simple logger. */
    class slogger
    {
        std::string prefix_;

      public:
        slogger() = default;
        explicit slogger(const std::string& prefix) : prefix_(std::move(prefix)) {}

        void warn(const std::string& msg) const;
        void warn(const std::string& msg1, const std::string& msg2) const;
        void error(const std::string& msg) const;
        void error(const std::string& msg1, const std::string& msg2) const;

      private:
        template <typename OUTSTREAM>
        void log_(OUTSTREAM& outs, const char* level, const std::string& msg) const
        {
            if (!prefix_.empty())
            {
                outs << prefix_ << ": ";
            }
            outs << msg << std::endl;
        }
    };

    /**
     * Document rotation.
     */
    struct rotations
    {
        int guessed_{-1};
        int done_[4]{0};
        int step{0};

        rotations() = default;

        /** Store guessed rotation. */
        void guessed(int rotation) { guessed_ = rotation; }
        /** Get next rotation. */
        int next();
    };

    /**
     * Runs of something.
     * Can be used for counting ON/OFF pixels.
     */
    struct runs
    {
        static constexpr int ON = 1;
        static constexpr int OFF = -1;

        std::list<int> r_;

      public:
        runs() = default;
        /** Set `on` to true if it signals ON position. */
        void add(bool on);
        /** Return if the run matches the definition. */
        bool matches(std::list<int> def) const;
        int at(size_t pos) const { return *std::next(r_.begin(), pos); }
        size_t size() const { return r_.size(); }
        /** Return first value matching ON/OFF. */
        int first(int on_off) const;
        std::list<int> vals() const { return r_; }
    };

    //===============================
    // conversion helpers
    //===============================

    template <typename T> T value(const std::string& val)
    {
        std::istringstream oss(val);
        T ret = T();
        oss >> ret;
        return ret;
    }

    template <typename T> void value(const std::string& val, T& ret)
    {

        std::istringstream oss(val);
        oss >> ret;
    }

    template <typename T> void value(const std::string& val, std::vector<T>& ret)
    {
        std::stringstream ss(val);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            T tmp = T();
            value(item, tmp);
            ret.push_back(tmp);
        }
    }

    template <typename T> T get_value(const std::string& input)
    {
        std::istringstream oss(input);
        T val = T();
        oss >> val;
        return val;
    }

    template <typename T> T get_value(const std::string& input, T)
    {
        std::istringstream oss(input);
        T val = T();
        oss >> val;
        return val;
    }

    template <typename T> std::string to_string(const T& val)
    {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }

    template <int PRECISION = 2> std::string to_string(double val)
    {
        std::ostringstream oss;
        oss.precision(PRECISION);
        oss << std::fixed << val;
        return oss.str();
    }

    template <> void value<std::string>(const std::string& val, std::string& ret);

    template <int SPACES, typename T> T round(T val)
    {
        return static_cast<int>(std::lround(val * pow(10.0, SPACES))) /
               pow(static_cast<T>(10.0), SPACES);
    }

    //===============================
    // getters
    //===============================

    bool in_array(char c, const std::string& chars);

    bool in_array(const std::string& str, const std::string& chars);

    //===============================
    // numerical
    //===============================

    template <typename Y> bool is_in_range(size_t val1, size_t val2, Y accepted_diff)
    {
        if (val1 > val2) return (val1 - val2) < static_cast<size_t>(accepted_diff);
        return (val2 - val1) < static_cast<size_t>(accepted_diff);
    }

    template <typename T, typename Y> bool is_in_range(T val1, T val2, Y accepted_diff)
    {
        return (fabs(val1 - val2) < static_cast<T>(accepted_diff));
    }

    template <typename T> bool between(T min_, T val, T max_) { return min_ < val && val < max_; }

    //===============================
    // generic strings
    //===============================

    bool only_numbers_specific(const char* cstr);

    bool only_numbers(const char* cstr);
    bool only_numbers(const std::string& s);

    bool has_number(const std::string& str);

    bool has_whitespace(const std::string& str);

    bool _isspace(int c);

    char maz_tolower(int c);
    char maz_toupper(int c);

    /** Remove whitespace from beginning. */
    void lstrip(std::string& s);

    /** Remove whitespace from end. */
    void rstrip(std::string& s);

    /** Remove whitespace from beginning/end (`lstrip` + `rstrip`). */
    void strip(std::string& s);

    bool strip_word(std::string& str);

    /** Remove all whitespace characters. */
    std::string remove_whitespace_copy(std::string str);
    void remove_whitespace(std::string& str);

    bool starts_with(std::string haystack, std::string prefix);
    bool ends_with(std::string haystack, std::string suffix);

    template <typename T> bool maz_is_digit(T c) { return '0' <= c && c <= '9'; }

    inline std::size_t count_digit(const std::string& s)
    {
        return std::count_if(s.begin(), s.end(), maz_is_digit<char>);
    }

    //===============================
    // other takeaways
    //===============================

    std::string base64_encode(unsigned char* data, size_t len);
    std::vector<unsigned char> base64_decode(const std::string& input);

    //===============================
    // kind of old compiler smart ptrs
    //===============================

    struct empty_deleter
    {
        template <typename T> void operator()(T ptr_) { delete ptr_; }
    };

    template <typename Type, typename Deleter = empty_deleter> struct smart_ptr
    {
        Type* ptr_{nullptr};
        Deleter del_;

        smart_ptr(Type* ptr, Deleter d = Deleter()) : ptr_(ptr), del_(d)
        {
            if (!ptr_)
            {
                assert(ptr_ && "ptr_ cannot be null");
            }
            PTR_DEBUG(ptr_, "secured");
        }

        smart_ptr(Type* ptr, Type* init_value, Deleter d = Deleter()) : ptr_(ptr), del_(d)
        {
            assert(ptr_);
            ptr_ = init_value;
            PTR_DEBUG(ptr_, "secured");
        }

        smart_ptr() = default;

        smart_ptr(smart_ptr& inst) : del_(inst.del_), ptr_(nullptr) { swap(inst); }

        smart_ptr& operator=(smart_ptr& sp) = delete;

        void swap(smart_ptr& inst)
        {
            // either not the same or both nullptr
            assert(inst.ptr_ != ptr_ || !inst.ptr_);
            std::swap(inst.del_, inst.del_);
            std::swap(inst.ptr_, ptr_);
            PTR_DEBUG(ptr_, "secured");
        }

        Type* get() { return ptr_; }

        const Type* get() const { return ptr_; }

        Type& operator*() { return *ptr_; }

        Type* operator->() { return ptr_; }

        Type* operator&() const { return ptr_; }

        ~smart_ptr()
        {
            clear();
            ptr_ = nullptr;
        }

        void clear()
        {
            if (ptr_)
            {
                PTR_DEBUG(ptr_, "deleted");
                del_(ptr_);
                ptr_ = nullptr;
            }
        }

        void reset() { clear(); }

        Type* forget()
        {
            Type* p = ptr_;
            set(nullptr);
            return p;
        }

        void set(Type* ptr)
        {
            ptr_ = ptr;
            PTR_DEBUG(ptr_, "secured");
        }

    }; // smart_ptr

    //===============================
    // simple callback ftor
    //===============================

    using std::shared_ptr;

    void empty_function();
    void empty_function(int);

    // use std::bind for member functions
    template <typename T> struct simple_callback_at_delete
    {
        T ftor_;
        mutable bool disabled_{false};

        explicit simple_callback_at_delete(T ftor) : ftor_(ftor) {}
        simple_callback_at_delete() = delete;
        simple_callback_at_delete& operator=(const simple_callback_at_delete&) = delete;
        simple_callback_at_delete(const simple_callback_at_delete& rhs) : ftor_(rhs.ftor_)
        {
            rhs.disabled_ = true;
        }
        void finish()
        {
            if (!disabled_) ftor_();
        }
        ~simple_callback_at_delete() { finish(); }
    };

    template <typename T, typename Y> struct simple_callback_at_delete1
    {
        T ftor_;
        Y param;
        mutable bool disabled_;

        // cppcheck-suppress uninitMemberVar
        simple_callback_at_delete1(T ftor, Y p) : ftor_(ftor), param(p), disabled_(false) {}
        // cppcheck-suppress uninitMemberVar
        simple_callback_at_delete1(const simple_callback_at_delete1& rhs)
            : ftor_(rhs.ftor_), param(rhs.param), disabled_(false)
        {
            rhs.disabled_ = true;
        }
        simple_callback_at_delete1() = delete;
        simple_callback_at_delete1& operator=(const simple_callback_at_delete1&) = delete;

        ~simple_callback_at_delete1() { finish(); }

        void finish()
        {
            if (!disabled_) ftor_();
        }
    };

    //===============================
    // time functions
    //===============================

    std::string now();

    //===============================
    // format
    //===============================

    template <typename T> std::string format(T val, size_t text_size, char padding_char)
    {
        std::ostringstream oss;
        oss << std::setw(static_cast<int>(text_size)) << std::setfill(padding_char) << val;
        return oss.str();
    }

    //===============================
    // format
    //===============================

    template <typename T> std::string join(T start, T end, const std::string& glue)
    {
        std::string ret;
        if (start == end) return ret;

        ret.append(*start++);
        while (start != end)
        {
            ret.append(glue);
            ret.append(*start++);
        }
        return ret;
    }

    template <typename T> std::string var_join(const std::string&, T v)
    {
        return std::to_string(v);
    }

    template <typename T, typename... Args>
    std::string var_join(const std::string& delim, T first, Args... args)
    {
        return std::to_string(first) + delim + var_join(delim, args...);
    }

    //===============================
    // timers
    //===============================

    struct timer
    {
        std::chrono::steady_clock::time_point s_;
        timer() : s_(std::chrono::steady_clock::now()) {}
        double took() const
        {
            auto e = std::chrono::steady_clock::now();
            using fsecs = std::chrono::duration<double, std::chrono::seconds::period>;
            auto as_fsecs = std::chrono::duration_cast<fsecs>(e - s_);
            return as_fsecs.count();
        }

    }; // timer

    /** Difference in seconds between `start` and now. */
    double clocks_diff_s(const clock_t& start);

    struct perf_timer
    {
        std::string name;
        double elapsed{-1.};
        std::list<perf_timer> blocks;

        clock_t s_{};

        perf_timer() = default;

        explicit perf_timer(const std::string& n) : name(n) {}

        perf_timer& operator=(perf_timer& sp) = delete;

        bool running() const { return 0. > elapsed; }

        void start() { s_ = clock(); }

        void end() { elapsed = clocks_diff_s(s_); }

    }; // perf_timer

    //========================================
    // specialities
    //========================================

    // double sse_rsqrt_nr(double x);

    //========================================
    // hash related
    //========================================

    /**
     * Simple hash function over a buffer.
     */
    std::size_t hash_fnv(const unsigned char* buf, std::size_t len);

} // namespace maz

//========================================
// current function
//========================================

#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || \
    (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)

#define CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__DMC__) && (__DMC__ >= 0x810)

#define CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__FUNCSIG__)

//# define CURRENT_FUNCTION __FUNCSIG__
#define CURRENT_FUNCTION __FUNCTION__

#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || \
    (defined(__IBMCPP__) && (__IBMCPP__ >= 500))

#define CURRENT_FUNCTION __FUNCTION__

#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)

#define CURRENT_FUNCTION __FUNC__

#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)

#define CURRENT_FUNCTION __func__

#else

#define CURRENT_FUNCTION "(unknown)"

#endif
