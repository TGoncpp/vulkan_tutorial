#include "Time.h"

float Time::GetElapesedSec()
{
	static auto start = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start).count();

    return time;
}
