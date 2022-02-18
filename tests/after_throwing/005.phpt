--TEST--
aop_add_after_throwing static class method
--FILE--
<?php

class Stuff
{
   public static function doStuffStatic()
   {
      echo "Stuff::doStuffStatic";
   }
}

aop_add_after_throwing('Stuff->doStuff*()', function() {
   echo "[after]";
});

Stuff::doStuffStatic();
--EXPECT--
Stuff::doStuffStatic
