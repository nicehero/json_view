// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version
// nicehero copy

#ifndef NICEHERO_DETAIL_PARSE_NUMBER_HPP
#define NICEHERO_DETAIL_PARSE_NUMBER_HPP

#include <system_error>
#include <stdexcept>
#include <string>
#include <vector>
#include <locale>
#include <string>
#include <limits> // std::numeric_limits
#include <type_traits> // std::enable_if
#include <exception>
#include <cctype>

#if defined(_MSC_VER)
#define NICEHERO_HAS_MSC_STRTOD_L
#endif
#if defined(ANDROID) || defined(__ANDROID__)
#if __ANDROID_API__ >= 21
#define NICEHERO_HAS_STRTOLD_L
#else
#define NICEHERO_NO_LOCALECONV
#endif
#endif

#ifndef CHAR_BIT
#define CHAR_BIT      8
#endif // !CHAR_BIT


namespace nicehero {

	namespace type_traits {

		// is_char8
		template <typename CharT, typename Enable = void>
		struct is_char8 : std::false_type {};

		template <typename CharT>
		struct is_char8<CharT, typename std::enable_if<std::is_integral<CharT>::value &&
			!std::is_same<CharT, bool>::value &&
			sizeof(uint8_t) == sizeof(CharT)>::type> : std::true_type {};

		// is_char16
		template <typename CharT, typename Enable = void>
		struct is_char16 : std::false_type {};

		template <typename CharT>
		struct is_char16<CharT, typename std::enable_if<std::is_integral<CharT>::value &&
			!std::is_same<CharT, bool>::value &&
			(std::is_same<CharT, char16_t>::value || sizeof(uint16_t) == sizeof(CharT))>::type> : std::true_type {};

		// is_char32
		template <typename CharT, typename Enable = void>
		struct is_char32 : std::false_type {};

		template <typename CharT>
		struct is_char32<CharT, typename std::enable_if<std::is_integral<CharT>::value &&
			!std::is_same<CharT, bool>::value &&
			(std::is_same<CharT, char32_t>::value || (!std::is_same<CharT, char16_t>::value && sizeof(uint32_t) == sizeof(CharT)))>::type> : std::true_type {};

		// is_int128

		template <class T, class Enable = void>
		struct is_int128_type : std::false_type {};

		// is_unsigned_integer

		template <class T, class Enable = void>
		struct is_uint128_type : std::false_type {};

		template <class T, class Enable = void>
		class integer_limits
		{
		public:
			static constexpr bool is_specialized = false;
		};

		template <class T>
		class integer_limits<T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value>::type>
		{
		public:
			static constexpr bool is_specialized = true;
			static constexpr bool is_signed = std::numeric_limits<T>::is_signed;
			static constexpr int digits = std::numeric_limits<T>::digits;
			static constexpr std::size_t buffer_size = static_cast<std::size_t>(sizeof(T)*CHAR_BIT*0.302) + 3;

			static constexpr T(max)() noexcept
			{
				return (std::numeric_limits<T>::max)();
			}
			static constexpr T(min)() noexcept
			{
				return (std::numeric_limits<T>::min)();
			}
			static constexpr T lowest() noexcept
			{
				return std::numeric_limits<T>::lowest();
			}
		};

		template <class T>
		class integer_limits<T, typename std::enable_if<!std::is_integral<T>::value && is_int128_type<T>::value>::type>
		{
		public:
			static constexpr bool is_specialized = true;
			static constexpr bool is_signed = true;
			static constexpr int digits = sizeof(T)*CHAR_BIT - 1;
			static constexpr std::size_t buffer_size = (sizeof(T)*CHAR_BIT*0.302) + 3;

			static constexpr T(max)() noexcept
			{
				return (((((T)1 << (digits - 1)) - 1) << 1) + 1);
			}
			static constexpr T(min)() noexcept
			{
				return -(max)() - 1;
			}
			static constexpr T lowest() noexcept
			{
				return (min)();
			}
		};

		template <class T>
		class integer_limits<T, typename std::enable_if<!std::is_integral<T>::value && is_uint128_type<T>::value>::type>
		{
		public:
			static constexpr bool is_specialized = true;
			static constexpr bool is_signed = false;
			static constexpr int digits = sizeof(T)*CHAR_BIT;

			static constexpr T(max)() noexcept
			{
				return T(T(~0));
			}
			static constexpr T(min)() noexcept
			{
				return 0;
			}
			static constexpr T lowest() noexcept
			{
				return std::numeric_limits<T>::lowest();
			}
		};


		// follows http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4436.pdf

		// detector

		// primary template handles all types not supporting the archetypal Op
		template<
			class Default,
			class, // always void; supplied externally
			template<class...> class Op,
			class... Args
		>
			struct detector
		{
			constexpr static auto value = false;
			using type = Default;
		};


		// is_detected, is_detected_t

		template< template<class...> class Op, class... Args >
		using
			is_detected = detector<void, void, Op, Args...>;

		template< template<class...> class Op, class... Args >
		using
			is_detected_t = typename is_detected<Op, Args...>::type;

		// detected_or, detected_or_t

		template< class Default, template<class...> class Op, class... Args >
		using
			detected_or = detector<Default, void, Op, Args...>;

		template< class Default, template<class...> class Op, class... Args >
		using
			detected_or_t = typename detected_or<Default, Op, Args...>::type;

		// is_detected_exact

		template< class Expected, template<class...> class Op, class... Args >
		using
			is_detected_exact = std::is_same< Expected, is_detected_t<Op, Args...> >;

		// is_detected_convertible

		template< class To, template<class...> class Op, class... Args >
		using
			is_detected_convertible = std::is_convertible< is_detected_t<Op, Args...>, To >;

		template <typename T>
		struct is_stateless
			: public std::integral_constant<bool,
			(std::is_default_constructible<T>::value &&
				std::is_empty<T>::value)>
		{};

		// to_plain_pointer

		template<class Pointer> inline
			typename std::pointer_traits<Pointer>::element_type* to_plain_pointer(Pointer ptr)
		{
			return (std::addressof(*ptr));
		}

		template<class T> inline
			T * to_plain_pointer(T * ptr)
		{
			return (ptr);
		}

		// is_byte

		template <class T, class Enable = void>
		struct is_byte : std::false_type {};


		// is_character

		template <class T, class Enable = void>
		struct is_character : std::false_type {};

		template <class T>
		struct is_character<T,
			typename std::enable_if<std::is_same<T, char>::value ||
			std::is_same<T, wchar_t>::value
			>::type> : std::true_type {};

		// is_narrow_character

		template <class T, class Enable = void>
		struct is_narrow_character : std::false_type {};

		template <class T>
		struct is_narrow_character<T,
			typename std::enable_if<is_character<T>::value && (sizeof(T) == sizeof(char))
			>::type> : std::true_type {};

		// is_wide_character

		template <class T, class Enable = void>
		struct is_wide_character : std::false_type {};

		template <class T>
		struct is_wide_character<T,
			typename std::enable_if<is_character<T>::value && (sizeof(T) != sizeof(char))
			>::type> : std::true_type {};

		// From boost
		namespace ut_detail {

			template<typename T>
			struct is_cstring_impl : public std::false_type {};

			template<typename T>
			struct is_cstring_impl<T const*> : public is_cstring_impl<T*> {};

			template<typename T>
			struct is_cstring_impl<T const* const> : public is_cstring_impl<T*> {};

			template<>
			struct is_cstring_impl<char*> : public std::true_type {};

			template<>
			struct is_cstring_impl<wchar_t*> : public std::true_type {};

		} // namespace ut_detail

		template<typename T>
		struct is_cstring : public ut_detail::is_cstring_impl<typename std::decay<T>::type> {};

		// is_bool

		template <class T, class Enable = void>
		struct is_bool : std::false_type {};

		template <class T>
		struct is_bool<T,
			typename std::enable_if<std::is_same<T, bool>::value
			>::type> : std::true_type {};

		// is_u8_u16_u32_or_u64

		template <class T, class Enable = void>
		struct is_u8_u16_u32_or_u64 : std::false_type {};

		template <class T>
		struct is_u8_u16_u32_or_u64<T,
			typename std::enable_if<std::is_same<T, uint8_t>::value ||
			std::is_same<T, uint16_t>::value ||
			std::is_same<T, uint32_t>::value ||
			std::is_same<T, uint64_t>::value
			>::type> : std::true_type {};

		// is_int

		template <class T, class Enable = void>
		struct is_i8_i16_i32_or_i64 : std::false_type {};

		template <class T>
		struct is_i8_i16_i32_or_i64<T,
			typename std::enable_if<std::is_same<T, int8_t>::value ||
			std::is_same<T, int16_t>::value ||
			std::is_same<T, int32_t>::value ||
			std::is_same<T, int64_t>::value
			>::type> : std::true_type {};

		// is_float_or_double

		template <class T, class Enable = void>
		struct is_float_or_double : std::false_type {};

		template <class T>
		struct is_float_or_double<T,
			typename std::enable_if<std::is_same<T, float>::value ||
			std::is_same<T, double>::value
			>::type> : std::true_type {};

		// make_unsigned
		template <class T>
		struct make_unsigned_impl { using type = typename std::make_unsigned<T>::type; };


		template <class T>
		struct make_unsigned
			: make_unsigned_impl<typename std::remove_cv<T>::type>
		{};

		// is_integer

		template <class T, class Enable = void>
		struct is_integer : std::false_type {};

		template <class T>
		struct is_integer<T, typename std::enable_if<integer_limits<T>::is_specialized>::type> : std::true_type {};

		// is_signed_integer

		template <class T, class Enable = void>
		struct is_signed_integer : std::false_type {};

		template <class T>
		struct is_signed_integer<T, typename std::enable_if<integer_limits<T>::is_specialized &&
			integer_limits<T>::is_signed>::type> : std::true_type {};

		// is_unsigned_integer

		template <class T, class Enable = void>
		struct is_unsigned_integer : std::false_type {};

		template <class T>
		struct is_unsigned_integer<T,
			typename std::enable_if<integer_limits<T>::is_specialized &&
			!integer_limits<T>::is_signed>::type> : std::true_type {};

		// is_primitive

		template <class T, class Enable = void>
		struct is_primitive : std::false_type {};

		template <class T>
		struct is_primitive<T,
			typename std::enable_if<is_integer<T>::value ||
			is_bool<T>::value ||
			std::is_floating_point<T>::value
			>::type> : std::true_type {};

		// Containers

		template <class Container>
		using
			container_npos_t = decltype(Container::npos);

		template <class Container>
		using
			container_allocator_type_t = typename Container::allocator_type;

		template <class Container>
		using
			container_mapped_type_t = typename Container::mapped_type;

		template <class Container>
		using
			container_key_type_t = typename Container::key_type;

		template <class Container>
		using
			container_value_type_t = typename std::iterator_traits<typename Container::iterator>::value_type;

		template <class Container>
		using
			container_char_traits_t = typename Container::traits_type::char_type;

		template<class Container>
		using
			container_push_back_t = decltype(std::declval<Container>().push_back(std::declval<typename Container::value_type>()));

		template<class Container>
		using
			container_push_front_t = decltype(std::declval<Container>().push_front(std::declval<typename Container::value_type>()));

		template<class Container>
		using
			container_insert_t = decltype(std::declval<Container>().insert(std::declval<typename Container::value_type>()));

		template<class Container>
		using
			container_reserve_t = decltype(std::declval<Container>().reserve(typename Container::size_type()));

		template<class Container>
		using
			container_data_t = decltype(std::declval<Container>().data());

		template<class Container>
		using
			container_size_t = decltype(std::declval<Container>().size());

		// is_string_or_string_view

		template <class T, class Enable = void>
		struct is_string_or_string_view : std::false_type {};

		template <class T>
		struct is_string_or_string_view<T,
			typename std::enable_if<is_character<typename T::value_type>::value &&
			is_detected_exact<typename T::value_type, container_char_traits_t, T>::value &&
			is_detected<container_npos_t, T>::value
			>::type> : std::true_type {};

		// is_basic_string

		template <class T, class Enable = void>
		struct is_basic_string : std::false_type {};

		template <class T>
		struct is_basic_string<T,
			typename std::enable_if<is_string_or_string_view<T>::value &&
			is_detected<container_allocator_type_t, T>::value
			>::type> : std::true_type {};

		// is_basic_string_view

		template <class T, class Enable = void>
		struct is_basic_string_view : std::false_type {};

		template <class T>
		struct is_basic_string_view<T,
			typename std::enable_if<is_string_or_string_view<T>::value &&
			!is_detected<container_allocator_type_t, T>::value
			>::type> : std::true_type {};

		// is_map_like

		template <class T, class Enable = void>
		struct is_map_like : std::false_type {};

		template <class T>
		struct is_map_like<T,
			typename std::enable_if<is_detected<container_mapped_type_t, T>::value &&
			is_detected<container_allocator_type_t, T>::value &&
			is_detected<container_key_type_t, T>::value &&
			is_detected<container_value_type_t, T>::value
		>::type>
			: std::true_type {};

		// is_std_array
		template<class T>
		struct is_std_array : std::false_type {};

		template<class E, std::size_t N>
		struct is_std_array<std::array<E, N>> : std::true_type {};

		// is_list_like

		template <class T, class Enable = void>
		struct is_list_like : std::false_type {};

		template <class T>
		struct is_list_like<T,
			typename std::enable_if<is_detected<container_value_type_t, T>::value &&
			is_detected<container_allocator_type_t, T>::value &&
			!is_std_array<T>::value &&
			!is_detected_exact<typename T::value_type, container_char_traits_t, T>::value &&
			!is_map_like<T>::value
		>::type>
			: std::true_type {};

		// is_constructible_from_const_pointer_and_size

		template <class T, class Enable = void>
		struct is_constructible_from_const_pointer_and_size : std::false_type {};

		template <class T>
		struct is_constructible_from_const_pointer_and_size<T,
			typename std::enable_if<std::is_constructible<T, typename T::const_pointer, typename T::size_type>::value
		>::type>
			: std::true_type {};

		// has_reserve

		template<class Container>
		using
			has_reserve = is_detected<container_reserve_t, Container>;

		// is_back_insertable

		template<class Container>
		using
			is_back_insertable = is_detected<container_push_back_t, Container>;

		// is_front_insertable

		template<class Container>
		using
			is_front_insertable = is_detected<container_push_front_t, Container>;

		// is_insertable

		template<class Container>
		using
			is_insertable = is_detected<container_insert_t, Container>;

		// has_data, has_data_exact

		template<class Container>
		using
			has_data = is_detected<container_data_t, Container>;

		template<class Ret, class Container>
		using
			has_data_exact = is_detected_exact<Ret, container_data_t, Container>;

		// has_size

		template<class Container>
		using
			has_size = is_detected<container_size_t, Container>;

		// has_data_and_size

		template<class Container>
		struct has_data_and_size
		{
			static constexpr bool value = has_data<Container>::value && has_size<Container>::value;
		};

		// is_byte_sequence

		template <class Container, class Enable = void>
		struct is_byte_sequence : std::false_type {};

		template <class Container>
		struct is_byte_sequence<Container,
			typename std::enable_if<has_data_exact<const typename Container::value_type*, const Container>::value &&
			has_size<Container>::value &&
			is_byte<typename Container::value_type>::value
			>::type> : std::true_type {};

		// is_char_sequence

		template <class Container, class Enable = void>
		struct is_char_sequence : std::false_type {};

		template <class Container>
		struct is_char_sequence<Container,
			typename std::enable_if<has_data_exact<const typename Container::value_type*, const Container>::value &&
			has_size<Container>::value &&
			is_character<typename Container::value_type>::value
			>::type> : std::true_type {};

		// is_sequence_of

		template <class Container, class ValueT, class Enable = void>
		struct is_sequence_of : std::false_type {};

		template <class Container, class ValueT>
		struct is_sequence_of<Container, ValueT,
			typename std::enable_if<has_data_exact<const typename Container::value_type*, const Container>::value &&
			has_size<Container>::value &&
			std::is_same<typename Container::value_type, ValueT>::value
			>::type> : std::true_type {};

		// is_back_insertable_byte_container

		template <class Container, class Enable = void>
		struct is_back_insertable_byte_container : std::false_type {};

		template <class Container>
		struct is_back_insertable_byte_container<Container,
			typename std::enable_if<is_back_insertable<Container>::value &&
			is_byte<typename Container::value_type>::value
			>::type> : std::true_type {};

		// is_back_insertable_char_container

		template <class Container, class Enable = void>
		struct is_back_insertable_char_container : std::false_type {};

		template <class Container>
		struct is_back_insertable_char_container<Container,
			typename std::enable_if<is_back_insertable<Container>::value &&
			is_character<typename Container::value_type>::value
			>::type> : std::true_type {};

		// is_back_insertable_container_of

		template <class Container, class ValueT, class Enable = void>
		struct is_back_insertable_container_of : std::false_type {};

		template <class Container, class ValueT>
		struct is_back_insertable_container_of<Container, ValueT,
			typename std::enable_if<is_back_insertable<Container>::value &&
			std::is_same<typename Container::value_type, ValueT>::value
			>::type> : std::true_type {};

		// is_c_array

		template<class T>
		struct is_c_array : std::false_type {};

		template<class T>
		struct is_c_array<T[]> : std::true_type {};

		template<class T, std::size_t N>
		struct is_c_array<T[N]> : std::true_type {};

		namespace impl {

			template<class C, class Enable = void>
			struct is_typed_array : std::false_type {};

			template<class T>
			struct is_typed_array
				<
				T,
				typename std::enable_if<is_list_like<T>::value &&
				(std::is_same<typename std::decay<typename T::value_type>::type, uint8_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, uint16_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, uint32_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, uint64_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, int8_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, int16_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, int32_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, int64_t>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, float>::value ||
					std::is_same<typename std::decay<typename T::value_type>::type, double>::value)>::type
				> : std::true_type{};

		} // namespace impl

		template <typename T>
		using is_typed_array = impl::is_typed_array<typename std::decay<T>::type>;

		// is_compatible_element

		template<class Container, class Element, class Enable = void>
		struct is_compatible_element : std::false_type {};

		template<class Container, class Element>
		struct is_compatible_element
			<
			Container, Element,
			typename std::enable_if<has_data<Container>::value>::type>
			: std::is_convertible< typename std::remove_pointer<decltype(std::declval<Container>().data())>::type(*)[], Element(*)[]>
		{};

		template<typename T>
		using
			construct_from_string_t = decltype(T(std::string{}));


		template<class T>
		using
			is_constructible_from_string = is_detected<construct_from_string_t, T>;

		template<typename T, typename Data, typename Size>
		using
			construct_from_data_size_t = decltype(T(static_cast<Data>(nullptr), Size{}));


		template<class T, typename Data, typename Size>
		using
			is_constructible_from_data_size = is_detected<construct_from_data_size_t, T, Data, Size>;

		// is_unary_function_object
		// is_unary_function_object_exact

		template<class FunctionObject, class Arg>
		using
			unary_function_object_t = decltype(std::declval<FunctionObject>()(std::declval<Arg>()));

		template<class FunctionObject, class Arg>
		using
			is_unary_function_object = is_detected<unary_function_object_t, FunctionObject, Arg>;

		template<class FunctionObject, class T, class Arg>
		using
			is_unary_function_object_exact = is_detected_exact<T, unary_function_object_t, FunctionObject, Arg>;

		// is_binary_function_object
		// is_binary_function_object_exact

		template<class FunctionObject, class Arg1, class Arg2>
		using
			binary_function_object_t = decltype(std::declval<FunctionObject>()(std::declval<Arg1>(), std::declval<Arg2>()));

		template<class FunctionObject, class Arg1, class Arg2>
		using
			is_binary_function_object = is_detected<binary_function_object_t, FunctionObject, Arg1, Arg2>;

		template<class FunctionObject, class T, class Arg1, class Arg2>
		using
			is_binary_function_object_exact = is_detected_exact<T, binary_function_object_t, FunctionObject, Arg1, Arg2>;

		template <class Source, class Enable = void>
		struct is_convertible_to_string_view : std::false_type {};

		template <class Source>
		struct is_convertible_to_string_view<Source, typename std::enable_if<is_string_or_string_view<Source>::value ||
			is_cstring<Source>::value
		>::type> : std::true_type {};

	} // type_traits
    enum class to_integer_errc : uint8_t {success=0, overflow, invalid_digit, invalid_number};

    class to_integer_error_category_impl
       : public std::error_category
    {
    public:
        const char* name() const noexcept override
        {
            return "nicehero/to_integer_unchecked";
        }
        std::string message(int ev) const override
        {
            switch (static_cast<to_integer_errc>(ev))
            {
                case to_integer_errc::overflow:
                    return "Integer overflow";
                case to_integer_errc::invalid_digit:
                    return "Invalid digit";
                case to_integer_errc::invalid_number:
                    return "Invalid number";
                default:
                    return "Unknown to_integer_unchecked error";
            }
        }
    };

    inline
    const std::error_category& to_integer_error_category()
    {
      static to_integer_error_category_impl instance;
      return instance;
    }

    inline 
    std::error_code make_error_code(to_integer_errc e)
    {
        return std::error_code(static_cast<int>(e),to_integer_error_category());
    }

} // namespace nicehero

namespace std {
    template<>
    struct is_error_code_enum<nicehero::to_integer_errc> : public true_type
    {
    };
}

namespace nicehero {

template <class T,class CharT>
struct to_integer_result
{
    const CharT* ptr;
    to_integer_errc ec;
    constexpr to_integer_result(const CharT* ptr)
        : ptr(ptr), ec(to_integer_errc())
    {
    }
    constexpr to_integer_result(const CharT* ptr, to_integer_errc ec)
        : ptr(ptr), ec(ec)
    {
    }

    to_integer_result(const to_integer_result&) = default;

    to_integer_result& operator=(const to_integer_result&) = default;

    constexpr explicit operator bool() const noexcept
    {
        return ec == to_integer_errc();
    }
    std::error_code error_code() const
    {
        return make_error_code(ec);
    }
};

enum class integer_chars_format : uint8_t {decimal=1,hex};
enum class integer_chars_state {initial,minus,integer,binary,octal,decimal,base16};

template <class CharT>
bool is_base10(const CharT* s, std::size_t length)
{
    integer_chars_state state = integer_chars_state::initial;

    const CharT* end = s + length; 
    for (;s < end; ++s)
    {
        switch(state)
        {
            case integer_chars_state::initial:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        state = integer_chars_state::decimal;
                        break;
                    case '-':
                        state = integer_chars_state::minus;
                        break;
                    default:
                        return false;
                }
                break;
            }
            case integer_chars_state::minus:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        state = integer_chars_state::decimal;
                        break;
                    default:
                        return false;
                }
                break;
            }
            case integer_chars_state::decimal:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        break;
                    default:
                        return false;
                }
                break;
            }
            default:
                break;
        }
    }
    return state == integer_chars_state::decimal ? true : false;
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && !type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer_decimal(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    integer_chars_state state = integer_chars_state::initial;

    const CharT* end = s + length; 
    while (s < end)
    {
        switch(state)
        {
            case integer_chars_state::initial:
            {
                switch(*s)
                {
                    case '0':
                        if (++s == end)
                        {
                            return (++s == end) ? to_integer_result<T,CharT>(s) : to_integer_result<T, CharT>(s, to_integer_errc());
                        }
                        else
                        {
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                        }
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': // Must be decimal
                        state = integer_chars_state::decimal;
                        break;
                    default:
                        return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                }
                break;
            }
            case integer_chars_state::decimal:
            {
                static constexpr T max_value = (type_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_10 = max_value / 10;
                for (; s < end; ++s)
                {
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            x = static_cast<T>(*s) - static_cast<T>('0');
                            break;
                        default:
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                    }
                    if (n > max_value_div_10)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n = n * 10;
                    if (n > max_value - x)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n += x;
                }
                break;
            }
            default:
                break;
        }
    }
    return (state == integer_chars_state::initial) ? to_integer_result<T,CharT>(s, to_integer_errc::invalid_number) : to_integer_result<T,CharT>(s, to_integer_errc());
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer_decimal(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    if (length == 0)
    {
        return to_integer_result<T,CharT>(s, to_integer_errc::invalid_number);
    }

    bool is_negative = *s == '-' ? true : false;
    if (is_negative)
    {
        ++s;
        --length;
    }

    using U = typename type_traits::make_unsigned<T>::type;

    U u;
    auto ru = to_integer_decimal(s, length, u);
    if (ru.ec != to_integer_errc())
    {
        return to_integer_result<T,CharT>(ru.ptr, ru.ec);
    }
    if (is_negative)
    {
        if (u > static_cast<U>(-((type_traits::integer_limits<T>::lowest)()+T(1))) + U(1))
        {
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc::overflow);
        }
        else
        {
            n = static_cast<T>(U(0) - u);
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc());
        }
    }
    else
    {
        if (u > static_cast<U>((type_traits::integer_limits<T>::max)()))
        {
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc::overflow);
        }
        else
        {
            n = static_cast<T>(u);
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc());
        }
    }
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && !type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer_base16(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    integer_chars_state state = integer_chars_state::initial;

    const CharT* end = s + length; 
    while (s < end)
    {
        switch(state)
        {
            case integer_chars_state::initial:
            {
                switch(*s)
                {
                    case '0':
                        if (++s == end)
                        {
                            return (++s == end) ? to_integer_result<T,CharT>(s) : to_integer_result<T, CharT>(s, to_integer_errc());
                        }
                        else
                        {
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                        }
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': // Must be base16
                    case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                        state = integer_chars_state::base16;
                        break;
                    default:
                        return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                }
                break;
            }
            case integer_chars_state::base16:
            {
                static constexpr T max_value = (type_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_16 = max_value / 16;
                for (; s < end; ++s)
                {
                    CharT c = *s;
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            x = c - '0';
                            break;
                        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                            x = c - ('a' - 10);
                            break;
                        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                            x = c - ('A' - 10);
                            break;
                        default:
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                    }
                    if (n > max_value_div_16)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n = n * 16;
                    if (n > max_value - x)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n += x;
                }
                break;
            }
            default:
                break;
        }
    }
    return (state == integer_chars_state::initial) ? to_integer_result<T,CharT>(s, to_integer_errc::invalid_number) : to_integer_result<T,CharT>(s, to_integer_errc());
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer_base16(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    if (length == 0)
    {
        return to_integer_result<T,CharT>(s, to_integer_errc::invalid_number);
    }

    bool is_negative = *s == '-' ? true : false;
    if (is_negative)
    {
        ++s;
        --length;
    }

    using U = typename type_traits::make_unsigned<T>::type;

    U u;
    auto ru = to_integer_base16(s, length, u);
    if (ru.ec != to_integer_errc())
    {
        return to_integer_result<T,CharT>(ru.ptr, ru.ec);
    }
    if (is_negative)
    {
        if (u > static_cast<U>(-((type_traits::integer_limits<T>::lowest)()+T(1))) + U(1))
        {
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc::overflow);
        }
        else
        {
            n = static_cast<T>(U(0) - u);
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc());
        }
    }
    else
    {
        if (u > static_cast<U>((type_traits::integer_limits<T>::max)()))
        {
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc::overflow);
        }
        else
        {
            n = static_cast<T>(u);
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc());
        }
    }
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && !type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    integer_chars_state state = integer_chars_state::initial;

    const CharT* end = s + length; 
    while (s < end)
    {
        switch(state)
        {
            case integer_chars_state::initial:
            {
                switch(*s)
                {
                    case '0':
                        state = integer_chars_state::integer; // Could be binary, octal, hex 
                        ++s;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': // Must be decimal
                        state = integer_chars_state::decimal;
                        break;
                    default:
                        return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                }
                break;
            }
            case integer_chars_state::integer:
            {
                switch(*s)
                {
                    case 'b':case 'B':
                    {
                        state = integer_chars_state::binary;
                        ++s;
                        break;
                    }
                    case 'x':case 'X':
                    {
                        state = integer_chars_state::base16;
                        ++s;
                        break;
                    }
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    {
                        state = integer_chars_state::octal;
                        break;
                    }
                    default:
                        return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                }
                break;
            }
            case integer_chars_state::binary:
            {
                static constexpr T max_value = (type_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_2 = max_value / 2;
                for (; s < end; ++s)
                {
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':
                            x = static_cast<T>(*s) - static_cast<T>('0');
                            break;
                        default:
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                    }
                    if (n > max_value_div_2)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n = n * 2;
                    if (n > max_value - x)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n += x;
                }
                break;
            }
            case integer_chars_state::octal:
            {
                static constexpr T max_value = (type_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_8 = max_value / 8;
                for (; s < end; ++s)
                {
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':
                            x = static_cast<T>(*s) - static_cast<T>('0');
                            break;
                        default:
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                    }
                    if (n > max_value_div_8)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n = n * 8;
                    if (n > max_value - x)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n += x;
                }
                break;
            }
            case integer_chars_state::decimal:
            {
                static constexpr T max_value = (type_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_10 = max_value / 10;
                for (; s < end; ++s)
                {
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            x = static_cast<T>(*s) - static_cast<T>('0');
                            break;
                        default:
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                    }
                    if (n > max_value_div_10)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n = n * 10;
                    if (n > max_value - x)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n += x;
                }
                break;
            }
            case integer_chars_state::base16:
            {
                static constexpr T max_value = (type_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_16 = max_value / 16;
                for (; s < end; ++s)
                {
                    CharT c = *s;
                    T x = 0;
                    switch (c)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            x = c - '0';
                            break;
                        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                            x = c - ('a' - 10);
                            break;
                        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                            x = c - ('A' - 10);
                            break;
                        default:
                            return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
                    }
                    if (n > max_value_div_16)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }
                    n = n * 16;
                    if (n > max_value - x)
                    {
                        return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
                    }

                    n += x;
                }
                break;
            }
            default:
                break;
        }
    }
    return (state == integer_chars_state::initial) ? to_integer_result<T,CharT>(s, to_integer_errc::invalid_number) : to_integer_result<T,CharT>(s, to_integer_errc());
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    if (length == 0)
    {
        return to_integer_result<T,CharT>(s, to_integer_errc::invalid_number);
    }

    bool is_negative = *s == '-' ? true : false;
    if (is_negative)
    {
        ++s;
        --length;
    }

    using U = typename type_traits::make_unsigned<T>::type;

    U u;
    auto ru = to_integer(s, length, u);
    if (ru.ec != to_integer_errc())
    {
        return to_integer_result<T,CharT>(ru.ptr, ru.ec);
    }
    if (is_negative)
    {
        if (u > static_cast<U>(-((type_traits::integer_limits<T>::lowest)()+T(1))) + U(1))
        {
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc::overflow);
        }
        else
        {
            n = static_cast<T>(U(0) - u);
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc());
        }
    }
    else
    {
        if (u > static_cast<U>((type_traits::integer_limits<T>::max)()))
        {
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc::overflow);
        }
        else
        {
            n = static_cast<T>(u);
            return to_integer_result<T,CharT>(ru.ptr, to_integer_errc());
        }
    }
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized,to_integer_result<T,CharT>>::type
to_integer(const CharT* s, T& n)
{
    return to_integer<T,CharT>(s, std::char_traits<CharT>::length(s), n);
}

// Precondition: s satisfies

// digit
// digit1-digits 
// - digit
// - digit1-digits

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && !type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer_unchecked(const CharT* s, std::size_t length, T& n)
{
    static_assert(type_traits::integer_limits<T>::is_specialized, "Integer type not specialized");

    n = 0;
    const CharT* end = s + length; 
    if (*s == '-')
    {
        static constexpr T min_value = (type_traits::integer_limits<T>::lowest)();
        static constexpr T min_value_div_10 = min_value / 10;
        ++s;
        for (; s < end; ++s)
        {
            T x = (T)*s - (T)('0');
            if (n < min_value_div_10)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }
            n = n * 10;
            if (n < min_value + x)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }

            n -= x;
        }
    }
    else
    {
        static constexpr T max_value = (type_traits::integer_limits<T>::max)();
        static constexpr T max_value_div_10 = max_value / 10;
        for (; s < end; ++s)
        {
            T x = static_cast<T>(*s) - static_cast<T>('0');
            if (n > max_value_div_10)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }
            n = n * 10;
            if (n > max_value - x)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }

            n += x;
        }
    }

    return to_integer_result<T,CharT>(s, to_integer_errc());
}

// Precondition: s satisfies

// digit
// digit1-digits 
// - digit
// - digit1-digits

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
to_integer_unchecked(const CharT* s, std::size_t length, T& n)
{
    static_assert(type_traits::integer_limits<T>::is_specialized, "Integer type not specialized");

    n = 0;

    const CharT* end = s + length; 
    if (*s == '-')
    {
        static constexpr T min_value = (type_traits::integer_limits<T>::lowest)();
        static constexpr T min_value_div_10 = min_value / 10;
        ++s;
        for (; s < end; ++s)
        {
            T x = (T)*s - (T)('0');
            if (n < min_value_div_10)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }
            n = n * 10;
            if (n < min_value + x)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }

            n -= x;
        }
    }
    else
    {
        static constexpr T max_value = (type_traits::integer_limits<T>::max)();
        static constexpr T max_value_div_10 = max_value / 10;
        for (; s < end; ++s)
        {
            T x = static_cast<T>(*s) - static_cast<T>('0');
            if (n > max_value_div_10)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }
            n = n * 10;
            if (n > max_value - x)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }

            n += x;
        }
    }

    return to_integer_result<T,CharT>(s, to_integer_errc());
}

// base16_to_integer

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
base16_to_integer(const CharT* s, std::size_t length, T& n)
{
    static_assert(type_traits::integer_limits<T>::is_specialized, "Integer type not specialized");
    n = 0;

    const CharT* end = s + length; 
    if (*s == '-')
    {
        static constexpr T min_value = (type_traits::integer_limits<T>::lowest)();
        static constexpr T min_value_div_16 = min_value / 16;
        ++s;
        for (; s < end; ++s)
        {
            CharT c = *s;
            T x = 0;
            switch (c)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    x = c - '0';
                    break;
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    x = c - ('a' - 10);
                    break;
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    x = c - ('A' - 10);
                    break;
                default:
                    return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
            }
            if (n < min_value_div_16)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }
            n = n * 16;
            if (n < min_value + x)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }
            n -= x;
        }
    }
    else
    {
        static constexpr T max_value = (type_traits::integer_limits<T>::max)();
        static constexpr T max_value_div_16 = max_value / 16;
        for (; s < end; ++s)
        {
            CharT c = *s;
            T x = 0;
            switch (c)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    x = c - '0';
                    break;
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    x = c - ('a' - 10);
                    break;
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    x = c - ('A' - 10);
                    break;
                default:
                    return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
            }
            if (n > max_value_div_16)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }
            n = n * 16;
            if (n > max_value - x)
            {
                return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
            }

            n += x;
        }
    }

    return to_integer_result<T,CharT>(s, to_integer_errc());
}

template <class T, class CharT>
typename std::enable_if<type_traits::integer_limits<T>::is_specialized && !type_traits::integer_limits<T>::is_signed,to_integer_result<T,CharT>>::type
base16_to_integer(const CharT* s, std::size_t length, T& n)
{
    static_assert(type_traits::integer_limits<T>::is_specialized, "Integer type not specialized");

    n = 0;
    const CharT* end = s + length; 

    static constexpr T max_value = (type_traits::integer_limits<T>::max)();
    static constexpr T max_value_div_16 = max_value / 16;
    for (; s < end; ++s)
    {
        CharT c = *s;
        T x = *s;
        switch (c)
        {
            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                x = c - '0';
                break;
            case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                x = c - ('a' - 10);
                break;
            case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                x = c - ('A' - 10);
                break;
            default:
                return to_integer_result<T,CharT>(s, to_integer_errc::invalid_digit);
        }
        if (n > max_value_div_16)
        {
            return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
        }
        n = n * 16;
        if (n > max_value - x)
        {
            return to_integer_result<T,CharT>(s, to_integer_errc::overflow);
        }

        n += x;
    }

    return to_integer_result<T,CharT>(s, to_integer_errc());
}

#if defined(NICEHERO_HAS_MSC_STRTOD_L)

class to_double_t
{
private:
    _locale_t locale_;
public:
    to_double_t()
    {
        locale_ = _create_locale(LC_NUMERIC, "C");
    }
    ~to_double_t() noexcept
    {
        _free_locale(locale_);
    }

    to_double_t(const to_double_t&)
    {
        locale_ = _create_locale(LC_NUMERIC, "C");
    }

    to_double_t& operator=(const to_double_t&) 
    {
        // Don't assign locale
        return *this;
    }

    char get_decimal_point() const
    {
        return '.';
    }

    template <class CharT>
    typename std::enable_if<std::is_same<CharT,char>::value,double>::type
    operator()(const CharT* s, std::size_t) const
    {
		isok = true;
		CharT *end = nullptr;
        double val = _strtod_l(s, &end, locale_);
        if (s == end)
        {
			isok = false;
		}
        return val;
    }

    template <class CharT>
    typename std::enable_if<std::is_same<CharT,wchar_t>::value,double>::type
    operator()(const CharT* s, std::size_t) const
    {
		isok = true;
        CharT *end = nullptr;
        double val = _wcstod_l(s, &end, locale_);
        if (s == end)
        {
			isok = false;
        }
        return val;
    }
	mutable bool isok = true;
};

#elif defined(NICEHERO_HAS_STRTOLD_L)

class to_double_t
{
private:
    locale_t locale_;
public:
    to_double_t()
    {
        locale_ = newlocale(LC_ALL_MASK, "C", (locale_t) 0);
    }
    ~to_double_t() noexcept
    {
        freelocale(locale_);
    }

    to_double_t(const to_double_t&)
    {
        locale_ = newlocale(LC_ALL_MASK, "C", (locale_t) 0);
    }

    to_double_t& operator=(const to_double_t&) 
    {
        return *this;
    }

    char get_decimal_point() const
    {
        return '.';
    }

    template <class CharT>
    typename std::enable_if<std::is_same<CharT,char>::value,double>::type
    operator()(const CharT* s, std::size_t) const
    {
		isok = true;
		char *end = nullptr;
        double val = strtold_l(s, &end, locale_);
        if (s == end)
        {
			isok = false;
		}
        return val;
    }

    template <class CharT>
    typename std::enable_if<std::is_same<CharT,wchar_t>::value,double>::type
    operator()(const CharT* s, std::size_t) const
    {
		isok = true;
		CharT *end = nullptr;
        double val = wcstold_l(s, &end, locale_);
        if (s == end)
        {
			isok = false;
		}
        return val;
    }
	mutable bool isok = true;
};

#else
class to_double_t
{
private:
    std::vector<char> buffer_;
    char decimal_point_;
public:
    to_double_t()
        : buffer_()
    {
        struct lconv * lc = localeconv();
        if (lc != nullptr && lc->decimal_point[0] != 0)
        {
            decimal_point_ = lc->decimal_point[0];    
        }
        else
        {
            decimal_point_ = '.'; 
        }
        buffer_.reserve(100);
    }

    to_double_t(const to_double_t&) = default;
    to_double_t& operator=(const to_double_t&) = default;

    char get_decimal_point() const
    {
        return decimal_point_;
    }

    template <class CharT>
    typename std::enable_if<std::is_same<CharT,char>::value,double>::type
    operator()(const CharT* s, std::size_t /*length*/) const
    {
		isok = true;
		CharT *end = nullptr;
        double val = strtod(s, &end);
        if (s == end)
        {
			isok = false;
		}
        return val;
    }

    template <class CharT>
    typename std::enable_if<std::is_same<CharT,wchar_t>::value,double>::type
    operator()(const CharT* s, std::size_t /*length*/) const
    {
		isok = true;
		CharT *end = nullptr;
        double val = wcstod(s, &end);
        if (s == end)
        {
			isok = false;
		}
        return val;
    }
	mutable bool isok = true;
};
#endif

}

#endif
