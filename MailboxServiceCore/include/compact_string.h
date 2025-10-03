#pragma once

#include <array>
#include <string>
#include <string_view>
#include <functional>

namespace compact_string {

	template<std::size_t Size>
	struct string {
		constexpr static std::size_t const_size = Size;

		std::array<char, Size> charArray;
		explicit string() {
			charArray.fill('\0');
		}

		explicit string(std::string_view sv) {
			if (sv.length() > charArray.size() - 1) {
				throw std::out_of_range("sv.length() > data.size()");
			}
			charArray.fill('\0');
			memcpy(charArray.data(), sv.data(), sv.length());
		}

		
		string(const string<Size>& copied) {
			memcpy(charArray.data(), copied.charArray.data(), copied.charArray.size());
		}

		string(string<Size>&& moved) noexcept {
			memcpy(charArray.data(), moved.charArray.data(), moved.charArray.size());
		}

		string& operator=(std::string_view sv) {
			if (sv.length() > charArray.size() - 1) {
				throw std::out_of_range("sv.length() > data.size()");
			}
			charArray.fill('\0');
			memcpy(charArray.data(), sv.data(), sv.length());
			return *this;
		}

		template<std::size_t N>
		string& operator=(const string<N>& other) {
			static_assert(Size >= N);
			charArray.fill('\0');
			memcpy(charArray.data(), other.charArray.data(), other.charArray.size());
			return *this;
		}

		string& operator=(const string<Size>& other) {
			if (this == &other) {
				return *this;
			}
			charArray.fill('\0');
			memcpy(charArray.data(), other.charArray.data(), other.charArray.size());
			return *this;
		}

#ifdef ENABLED_COMPARISON_COMPACT_STRING_WITH_STRING_VIEW
		bool operator<(std::string_view right) const {
			return std::string_view(charArray.data()) < right;
		}
#endif

		bool operator<(const string& right) const {
			return charArray < right.charArray;
		}

#ifdef ENABLED_COMPARISON_COMPACT_STRING_WITH_STRING_VIEW
		bool operator>(std::string_view right) const {
			return std::string_view(charArray.data()) > right;
		}
#endif

		bool operator>(const string& right) const {
			return charArray > right.charArray;
		}

#ifdef ENABLED_COMPARISON_COMPACT_STRING_WITH_STRING_VIEW
		bool operator==(std::string_view right) const {
			return std::string_view(charArray.data()) == right;
		}
#endif

		bool operator==(const string& right) const {
			return charArray == right.charArray;
		}

#ifdef ENABLED_COMPARISON_COMPACT_STRING_WITH_STRING_VIEW
		bool operator!=(std::string_view right) const {
			return std::string_view(charArray.data()) != right;
		}
#endif

		bool operator!=(const string& right) const {
			return charArray != right.charArray;
		}

		char& operator[](std::size_t i) {
			return charArray[i];
		}

		const char* data() const {
			return charArray.data();
		}

		const char* c_str() const {
			return charArray.data();
		}

	};

	template<std::size_t Size>
	struct string_comparator {
		using is_transparent = void;

		bool operator()(const string<Size>& a, const string<Size>& b) const {
			return a < b;
		}

		bool operator()(const string<Size>& a, std::string_view b) const {
			return a < b;
		}

		bool operator()(std::string_view a, const string<Size>& b) const {
			return b > a;
		}
	};

	template<std::size_t Size>
	struct std::hash<compact_string::string<Size>>
	{
		std::size_t operator()(const compact_string::string<Size>& s) const noexcept
		{
			return std::hash<std::string_view>{}(s.charArray.data());
		}
	};

	template<std::size_t Size>
	inline std::ostream& operator<<(std::ostream& out, const compact_string::string<Size>& string_wrap) {
		out << std::string_view(string_wrap.charArray.data());
		return out;
	}

	/*struct hash {
		std::size_t hash_value;
		template<typename String>
		hash(String s) {
			hash_value = std::hash<String>()(s);
		}

	};*/
	
	template<std::size_t N = 32>
	struct compact_string_converter {
		using result_type = string<N>;
		//constexpr static result_type default_result_value = string<N>();
		using comparator = string_comparator<N>;

		template<typename T>
		string<N> operator()(T orig) const {
			string<N> result;
			result = orig;
			return result;
		}
	};

	struct std_string_converter {
		using result_type = std::string;
		using comparator = std::less<std::string>;

		template<typename T>
		std::string operator()(T orig) const {
			return std::string(orig.data());
		}
		std::string operator()(std::size_t hash) const {
			return std::to_string(hash);
		}
	};

	template<typename T = std::string_view, typename HashFunction = std::hash<typename T>>
	struct hash_converter {
		using result_type = std::size_t;
		constexpr static result_type default_result_value = static_cast<std::size_t>(0);
		using comparator = std::less<result_type>;
		result_type operator()(T orig) const {
			HashFunction f;
			return f(orig);
		}
	};

	typedef hash_converter<std::string_view, std::hash<std::string_view>> string_view_hash_converter;

	template<std::size_t N>
	using compact_string_hash_converter = hash_converter<compact_string::string<N>, std::hash<compact_string::string<N>>>;
}


