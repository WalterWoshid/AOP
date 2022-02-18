--TEST--
aop_add_after_returning global function
--FILE--
<?php

function doStuff()
{
    echo 'doStuff';
}

aop_add_after_returning('doStuff*()', function() {
    echo '[after]';
});

doStuff();
--EXPECT--
doStuff[after]