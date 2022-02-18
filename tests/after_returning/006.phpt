--TEST--
aop_add_after_returning static class method with exception
--FILE--
<?php

class Stuff
{
    public static function doStuffStaticException()
    {
        echo 'Stuff::doStuffStaticException';
        throw new Exception('Exception doStuffStaticException');
    }
}

aop_add_after_returning('Stuff->doStuff*()', function() {
    echo '[after]';
});

try {
    Stuff::doStuffStaticException();
} catch (Exception $e) {
    echo '[caught]';
}
--EXPECT--
Stuff::doStuffStaticException[caught]