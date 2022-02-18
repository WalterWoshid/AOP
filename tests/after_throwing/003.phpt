--TEST--
aop_add_after_throwing class method
--FILE--
<?php

class Stuff
{
   public function doStuff()
   {
      echo "Stuff->doStuff";
   }
}

aop_add_after_throwing('Stuff->doStuff*()', function() {
   echo "[after]";
});

$stuff = new Stuff();
$stuff->doStuff();
--EXPECT--
Stuff->doStuff
