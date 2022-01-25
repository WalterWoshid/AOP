PHP_ARG_ENABLE(AOP, whether to enable AOP support,
[ --enable-AOP   Enable AOP support])

if test "$PHP_AOP" = "yes"; then
  AC_DEFINE(HAVE_AOP, 1, [aop])
  dnl for PHP 8.x
  PHP_NEW_EXTENSION(aop, php8/aop.c php8/lexer.c php8/aop_execute.c php8/aop_joinpoint.c, $ext_shared)
  PHP_ADD_EXTENSION_DEP(aop, [pcre])

  dnl for PHP 7.x
  dnl PHP_NEW_EXTENSION(aop, php7/aop.c php7/lexer.c php7/aop_execute.c php7/aop_joinpoint.c, $ext_shared)

  dnl for PHP 5.x
  dnl PHP_NEW_EXTENSION(aop, php5/Lexer.c php5/aop.c php5/aop_joinpoint.c, $ext_shared)
fi
