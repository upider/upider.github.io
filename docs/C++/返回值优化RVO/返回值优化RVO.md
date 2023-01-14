# c++ RVO

(1096 words) Fri, Aug 18, 2017

_Return Value Optimization_ (RVO), _Named RVO_ (NRVO) and _Copy-Elision_ are in C++ since C++98. In this post I will explain what these concepts mean and how they help improve runtime performance.

I will use our old friend `Snitch` - a class dedicated to printing at key events:

```c++
struct Snitch {   
  Snitch() { cout << "c'tor" << endl; }
  ~Snitch() { cout << "d'tor" << endl; }

  Snitch(const Snitch&) { cout << "copy c'tor" << endl; }
  Snitch(Snitch&&) { cout << "move c'tor" << endl; }

  Snitch& operator=(const Snitch&) {
    cout << "copy assignment" << endl;
    return *this;
  }

  Snitch& operator=(Snitch&&) {
    cout << "move assignment" << endl;
    return *this;
  }
};
```

Let’s get goin’.

## Return Value Optimization

_RVO_ basically means the compiler is allowed to avoid creating temporary objects for return values, **even if they have side effects**.

Here’s a simple example:

```c++
Snitch ExampleRVO() {
  return Snitch();
}

int main() {
  Snitch snitch = ExampleRVO();
}
```

Output (note that `-fno-elide-constructors` disables RVO in clang):

```shell
$ clang++ -std=c++11 main.cpp && ./a.out
c'tor
d'tor

$ clang++ -std=c++11 -fno-elide-constructors main.cpp && ./a.out
c'tor
move c'tor
d'tor
move c'tor
d'tor
d'tor
```

In the first run (without `-fno-elide-constructors`) the compiler refrained from calling user code despite it having a clear side effect (being printing to bash). This is also the **default** behavior, meaning practically all C++ programs utilize RVO.

Without RVO the compiler creates 3 `Snitch` objects instead of 1:

1.  A temporary object inside `ExampleRVO()` (when printing `c'tor`);
2.  A temporary object for the returned object inside `main()` (when printing the first `move c'tor`);
3.  The named object `snitch` (when printing the second `move c'tor`).

## Performance

The neat thing about RVO is that it makes returning objects **free**. It works via allocating memory for the to-be-returned object in the caller’s stack frame. The returning function then uses that memory as if it was in its own frame without the programmer knowing / caring.

In C++98 days this was significant:

```c++
#include <vector>
using namespace std;

vector<int> ReturnVector() {
  return vector<int>(1, 1);
}

int main() {
  for (int i = 0; i < 1000000000; ++i) {
    ReturnVector();
  }
}
```

Output:

```shell
$ clang++ -fno-elide-constructors -std=c++98 -stdlib=libc++ -O3 main.cpp && time ./a.out
real	0m37.235s
user	0m37.168s
sys	0m0.024s

$ clang++ -std=c++98 -stdlib=libc++ -O3 main.cpp && time ./a.out
real	0m17.681s
user	0m17.668s
sys	0m0.000s
```

217% difference on my machine by simply avoiding the copy of the `vector`. In C++11 (or newer) environments it is even marginally faster to disable RVO:

```shell
$ clang++ -fno-elide-constructors -std=c++11 -stdlib=libc++ -O3 main.cpp && time ./a.out
real	0m18.195s
user	0m18.188s
sys	0m0.000s

$ clang++ -std=c++11 -stdlib=libc++ -O3 main.cpp && time ./a.out
real	0m18.356s
user	0m18.340s
sys	0m0.000s

```

This is due to Move Semantics, which is the subject of the next post.

In trying to come up with an example where RVO is faster on modern C++ using STL containers I hit a wall, mostly because of move-semantics but also because on x86_84 RVO is in the ABI so disabling it is harder. **Please post such examples if you have any!**

## Named Return Value Optimization (NRVO)

_Named_ RVO is when an object with a name is returned but is nevertheless not copied. A simple example is:

```c++
Snitch ExampleNRVO() {
  Snitch snitch;
  return snitch;
}

int main() {
  ExampleNRVO();
}
```

Which has a similar output to `ExampleRVO()` above:

```shell
$ clang++ -std=c++11 main.cpp && ./a.out
c'tor
d'tor
```

While RVO is almost always going to happen, NRVO is more restricted, as we will see below. I personally don’t think NRVO deserves its own acronym.

## Copy Elision

RVO is part of a larger group of optimizations called _copy-elision_. Essentials are the same, except copy-elision is not required to happen as part of return statements, for example:

```c++
void foo(Snitch s) {
}

int main() {
  foo(Snitch());
}
```

Output:

```shell
c'tor
d'tor

```

In my experience, RVO is more frequent (and thus useful) than other copy-elision practices, but your mileage may vary.

## When RVO doesn’t / can’t happen

RVO is an optimization the compiler is _allowed_ to apply (starting C++17 it is in fact required to in certain cases). However, even in C++17 it is not always guaranteed. Let’s look at a few examples.

_The following examples are cases where, on my environment, RVO doesn’t happen. Some of them may change with other compiler / versions._

### Deciding on Instance at Runtime

When the compiler can’t know from within the function which instance will be returned it must disable RVO:

```c++
Snitch CreateSnitch(bool runtime_condition) {
  Snitch a, b;
  if (runtime_condition) {
    return a;
  } else {
    return b;
  }
}

int main() {
  Snitch snitch = CreateSnitch(true);
}
```

Output:

```shell
$ clang++ -std=c++11 -stdlib=libc++ main.cpp && ./a.out
c'tor
c'tor
move c'tor
d'tor
d'tor
d'tor

```

### Returning a Parameter / Global

When returning an object that is not created in the scope of the function there is no way to do RVO:

```c++
Snitch global_snitch;

Snitch ReturnParameter(Snitch snitch) {
  return snitch;
}

Snitch ReturnGlobal() {
  return global_snitch;
}

int main() {
  Snitch snitch = ReturnParameter(global_snitch);
  Snitch snitch2 = ReturnGlobal();
}
```

Output:

```shell
$ clang++ -std=c++11 -stdlib=libc++ main.cpp && ./a.out
c'tor
copy c'tor
move c'tor
d'tor
copy c'tor
d'tor
d'tor
d'tor

```

### Returning by `std::move()`

Returning by calling `std::move()` on the return value is an anti-pattern. It is wrong most of the times. It will indeed attempt to force move-constructor, but in doing so it will disable RVO. It is also redundant, as move will happen if it can even without explicitly calling `std::move()` (see [here](https://stackoverflow.com/questions/14856344/when-should-stdmove-be-used-on-a-function-return-value)).

```c++
Snitch CreateSnitch() {
  Snitch snitch;
  return std::move(snitch);
}

int main() {
  Snitch snitch = CreateSnitch();
}
```

Output:

```bash
$ clang++ -std=c++11 -stdlib=libc++ main.cpp && ./a.out
c'tor
move c'tor
d'tor
d'tor

```

### Assignment

RVO can only happen when an object is **created** from a returned value. Using `operator=` on an existing object rather than copy/move _constructor_ might be mistakenly thought of as RVO, but it isn’t:

```c++
Snitch CreateSnitch() {
  return Snitch();
}

int main() {
  Snitch s = CreateSnitch();
  s = CreateSnitch();
}
```

```bash
$ clang++ -std=c++11 -stdlib=libc++ main.cpp && ./a.out
c'tor
c'tor
move assignment
d'tor
d'tor

```

### Returning Member

In some cases even an unnamed variable can’t RVO:

```c++
struct Wrapper {
  Snitch snitch;
};

Snitch foo() {
  return Wrapper().snitch;
}

int main() {
  Snitch s = foo();
}
```

Output:

```bash
$ clang++ -std=c++11 -stdlib=libc++ main.cpp && ./a.out
c'tor
move c'tor
d'tor
d'tor

```

## Conclusion

While we can’t count on RVO to always take place, it will in most cases. For those cases where it doesn’t we always have Move Semantics, which is the topic of the [next post](https://shaharmike.com/cpp/move-semantics/). As always, optimize for readability rather than performance when writing code, unless you have a quantifiable reason. 
 [https://shaharmike.com/cpp/rvo/](https://shaharmike.com/cpp/rvo/)