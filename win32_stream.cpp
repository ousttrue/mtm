#include "input_stream.h"

InputStream::InputStream() {}

void InputStream::Register(void *handle) {}
void InputStream::Unregister(void *handle) {}
void InputStream::Poll() {}
std::optional<std::span<char>> InputStream::Read(void *handle) { return {}; }
