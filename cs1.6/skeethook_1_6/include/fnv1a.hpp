#pragma once

using fnv1a_t = uint64_t;

namespace fnv1a
{
	constexpr fnv1a_t ullBasis = 0xCBF29CE484222325ULL;
	constexpr fnv1a_t ullPrime = 0x100000001B3ULL;
	constexpr fnv1a_t val_64_const = 0xcbf29ce484222325;
	constexpr fnv1a_t prime_64_const = 0x100000001b3;

	consteval fnv1a_t hash_const(const char* szString, const fnv1a_t uKey = ullBasis) noexcept
	{
		return (szString[0] == '\0') ? uKey : hash_const(&szString[1], (uKey ^ static_cast<fnv1a_t>(szString[0])) * ullPrime);
	}

	inline fnv1a_t hash(const char* szString, fnv1a_t uKey = ullBasis) noexcept
	{
		const size_t lenght = CRT::StringLength(szString);

		for (size_t i = 0U; i < lenght; ++i)
			uKey = (uKey ^ szString[i]) * ullPrime;

		return uKey;
	}
}