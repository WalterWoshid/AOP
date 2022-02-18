--TEST--
aop_add_after_throwing global function
--FILE--
<?php

function doStuff()
{
   echo "doStuff";
}

aop_add_after_throwing('doStuff*()', function() {
   echo "[after]";
});

doStuff();
--EXPECT--
doStuff
