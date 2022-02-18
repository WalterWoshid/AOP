--TEST--
aop_add_after_returning method with exception
--FILE--
<?php

function doStuffException()
{
    echo 'doStuffException';
    throw new Exception('Exception in doStuffException');
}

aop_add_after_returning('doStuff*()', function() {
    echo '[after]';
});

try {
    doStuffException();
} catch (Exception $e) {
    echo '[caught]';
}
--EXPECT--
doStuffException[caught]