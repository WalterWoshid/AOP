--TEST--
aop_add_after_returning static class method
--FILE--
<?php

class Stuff
{
    public static function doStuffStatic()
    {
        echo 'Stuff::doStuffStatic';
    }
}

aop_add_after_returning('Stuff->doStuff*()', function() {
    echo '[after]';
});

Stuff::doStuffStatic();
--EXPECT--
Stuff::doStuffStatic[after]