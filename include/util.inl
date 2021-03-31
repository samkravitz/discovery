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

	static u32 mask = ~0;
    val &= mask >> (31 - end);
    val >>= start;
    return val;
}

template <int end, int start>
inline u16 bitseq(u16 val)
{
	if (end < start)
		return 0;

	static u16 mask = ~0;
    val &= mask >> (15 - end);
    val >>= start;
    return val;
}

inline u32 u16ToU32Color(u16 color_u16)
{
    u32 r, g, b;

    r = color_u16 & 0x1F; color_u16 >>= 5; // bits  0 - 5
    g = color_u16 & 0x1F; color_u16 >>= 5; // bits  6 - 10
    b = color_u16 & 0x1F; color_u16 >>= 5; // bits 11 - 15

    return r << 19 | g << 11 | b << 3;
}

// inline bool pathExists(const std::string &path)
// {
// 	std::fstream fin(path);
// 	return fin.fail();
// }