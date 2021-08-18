// apu inline methods

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

inline void APU::setBufferSize(int val) {
	this->BUFFER_SIZE = val;
}

inline s8 APU::getDriverID() { 
	return this->driver_id; 
}

inline void APU::setDriverID(s8 val) {
	this->driver_id = val;
}
