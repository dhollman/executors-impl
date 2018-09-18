
#pragma once

#include <experimental/execution>

template <class BlockingProperty = std::experimental::execution::blocking_t::possibly_t>
class cuda_stream_executor;