#pragma once

namespace selector
{

void set(int fd);
void close(int fd);
void select();
bool isSet(int fd);

} // namespace selector
