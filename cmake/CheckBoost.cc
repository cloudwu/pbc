#include <boost/version.hpp>
#include <stdio.h>

int
main() {
  int major = BOOST_VERSION / 100000;
  int minor = BOOST_VERSION / 100 % 1000;
  puts(BOOST_LIB_VERSION);
  return major == 1 && minor >= 34 ? 0 : 1;
}
