/*
 * ctype-char32_t.cc
 */

#include "ctype-char32_t.h"

namespace std {

// `ctype<char32_t>` ----------------------------------------------------------------------------------------

ctype<char32_t>::ctype(size_t __refs) :
    __ctype_abstract_base<char32_t>(__refs),
    _M_c_locale_ctype(_S_get_c_locale()),
    _M_narrow_ok(false) {
  _M_initialize_ctype();
}

ctype<char32_t>::ctype(__c_locale __cloc, size_t __refs) :
    __ctype_abstract_base<char32_t>(__refs),
    _M_c_locale_ctype(_S_clone_c_locale(__cloc)),
    _M_narrow_ok(false) {
  _M_initialize_ctype();
}

ctype<char32_t>::~ctype() noexcept {
  _S_destroy_c_locale(_M_c_locale_ctype);
}

ctype<char32_t>::__wmask_type
ctype<char32_t>::_M_convert_to_wmask(const mask __m) const
{
  __wmask_type __ret;
  switch (__m)
  {
  case space:
    __ret = wctype("space");
    break;
  case print:
    __ret = wctype("print");
    break;
  case cntrl:
    __ret = wctype("cntrl");
    break;
  case upper:
    __ret = wctype("upper");
    break;
  case lower:
    __ret = wctype("lower");
    break;
  case alpha:
    __ret = wctype("alpha");
    break;
  case digit:
    __ret = wctype("digit");
    break;
  case punct:
    __ret = wctype("punct");
    break;
  case xdigit:
    __ret = wctype("xdigit");
    break;
  case alnum:
    __ret = wctype("alnum");
    break;
  case graph:
    __ret = wctype("graph");
    break;
  default:
    // Different from the generic version, xdigit and print in newlib are defined as bitwise-OR result of
    // bitmasks:
    //   xdigit = _X | _N;
    //   print  = _P | _U | _L | _N | _B;
    // in which _X and _B don't correspond to any ctype mask. In order to get the wmask correctly converted
    // when __m is equal to _X or _B, the two cases are specifically handled here
    if (__m & xdigit)
      __ret = wctype("xdigit");
    else if (__m & print)
      __ret = wctype("print");
    else
      __ret = __wmask_type();
  }
  return __ret;
};

void
ctype<char32_t>::_M_initialize_ctype() {
  wint_t __i;
  for (__i = 0; __i < 128; ++__i) {
    const int __c = wctob(__i);
    if (__c == EOF)
      break;
    else
      _M_narrow[__i] = static_cast<char>(__c);
  }
  if (__i == 128)
    _M_narrow_ok = true;
  else
    _M_narrow_ok = false;
  for (size_t __i = 0; __i < sizeof(_M_widen) / sizeof(wint_t); ++__i) // cppcheck-suppress shadowVariable
    _M_widen[__i] = btowc(__i);

  for (size_t __i = 0; __i <= 7; ++__i) { // cppcheck-suppress shadowVariable
    _M_bit[__i] = static_cast<mask>(1 << __i);
    _M_wmask[__i] = _M_convert_to_wmask(_M_bit[__i]);
  }
}

char32_t
ctype<char32_t>::do_toupper(char32_t __c) const {
  return towupper(__c);
}

const char32_t*
ctype<char32_t>::do_toupper(char32_t* __lo, const char32_t* __hi) const {
  while (__lo < __hi) {
    *__lo = towupper(*__lo);
    ++__lo;
  }
  return __hi;
}

char32_t
ctype<char32_t>::do_tolower(char32_t __c) const {
  return towlower(__c);
}

const char32_t*
ctype<char32_t>::do_tolower(char32_t* __lo, const char32_t* __hi) const {
  while (__lo < __hi) {
    *__lo = towlower(*__lo);
    ++__lo;
  }
  return __hi;
}

bool
ctype<char32_t>::do_is(mask __m, char32_t __c) const {
  bool __ret = false;
  // Newlib C library has a compact encoding that uses 8 bits only
  const size_t __bitmasksize = 7;
  for (size_t __bitcur = 0; __bitcur <= __bitmasksize; ++__bitcur) {
    if (__m & _M_bit[__bitcur] && iswctype(__c, _M_wmask[__bitcur])) {
      __ret = true;
      break;
    }
  }
  return __ret;
}

const char32_t*
ctype<char32_t>::do_is(const char32_t* __lo, const char32_t* __hi, mask* __vec) const {
  for (; __lo < __hi; ++__vec, ++__lo) {
    // Newlib C library has a compact encoding that uses 8 bits only
    const size_t __bitmasksize = 7;
    mask __m = 0;
    for (size_t __bitcur = 0; __bitcur <= __bitmasksize; ++__bitcur) {
      if (iswctype(*__lo, _M_wmask[__bitcur]))
        __m |= _M_bit[__bitcur];
    }
    *__vec = __m;
  }
  return __hi;
}

const char32_t*
ctype<char32_t>::do_scan_is(mask __m, const char32_t* __lo, const char32_t* __hi) const {
  while (__lo < __hi && !this->do_is(__m, *__lo))
    ++__lo;
  return __lo;
}

const char32_t*
ctype<char32_t>::do_scan_not(mask __m, const char_type* __lo, const char_type* __hi) const {
  while (__lo < __hi && this->do_is(__m, *__lo) != 0)
    ++__lo;
  return __lo;
}

char32_t
ctype<char32_t>::do_widen(char __c) const {
  return _M_widen[static_cast<unsigned char>(__c)];
}

const char*
ctype<char32_t>::do_widen(const char* __lo, const char* __hi, char32_t* __dest) const {
  while (__lo < __hi) {
    *__dest = _M_widen[static_cast<unsigned char>(*__lo)];
    ++__lo;
    ++__dest;
  }
  return __hi;
}

char
ctype<char32_t>::do_narrow(char32_t __wc, char __dfault) const {
  if (__wc < 128 && _M_narrow_ok)
    return _M_narrow[__wc];
  const int __c = wctob(__wc);
  return (__c == EOF ? __dfault : static_cast<char>(__c));
}

const char32_t*
ctype<char32_t>::do_narrow(const char32_t* __lo, const char32_t* __hi, char __dfault, char* __dest) const {
  if (_M_narrow_ok) {
    while (__lo < __hi) {
      if (*__lo < 128)
        *__dest = _M_narrow[*__lo];
      else {
        const int __c = wctob(*__lo);
        *__dest = (__c == EOF ? __dfault : static_cast<char>(__c));
      }
      ++__lo;
      ++__dest;
    }
  } else {
    while (__lo < __hi) {
      const int __c = wctob(*__lo);
      *__dest = (__c == EOF ? __dfault : static_cast<char>(__c));
      ++__lo;
      ++__dest;
    }
  }
  return __hi;
}

// `numpunct<char32_t>` -------------------------------------------------------------------------------------

template<> 
numpunct<char32_t>::~numpunct() {
  delete _M_data;
}

template<> 
void
numpunct<char32_t>::_M_initialize_numpunct(__c_locale)
{
  // "C" locale
  if (!_M_data)
    _M_data = new __numpunct_cache<char32_t>;

  _M_data->_M_grouping = "";
  _M_data->_M_grouping_size = 0;
  _M_data->_M_use_grouping = false;
  
  _M_data->_M_decimal_point = U'.';
  _M_data->_M_thousands_sep = U',';
  
  // Use `ctype::widen` code without the facet ...
  for (size_t __i = 0; __i < __num_base::_S_oend; ++__i)
    _M_data->_M_atoms_out[__i] = static_cast<char32_t>(__num_base::_S_atoms_out[__i]);
  
  for (size_t __i = 0; __i < __num_base::_S_iend; ++__i)
    _M_data->_M_atoms_in[__i] = static_cast<char32_t>(__num_base::_S_atoms_in[__i]);

  _M_data->_M_truename = U"true";
  _M_data->_M_truename_size = 4;
  _M_data->_M_falsename = U"false";
  _M_data->_M_falsename_size = 5;
}

} // namespace std

// EOF
