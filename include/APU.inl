// apu inline methods

inline s16 APU::getChannelStream(int channel_index, int buffer_index) {
	std::cout<<"ch_index: "<<channel_index<<", buf_ind: "<<buffer_index<<std::endl;
	return this->channel[channel_index].stream[buffer_index];
}

inline void APU::setChannelStream(int channel_index, u16 data_buffer_len, s16 *stream_data) {
	std::cout<<"bufflen: "<<data_buffer_len<<", sizeof: "<<sizeof(stream_data)<<std::endl;
	this->buffer_len = data_buffer_len;
	std::memcpy(this->channel[channel_index].stream, stream_data, sizeof(stream_data));
	// for(int i = 0; i < this->buffer_len; i++) {
		// this->channel[channel_index].stream[i] = stream_data[i];
	// }
}

inline int APU::getAmplitude() {
	return this->AMPLITUDE;
}

inline void APU::setAmplitude(int val) {
	this->AMPLITUDE = val;
}

inline int APU::getSampleRate() {
	return this->SAMPLE_RATE;
}

inline void APU::setSampleRate(int val) {
	this->SAMPLE_RATE = val;
}

inline int APU::getSampleSize() {
	return this->sample_size;
}

inline void APU::setSampleSize(int val) {
	this->sample_size = val;
}

inline int APU::getBufferSize() {
	return this->BUFFER_SIZE;
}

inline void APU::setBufferSize(int val) {
	this->BUFFER_SIZE = val;
}

inline s8 APU::getDriverID() { 
	return this->driver_id; 
}

inline void APU::setDriverID(s8 val) {
	this->driver_id = val;
}
