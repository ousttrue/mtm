#pragma once

namespace selector
{

void set(int fd);
void close(int fd);
void select(int nfds);
bool isSet(int fd);

} // namespace selector
