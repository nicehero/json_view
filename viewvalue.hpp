#ifndef ___NICEHERO_VIEWVALUE___
#define ___NICEHERO_VIEWVALUE___
#include <string>
#include <memory>
#include <stdint.h>
#include <sstream>
#include <cstring>
#include "write_number.hpp"
#include <vector>

#if !defined(NICEHERO_HAS_2017)
#  if defined(__clang__)
#   if (__cplusplus >= 201703)
#     define NICEHERO_HAS_2017 1
#   endif // (__cplusplus >= 201703)
#  endif // defined(__clang__)
#  if defined(__GNUC__)
#   if (__GNUC__ >= 7)
#    if (__cplusplus >= 201703)
#     define NICEHERO_HAS_2017 1
#    endif // (__cplusplus >= 201703)
#   endif // (__GNUC__ >= 7)
#  endif // defined(__GNUC__)
#  if defined(_MSC_VER)
#   if (_MSC_VER >= 1910 && _MSVC_LANG >= 201703)
#    define NICEHERO_HAS_2017 1
#   endif // (_MSC_VER >= 1910 && MSVC_LANG >= 201703)
#  endif // defined(_MSC_VER)
#endif
#ifdef NICEHERO_HAS_2017
#include <string_view>
#endif
namespace nicehero {

	enum class value_type : uint8_t
	{
		string_view_value,
		uint64_value,
		uint32_value,
		int64_value,
		int32_value,
		double_value,
		string_short_value,
		string_long_value,
		boolean_value,
	};
	class viewvalue;
	struct copy_str_ref {
		const char* m_str;
		uint32_t m_len;

		explicit copy_str_ref(const char* s, uint32_t len = 0) {
			if (s == nullptr) {
				s = "";
				len = 0;
			}
			m_str = s;
			m_len = len;
		}
		explicit copy_str_ref(const viewvalue& v);
	};

	class viewvalue {
	public:
		static constexpr size_t string_short_size = 15;
		viewvalue() {
			m_start = nullptr;
			m_len = 0;
			get_type_ref() = value_type::string_view_value;
		}

		viewvalue(const char* s, uint32_t len = 0) {
			m_start = s;
			m_len = len;
			get_type_ref() = value_type::string_view_value;
		}
		viewvalue(const copy_str_ref& cpystr) {
			init(cpystr);
		}
		viewvalue(uint64_t v) {
			m_ui64 = v;
			get_type_ref() = value_type::uint64_value;
		}
		viewvalue(int64_t v) {
			m_i64 = v;
			get_type_ref() = value_type::uint32_value;
		}
		viewvalue(uint32_t v) {
			m_ui32 = v;
			get_type_ref() = value_type::int64_value;
		}
		viewvalue(int32_t v) {
			m_i32 = v;
			get_type_ref() = value_type::int32_value;
		}
		viewvalue(double v) {
			m_double = v;
			get_type_ref() = value_type::double_value;
		}
		viewvalue(bool v) {
			m_boolean = v;
			get_type_ref() = value_type::boolean_value;
		}
		viewvalue(const std::string& str_) {
			init(copy_str_ref(str_.c_str(), (uint32_t)str_.size()));
		}
		void init(double v) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			m_double = v;
			get_type_ref() = value_type::double_value;
		}
		void init(uint64_t v) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			m_ui64 = v;
			get_type_ref() = value_type::uint64_value;
		}
		void init(bool v) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			m_boolean = v;
			get_type_ref() = value_type::boolean_value;
		}
		void init(uint32_t v) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			m_ui32 = v;
			get_type_ref() = value_type::uint32_value;
		}
		void init(int64_t v) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			m_i64 = v;
			get_type_ref() = value_type::int64_value;
		}
		void init(int32_t v) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			m_i32 = v;
			get_type_ref() = value_type::int32_value;
		}
		void init(const copy_str_ref& cpystr) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			auto len_ = cpystr.m_len;
			if (len_ < 1) {
				get_type_ref() = value_type::string_short_value;
				bool isOver = false;
				for (size_t i = 0; i < string_short_size; ++i) {
					m_strshort[i] = cpystr.m_str[i];
					if (cpystr.m_str[i] == '\0') {
						isOver = true;
						break;
					}
				}
				if (isOver) {
					return;
				}
				if (cpystr.m_str[string_short_size] == '\0') {
					return;
				}
				get_type_ref() = value_type::string_long_value;
				len_ = string_short_size + 1;
				for (size_t i = string_short_size + 1;; ++i) {
					if (cpystr.m_str[i] == '\0') {
						break;
					}
					++len_;
				}
				m_strlong = new char[len_ + 1];
				memcpy(m_strlong, cpystr.m_str, len_ + 1);
				m_strlong[len_] = '\0';
				return;
			}
			if (len_ <= string_short_size) {
				memcpy(m_strshort, cpystr.m_str, len_);
				get_type_ref() = value_type::string_short_value;
				if (len_ < string_short_size) {
					m_strshort[string_short_size - 1] = '\0';
					m_strshort[len_] = '\0';
				}
			}
			else {
				m_strlong = new char[len_ + 1];
				m_len = len_;
				memcpy(m_strlong, cpystr.m_str, len_ + 1);
				m_strlong[len_] = '\0';
				get_type_ref() = value_type::string_long_value;
			}
		}
		void init(const char* start, uint32_t len = 0) {
			if (get_type_ref() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
			if (start == nullptr) {
				start = "";
				len = 0;
			}
			m_start = start;
			m_len = len;
			get_type_ref() = value_type::string_view_value;
		}
		void init(const std::string& s) {
			init(copy_str_ref(s.c_str(), (uint32_t)s.size()));
		}
		void init(const viewvalue& v) {
			*this = v;
		}
		value_type& get_type_ref() {
			return *(value_type*)(m_strshort + string_short_size);
		}

		value_type get_type() const {
			return value_type(m_strshort[string_short_size]);
		}

		viewvalue(viewvalue&& rhs) noexcept {
			memcpy(m_strshort, rhs.m_strshort, string_short_size + 1);
			if (rhs.get_type_ref() == value_type::string_long_value) {
				rhs.m_strlong = nullptr;
			}
		}
		viewvalue& operator=(viewvalue&& rhs) noexcept
		{
			memcpy(m_strshort, rhs.m_strshort, string_short_size + 1);
			if (rhs.get_type() == value_type::string_long_value) {
				rhs.m_strlong = nullptr;
			}
			return *this;
		}

		viewvalue(const viewvalue& rhs) {
			auto t = rhs.get_type();
			if (t == value_type::string_long_value) {
				init(copy_str_ref(rhs.m_strlong, rhs.m_len));
			}
			else {
				memcpy(m_strshort, rhs.m_strshort, string_short_size + 1);
			}
		}
		~viewvalue() {
			if (get_type() == value_type::string_long_value && m_strlong) {
				delete[] m_strlong;
				m_strlong = nullptr;
			}
		}
	private:
	public:
#ifdef NICEHERO_HAS_2017
		std::string_view to_stringview() const {
			switch (get_type())
			{
			case value_type::string_view_value:
				if (m_len < 1) {
					return std::string_view(m_start);
				}
				return std::string_view(m_start, m_len);
				break;
			case value_type::string_short_value:
				return std::string_view(m_strshort);
				break;
			case value_type::string_long_value:
				return std::string_view(m_strlong, m_len);
				break;
			default:
				break;
			}
			return std::string_view();
		}
#endif
		bool operator ==(const viewvalue& other) const {
			//need optimize
#ifdef NICEHERO_HAS_2017
			auto l = to_stringview();
			auto r = other.to_stringview();
			if (l != "" && l == r) {
				return true;
			}
			else if (l != "" && r != "" && l != r) {
				return false;
			}
			else if (l == "" && r == "") {
				return true;
			}
			return false;
#endif
			return to_string() == other.to_string();
		}
		//only for string type return string size
		uint32_t write2cstrbuffer(std::vector<uint8_t>& o) {
			switch (get_type())
			{
			case value_type::string_view_value:
			{
				uint32_t len = 0;
				if (m_len < 1) {
					for (size_t i = 0;; ++ i) {
						o.push_back((uint8_t)m_start[i]);
						if (m_start[i] == 0x00) {
							len = (uint32_t)i;
							break;
						}
					}
					return len;
				}
				else {
					size_t oldSize = o.size();
					o.resize(oldSize + m_len + 1);
					memcpy(&o[oldSize], m_start, m_len);
					o.back() = 0x00;
					return m_len;
				}
			}
			case value_type::string_short_value:
			{
				uint32_t len = 0;
				for (size_t i = 0; i < string_short_size; ++i) {
					o.push_back((uint8_t)m_strshort[i]);
					if (m_strshort[i] == 0x00) {
						len = (uint32_t)i;
						break;
					}
				}
				if (o.back() != 0x00) {
					len = (uint32_t)string_short_size;
					o.push_back(0x00);
				}
				return len;
			}
			case value_type::string_long_value:
			{
				size_t oldSize = o.size();
				o.resize(oldSize + m_len + 1);
				memcpy(&o[oldSize], m_strlong, m_len + 1);
				return m_len;
			}
			default:
				break;
			}
			return 0;
		}
		char front() const {
			const viewvalue* p = this;
			if (p->get_type() == value_type::string_view_value) {
				if (p->m_len < 1) {
					return '\0';
				}
				return p->m_start[0];
			}
			else if (p->get_type() == value_type::string_short_value) {
				if (p->m_strshort[0] == '\0') {
					return '\0';
				}
				return p->m_strshort[0];
			}
			else if (p->get_type() == value_type::string_long_value) {
				return p->m_strlong[0];
			}
			return p->to_string().front();
		}
		//only for view
		char operator*() const {
			return *m_start;
		}
		//only for view
		inline viewvalue& operator++() {
			if (!eof()) {
				++m_start;
				--m_len;
				if (m_len < 1) {
					m_start = "";
				}
			}
			else {
				m_start = "";
				m_len = 0;
			}
			return *this;
		}
		//only for view
		inline viewvalue& operator+=(size_t len_) {
			uint32_t len = uint32_t(len_);
			if (m_len >= len) {
				m_start += len_;
				m_len -= len;
				if (m_len < 1) {
					m_start = "";
				}
			}
			else {
				m_start = "";
				m_len = 0;
			}
			return *this;
		}
		//only for string type
		inline operator const char* () const {
			switch (get_type()) {
			case value_type::string_view_value:
				return m_start;
			case value_type::string_long_value:
				return m_strlong;
			case value_type::string_short_value:
				return m_strshort;
			default:
				return "";
			}
			return "";
		}
		//only for view
		viewvalue operator+(size_t len_) const {
			uint32_t len = uint32_t(len_);
			if (get_type() == value_type::string_view_value && m_len >= len) {
				return viewvalue(m_start + len, m_len - len);
			}
			return viewvalue("");
		}
		//only for type view
		inline bool eof() const {
			return m_len < 1;
		}
		//only for type view
		inline bool eof(uint32_t add_len) const {
			return m_len < 1 + add_len;
		}
		char back() {
			viewvalue* p = this;
			if (p->get_type_ref() == value_type::string_view_value) {
				if (p->m_len < 1) {
					return '\0';
				}
				return p->m_start[p->m_len - 1];
			}
			else if (p->get_type_ref() == value_type::string_short_value) {
				for (size_t i = string_short_size - 1;; --i) {
					if (p->m_strshort[i] != '\0') {
						return p->m_strshort[i];
					}
					if (i == 0) {
						return '\0';
					}
				}
				return '\0';
			}
			else if (p->get_type_ref() == value_type::string_long_value) {
				return p->m_strlong[m_len - 1];
			}
			return p->to_string().back();
		}
		std::string to_string() const;
		std::string& parse2string(std::string& ss) const;

		viewvalue& operator=(const viewvalue& rhs) {
			auto t = rhs.get_type();
			if (t == value_type::string_long_value) {
				init(copy_str_ref(rhs.m_strlong, rhs.m_len));
			}
			else {
				memcpy(m_strshort, rhs.m_strshort, string_short_size);
				get_type_ref() = t;
			}
			return *this;
		}

		template <class T>
		viewvalue& operator=(const T& rhs) {
			init(rhs);
			return*this;
		}

		bool is_string() const {
			auto t = get_type();
			if (t == value_type::string_long_value || t == value_type::string_short_value || t == value_type::string_view_value) {
				return true;
			}
			return false;
		}

		//fields start 
		union {
			struct {
				const char* m_start;
				uint32_t m_len;
			};
			char m_strshort[string_short_size];
			char* m_strlong;
			uint64_t m_ui64;
			int64_t m_i64;
			uint32_t m_ui32;
			int32_t m_i32;
			double m_double;
			bool m_boolean;
		};
		//fields end
	};

	inline std::string viewvalue::to_string() const
	{
		std::string ss;
		return parse2string(ss);
	}

	inline std::string& viewvalue::parse2string(std::string& ss) const
	{
		auto* cur = this;
		auto t = cur->get_type();
		switch (t)
		{
		case value_type::uint64_value: {
			from_integer(cur->m_ui64, ss);
			break;
		}
		case value_type::uint32_value: {
			from_integer(cur->m_ui32, ss);
			break;
		}
		case value_type::int64_value: {
			from_integer(cur->m_i64, ss);
			break;
		}
		case value_type::int32_value: {
			from_integer(cur->m_i32, ss);
			break;
		}
		case value_type::double_value: {
			static write_double f(float_chars_format::general, 0);
			f(cur->m_double, ss);
// 			ss += std::to_string(cur->m_double);
			break;
		}
		case value_type::boolean_value: {
			if (cur->m_boolean) {
				ss += "true";
			}
			else {
				ss += "false";
			}
			break;
		}
		case value_type::string_short_value: {
			if (cur->m_strshort[viewvalue::string_short_size - 1] == '0') {
				ss += cur->m_strshort;
			}
			else {
				for (size_t i = 0; i < sizeof(cur->m_strshort); ++i) {
					if (cur->m_strshort[i] == '\0') {
						break;
					}
					ss += cur->m_strshort[i];
				}
			}
			break;
		}
		case value_type::string_long_value: {
			ss += cur->m_strlong;
			break;
		}
		case value_type::string_view_value: {
			if (cur->m_len > 0 && cur->m_start) {
#ifndef NICEHERO_HAS_2017
				for (uint32_t i = 0; i < m_len; ++i) {
					ss += cur->m_start[i];
				}
#else
				ss += std::string_view(cur->m_start, cur->m_len);
#endif
			}
			else if (cur->m_start) {
				ss += cur->m_start;
			}
			break;
		}
		default:
			break;
		}
		return ss;
	}

	inline std::string& operator <<  (std::string& receiver, const char* v) {
		return receiver += v;
	}

	inline std::string& operator <<  (std::string& receiver, const viewvalue& v) {
#ifdef NICEHERO_HAS_2017
		if (v.is_string()) {
			receiver += v.to_stringview();
			return receiver;
		}
#endif // NICEHERO_HAS_2017
		return v.parse2string(receiver);
	}

	inline std::ostream& operator << (std::ostream& cout_, const viewvalue& v) {
		auto t = v.get_type();
		if (t == value_type::uint64_value) {
			cout_ << v.m_ui64;
		}
		else if (t == value_type::uint32_value) {
			cout_ << v.m_i64;
		}
		else if (t == value_type::int64_value) {
			cout_ << v.m_ui32;
		}
		else if (t == value_type::int32_value) {
			cout_ << v.m_i32;
		}
		else if (t == value_type::double_value) {
			static write_double f(float_chars_format::general, 0);
			std::string ss;
			f(v.m_double, ss);
			cout_ << ss;
		}
		else if (t == value_type::boolean_value) {
			if (v.m_boolean) {
				cout_ << "true";
			}
			else {
				cout_ << "false";
			}
		}
		else if (t == value_type::string_short_value) {
			if (v.m_strshort[viewvalue::string_short_size - 1] == '\0') {
				cout_ << v.m_strshort;
			}
			else {
				for (size_t i = 0; i < sizeof(v.m_strshort); ++i) {
					if (v.m_strshort[i] == '\0') {
						break;
					}
					cout_ << v.m_strshort[i];
				}
			}
		}
		else if (t == value_type::string_long_value) {
			cout_ << v.m_strlong;
		}
		else if (v.m_start) {
			if (v.m_len > 0) {
#ifndef NICEHERO_HAS_2017
				for (uint32_t i = 0; i < v.m_len; ++i) {
					cout_ << v.m_start[i];
				}
#else
				cout_ << std::string_view(v.m_start, v.m_len);
#endif
			}
			else {
				cout_ << v.m_start;
			}
		}
		return cout_;
	}

	inline copy_str_ref::copy_str_ref(const viewvalue& v)
	{
		switch (v.get_type())
		{
		case value_type::string_view_value:
		{
			m_str = v.m_start;
			m_len = v.m_len;
			return;
		}
		case value_type::string_short_value:
		{
			m_str = v.m_strshort;
			m_len = 0;
			return;
		}
		case value_type::string_long_value:
		{
			m_str = v.m_strlong;
			m_len = v.m_len;
			return;
		}
		default:
			break;
		}
		m_str = "";
		m_len = 0;
	}
	inline namespace literals {
		inline viewvalue operator ""_vv(const char* s, std::size_t n)
		{
			return viewvalue(s, uint32_t(n));
		}
	}
}
namespace std {
	template<>
	struct hash<nicehero::viewvalue> {
		size_t operator()(const nicehero::viewvalue& rhs) const noexcept {
			using namespace nicehero;
#ifdef NICEHERO_HAS_2017
			switch (rhs.get_type())
			{
			case value_type::string_view_value:
			{
				if (rhs.m_len < 1) {
					return std::hash<string_view>{}(string_view(rhs.m_start));
				}
				else {
					return std::hash<string_view>{}(string_view(rhs.m_start, rhs.m_len));
				}
				break;
			}
			case value_type::string_short_value:
				return std::hash<string_view>{}(string_view(rhs.m_strshort));
				break;
			case value_type::string_long_value:
				return std::hash<string_view>{}(string_view(rhs.m_strlong));
				break;
			default:
				return std::hash<std::string>{}(rhs.to_string());
			}

#else
			return std::hash<std::string>{}(rhs.to_string());
#endif
		}
	};

}
#undef NICEHERO_HAS_2017

#endif
