typedef unsigned char byte;
#define err(...) fprintf(stderr, __VA_ARGS__), exit(1)
#define perr(x) perror(x), exit(1)
#define MAX(x, y) (x>y)?x:y
#define E(X,Y) if (-1 == Y) perr(X)
