--TEST--
An around method test without call to process
--FILE--
<?php 

class Mytest
{
	public function test()
	{
		return "intest";
	}
}

aop_add_around("Mytest::test()", function ($pObj) {
    return "nocall";
});

$test = new Mytest();
echo $test->test();

?>
--EXPECT--
nocall
