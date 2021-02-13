/* get subset of bits for purposes like destination register, opcode, shifts
 * All instructions have data hidden within their codes;
 * ex: A branch instruction holds the offset in bits [23-0]
 * This function will extract the bits.
 * Since the reference I am using is in reverse bit order, end >= start must be true
 * 
 * ex: bitseq<7, 4>(0b11110000) = 0b1111
 */

template <int end, int start>
inline u32 bitseq(u32 val)
{
	if (end < start)
		return 0;

	std::bitset<32> bits(val);
	u32 subset = 0;

	for (int i = end; i >= start; --i)
	{
		subset <<= 1;
		subset |= bits[i];
	}

	return subset;
}

template <int end, int start>
inline u16 bitseq(u16 val)
{
	if (end < start)
		return 0;

	std::bitset<16> bits(val);
	u16 subset = 0;

	for (int i = end; i >= start; --i)
	{
		subset <<= 1;
		subset |= bits[i];
	}

	return subset;
}