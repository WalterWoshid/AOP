--TEST--
Common argument overload
--FILE--
<?php

function test($p1, $p2 = 200)
{
    $a = 100;

    print_r(get_defined_vars());
    return $p2 + $a;
}

aop_add_before("test()", function($jp) {
    $args = $jp->getArguments();
    $args[1] = 300;

    $jp->setArguments($args);
});

echo test("this is param 1");
?>
--EXPECT--
Array
(
    [p1] => this is param 1
    [p2] => 300
    [a] => 100
)
400
