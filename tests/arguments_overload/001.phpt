--TEST--
Args overload arguments call (try in the first around declared)
--FILE--
<?php 

class mytest
{
	public function test($data)
	{
		var_dump($data);
	}
}

aop_add_around("mytest::test()", function ($pObj) {
    echo "NO OVERLOAD\n";
    var_dump($pObj->getArguments());
    $pObj->process();
});

aop_add_around("mytest::test()", function ($pObj) {
    echo "OVERLOAD\n";
    var_dump($pObj->getArguments());
    $pObj->setArguments(['overload']);
    $pObj->process();
});


$test = new mytest();
$test->test("first");

?>
--EXPECT--
NO OVERLOAD
array(1) {
  [0]=>
  string(5) "first"
}
OVERLOAD
array(1) {
  [0]=>
  string(5) "first"
}
string(8) "overload"

