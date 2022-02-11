PHP_ARG_ENABLE(AOP, whether to enable AOP support,
[ --enable-AOP   Enable AOP support])

if test "$PHP_AOP" = "yes"; then
  AC_DEFINE(HAVE_AOP, 1, [aop])

  aop_sources="php8/aop.c php8/aop_execute.c php8/aop_joinpoint.c php8/lexer.c"
  PHP_NEW_EXTENSION(aop, $aop_sources, $ext_shared)

  PHP_ADD_EXTENSION_DEP(aop, [pcre])
fi
