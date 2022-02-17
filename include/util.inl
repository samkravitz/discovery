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

inline s8 signum(double val) 
{
    if(val != val) throw std::invalid_argument("Invalid argument passed to util::signum");
    else return (s8) (0. < val) - (val < 0.);
}

// convert lapsed processor time to milliseconds
inline double systime_to_ms(double time)
{
  return (double) time / CLOCKS_PER_SEC * 1000;
}

// convert lapsed processor time to seconds
inline double systime_to_sec(double time)
{
  return (double) time / CLOCKS_PER_SEC;
}
