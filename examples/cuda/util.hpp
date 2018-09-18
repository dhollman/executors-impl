#pragma once

#include <string>
#include <stdexcept>
#include <iostream>

#define __REQUIRES(...) typename std::enable_if<(__VA_ARGS__)>::type* = nullptr

#define CHECK_CUDA_ERROR(...) \
  if(auto _error_##__LINE__ = __VA_ARGS__) { \
    throw ::std::runtime_error("Error executing " \
      + std::string(#__VA_ARGS__) + std::string(" in ") \
      + std::string(__PRETTY_FUNCTION__) + std::string(": ") \
      + std::string(cudaGetErrorString(_error_##__LINE__)) \
    ); \
  }


#define __CONTRACT_IMPL(action, what, exprstr, ...) \
  if(!(__VA_ARGS__)) { \
    throw ::std::runtime_error("Contract failure at " \
      + std::string(__FILE__) + std::string(":") \
      + std::to_string(__LINE__) + std::string(": ") \
      + std::string(__PRETTY_FUNCTION__) \
      + std::string(" ") + std::string(action) \
      + std::string(" the following expression is ") \
      + std::string(what) + std::string(": ") \
      + std::string(exprstr) \
    ); \
  }

//#define __DEVICE_CONTRACT_IMPL(action, what, exprstr, ...) \
//  if(!(__VA_ARGS__)) { \
//    std::printf(("Contract failure at " \
//      + std::string(__FILE__) + std::string(":") \
//      + std::to_string(__LINE__) + std::string(": ") \
//      + std::string(__PRETTY_FUNCTION__) \
//      + std::string(" ") + std::string(action) \
//      + std::string(" the following expression is ") \
//      + std::string(what) + std::string(": ") \
//      + std::string(exprstr) \
//    )); \
//    std::terminate(); \
//  }

#define EXPECTS_NOT_NULL(...) \
  __CONTRACT_IMPL("expects that", "not null", #__VA_ARGS__, __VA_ARGS__ != nullptr)
  
//#define DEVICE_EXPECTS_NOT_NULL(...) \
//  __DEVICE_CONTRACT_IMPL("expects that", "not null", #__VA_ARGS__, __VA_ARGS__ != nullptr)

#define ENSURES_NOT_NULL(...) \
  __CONTRACT_IMPL("should ensure that", "not null", #__VA_ARGS__, __VA_ARGS__ != nullptr)

#define __EXPECTS(...) \
  __CONTRACT_IMPL("expects that", "true", #__VA_ARGS__, __VA_ARGS__)

#define __ENSURES(...) \
  __CONTRACT_IMPL("should ensure that", "true", #__VA_ARGS__, __VA_ARGS__)