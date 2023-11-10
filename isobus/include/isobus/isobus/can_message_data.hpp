//================================================================================================
/// @file can_message_data.hpp
///
/// @brief Contains common types and functions for working with the data of a CAN message.
/// @author Daan Steenbergen
///
/// @copyright 2022 Open Agriculture
//================================================================================================
#ifndef CAN_MESSAGE_DATA_HPP
#define CAN_MESSAGE_DATA_HPP

#include <array>
#include <cstddef>

namespace isobus
{
	//================================================================================================
	/// @class DataSpan
	///
	/// @brief A class that represents a span of data of arbitrary length.
	//================================================================================================
	template<typename T>
	class DataSpan
	{
	public:
		/// @brief Construct a new DataSpan object of a writeable array.
		/// @param ptr pointer to the buffer to use.
		/// @param len The number of elements in the buffer.
		DataSpan(T *ptr, std::size_t size) :
		  ptr(ptr),
		  _size(size)
		{
		}

		/// @brief Get the element at the given index.
		/// @param index The index of the element to get.
		/// @return The element at the given index.
		T &operator[](std::size_t index)
		{
			return ptr[index * sizeof(T)];
		}

		/// @brief Get the element at the given index.
		/// @param index The index of the element to get.
		/// @return The element at the given index.
		T const &operator[](std::size_t index) const
		{
			return ptr[index * sizeof(T)];
		}

		/// @brief Get the size of the data span.
		/// @return The size of the data span.
		std::size_t size() const
		{
			return _size;
		}

		/// @brief Get the begin iterator.
		/// @return The begin iterator.
		T *begin()
		{
			return ptr;
		}

		/// @brief Get the end iterator.
		/// @return The end iterator.
		T *end()
		{
			return ptr + _size * sizeof(T);
		}

	private:
		T *ptr;
		std::size_t _size;
	};

	//================================================================================================
	/// @class DataSpanFactory
	///
	/// @brief A class that can be used to construct a DataSpan from various types.
	//================================================================================================
	class DataSpanFactory
	{
	public:
		/// @brief Factory method to create a DataSpan from a std::array.
		/// @param array The array to create the DataSpan from.
		/// @return The created DataSpan.
		template<typename T, std::size_t N>
		static DataSpan<T> fromArray(std::array<T, N> &array)
		{
			return DataSpan<T>(array.data(), N);
		}

		/// @brief Factory method to create a DataSpan of consts elements from a std::array of non-const elements.
		/// @param array The array to create the DataSpan from.
		/// @return The created DataSpan of const elements.
		template<typename T, std::size_t N>
		static DataSpan<std::add_const_t<T>> cfromArray(std::array<T, N> &array)
		{
			return DataSpan<std::add_const_t<T>>(array.data(), N);
		}
	};
}
#endif // CAN_MESSAGE_DATA_HPP
