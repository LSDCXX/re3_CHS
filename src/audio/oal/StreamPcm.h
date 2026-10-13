#pragma once

#include <cstdint>
#include <vector>

// Decoders produce planar stereo. Restore the original interleaved sample
// order without mixing, scaling or changing the source sample rate.
static bool
InterleaveStreamPcm(const int16_t *input, size_t samples, unsigned channels, std::vector<int16_t> &output)
{
	if(channels != 1 && channels != 2) return false;
	if(samples % channels) return false;
	const size_t frames = samples / channels;
	output.resize(frames * 2);
	for(size_t i = 0; i < frames; i++) {
		output[i * 2] = input[i];
		output[i * 2 + 1] = input[channels == 2 ? frames + i : i];
	}
	return true;
}
