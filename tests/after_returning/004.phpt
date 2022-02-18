--TEST--
aop_add_after_returning class method with exception
--FILE--
<?php

class Stuff
{
    public function doStuffException()
    {
        echo 'Stuff->doStuffException';
        throw new Exception('Exception doStuffException');
    }
}

aop_add_after_returning('Stuff->doStuff*()', function() {
    echo '[after]';
});

$stuff = new Stuff();

try {
    $stuff->doStuffException();
} catch (Exception $e) {
    echo '[caught]';
}
--EXPECT--
Stuff->doStuffException[caught]