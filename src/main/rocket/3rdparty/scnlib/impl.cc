/*
 * impl.cc
 *
 * This fixes a linker error related to scnlib.
 */

#include "impl.h"

/*
 * The original file is `scnlib/src/scn/impl.cpp`.
 */

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {

template <typename T, typename Context>
scan_expected<typename Context::iterator>
scanner_scan_for_builtin_type(T& val, Context& ctx, const format_specs& specs)
{
    if constexpr (!detail::is_type_disabled<T>) {
        return impl::arg_reader<Context>{ctx.range(), specs, {}}(val);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

template <typename Range>
scan_expected<ranges::iterator_t<Range>> internal_skip_classic_whitespace(
    Range r,
    bool allow_exhaustion)
{
    return impl::skip_classic_whitespace(r, allow_exhaustion)
        .transform_error(impl::make_eof_scan_error);
}

#define SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(T, Context)     \
    template SCN_PUBLIC scan_expected<Context::iterator> \
    scanner_scan_for_builtin_type(T&, Context&, const format_specs&);

#define SCN_DEFINE_SCANNER_SCAN_FOR_CTX(Context)                               \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(Context::char_type, Context)              \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(bool, Context)                            \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(signed char, Context)                     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(short, Context)                           \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(int, Context)                             \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long, Context)                            \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long long, Context)                       \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned char, Context)                   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned short, Context)                  \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned int, Context)                    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long, Context)                   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long long, Context)              \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(float, Context)                           \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(double, Context)                          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long double, Context)                     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string, Context)                     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring, Context)                    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string_view, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring_view, Context)               \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(regex_matches, Context)                   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(wregex_matches, Context)                  \
    template SCN_PUBLIC scan_expected<ranges::iterator_t<Context::range_type>> \
    internal_skip_classic_whitespace(Context::range_type, bool); // Patched!

SCN_DEFINE_SCANNER_SCAN_FOR_CTX(scan_context)
SCN_DEFINE_SCANNER_SCAN_FOR_CTX(wscan_context)

} // namespace detail

SCN_END_NAMESPACE
} // namespace scn

// EOF
