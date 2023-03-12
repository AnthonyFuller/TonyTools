#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <map>

class buffer {
	std::vector<char> buff;

	template <typename>
	struct is_string_type : std::false_type {};

	template <>
	struct is_string_type<std::string> : std::true_type {};

	template <typename>
	struct is_vector_type : std::false_type {};

	template <typename T>
	struct is_vector_type<std::vector<T>> : std::true_type {};

	template <typename>
	struct is_map_type : std::false_type {};

	template <typename K, typename V>
	struct is_map_type<std::map<K, V>> : std::true_type {};

	template <typename T>
	void swap_endianess(T* u) {
		static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

		union {
			T u;
			unsigned char u8[sizeof(T)];
		} source, dest;

		source.u = *u;

		for (size_t k = 0; k < sizeof(T); k++) dest.u8[k] = source.u8[sizeof(T) - k - 1];

		*u = dest.u;
	}

	template <typename T>
	void write_trivial(T v) noexcept {
		const std::size_t size = sizeof(T);

		if (write_swap_endianness) swap_endianess<T>(&v);

		if (index == buff.size()) buff.resize(buff.size() + size);
		std::memcpy(buff.data() + index, (std::uint8_t*) &v, size);

		index += size;
	}

	template <typename T>
	void write_string(T v) noexcept {
		const std::size_t size = v.size() + 1;

		if (index == buff.size()) buff.resize(buff.size() + size);
		std::memcpy(buff.data() + index, v.data(), size);

		index += size;
	}

	template <typename T>
	void write_vector(T v) noexcept {
		write<std::uint32_t>(v.size());

		for (const auto& e : v) write<typename T::value_type>(e);
	}

	template <typename T>
	void write_map(T m) {
		write<size_t>(m.size());

		for (auto const& [k, v] : m) {
			write<typename T::key_type>(k);
			write<typename T::mapped_type>(v);
		}
	}

	template <typename T>
	T read_trivial(bool peek = false) noexcept {
		const std::size_t size = sizeof(T);

		T result;

		std::memcpy(&result, buff.data() + index, size);
		if (!peek) index += size;

		if (read_swap_endianness) swap_endianess<T>(&result);

		return result;
	}

	template <typename T>
	T read_string() noexcept {
		std::string result;

		char b;
		while ((b = buff.at(index++)) != '\0') {
			result.push_back(b);
		}

		return result;
	}

	template <typename T>
	T read_vector() noexcept {
		const std::uint32_t v_size = read<std::uint32_t>();

		T v;

		for (int i = 0; i < v_size; i++) {
			v.push_back(read<typename T::value_type>());
		}

		return v;
	}

	template <typename T>
	T read_map() noexcept {
		const std::size_t size = read<std::size_t>();

		T m;

		for (std::size_t i = 0; i < size; i++) {
			auto& dest = m[read<typename T::key_type>()];
			dest = read<typename T::mapped_type>();
		}

		return m;
	}
public:
	std::size_t index = 0;
	bool write_swap_endianness = false;
	bool read_swap_endianness = false;

	buffer() = default;

	explicit buffer(const std::vector<char>& nbuff) : buff(nbuff) {}

	void reset() {
		index = 0;
	}

	void clear() {
		buff.clear();
	}

	void set(const std::vector<char>& nbuff) {
		buff = nbuff;
	}

	void insert(size_t num, char val = '\0') {
		buff.insert(buff.end(), num, val);
		index += num;
	}

	std::size_t size() const {
		return buff.size();
	}

	std::vector<char> data() const {
		return buff;
	}

	template <typename T, std::enable_if_t<std::conjunction_v<std::is_trivial<T>, std::is_standard_layout<T>>>* = nullptr>
	void write(T v) noexcept {
		write_trivial<T>(v);
	}

	template <typename T, std::enable_if_t<is_string_type<T>::value>* = nullptr>
	void write(T v) noexcept {
		write_string<T>(v);
	}

	template <typename T, std::enable_if_t<is_vector_type<T>::value>* = nullptr>
	void write(T v) noexcept {
		write_vector<T>(v);
	}

	template <typename T, std::enable_if_t<is_map_type<T>::value>* = nullptr>
	void write(T v) noexcept {
		write_map<T>(v);
	}

	template <typename T, std::enable_if_t<std::conjunction_v<std::is_trivial<T>, std::is_standard_layout<T>>>* = nullptr>
	T read() noexcept {
		return read_trivial<T>();
	}

	template <typename T, std::enable_if_t<std::conjunction_v<std::is_trivial<T>, std::is_standard_layout<T>>>* = nullptr>
	T peek() noexcept {
		return read_trivial<T>(true);
	}

	template <typename T, std::enable_if_t<is_string_type<T>::value>* = nullptr>
	T read() noexcept {
		return read_string<T>();
	}

	template <typename T, std::enable_if_t<is_vector_type<T>::value>* = nullptr>
	T read() noexcept {
		return read_vector<T>();
	}

	template <typename T, std::enable_if_t<is_map_type<T>::value>* = nullptr>
	T read() noexcept {
		return read_map<T>();
	}
};

class file_buffer : public buffer {
public:
	void load(std::string path) {
		std::basic_ifstream<uint8_t> file(path, std::ios::binary);

		clear();
		reset();
		set(std::vector<char>((std::istreambuf_iterator<uint8_t>(file)), std::istreambuf_iterator<uint8_t>()));
	}

	void save(std::string path) {
		std::ofstream file(path, std::ios::out | std::ios::binary);
		file.write(reinterpret_cast<const char*>(data().data()), size());
	}
};
