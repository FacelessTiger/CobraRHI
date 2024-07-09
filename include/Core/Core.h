#pragma once

#include <Core/Math.h>
#include <memory>
#include <span>

namespace Cobra {

	template<typename T>
	struct Impl;

	template<class T>
	concept IsContiguousContainer = requires(T t)
	{
		{ t.begin() } -> std::contiguous_iterator;
		{ t.end() }   -> std::contiguous_iterator;
	};

	template<typename T>
	struct Span
	{
	public:
		Span(const std::initializer_list<std::reference_wrapper<T>>& initializer)
			: m_Span(initializer.begin(), initializer.end())
		{ }

		template <typename C>
		requires IsContiguousContainer<C>
		Span(C&& c)
			: m_Span(c.begin(), c.end())
		{ }

		Span(const T& v)
			: m_Span(&v, 1)
		{ }

		Span(const T* first, size_t count)
			: m_Span(first, count)
		{ }

		const T& operator[](uint32_t i) const noexcept { return m_Span.begin()[i]; }
		T& operator[](uint32_t i) noexcept { return m_Span.begin()[i]; }

		Span(const Span& other) : m_Span(other.m_Span) { }
		Span& operator=(const Span& other) { m_Span = other.m_Span; }

		decltype(auto) begin() const { return m_Span.data(); }
		decltype(auto) end() const { return m_Span.data() + m_Span.size(); }

		const T* data() const { return m_Span.data(); }
		constexpr size_t size() const { return m_Span.size(); }
		bool empty() const { return m_Span.empty(); }
	private:
		std::span<const std::reference_wrapper<T>> m_Span;
	};

}