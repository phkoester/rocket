/*
 * impl.cc
 *
 * This fixes a linker error related to scnlib.
 */

#include <scn/impl.h>

/*
 * The original file is `scnlib/src/scn/impl.cpp`.
 */

namespace scn { // NOLINT
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

#define SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(T, Context) \
  template SCN_PUBLIC scan_expected<Context::iterator> \
  scanner_scan_for_builtin_type(T&, Context&, const format_specs&);

#define SCN_DEFINE_SCANNER_SCAN_FOR_CTX(Context) \
  SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(bool, Context)

SCN_DEFINE_SCANNER_SCAN_FOR_CTX(scan_context)
SCN_DEFINE_SCANNER_SCAN_FOR_CTX(wscan_context)

} // namespace detail

SCN_END_NAMESPACE
} // namespace scn

// EOF
