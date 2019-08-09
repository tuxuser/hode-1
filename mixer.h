
#ifndef MIXER_H__
#define MIXER_H__

struct Mixer {

	static const int kPcmChannels = 32;

	void (*_lock)(int);
	int _rate;

	struct {
		const int16_t *ptr;
		const int16_t *end;
		int panL;
		int panR;
		uint8_t panType;
		bool stereo;
	} _mixingQueue[kPcmChannels];
	int _mixingQueueSize;

	Mixer();
	~Mixer();

	void init(int rate);
	void fini();

	void mix(int16_t *buf, int len);
};

#endif
