dnl config.m4 for extension aop

dnl Comments in this file start with the string 'dnl'

PHP_ARG_ENABLE(AOP, whether to enable AOP support,
[  --enable-AOP            Enable AOP support])

PHP_ARG_ENABLE([AOP],
    [whether to enable AOP support],
    [AS_HELP_STRING([--enable-AOP],
        [Enable AOP support])], [yes], [yes])

PHP_ARG_ENABLE([debug],
    [whether to enable debug mode],
    [AS_HELP_STRING([--enable-debug],
        [Enable debug mode])], [no], [no])

if test "$PHP_AOP" != "no"; then
  AC_DEFINE(HAVE_AOP, 1, [aop])

  if test "$PHP_DEBUG" = "yes"; then
    CFLAGS="$CFLAGS -O0 -g3 -ggdb -fprofile-arcs"
  fi

  aop_sources="php8/aop.c php8/aop_execute.c php8/aop_joinpoint.c php8/lexer.c"
  PHP_NEW_EXTENSION(aop, $aop_sources, $ext_shared)

  PHP_ADD_EXTENSION_DEP(aop, [pcre])
fi
