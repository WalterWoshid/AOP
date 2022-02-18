--TEST--
aop_add_after_throwing method with exception
--FILE--
<?php

function doStuffException()
{
   echo "doStuffException";
   throw new Exception('Exception doStuffException');
}

aop_add_after_throwing('doStuff*()', function() {
   echo "[after]";
});

try {
   doStuffException();
} catch (Exception $e) {
   echo "[caught]";
}
--EXPECT--
doStuffException[after][caught]
