// apu inline methods

inline s16 APU::getChannelStream(int channel_index, int buffer_index) {
	return this->channel[channel_index].stream[buffer_index];
}

// inline void APU::setChannelStream(int channel_index, u16 data_buffer_len, s16 *stream_data) {
// 	this->buffer_len = data_buffer_len;
// 	std::memcpy(this->channel[channel_index].stream, stream_data, data_buffer_len * sizeof(stream_data));
// }

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

inline int APU::getBufferSize() {
	return this->BUFFER_SIZE;
}

inline int APU::getSampleCount() {
	return this->NUM_SAMPLES;
}

inline u16 APU::getBufferLength() {
	return this->BUFFER_LEN;
}

inline void APU::setBufferLength(u16 val) {
	this->BUFFER_LEN = val;
}

inline s8 APU::getDriverID() { 
	return this->driver_id; 
}

inline void APU::setDriverID(s8 val) {
	this->driver_id = val;
}

inline u32 APU::getInternalBufferSize(u8 channel) {
	return this->channel[channel].stream.size();
}

inline void APU::popInternalBuffer(u8 channel) {
	this->channel[channel].stream.pop_back();
}

inline double APU::getSamplesPerFrame() {
	return this->SAMPLE_RATE / config::framerate * 2;
}

inline double APU::getBytesPerFrame() {
	return this->getSamplesPerFrame() * this->NUM_SAMPLES;
}

inline double APU::getAudioBufferSize() {
	return this->getBytesPerFrame() * 10;
}

inline CircularBuffer<s16> *APU::getAudioBufferRef() {
	return this->audio_buffer;
}
