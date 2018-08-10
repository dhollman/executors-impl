
#include <omp.h>

#include <experimental/execution>
#include <experimental/type_traits>

#include <type_traits>

// have to emulate concepts for now:
template <typename C>
using value_element_archetype = decltype(std::declval<C>().value_element(
  std::declval<C>().shape(), std::declval<C>().value_shared_factory(), std::declval<C>().value_result_factory()
));

struct OpenMPBulkExecutor {
  template <class BulkContinuation>
  auto execute(BulkContinuation&& c)
    -> std::enable_if_t<std::experimental::is_detected_v<value_element_archetype, BulkContinuation>>
  {
    auto& sf = c.value_shared_factory();
    auto& rf = c.value_result_factory();
    auto shape = c.shape();
    const auto chunk = shape / omp_get_max_threads();
    const auto extra = shape % (chunk * omp_get_max_threads());
    #pragma omp parallel shared(sf, rf)
    {
      auto my_chunk = chunk + (omp_get_thread_num() < extra);
      auto my_off = chunk * omp_get_thread_num() + min(omp_get_thread_num(), extra);
      for(decltype(shape) i = my_off; i < my_off + my_chunk; ++i) {
        c.value_element(i, sf, rf);
      }
    } // end omp parallel
  }
  template <class Continuation>
  auto execute(Continuation&& c)
    -> std::enable_if_t<!std::experimental::is_detected_v<value_element_archetype, Continuation>>
  {
    std::forward<Continuation>(c).value(c);
  }
};

//==============================================================================
// This would go in the helpers library

template <typename BulkF>
struct _on_bulk_void {

  // TODO figure out what the bulk helper function does

};


//==============================================================================

int main() {

}