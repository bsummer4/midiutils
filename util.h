// util.h -- Just some nice macros
typedef unsigned char byte;
#define debug(...) fprintf(stderr, __VA_ARGS__)
#define MAX(x, y) (x>y)?x:y
#define E(X,Y) if (-1 == Y) err(1, X)
#define SI static inline
