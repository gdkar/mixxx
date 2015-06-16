#ifndef UTIL_H
#define UTIL_H

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName &) = delete;\
  TypeName &operator=(const TypeName&) = delete;

#endif /* UTIL_H */
