# 1 "<stdin>"
# 1 "/home/anthony/Repositories/so3/usr/src/micropython/ports/soo//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/opt/toolchains/aarch64-none-linux-gnu_10.2/aarch64-none-linux-gnu/libc/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "<stdin>"
# 29 "<stdin>"
# 1 "../../py/mpconfig.h" 1
# 75 "../../py/mpconfig.h"
# 1 "./mpconfigport.h" 1
# 1 "../../../../lib/libc/include/stdint.h" 1
# 20 "../../../../lib/libc/include/stdint.h"
# 1 "../../../../lib/libc/include/bits/alltypes.h" 1







typedef __builtin_va_list va_list;
typedef __builtin_va_list __isoc_va_list;

typedef struct _IO_FILE FILE;

typedef unsigned long uintptr_t;

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef long long intmax_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long u_int64_t;
typedef unsigned long long uintmax_t;

typedef uint32_t mode_t;

typedef unsigned long long ino_t;

typedef uint16_t u_short;

typedef void *timer_t;
typedef int clockid_t;
typedef int pid_t;
typedef unsigned uid_t;
typedef unsigned gid_t;

typedef long clock_t;
typedef uint64_t time_t;

typedef uint64_t suseconds_t;
struct timeval { time_t tv_sec; suseconds_t tv_usec; };
struct timespec { time_t tv_sec; time_t tv_nsec; };


typedef unsigned wchar_t;

typedef unsigned long size_t;
typedef long ssize_t;
typedef unsigned int locale_t;

typedef unsigned int off_t;

typedef float float_t;
typedef double double_t;

typedef unsigned wint_t;
typedef struct __mbstate_t { unsigned __opaque1, __opaque2; } mbstate_t;
typedef unsigned long wctype_t;
typedef long ptrdiff_t;

typedef struct { long long __ll; long double __ld; } max_align_t;

struct iovec { void *iov_base; size_t iov_len; };

typedef struct { union { int __i[9]; volatile int __vi[9]; unsigned __s[9]; } __u; } pthread_attr_t;
typedef struct { union { int __i[6]; volatile int __vi[6]; volatile void *volatile __p[6]; } __u; } pthread_mutex_t;
typedef struct { union { int __i[6]; volatile int __vi[6]; volatile void *volatile __p[6]; } __u; } mtx_t;
typedef struct { union { int __i[12]; volatile int __vi[12]; void *__p[12]; } __u; } pthread_cond_t;
typedef struct { union { int __i[12]; volatile int __vi[12]; void *__p[12]; } __u; } cnd_t;
typedef struct { union { int __i[8]; volatile int __vi[8]; void *__p[8]; } __u; } pthread_rwlock_t;
typedef struct { union { int __i[5]; volatile int __vi[5]; void *__p[5]; } __u; } pthread_barrier_t;

typedef struct __sigset_t { unsigned long __bits[128/sizeof(long)]; } sigset_t;

typedef uint32_t socklen_t;
# 21 "../../../../lib/libc/include/stdint.h" 2

typedef int8_t int_fast8_t;
typedef int64_t int_fast64_t;

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef uint8_t uint_fast8_t;
typedef uint64_t uint_fast64_t;

typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;
# 95 "../../../../lib/libc/include/stdint.h"
# 1 "../../../../lib/libc/include/bits/stdint.h" 1
typedef int32_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
# 96 "../../../../lib/libc/include/stdint.h" 2
# 2 "./mpconfigport.h" 2
# 1 "../../../../lib/libc/include/unistd.h" 1







# 1 "../../../../lib/libc/include/features.h" 1
# 9 "../../../../lib/libc/include/unistd.h" 2
# 35 "../../../../lib/libc/include/unistd.h"
typedef long intptr_t;

ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);
int pipe(int [2]);
pid_t fork(void);

pid_t getpid(void);
int dup2(int, int);
int dup(int);

int execve(const char *path, char *const argv[], char *const envp[]);
int execv(const char *, char *const []);
int execle(const char *, const char *, ...);
int execl(const char *, const char *, ...);
int execvp(const char *, char *const []);
int execlp(const char *, const char *, ...);
int close(int);

void *sbrk(intptr_t);
extern char **__environ;

unsigned sleep(unsigned);
int usleep(unsigned);
# 3 "./mpconfigport.h" 2
# 25 "./mpconfigport.h"
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef long mp_off_t;


# 1 "../../../../lib/libc/include/alloca.h" 1
# 11 "../../../../lib/libc/include/alloca.h"
void *alloca(size_t);
# 31 "./mpconfigport.h" 2
# 76 "../../py/mpconfig.h" 2
# 30 "<stdin>" 2





QCFG(BYTES_IN_LEN, (1))
QCFG(BYTES_IN_HASH, (1))

Q()
Q(*)
Q(_)
Q(/)
# 50 "<stdin>"
Q({:#o})
Q({:#x})

Q({:#b})
Q( )
Q(\n)
Q(maximum recursion depth exceeded)
Q(<module>)
Q(<lambda>)
Q(<listcomp>)
Q(<dictcomp>)
Q(<setcomp>)
Q(<genexpr>)
Q(<string>)
Q(<stdin>)
Q(utf-8)


Q(.frozen)







Q(ArithmeticError)

Q(ArithmeticError)

Q(AssertionError)

Q(AssertionError)

Q(AssertionError)

Q(AttributeError)

Q(AttributeError)

Q(BaseException)

Q(BaseException)

Q(EOFError)

Q(EOFError)

Q(Ellipsis)

Q(Ellipsis)

Q(Exception)

Q(Exception)

Q(GeneratorExit)

Q(GeneratorExit)

Q(ImportError)

Q(ImportError)

Q(IndentationError)

Q(IndentationError)

Q(IndexError)

Q(IndexError)

Q(KeyError)

Q(KeyError)

Q(KeyboardInterrupt)

Q(KeyboardInterrupt)

Q(LookupError)

Q(LookupError)

Q(MemoryError)

Q(MemoryError)

Q(NameError)

Q(NameError)

Q(NoneType)

Q(NotImplementedError)

Q(NotImplementedError)

Q(OSError)

Q(OSError)

Q(OverflowError)

Q(OverflowError)

Q(RuntimeError)

Q(RuntimeError)

Q(StopIteration)

Q(StopIteration)

Q(SyntaxError)

Q(SyntaxError)

Q(SystemExit)

Q(SystemExit)

Q(TypeError)

Q(TypeError)

Q(ValueError)

Q(ValueError)

Q(ZeroDivisionError)

Q(ZeroDivisionError)

Q(_0x0a_)

Q(__add__)

Q(__bool__)

Q(__build_class__)

Q(__call__)

Q(__class__)

Q(__class__)

Q(__class__)

Q(__class__)

Q(__class__)

Q(__contains__)

Q(__delitem__)

Q(__delitem__)

Q(__enter__)

Q(__eq__)

Q(__eq__)

Q(__exit__)

Q(__ge__)

Q(__getattr__)

Q(__getattr__)

Q(__getitem__)

Q(__getitem__)

Q(__getitem__)

Q(__getitem__)

Q(__gt__)

Q(__hash__)

Q(__iadd__)

Q(__import__)

Q(__init__)

Q(__init__)

Q(__int__)

Q(__isub__)

Q(__iter__)

Q(__le__)

Q(__len__)

Q(__lt__)

Q(__main__)

Q(__main__)

Q(__module__)

Q(__name__)

Q(__name__)

Q(__name__)

Q(__name__)

Q(__name__)

Q(__name__)

Q(__name__)

Q(__ne__)

Q(__new__)

Q(__new__)

Q(__next__)

Q(__next__)

Q(__next__)

Q(__next__)

Q(__path__)

Q(__path__)

Q(__path__)

Q(__path__)

Q(__qualname__)

Q(__repl_print__)

Q(__repl_print__)

Q(__repr__)

Q(__repr__)

Q(__setitem__)

Q(__setitem__)

Q(__str__)

Q(__sub__)

Q(__traceback__)

Q(_brace_open__colon__hash_b_brace_close_)

Q(_brace_open__colon__hash_o_brace_close_)

Q(_brace_open__colon__hash_x_brace_close_)

Q(_lt_dictcomp_gt_)

Q(_lt_dictcomp_gt_)

Q(_lt_genexpr_gt_)

Q(_lt_genexpr_gt_)

Q(_lt_lambda_gt_)

Q(_lt_lambda_gt_)

Q(_lt_listcomp_gt_)

Q(_lt_listcomp_gt_)

Q(_lt_module_gt_)

Q(_lt_module_gt_)

Q(_lt_setcomp_gt_)

Q(_lt_setcomp_gt_)

Q(_lt_stdin_gt_)

Q(_lt_stdin_gt_)

Q(_lt_string_gt_)

Q(_space_)

Q(_star_)

Q(_star_)

Q(abs)

Q(all)

Q(any)

Q(append)

Q(args)

Q(bin)

Q(bool)

Q(bool)

Q(bound_method)

Q(builtins)

Q(builtins)

Q(bytecode)

Q(bytes)

Q(bytes)

Q(callable)

Q(chr)

Q(classmethod)

Q(classmethod)

Q(clear)

Q(clear)

Q(close)

Q(close)

Q(closure)

Q(copy)

Q(copy)

Q(count)

Q(count)

Q(dict)

Q(dict)

Q(dict_view)

Q(dir)

Q(divmod)

Q(end)

Q(endswith)

Q(errno)

Q(eval)

Q(exec)

Q(extend)

Q(find)

Q(format)

Q(from_bytes)

Q(function)

Q(function)

Q(function)

Q(function)

Q(function)

Q(function)

Q(function)

Q(generator)

Q(generator)

Q(get)

Q(getattr)

Q(globals)

Q(hasattr)

Q(hash)

Q(hex)

Q(id)

Q(index)

Q(index)

Q(index)

Q(insert)

Q(int)

Q(int)

Q(isalpha)

Q(isdigit)

Q(isinstance)

Q(islower)

Q(isspace)

Q(issubclass)

Q(isupper)

Q(items)

Q(iter)

Q(iterator)

Q(iterator)

Q(iterator)

Q(iterator)

Q(join)

Q(key)

Q(keys)

Q(keys)

Q(len)

Q(list)

Q(list)

Q(little)

Q(little)

Q(locals)

Q(lower)

Q(lstrip)

Q(map)

Q(map)

Q(micropython)

Q(module)

Q(next)

Q(object)

Q(object)

Q(oct)

Q(ord)

Q(pop)

Q(pop)

Q(popitem)

Q(pow)

Q(print)

Q(range)

Q(range)

Q(range)

Q(remove)

Q(replace)

Q(repr)

Q(reverse)

Q(reverse)

Q(rfind)

Q(rindex)

Q(round)

Q(rsplit)

Q(rstrip)

Q(send)

Q(send)

Q(sep)

Q(setattr)

Q(setdefault)

Q(sort)

Q(sorted)

Q(split)

Q(startswith)

Q(staticmethod)

Q(staticmethod)

Q(str)

Q(str)

Q(strip)

Q(sum)

Q(super)

Q(super)

Q(super)

Q(throw)

Q(throw)

Q(to_bytes)

Q(tuple)

Q(tuple)

Q(type)

Q(type)

Q(update)

Q(upper)

Q(value)

Q(values)

Q(zip)

Q(zip)
