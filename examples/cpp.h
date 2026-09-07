/* cpp.h -- included by the body of examples/cpp.mx with #include "cpp.h".
   Its text goes through the same rules it was included into, so the macro
   it defines is known to the lines after the include, and the macro's use
   inside the function below is expanded before the function is emitted. */
#define FROM_HEADER 100
static int helper(int x) { return x + FROM_HEADER; }
