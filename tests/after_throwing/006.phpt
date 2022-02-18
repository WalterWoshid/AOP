--TEST--
aop_add_after_throwing static class method with exception
--FILE--
<?php

class Stuff
{
   public static function doStuffStaticException()
   {
      echo "Stuff::doStuffStaticException";
      throw new Exception('Exception doStuffStaticException');
   }
}

aop_add_after_throwing('Stuff->doStuff*()', function() {
   echo "[after]";
});

try {
   Stuff::doStuffStaticException();
} catch (Exception $e) {
   echo "[caught]";
}
--EXPECT--
Stuff::doStuffStaticException[after][caught]
