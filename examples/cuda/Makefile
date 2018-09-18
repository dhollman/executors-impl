

CUDALIB=/usr/local/cuda/lib
CLANG=/Users/dshollm/install/clang-4_opt/bin/clang
GPUARCH=sm_30
CPPFLAGS=-isystem /usr/local/opt/llvm/include/c++/v1
CPPFLAGS+=-isystem /Users/dshollm/Code/llvm/build/concepts/clang-4_opt/lib/clang/6.0.0/include
CPPFLAGS+='-I../../include'
LDFLAGS='-L/usr/local/opt/llvm/Toolchains/LLVM8.0.0svn.xctoolchain/usr/lib/'
LDFLAGS+=-lc++ -L${CUDALIB} -lcudart
CXXFLAGS=--cuda-gpu-arch=${GPUARCH} -std=c++2a -Xclang -fconcepts-ts
CXXFLAGS+=-Wno-unknown-attributes -Wno-exceptions


demo4: demo4.cu *.hpp
	${CLANG} ${CXXFLAGS} $< -o $@  ${CPPFLAGS} ${LDFLAGS}


clean:
	rm -f test-clang

