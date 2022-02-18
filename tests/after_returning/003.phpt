--TEST--
aop_add_after_returning class method
--FILE--
<?php

class Stuff
{
    public function doStuff()
    {
        echo 'Stuff->doStuff';
    }
}

aop_add_after_returning('Stuff->doStuff*()', function() {
    echo '[after]';
});

$stuff = new Stuff();
$stuff->doStuff();
--EXPECT--
Stuff->doStuff[after]