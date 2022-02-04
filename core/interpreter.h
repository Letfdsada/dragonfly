// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <absl/types/span.h>

#include <functional>
#include <string_view>

typedef struct lua_State lua_State;

namespace dfly {

class ObjectExplorer {
 public:
  virtual ~ObjectExplorer() {
  }

  virtual void OnBool(bool b) = 0;
  virtual void OnString(std::string_view str) = 0;
  virtual void OnDouble(double d) = 0;
  virtual void OnInt(int64_t val) = 0;
  virtual void OnArrayStart(unsigned len) = 0;
  virtual void OnArrayEnd() = 0;
  virtual void OnNil() = 0;
  virtual void OnStatus(std::string_view str) = 0;
  virtual void OnError(std::string_view str) = 0;
};

class Interpreter {
 public:
  using MutableSlice = absl::Span<char>;
  using MutSliceSpan = absl::Span<MutableSlice>;
  using RedisFunc = std::function<void(MutSliceSpan, ObjectExplorer*)>;

  Interpreter();
  ~Interpreter();

  Interpreter(const Interpreter&) = delete;
  void operator=(const Interpreter&) = delete;

  // Note: We leak the state for now.
  // Production code should not access this method.
  lua_State* lua() {
    return lua_;
  }

  enum AddResult {
    OK = 0,
    ALREADY_EXISTS = 1,
    COMPILE_ERR = 2,
  };

  // returns false if an error happenned, sets error string into result.
  // otherwise, returns true and sets result to function id.
  // function id is sha1 of the function body.
  AddResult AddFunction(std::string_view body, std::string* result);

  // Runs already added function f_id returned by a successful call to AddFunction().
  // Returns: true if the call succeeded, otherwise fills error and returns false.
  bool RunFunction(const char* f_id, std::string* err);

  void SetGlobalArray(const char* name, MutSliceSpan args);

  bool Execute(std::string_view body, char f_id[41], std::string* err);
  bool Serialize(ObjectExplorer* serializer, std::string* err);

  // fp must point to buffer with at least 41 chars.
  // fp[40] will be set to '\0'.
  static void FuncSha1(std::string_view body, char* fp);

  template <typename U> void SetRedisFunc(U&& u) {
    redis_func_ = std::forward<U>(u);
  }

 private:
  // Returns true if function was successfully added,
  // otherwise returns false and sets the error.
  bool AddInternal(const char* f_id, std::string_view body, std::string* error);

  int RedisGenericCommand(bool raise_error);

  static int RedisCallCommand(lua_State* lua);
  static int RedisPCallCommand(lua_State* lua);

  lua_State* lua_;
  unsigned cmd_depth_ = 0;
  RedisFunc redis_func_;
};

}  // namespace dfly
