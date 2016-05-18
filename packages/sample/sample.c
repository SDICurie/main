#include <stdint.h>

static void (*_callback)(uint32_t) = 0;

static uint32_t second = 1;
static uint32_t first = 0;
static uint32_t fibo = 0;

void sample_install(void (*callback)(uint32_t))
{
	_callback = callback;
	return;
}

void sample_trigger(void)
{
	if (_callback) {
		fibo = first + second;
		first = second;
		second = fibo;
		_callback(fibo);
	}
}
