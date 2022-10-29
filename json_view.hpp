#ifndef __NICEHERO_JSON__
#define __NICEHERO_JSON__

#include <stdint.h>
#include "viewvalue.hpp"
#include <vector>
#include <unordered_map>
#include "parse_number.hpp"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <array>
#include <algorithm>

namespace nicehero
{
    enum class json_type : uint8_t 
    {
		json_null,
		json_int64,
		json_uint64,
		json_double,
		json_string,
		json_array,
		json_object,
		json_boolean,
		json_delete,
		json_lazy
    };
	
	enum class semantic_tag : uint8_t 
	{
		none = 0,
		undefined = 0x01,
		datetime = 0x02,
		epoch_second = 0x03,
		epoch_milli = 0x04,
		epoch_nano = 0x05,
		bigint = 0x06,
		bigdec = 0x07,
		bigfloat = 0x08,
		float128 = 0x09,
		base16 = 0x1a,
		base58 = 0x1b,
		base64url = 0x1c,
		uri = 0x0d,
		clamped = 0x0e,
		multi_dim_row_major = 0x0f,
		multi_dim_column_major = 0x10,
		ext = 0x11,
		object_id = 0x12,
		regex = 0x13,
		code = 0x14,
		mongo_timestamp = 0x15,
		mongo_number_int = 0x16,
	};
	class json_view{
	public:
		using jvalue_type = viewvalue;
		using keys_type = std::unordered_map<jvalue_type, uint32_t>;
		using kjson_view = std::pair<jvalue_type, json_view>;

		class exception
		{
		public:
			std::string msg;
			exception(std::string s) : msg(s) {}
		};

		json_view(): m_value() {
		}
		json_view(json_type t) : m_value() {
			init(t);
		}
		json_view(int64_t t) : m_value(t), m_type(json_type::json_int64) {
		}
		json_view(double t) : m_value(t), m_type(json_type::json_double) {
		}
		json_view(const std::string& t) : m_value(t), m_type(json_type::json_string) {
		}
		json_view(const copy_str_ref& t) : m_value(t), m_type(json_type::json_string) {
		}
		json_view(bool t) : m_value(t), m_type(json_type::json_boolean) {
		}
		json_view(json_view&& rhs) noexcept : m_value() {
			m_type = rhs.m_type;
			if (m_type == json_type::json_object || m_type == json_type::json_array) {
				m_container = rhs.m_container;
				// 				m_key = rhs.m_key;
				m_keys = rhs.m_keys;
				rhs.m_keys = nullptr;
				rhs.m_container = nullptr;
			}
			else {
				m_value = std::move(rhs.m_value);
			}
			rhs.m_type = json_type::json_delete;
		}
		json_view(const json_view& rhs) : m_value() {
			m_type = rhs.m_type;
			if (m_type == json_type::json_object) {
				m_container = new std::vector<kjson_view>();
				m_keys = new keys_type();
				m_container->resize(rhs.m_container->size());
				m_keys->reserve(rhs.m_keys->size());
				for (size_t i = 0;i < rhs.m_container->size(); ++ i) {
					kjson_view& kjv = rhs.m_container->at(i);
					auto rkey = copy_str_ref(kjv.first);
					auto& v = m_container->at(i);
 					v.first.init(rkey);
					v.second = kjv.second;
					if (kjv.second.m_type != json_type::json_delete) {
						m_keys->emplace(rkey, uint32_t(i));
					}
				}
			}
			else if (m_type == json_type::json_array)
			{
				m_container = new std::vector<kjson_view>();
				m_keys = nullptr;
				m_container->resize(rhs.m_container->size());
				for (size_t i = 0; i < rhs.m_container->size(); ++i) {
					kjson_view& kjv = rhs.m_container->at(i);
					auto rkey = copy_str_ref(kjv.first);
					m_container->at(i).first.init(rkey);
					m_container->at(i).second = kjv.second;
				}
			}
			else {
				m_value = rhs.m_value;
			}
		}
		json_view& operator=(const json_view& rhs) noexcept {
			if (this == &rhs) {
				return *this;
			}
			if (m_type == json_type::json_object) {
				this->~json_view();
			}
			m_type = rhs.m_type;
			if (m_type == json_type::json_object) {
				m_container = new std::vector<kjson_view>();
				m_keys = new keys_type();
				m_container->resize(rhs.m_container->size());
				m_keys->reserve(rhs.m_keys->size());
				for (size_t i = 0; i < rhs.m_container->size(); ++i) {
					kjson_view& kjv = rhs.m_container->at(i);
					auto rkey = copy_str_ref(kjv.first);
					auto& v = m_container->at(i);
					v.first.init(rkey);
					v.second = kjv.second;
					if (kjv.second.m_type != json_type::json_delete) {
						m_keys->emplace(rkey, uint32_t(i));
					}
				}
			}
			else if (m_type == json_type::json_array)
			{
				m_container = new std::vector<kjson_view>();
				m_keys = nullptr;
				m_container->resize(rhs.m_container->size());
				for (size_t i = 0; i < rhs.m_container->size(); ++i) {
					kjson_view& kjv = rhs.m_container->at(i);
					m_container->at(i).second = kjv.second;
				}
			}
			else {
				m_value = rhs.m_value;
			}
			return *this;
		}
		json_view& operator=(json_view&& rhs) noexcept {
			if (m_type != json_type::json_object && m_type != json_type::json_array) {
				m_value.init("");
			}
			m_type = rhs.m_type;
			if (m_type == json_type::json_object || m_type == json_type::json_array) {
				m_container = rhs.m_container;
				// 				m_key = rhs.m_key;
				m_keys = rhs.m_keys;
				rhs.m_keys = nullptr;
				rhs.m_container = nullptr;
			}
			else {
				m_value = std::move(rhs.m_value);
			}
			rhs.m_type = json_type::json_delete;
			return *this;
		}

		json_view& operator=(int64_t t) noexcept {
			init(json_type::json_int64);
			m_value.init(t);
			return *this;
		}
		json_view& operator=(int t) noexcept {
			init(json_type::json_int64);
			m_value.init(int64_t(t));
			return *this;
		}
		json_view& operator=(double t) noexcept {
			init(json_type::json_double);
			m_value.init(t);
			return *this;
		}
		json_view& operator=(const std::string& t) noexcept {
			init(json_type::json_string);
			m_value.init(t);
			return *this;
		}
		json_view& operator=(const char* t) noexcept {
			init(json_type::json_string);
			m_value.init(copy_str_ref(t, uint32_t(strlen(t))));
			return *this;
		}
		json_view& operator=(const copy_str_ref& t) noexcept {
			init(json_type::json_string);
			m_value.init(t);
			return *this;
		}
		json_view& operator=(bool t) noexcept {
			init(json_type::json_boolean);
			m_value.init(t);
			return *this;
		}
		json_view& operator=(nullptr_t t) noexcept {
			init(json_type::json_null);
			return *this;
		}
		~json_view() {
			if (m_type == json_type::json_object || m_type == json_type::json_array) {
				if (m_container) {
					m_container->clear();
					delete m_container;
					m_container = nullptr;
				}
				if (m_keys) {
					delete m_keys;
					m_keys = nullptr;
				}
				m_type = json_type::json_delete;
				m_value.get_type_ref() = value_type::string_view_value;
				m_value.init("");
			}
		}
		void init(json_type t, semantic_tag tag_ = semantic_tag::none) {
			if (t == json_type::json_lazy) {
				t = json_type::json_null;
			}
			if (m_type == json_type::json_object || m_type == json_type::json_array) {
				if (m_container) {
					delete m_container;
					m_container = nullptr;
				}
				if (m_keys) {
					delete m_keys;
					m_keys = nullptr;
				}
			}
			m_tag = tag_;
			m_value.init("");
			m_type = t;
			switch (t) {
			case json_type::json_int64: {
				m_value.init(int64_t(0));
				break;
			}
			case json_type::json_boolean: {
				m_value.init(false);
				break;
			}
			case json_type::json_uint64: {
				m_value.init(uint64_t(0));
				break;
			}
			case json_type::json_double: {
				m_value.init(0.0);
				break;
			}
			case json_type::json_array: {
				m_container = new std::vector<kjson_view>();
				m_keys = nullptr;
				break;
			}
			case json_type::json_object: {
				m_keys = new keys_type();
				m_container = new std::vector<kjson_view>();
				break;
			}
			default:
				break;
			}
		}
		struct iterator {
			json_type m_type = json_type::json_null;
			size_t m_arrayIt = 0;
			json_view* m_parent = nullptr;
			inline bool operator == (const iterator& rhs) const {
				if (m_type != rhs.m_type) {
					return false;
				}
				if (m_parent != rhs.m_parent) {
					return false;
				}
				if (m_type == json_type::json_object) {
					return m_arrayIt == rhs.m_arrayIt;
				}
				if (m_type == json_type::json_array) {
					return m_arrayIt == rhs.m_arrayIt;
				}
				return false;
			}
			inline bool operator != (const iterator& rhs) const {
				if (m_type != rhs.m_type) {
					return true;
				}
				if (m_parent != rhs.m_parent) {
					return true;
				}
				if (m_type == json_type::json_object) {
					return m_arrayIt != rhs.m_arrayIt;
				}
				if (m_type == json_type::json_array) {
					return m_arrayIt != rhs.m_arrayIt;
				}
				return false;
			}
			inline iterator& operator++() {
				if (m_type == json_type::json_object) {
					++m_arrayIt;
					while (m_arrayIt < m_parent->m_container->size()
						&& m_parent->m_container->at(m_arrayIt).second.m_type == json_type::json_delete) {
						++m_arrayIt;
					}
				}
				else if (m_type == json_type::json_array) {
					++m_arrayIt;
				}
				return *this;
			}
			inline iterator operator++(int) {
				auto rit = *this;
				++(*this);
				return rit;
			}
			inline auto& operator*() {
				return *this;
			}
			jvalue_type key() {
				if (m_type == json_type::json_object) {
					return (m_parent->m_container->at(m_arrayIt)).first;
				}
				if (m_type == json_type::json_array) {
					return jvalue_type(m_arrayIt);
				}
				return jvalue_type();
			}
			json_view& val() {
				if (m_type == json_type::json_object) {
					return m_parent->m_container->at(m_arrayIt).second;
				}
				if (m_type == json_type::json_array) {
					return m_parent->m_container->at(m_arrayIt).second;
				}
				throw exception("error iterator");
			}
		};
		iterator begin() {
			iterator it;
			it.m_parent = this;
			it.m_type = m_type;
			if (m_type == json_type::json_object) {
				it.m_arrayIt = 0;
				while (it.m_arrayIt < it.m_parent->m_container->size()
					&& it.m_parent->m_container->at(it.m_arrayIt).second.m_type == json_type::json_delete) {
					++it.m_arrayIt;
				}
			}
			else if (m_type == json_type::json_array) {
				it.m_arrayIt = 0;
			}
			return it;
		}
		iterator end() {
			iterator it;
			it.m_parent = this;
			it.m_type = m_type;
			if (m_type == json_type::json_object) {
				it.m_arrayIt = m_container->size();
			}
			else if (m_type == json_type::json_array) {
				it.m_arrayIt = m_container->size();
			}
			return it;
		}
		struct const_iterator {
			json_type m_type = json_type::json_null;
			size_t m_arrayIt = 0;
			const json_view* m_parent = nullptr;
			inline bool operator == (const const_iterator& rhs) const {
				if (m_type != rhs.m_type) {
					return false;
				}
				if (m_parent != rhs.m_parent) {
					return false;
				}
				if (m_type == json_type::json_object) {
					return m_arrayIt == rhs.m_arrayIt;
				}
				if (m_type == json_type::json_array) {
					return m_arrayIt == rhs.m_arrayIt;
				}
				return false;
			}
			inline bool operator != (const const_iterator& rhs) const {
				if (m_type != rhs.m_type) {
					return true;
				}
				if (m_parent != rhs.m_parent) {
					return true;
				}
				if (m_type == json_type::json_object) {
					return m_arrayIt != rhs.m_arrayIt;
				}
				if (m_type == json_type::json_array) {
					return m_arrayIt != rhs.m_arrayIt;
				}
				return false;
			}
			inline const_iterator& operator++() {
				if (m_type == json_type::json_object) {
					++m_arrayIt;
					while (m_arrayIt < m_parent->m_container->size()
						&& m_parent->m_container->at(m_arrayIt).second.m_type == json_type::json_delete) {
						++m_arrayIt;
					}
				}
				else if (m_type == json_type::json_array) {
					++m_arrayIt;
				}
				return *this;
			}
			inline const_iterator operator++(int) {
				auto rit = *this;
				++(*this);
				return rit;
			}
			inline auto& operator*() {
				return *this;
			}
			jvalue_type key() {
				if (m_type == json_type::json_object) {
					return (m_parent->m_container->at(m_arrayIt)).first;
				}
				if (m_type == json_type::json_array) {
					return jvalue_type(m_arrayIt);
				}
				return jvalue_type();
			}
			const json_view& val() {
				if (m_type == json_type::json_object) {
					return m_parent->m_container->at(m_arrayIt).second;
				}
				if (m_type == json_type::json_array) {
					return m_parent->m_container->at(m_arrayIt).second;
				}
				throw exception("error iterator");
			}
		};
		const_iterator begin() const {
			const_iterator it;
			it.m_parent = this;
			it.m_type = m_type;
			if (m_type == json_type::json_object) {
				it.m_arrayIt = 0;
				while (it.m_arrayIt < it.m_parent->m_container->size()
					&& it.m_parent->m_container->at(it.m_arrayIt).second.m_type == json_type::json_delete) {
					++it.m_arrayIt;
				}
			}
			else if (m_type == json_type::json_array) {
				it.m_arrayIt = 0;
			}
			return it;
		}
		const_iterator end() const {
			const_iterator it;
			it.m_parent = this;
			it.m_type = m_type;
			if (m_type == json_type::json_object) {
				it.m_arrayIt = m_container->size();
			}
			else if (m_type == json_type::json_array) {
				it.m_arrayIt = m_container->size();
			}
			return it;
		}
		iterator find(const jvalue_type& key) {
			if (m_type != json_type::json_object) {
				return end();
			}
			auto it2 = m_keys->find(key);
			if (it2 == m_keys->end()) {
				return end();
			}
			iterator it;
			it.m_parent = this;
			it.m_type = m_type;
			if (m_type == json_type::json_object) {
				it.m_arrayIt = it2->second;
				if ((*m_container)[it2->second].second.m_type == json_type::json_delete) {
					return end();
				}
			}
			return it;
		}
		const_iterator find(const jvalue_type& key) const {
			if (m_type != json_type::json_object) {
				return end();
			}
			auto it2 = m_keys->find(key);
			if (it2 == m_keys->end()) {
				return end();
			}
			const_iterator it;
			it.m_parent = this;
			it.m_type = m_type;
			if (m_type == json_type::json_object) {
				it.m_arrayIt = it2->second;
				if ((*m_container)[it2->second].second.m_type == json_type::json_delete) {
					return end();
				}
			}
			return it;
		}

		std::string dump(int format_ = 0) {
			std::string vl;
// 			vl.reserve(1024 * 1024);
			_dump(vl,1,format_);
			return vl;
		}
		void _dump(std::string& vl,size_t layer, int format_) const {
			if (m_type == json_type::json_object) {
				if (m_container->size() < 1) {
					vl << "{}";
					return;
				}
				vl << "{";
				bool isFirst = true;
				for (auto it : *this) {
					if (format_ > 0) {
						if (isFirst) {
							isFirst = false;
						}
						else {
							vl << ",";
						}
						vl << "\n";
						for (size_t i = 0; i < layer; ++i) {
							vl << "\t";
						}
						vl << "\"";
					}
					else {
						if (isFirst) {
							isFirst = false;
							vl << "\"";
						}
						else {
							vl << ",\"";
						}
					}
					vl << it.key();
					vl << "\":";
					const auto& v = it.val();
					if (v.m_type == json_type::json_string) {
						vl << "\"" << v.m_value << "\"";
					}
					else if (v.m_type == json_type::json_object || v.m_type == json_type::json_array) {
						v._dump(vl, layer + 1,format_);
					}
					else if (v.m_type == json_type::json_null)
					{
						vl << "null";
					}
					else {
						vl << v.m_value;
					}
				}
				if (format_ > 0) {
					vl << "\n";
					for (size_t i = 0; i < layer - 1; ++i) {
						vl << "\t";
					}
				}
				vl << "}";
			}
			else if (m_type == json_type::json_array) {
				if (m_container->size() < 1) {
					vl << "[]";
					return;
				}
				vl << "[";
				bool isFirst = true;
				for (auto it : *this) {
					if (isFirst) {
						isFirst = false;
					}
					else {
						vl << ",";
					}
					if (format_ > 0) {
						vl << "\n";
						for (size_t i = 0; i < layer; ++i) {
							vl << "\t";
						}
					}
					const auto& v = it.val();
					if (v.m_type == json_type::json_string) {
						vl << "\"" << v.m_value << "\"";
					}
					else if (v.m_type == json_type::json_object || v.m_type == json_type::json_array) {
						v._dump(vl, layer + 1, format_);
					}
					else if (v.m_type == json_type::json_null)
					{
						vl << "null";
					}
					else {
						vl << v.m_value;
					}
				}
				if (format_ > 0) {
					vl << "\n";
					for (size_t i = 0; i < layer - 1; ++i) {
						vl << "\t";
					}
				}
				vl << "]";
			}
		}

		bool parse(const char* jdata,bool isLazy = true) {
			jdata = lstrip(jdata);
			if (!jdata) {
				return false;
			}
			m_value = jdata;
			if (*jdata == '{') {
				init(json_type::json_object);
				return _parseObject(jdata, isLazy);
			}
			if (*jdata == '[') {
				init(json_type::json_array);
				return _parseArray(jdata, isLazy);
			}
			return false;
		}
		const char* _parseObject(const char* jdata,bool isLazy) {
			++jdata;
			jdata = lstrip(jdata);
			if (!jdata) {
				return nullptr;
			}
			if (*jdata == '}') {
				++jdata;
				return jdata;
			}
			while (true) {
				jvalue_type vl = _parseStringQuta(jdata, ":");
				if (vl.m_len < 1) {
					return nullptr;
				}
				jdata = vl.m_start + vl.m_len;
				if (_cInQuotas(vl.m_start[0],"\"\'") && _cInQuotas(vl.m_start[vl.m_len - 1], "\"\'")) {
					if (vl.m_len < 3) {
						return nullptr;
					}
					vl.m_start += 1;
					vl.m_len -= 2;
				}
				jdata = lstrip(jdata);
				if (*jdata != ':') {
					return nullptr;
				}
				m_container->emplace_back();
				m_container->back().first.init(vl.m_start, vl.m_len);
				json_view& newJV = m_container->back().second;
				++jdata;
				jdata = lstrip(jdata);
				if (*jdata == '{') {
					newJV.init(json_type::json_object);
					jdata = newJV._parseObject(jdata,isLazy);
					if (!jdata) {
						return nullptr;
					}
				}
				else if (*jdata == '[') {
					newJV.init(json_type::json_array);
					jdata = newJV._parseArray(jdata, isLazy);
					if (!jdata) {
						return nullptr;
					}
				}
				else {
					jvalue_type& vl2 = newJV.m_value;
					vl2 = _parseStringQuta(jdata, ",}");
					if (vl2.m_len < 1) {
						return nullptr;
					}
					newJV.m_type = json_type::json_lazy;
					jdata = vl2.m_start + vl2.m_len;
					if (!isLazy) {
						newJV.check_type();
					}
				}
				jdata = lstrip(jdata);
				if (!jdata) {
					return nullptr;
				}
				if (*jdata == ','){
					++jdata;
					jdata = lstrip(jdata);
					if (!jdata) {
						return nullptr;
					}
					continue;
				}
				if (*jdata == '}') {
					++jdata;
					if (!jdata) {
						return nullptr;
					}
					jdata = lstrip(jdata);
					break;
				}
// 				*m_keys.emplace({ jdata,n }, m_container->size());
			}
			auto memberCount = m_container->size();
			m_keys->reserve(memberCount);
			for (size_t i = 0; i < memberCount; ++ i) {
				jvalue_type& vl = m_container->at(i).first;
				m_keys->emplace(jvalue_type(vl.m_start,vl.m_len), uint32_t(i));
			}
#if 0
			size_t conflict = 0;
			for (size_t i = 0; i < m_keys->bucket_count(); ++i) {
				size_t bs = m_keys->bucket_size(i);
				if (bs > 1) {
					conflict += bs - 1;
				}
			}
			if (conflict > 0) {
				std::cout << "hash conflict " << conflict << " in " << memberCount << std::endl;
			}
#endif
			return jdata;
		}
		const char* _parseArray(const char* jdata,bool isLazy) {
			++jdata;
			while (true) {
				jdata = lstrip(jdata);
				if (!jdata) {
					return nullptr;
				}
				if (*jdata == ']') {
					++jdata;
					break;
				}
				m_container->emplace_back();
				json_view& newJV = m_container->back().second;
				jdata = lstrip(jdata);
				if (*jdata == '{') {
					newJV.init(json_type::json_object);
					jdata = newJV._parseObject(jdata, isLazy);
					if (!jdata) {
						return nullptr;
					}
				}
				else if (*jdata == '[') {
					newJV.init(json_type::json_array);
					jdata = newJV._parseArray(jdata, isLazy);
					if (!jdata) {
						return nullptr;
					}
				}
				else {
					jvalue_type& vl2 = newJV.m_value;
					vl2 = _parseStringQuta(jdata, ",]");
					if (vl2.m_len < 1) {
						return nullptr;
					}
					newJV.m_type = json_type::json_lazy;
					jdata = vl2.m_start + vl2.m_len;
					if (!isLazy) {
						newJV.check_type();
					}
				}
				jdata = lstrip(jdata);
				if (*jdata == ',') {
					++jdata;
					continue;
				}
				if (*jdata == ']') {
					++jdata;
					break;
				}
				// 					*m_keys.emplace({ jdata,n }, m_container->size());
			}
			return jdata;
		}
		static inline bool _cInQuotas(char c, const char* otherQuotas){
			for (size_t i = 0;; ++i) {
				if (otherQuotas[i] == '\0') {
					return false;
				}
				if (otherQuotas[i] == c) {
					return true;
				}
			}
			return false;
		}
		static jvalue_type _parseStringQuta(const char* jdata,const char* otherQuotas) {
			char quta = '\0';
			if (*jdata == '\'') {
				quta = *jdata;
				++jdata;
			}
			else if (*jdata == '\"') {
				quta = *jdata;
				++jdata;
			}
			uint32_t r = 0;
			while (true) {
				if (jdata[r] == '\0') {
					return jvalue_type();
				}
				if (quta != '\0' && jdata[r] == '\\') {
					++r;
					if (jdata[r] == '\''
						|| jdata[r] == '\"' 
						|| jdata[r] == 'r'
						|| jdata[r] == 'n'
						|| jdata[r] == 't'
						|| jdata[r] == 'b') {
						++r;
						continue;
					}
					if (jdata[r] == 'u') {
						for (size_t i = 0; i < 4; ++ i) {
							++r;
							if (jdata[r] < '0') {
								return jvalue_type();
							}
							else if (jdata[r] <= '9') {
								continue;
							}
							else if (jdata[r] < 'A') {
								return jvalue_type();
							}
							else if (jdata[r] <= 'F') {
								continue;
							}
							else if (jdata[r] < 'a') {
								return jvalue_type();
							}
							else if (jdata[r] < 'f') {
								continue;
							}
							return jvalue_type();
						}
						continue;
					}
					return jvalue_type();
				}
				if (jdata[r] == quta || (quta == '\0' && _cInQuotas(jdata[r],otherQuotas))) {
					if (jdata[r] != quta) {
						r = uint32_t(rstrip(jdata, r));
						return jvalue_type(jdata, r);
					}
					return jvalue_type(jdata - 1, r + 2);
				}
				++r;
			}
		}
		static const char* lstrip(const char* jdata) {
			while (true) {
				if (*jdata == '\0') {
					return jdata;
				}
				if ((*jdata >= '\t' && *jdata <= '\r') || *jdata == ' ') {
					++jdata;
					continue;
				}
				return jdata;
			}
		}
		static size_t rstrip(const char* jdata,size_t size_) {
			size_t r = size_;
			while (true) {
				if (r < 1) {
					return r;
				}
				if ((jdata[r - 1] >= '\t' && jdata[r - 1] <= '\r') || jdata[r - 1] == ' ') {
					--r;
					continue;
				}
				return r;
			}
		}

		json_view& operator[](const jvalue_type& key) {
			if (m_type == json_type::json_null || m_type == json_type::json_delete) {
				init(json_type::json_object);
			}
			if (m_type == json_type::json_object) {
				auto it = m_keys->find(key);
				if (it == m_keys->end()) {
					m_container->emplace_back();
					copy_str_ref rkey = copy_str_ref(key);
					m_container->back().first.init(rkey);
					m_keys->insert({ rkey , uint32_t(m_container->size() - 1) });
					return m_container->back().second;
				}
				return m_container->at(it->second).second;
			}
			throw exception("error type");
		}

		json_view& operator[](int index_) {
			if (m_type != json_type::json_array || !m_container || index_ >= m_container->size())
			{
				throw exception("error index");
			}
			return m_container->at(index_).second;
		}

		json_view& add_member(const jvalue_type& key) {
			if (m_type == json_type::json_null || m_type == json_type::json_delete) {
				init(json_type::json_object);
			}
			if (m_type == json_type::json_object) {
				auto it = m_keys->find(key);
				if (it == m_keys->end()) {
					m_container->emplace_back();
					copy_str_ref rkey = copy_str_ref(key);
					m_container->back().first.init(rkey);
					m_keys->insert({ rkey,uint32_t(m_container->size() - 1) });
					return m_container->back().second;
				}
				return m_container->at(it->second).second;
			}
			throw exception("error type");
		}

		void push_back(const json_view& jv) {
			if (m_type == json_type::json_null || m_type == json_type::json_delete) {
				init(json_type::json_array);
			}
			if (m_type == json_type::json_array) {
				m_container->emplace_back();
				m_container->back().second = jv;
			}
		}

		void pop_back() {
			if (m_type == json_type::json_array) {
				m_container->pop_back();
			}
		}

		void erase(const viewvalue& key) {
			if (m_type == json_type::json_object) {
				auto it = m_keys->find(key);
				if (it != m_keys->end()) {
					m_container->at(it->second).second.init(json_type::json_delete);
					m_keys->erase(key);
				}
			}
		}

		size_t size() {
			if (m_type == json_type::json_object) {
				return m_keys->size();
			}
			if (m_type == json_type::json_array) {
				return m_container->size();
			}
		}

		bool check_type() const {
			if (m_type == json_type::json_lazy) {
				if (m_value.get_type() != value_type::string_view_value) {
					return false;
				}
				if (_cInQuotas(m_value.front(), "\"\'")) {
					if (m_value.back() == m_value.front() && m_value.m_len > 1) {
						m_value.m_start += 1;
						m_value.m_len -= 2;
						if (m_value.m_len < 1) {
							m_value.m_start = "";
						}
						m_type = json_type::json_string;
						return true;
					}
					return false;
				}
				if (m_value.front() == 't' && m_value.m_len == 4
					&& m_value.m_start[1] == 'r'
					&& m_value.m_start[2] == 'u'
					&& m_value.m_start[3] == 'e'
					) {
					m_value.init(true);
					m_type = json_type::json_boolean;
					return true;
				}
				if (m_value.front() == 'f' && m_value.m_len == 5
					&& m_value.m_start[1] == 'a'
					&& m_value.m_start[2] == 'l'
					&& m_value.m_start[3] == 's'
					&& m_value.m_start[4] == 'e'
					) {
					m_value.init(false);
					m_type = json_type::json_boolean;
					return true;
				}
				json_type preType = json_type::json_uint64;
				jvalue_type vl = m_value;
				if (vl.m_len < 1) {
					return false;
				}
				if (*vl.m_start == '-') {
					preType = json_type::json_int64;
					++vl.m_start;
					vl.m_len -= 1;
				}
				if (vl.m_len < 1) {
					return false;
				}
				if (*vl.m_start == '0' && vl.m_len > 1
					&& vl.m_start[1] >= '0' && vl.m_start[1] <= '9') {
					return false;
				}
				if (vl.m_start[0] < '0' || vl.m_start[0] > '9') {
					return false;
				}
				for (uint64_t i = 1; i < vl.m_len; ++ i) {
					if (vl.m_start[i] == '.' || vl.m_start[i] == 'e'
						|| vl.m_start[i] == 'E') {
						preType = json_type::json_double;
						break;
					}
					if (vl.m_start[i] < '0' || vl.m_start[i] > '9') {
						return false;
					}
				}
				switch (preType)
				{
				case nicehero::json_type::json_int64: {
					int64_t t;
					auto result = to_integer_unchecked(m_value.m_start, m_value.m_len, t);
					if (!result) {
						return false;
					}
					m_value.init(t);
					break;
				}
				case nicehero::json_type::json_uint64: {
					uint64_t t;
					auto result = to_integer_unchecked(m_value.m_start, m_value.m_len, t);
					if (!result) {
						return false;
					}
					m_value.init(t);
					break;
				}
				case nicehero::json_type::json_double: {
					static to_double_t td;
					double t;
					t = td(m_value.m_start, m_value.m_len);
					if (!td.isok) {
						return false;
					}
					m_value.init(t);
					break;
				}
				default:
					return false;
					break;
				}
				m_type = preType;
			}
			return true;
		}
		static inline bool little_endianness(int num = 1) noexcept {
			return *reinterpret_cast<char*>(&num) == 1;
		}
		template<typename NumberType, bool InputIsLittleEndian = true>
		static void get_bson_number(NumberType& result, viewvalue& bson_data) {
			if (little_endianness() == InputIsLittleEndian) {
				std::memcpy(&result, bson_data.m_start, sizeof(NumberType));
				bson_data += sizeof(NumberType);
				return;
			}
			std::array<uint8_t, sizeof(NumberType)> vec{};
			for (std::size_t i = 0; i < sizeof(NumberType); ++i) {
				vec[sizeof(NumberType) - i - 1] = static_cast<uint8_t>(bson_data.m_start[i]);
			}
			std::memcpy(&result, vec.data(), sizeof(NumberType));
			bson_data += sizeof(NumberType);
			return;
		}
		template<typename NumberType, bool OutputIsLittleEndian = true>
		void write_bson_number(const NumberType n, NumberType* r)
		{
			std::memcpy(r, &n, sizeof(NumberType));
			if (little_endianness() != OutputIsLittleEndian) {
				std::reverse(r, r + sizeof(NumberType));
			}
		}

		template<typename NumberType, bool OutputIsLittleEndian = true>
		void write_bson_number(const NumberType n, std::vector<uint8_t>& o)
		{
			size_t oldSize = o.size();
			o.resize(oldSize + sizeof(NumberType));
			NumberType* r = (NumberType*)(&(o[oldSize]));
			write_bson_number(n, r);
		}

		uint8_t get_bson_type(const viewvalue& bson_data) {
			return uint8_t (*(bson_data.m_start));
		}
		static void get_bson_cstr(jvalue_type& result, viewvalue& bson_data) {
			uint32_t r = 1;
			result.m_start = bson_data.m_start;
			result.m_len = 0;
			while (true) {
				if (*bson_data.m_start == 0x00) {
					++bson_data;
					return;
				}
				++result.m_len;
				++bson_data;
			}
		}
		void parse_bson_element(const jvalue_type& k,const uint8_t element_type, viewvalue& bson_data)
		{
			switch (element_type) {
			case 0x01: // double
			{
				init(json_type::json_double);
				double number{};
				get_bson_number<double>(number, bson_data);
				m_value.init(number);
				return;
			}
			case 0x02: // string
			{
				init(json_type::json_string);
				int32_t len{};
				get_bson_number<int32_t>(len, bson_data);
				m_value.init(bson_data.m_start, len - 1);
				bson_data += len;
				return;
			}
			case 0x03: // object
			{
				init(json_type::json_object);
				parse_bson(bson_data);
				return;
			}
			case 0x04: // array
			{
				init(json_type::json_array);
				parse_bson(bson_data,true);
				return;
			}
			case 0x05: // binary
			{
				init(json_type::json_string, semantic_tag::base58);
				int32_t len{};
				get_bson_number<int32_t>(len, bson_data);
				m_value.init(bson_data.m_start, len + 1);
				bson_data += (len + 1);
				return;
			}
			case 0x0C: // 	DBPointer Deprecated
			case 0x07: // 	ObjectId
			{
				constexpr size_t objIDSize = 12;
				init(json_type::json_string, semantic_tag::object_id);
				m_value.init(bson_data.m_start, objIDSize);
				bson_data += objIDSize;
				return;
			}
			case 0x08: // boolean
			{
				uint8_t b = 0;
				get_bson_number<uint8_t>(b, bson_data);
				init(json_type::json_boolean);
				m_value.init(bool(b));
				return;
			}
			case 0x09: // UTC datetime
			{
				uint64_t dt = 0;
				get_bson_number<uint64_t>(dt, bson_data);
				init(json_type::json_uint64, semantic_tag::epoch_milli);
				m_value.init(dt);
				return;
			}
			case 0xFF: // Min key unsupport
			case 0x7F: // Max key unsupport
			case 0x06: // Undefined (value) ¡ª Deprecated
			case 0x0A: // null
			{
				init(json_type::json_null);
				return;
			}
			case 0x0B: // Regular expression cstring cstring
			{
				init(json_type::json_string, semantic_tag::regex);
				get_bson_cstr(m_value, bson_data);
				jvalue_type vl;
				get_bson_cstr(vl, bson_data);
				m_value.m_len += vl.m_len + 1;
				return;
			}
			case 0x0D: // JavaScript code
			{
				init(json_type::json_string, semantic_tag::code);
				int32_t len{};
				get_bson_number<int32_t>(len, bson_data);
				m_value.init(bson_data.m_start, len - 1);
				bson_data += len;
				return;
			}
			case 0x10: // int32
			{
				int32_t value{};
				get_bson_number(value, bson_data);
				init(json_type::json_int64, semantic_tag::mongo_number_int);
				m_value.init((int64_t)value);
				return;
			}
			case 0x11: // Timestamp uint64
			{
				uint32_t value{};
				get_bson_number(value, bson_data);
				init(json_type::json_uint64);
				m_value.init(value);
				return;
			}
			case 0x12: // int64
			{
				int64_t value{};
				get_bson_number(value, bson_data);
				init(json_type::json_int64);
				m_value.init(value);
				return;
			}
			case 0x13: // decimal128 	128-bit decimal floating point
			{
				constexpr size_t float128_size = 16;
				init(json_type::json_string, semantic_tag::float128);
				m_value.init(bson_data.m_start, float128_size);
				bson_data += float128_size;
				return;
			}
			default: 
				bson_data.init("");
				return;
			}
		}
		static void parse_bson_element_fake(const jvalue_type& k, const uint8_t element_type, viewvalue& bson_data)
		{
			switch (element_type) {
			case 0x01: // double
				bson_data += sizeof(double);
				return;
			case 0x02: // string
			{
				int32_t len{};
				get_bson_number<int32_t>(len, bson_data);
				bson_data += len;
				return;
			}
			case 0x03: // object
			case 0x04: // array
			{
				std::int32_t document_size{};
				get_bson_number<int32_t>(document_size, bson_data);
				bson_data += (document_size - sizeof(int32_t));
				return;
			}
			case 0x05: // binary
			{
				int32_t len{};
				get_bson_number<int32_t>(len, bson_data);
				bson_data += (len + 1);
				return;
			}
			case 0x0C: // 	DBPointer Deprecated
			case 0x07: // 	ObjectId
			{
				constexpr size_t objIDSize = 12;
				bson_data += objIDSize;
				return;
			}
			case 0x08: // boolean
			{
				++bson_data;
				return;
			}
			case 0x09: // UTC datetime
			{
				bson_data += sizeof(uint64_t);
				return;
			}
			case 0xFF: // Min key unsupport
			case 0x7F: // Max key unsupport
			case 0x06: // Undefined (value) ¡ª Deprecated
			case 0x0A: // null
			{
				return;
			}
			case 0x0B: // Regular expression cstring cstring
			{
				jvalue_type vl;
				get_bson_cstr(vl, bson_data);
				get_bson_cstr(vl, bson_data);
				return;
			}
			case 0x0D: // JavaScript code
			{
				int32_t len{};
				get_bson_number<int32_t>(len, bson_data);
				bson_data += len;
				return;
			}
			case 0x10: // int32
			{
				bson_data += sizeof(int32_t);
				return;
			}
			case 0x11: // Timestamp uint64
			{
				bson_data += sizeof(int64_t);
				return;
			}
			case 0x12: // int64
			{
				bson_data += sizeof(int64_t);
				return;
			}
			case 0x13: // decimal128 	128-bit decimal floating point
			{
				constexpr size_t float128_size = 16;
				bson_data += float128_size;
				return;
			}
			default:
				bson_data.init("");
				return;
			}
		}

		void parse_bson_element_list(const bool is_barray, viewvalue& bson_data) {
			if (is_barray) {
				init(json_type::json_array);
			}
			else {
				init(json_type::json_object);
			}
			jvalue_type k;
			size_t element_count = 0;
			const char* start_old = bson_data.m_start;
			uint32_t len_old = bson_data.m_len;
			while (auto element_type = get_bson_type(bson_data)) {
				++bson_data;
				get_bson_cstr(k, bson_data);
				parse_bson_element_fake(k, element_type, bson_data);
				++element_count;
			}
			m_container->reserve(element_count);
			bson_data.m_start = start_old;
			bson_data.m_len = len_old;
			while (auto element_type = get_bson_type(bson_data)) {
				++bson_data;
				m_container->emplace_back();
				get_bson_cstr(k, bson_data);
				auto& kjv = m_container->back();
				kjv.first = k;
				kjv.second.parse_bson_element(k, element_type, bson_data);
			}
			++bson_data;
			if (!is_barray) {
				auto memberCount = m_container->size();
				m_keys->reserve(memberCount);
				for (size_t i = 0; i < memberCount; ++i) {
					jvalue_type& vl = m_container->at(i).first;
					m_keys->emplace(jvalue_type(vl.m_start, vl.m_len), uint32_t(i));
				}
			}
			return;
		}

		void parse_bson(const viewvalue& bson_data_, bool is_barray = false) {
			if (bson_data_.get_type() != value_type::string_view_value) {
				return;
			}
			viewvalue bson_data = bson_data_;
			std::int32_t document_size{};
			get_bson_number<int32_t>(document_size, bson_data);
			return parse_bson_element_list(is_barray, bson_data);
		}

		void parse_bson(const std::vector<uint8_t>& bson_data_, bool is_barray = false) {
			viewvalue bson_data = viewvalue((const char*)bson_data_.data(), (uint32_t)bson_data_.size());
			std::int32_t document_size{};
			get_bson_number<int32_t>(document_size, bson_data);
			return parse_bson_element_list(is_barray, bson_data);
		}

		void parse_bson(viewvalue& bson_data, bool is_barray = false) {
			std::int32_t document_size{};
			get_bson_number<int32_t>(document_size, bson_data);
			return parse_bson_element_list(is_barray, bson_data);
		}

		std::vector<uint8_t> dump_bson() {
			std::vector<uint8_t> o;
			uint8_t btype = get_bson_type();
			dump_bson(btype, o);
			return o;
		}
		inline void dump_bson_string(std::vector<uint8_t>& o) {
			size_t oldSize = o.size();
			o.resize(oldSize + sizeof(uint32_t));
			uint32_t s = m_value.write2cstrbuffer(o) + 1;
			write_bson_number(s, (uint32_t*)(&(o[oldSize])));
		}
		void dump_bson(uint8_t btype, std::vector<uint8_t>& o) {
			switch (btype) {
			case 0x01:
				write_bson_number(m_value.m_double, o);
				break;
			case 0x02:
				dump_bson_string(o);
				break;
			case 0x03:
				dump_bson_object(o);
				break;
			case 0x04:
				dump_bson_array(o);
				break;
			case 0x08:
				if (m_value.m_boolean) {
					o.push_back(0x01);
				}
				else {
					o.push_back(0x00);
				}
				break;
			case 0x10:
				write_bson_number(int32_t(m_value.m_i64), o);
				break;
			case 0x12:
				write_bson_number(m_value.m_i64, o);
				break;
			default:
				break;
			}
		}
		inline uint8_t get_bson_type() {
			if (is_object()) {
				return 0x03;
			}
			if (is_array()) {
				return 0x04;
			}
			if (is_string()) {
				return 0x02;
			}
			if (is_integer()) {
				if (m_tag == semantic_tag::mongo_number_int) {
					return 0x10;
				}
				return 0x12;
			}
			if (is_double()) {
				return 0x01;
			}
			if (is_boolean()) {
				return 0x08;
			}
			if (is_null()) {
				return 0x0A;
			}
			return 0;
		}
		void dump_bson_object(std::vector<uint8_t>& o) {
			uint32_t s = uint32_t(m_container->size());
			size_t oldSize = o.size();
			o.resize(oldSize + sizeof(uint32_t));
			for (size_t i = 0; i < s; ++ i) {
				auto& kv = (*m_container)[i];
				auto& v = kv.second;
				if (v.m_type == json_type::json_delete) {
					continue;
				}
				uint8_t btype = v.get_bson_type();
				o.push_back(btype);
				kv.first.write2cstrbuffer(o);
				v.dump_bson(btype, o);
			}
			o.push_back(0x00);
			write_bson_number(uint32_t(o.size() - oldSize), (uint32_t*)(&(o[oldSize])));
		}
		void dump_bson_array(std::vector<uint8_t>& o) {
			uint32_t s = uint32_t(m_container->size());
			size_t oldSize = o.size();
			o.resize(oldSize + sizeof(uint32_t));
			for (size_t i = 0; i < s; ++i) {
				auto& kv = (*m_container)[i];
				auto& v = kv.second;
				uint8_t btype = v.get_bson_type();
				o.push_back(btype);
				from_integer(uint32_t(i), o);
				o.push_back(0x00);
				v.dump_bson(btype, o);
			}
			o.push_back(0x00);
			write_bson_number(uint32_t(o.size() - oldSize), (uint32_t*)(&(o[oldSize])));
		}

		inline bool is_object() const {
			return m_type == json_type::json_object;
		}
		inline bool is_array() const {
			return m_type == json_type::json_array;
		}
		inline bool is_integer() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			return m_type == json_type::json_uint64 || m_type == json_type::json_int64;
		}
		inline bool is_number() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			return m_type == json_type::json_uint64 || m_type == json_type::json_int64 || m_type == json_type::json_double;
		}
		inline bool is_double() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			return m_type == json_type::json_double;
		}
		inline bool is_string() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			return m_type == json_type::json_string;
		}
		inline bool is_boolean() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			return m_type == json_type::json_boolean;
		}
		inline bool is_null() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			return m_type == json_type::json_null;
		}

		inline uint64_t as_uint64() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			if (m_type == json_type::json_uint64) {
				return m_value.m_ui64;
			}
			if (m_type == json_type::json_int64) {
				return (uint64_t)m_value.m_i64;
			}
			if (m_type == json_type::json_double) {
				return (uint64_t)m_value.m_double;
			}
			return 0;
		}

		inline int64_t as_int64() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			if (m_type == json_type::json_int64) {
				return m_value.m_i64;
			}
			if (m_type == json_type::json_uint64) {
				return (int64_t)m_value.m_ui64;
			}
			if (m_type == json_type::json_double) {
				return (int64_t)m_value.m_double;
			}
			return 0;
		}

		inline double as_number() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			if (m_type == json_type::json_double) {
				return m_value.m_double;
			}
			if (m_type == json_type::json_int64) {
				return (double)m_value.m_i64;
			}
			if (m_type == json_type::json_uint64) {
				return (double)m_value.m_ui64;
			}
			return 0;
		}

		inline std::string as_string() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			if (m_type == json_type::json_string) {
				return m_value.to_string();
			}
			return "";
		}
		inline bool as_boolean() const {
			if (m_type == json_type::json_lazy) {
				check_type();
			}
			if (m_type == json_type::json_boolean) {
				return m_value.m_boolean;
			}
			return false;
		}

		//fields start
		union 
		{
			mutable jvalue_type m_value;
			struct  
			{
				std::vector<kjson_view>* m_container;
				keys_type* m_keys;
			};
		};
		mutable json_type m_type = json_type::json_null;
		mutable semantic_tag m_tag = semantic_tag::none;
		//fields end
	};
	static const json_view null_json_view;
}

#endif

