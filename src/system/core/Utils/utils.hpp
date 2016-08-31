#ifndef UTILS_HPP
#define UTILS_HPP
#include <tuple>
#include <memory>
#include <mutex>

template<class F, class...Args>
constexpr void for_each_arg(F&&f,Args&&...args){
  using discard=int[];
  (void)discard{0,((void)(
    f(std::forward<Args>(args))
  ),0)...};
}

template<size_t... Is, class Tuple, class F>
constexpr void for_each_tuple(Tuple&& tup, std::index_sequence<Is...>, F&& f) {
  using std::get;
  for_each_arg(
    std::forward<F>(f),
    get<Is>(std::forward<Tuple>(tup))...
  );
}

template<class T>
class Singleton {
    static std::unique_ptr<T> ptr;
    static std::once_flag once;
public:
    template<class ...Args>
    static decltype (auto) getHandler (Args&&... args) {
        std::call_once(once,[&] (Args&&... args) {
            Singleton<T>::ptr.reset(new T(std::forward<Args>(args)...));
        },std::forward<Args>(args)...);
        return ptr.get();
    }
};

template<class T> std::unique_ptr<T> Singleton<T>::ptr = nullptr;
template<class T> std::once_flag Singleton<T>::once;

#endif // UTILS_HPP
